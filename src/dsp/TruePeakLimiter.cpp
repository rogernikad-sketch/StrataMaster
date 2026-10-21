#include "TruePeakLimiter.h"
#include "common/DecibelUtils.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace strata::dsp {

TruePeakLimiter::TruePeakLimiter() {
    prepare(44100.0, 1024);
}

void TruePeakLimiter::prepare(double sampleRate, size_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;

    // Buffer size power of 2 for fast masking (at least 2048 to support up to 10 ms lookahead)
    bufferSize_ = 4096;
    bufferMask_ = bufferSize_ - 1;

    delayBufferL_.assign(bufferSize_, 0.0f);
    delayBufferR_.assign(bufferSize_, 0.0f);
    gainBuffer_.assign(bufferSize_, 1.0f);

    writePos_ = 0;
    currentGain_ = 1.0f;

    // 4-Phase Polyphase FIR Coefficients for 4x True-Peak Interpolation
    // Designed with a 16-tap Kaiser window (beta = 6.0)
    constexpr size_t taps = INTERP_TAPS;
    constexpr double pi = std::numbers::pi_v<double>;

    for (size_t phase = 0; phase < 4; ++phase) {
        const double frac = static_cast<double>(phase) * 0.25;
        double sum = 0.0;

        for (size_t k = 0; k < taps; ++k) {
            const double t = static_cast<double>(k) - 7.5 + frac;
            double sinc = 1.0;
            if (std::abs(t) > 1.0e-7) {
                sinc = std::sin(pi * t) / (pi * t);
            }

            // Blackman-Harris 4-term window
            const double alpha = static_cast<double>(k) / static_cast<double>(taps - 1);
            const double win = 0.35875
                             - 0.48829 * std::cos(2.0 * pi * alpha)
                             + 0.14128 * std::cos(4.0 * pi * alpha)
                             - 0.01168 * std::cos(6.0 * pi * alpha);

            const double val = sinc * win;
            interpCoeffs_[phase][k] = static_cast<float>(val);
            sum += val;
        }

        // Normalize DC gain to unity
        if (std::abs(sum) > 1.0e-9) {
            const float invSum = static_cast<float>(1.0 / sum);
            for (size_t k = 0; k < taps; ++k) {
                interpCoeffs_[phase][k] *= invSum;
            }
        }
    }

    updateParameters();
    reset();
}

void TruePeakLimiter::reset() noexcept {
    std::fill(delayBufferL_.begin(), delayBufferL_.end(), 0.0f);
    std::fill(delayBufferR_.begin(), delayBufferR_.end(), 0.0f);
    std::fill(gainBuffer_.begin(), gainBuffer_.end(), 1.0f);

    writePos_ = 0;
    currentGain_ = 1.0f;
    gainReductionDb_.store(0.0f, std::memory_order_relaxed);
}

void TruePeakLimiter::setCeilingDb(float ceilingDb) noexcept {
    ceilingDb_ = std::clamp(ceilingDb, -12.0f, 0.0f);
    updateParameters();
}

void TruePeakLimiter::setThresholdDb(float thresholdDb) noexcept {
    thresholdDb_ = thresholdDb;
    updateParameters();
}

void TruePeakLimiter::setReleaseMs(float releaseMs) noexcept {
    releaseMs_ = std::clamp(releaseMs, 1.0f, 1000.0f);
    updateParameters();
}

void TruePeakLimiter::setLookaheadMs(float lookaheadMs) noexcept {
    lookaheadMs_ = std::clamp(lookaheadMs, 0.5f, 10.0f);
    updateParameters();
}

void TruePeakLimiter::updateParameters() noexcept {
    ceilingLinear_ = std::pow(10.0f, ceilingDb_ / 20.0f);
    inputGain_ = std::pow(10.0f, -thresholdDb_ / 20.0f);

    lookaheadSamples_ = std::clamp(static_cast<size_t>(lookaheadMs_ * 0.001 * sampleRate_),
                                   size_t{16}, bufferSize_ / 2);

    // Fast release: ~ 15 ms
    fastReleaseCoeff_ = std::exp(-1.0f / (0.015f * static_cast<float>(sampleRate_)));
    // Slow release: user release parameter
    slowReleaseCoeff_ = std::exp(-1.0f / ((releaseMs_ * 0.001f) * static_cast<float>(sampleRate_)));
}

float TruePeakLimiter::estimateTruePeak(const float* history, size_t historyPos, size_t historyMask) const noexcept {
    float maxPeak = 0.0f;

    for (size_t phase = 0; phase < 4; ++phase) {
        float sum = 0.0f;
        for (size_t k = 0; k < INTERP_TAPS; ++k) {
            const size_t idx = (historyPos + bufferSize_ - k) & historyMask;
            sum += interpCoeffs_[phase][k] * history[idx];
        }
        maxPeak = std::max(maxPeak, std::abs(sum));
    }

    return maxPeak;
}

void TruePeakLimiter::processStereo(float* left, float* right, size_t numSamples) noexcept {
    const size_t lookahead = lookaheadSamples_;
    const size_t mask = bufferMask_;
    const float ceiling = ceilingLinear_;
    const float inGain = inputGain_;

    float minGainReduction = 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        // 1. Input drive & optional pre-limiter clipping
        float inL = left[i] * inGain;
        float inR = right[i] * inGain;

        if (clipper_.isEnabled()) {
            inL = clipper_.processSample(inL);
            inR = clipper_.processSample(inR);
        }

        // 2. Write to circular lookahead delay buffer
        delayBufferL_[writePos_] = inL;
        delayBufferR_[writePos_] = inR;

        // 3. 4x Oversampled True-Peak Detection on oncoming input
        const float truePeakL = estimateTruePeak(delayBufferL_.data(), writePos_, mask);
        const float truePeakR = estimateTruePeak(delayBufferR_.data(), writePos_, mask);
        const float maxTruePeak = std::max(truePeakL, truePeakR);

        // Required gain reduction to hold true peak below ceiling
        float reqGain = 1.0f;
        if (maxTruePeak > ceiling) {
            reqGain = ceiling / maxTruePeak;
        }
        gainBuffer_[writePos_] = reqGain;

        // 4. Lookahead minimum search over the lookahead horizon
        // Determines the lowest gain needed in the upcoming lookahead window
        const size_t readPos = (writePos_ + bufferSize_ - lookahead) & mask;
        float targetGain = 1.0f;

        for (size_t d = 0; d <= lookahead; ++d) {
            const size_t checkPos = (readPos + d) & mask;
            targetGain = std::min(targetGain, gainBuffer_[checkPos]);
        }

        // 5. Dual-stage release ballistics
        if (targetGain < currentGain_) {
            // Instant attack: pull gain down smoothly ahead of the peak
            currentGain_ = targetGain;
        } else {
            // Adaptive dual-stage release recovery
            const float fastRel = 1.0f - fastReleaseCoeff_ * (1.0f - currentGain_);
            const float slowRel = 1.0f - slowReleaseCoeff_ * (1.0f - currentGain_);
            // 70% fast release (transient punch), 30% slow release (transparency)
            currentGain_ = std::min(targetGain, 0.7f * fastRel + 0.3f * slowRel);
        }

        // 6. Apply gain to delayed audio sample emerging from lookahead buffer
        float outL = delayBufferL_[readPos] * currentGain_;
        float outR = delayBufferR_[readPos] * currentGain_;

        // 7. Brickwall ceiling safety clamp
        const float outPeak = std::max(std::abs(outL), std::abs(outR));
        if (outPeak > ceiling) {
            const float clampScale = ceiling / outPeak;
            outL *= clampScale;
            outR *= clampScale;
        }

        left[i]  = outL;
        right[i] = outR;

        minGainReduction = std::min(minGainReduction, currentGain_);
        writePos_ = (writePos_ + 1) & mask;
    }

    gainReductionDb_.store(gainToDecibels(minGainReduction), std::memory_order_relaxed);
}

} // namespace strata::dsp

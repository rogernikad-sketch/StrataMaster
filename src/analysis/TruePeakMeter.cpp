#include "TruePeakMeter.h"
#include "../dsp/common/DecibelUtils.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace strata::analysis {

TruePeakMeter::TruePeakMeter() {
    prepare(44100.0);
}

void TruePeakMeter::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;

    bufferSize_ = 64;
    bufferMask_ = bufferSize_ - 1;
    historyL_.assign(bufferSize_, 0.0f);
    historyR_.assign(bufferSize_, 0.0f);
    writePos_ = 0;

    // 4-Phase Polyphase FIR Coefficients for 4x True-Peak Interpolation
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

            const double alpha = static_cast<double>(k) / static_cast<double>(taps - 1);
            const double win = 0.35875
                             - 0.48829 * std::cos(2.0 * pi * alpha)
                             + 0.14128 * std::cos(4.0 * pi * alpha)
                             - 0.01168 * std::cos(6.0 * pi * alpha);

            const double val = sinc * win;
            interpCoeffs_[phase][k] = static_cast<float>(val);
            sum += val;
        }

        if (std::abs(sum) > 1.0e-9) {
            const float invSum = static_cast<float>(1.0 / sum);
            for (size_t k = 0; k < taps; ++k) {
                interpCoeffs_[phase][k] *= invSum;
            }
        }
    }

    // Ballistics release time: ~ 300 ms decay
    releaseCoeff_ = std::exp(-1.0f / (0.300f * static_cast<float>(sampleRate_)));
    reset();
}

void TruePeakMeter::reset() noexcept {
    std::fill(historyL_.begin(), historyL_.end(), 0.0f);
    std::fill(historyR_.begin(), historyR_.end(), 0.0f);
    writePos_ = 0;

    currentPeakL_ = 0.0f;
    currentPeakR_ = 0.0f;

    truePeakDbL_.store(-100.0f, std::memory_order_relaxed);
    truePeakDbR_.store(-100.0f, std::memory_order_relaxed);
    maxTruePeakDb_.store(-100.0f, std::memory_order_relaxed);
}

float TruePeakMeter::estimateTruePeak(const float* buffer, size_t pos, size_t mask) const noexcept {
    float maxVal = 0.0f;

    for (size_t phase = 0; phase < 4; ++phase) {
        float sum = 0.0f;
        for (size_t k = 0; k < INTERP_TAPS; ++k) {
            const size_t idx = (pos + bufferSize_ - k) & mask;
            sum += interpCoeffs_[phase][k] * buffer[idx];
        }
        maxVal = std::max(maxVal, std::abs(sum));
    }

    return maxVal;
}

void TruePeakMeter::processStereo(const float* left, const float* right, size_t numSamples) noexcept {
    const size_t mask = bufferMask_;
    float blockMaxL = 0.0f;
    float blockMaxR = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        historyL_[writePos_] = left[i];
        historyR_[writePos_] = right[i];

        const float tpL = estimateTruePeak(historyL_.data(), writePos_, mask);
        const float tpR = estimateTruePeak(historyR_.data(), writePos_, mask);

        blockMaxL = std::max(blockMaxL, tpL);
        blockMaxR = std::max(blockMaxR, tpR);

        writePos_ = (writePos_ + 1) & mask;
    }

    // Ballistics tracking
    currentPeakL_ = std::max(blockMaxL, currentPeakL_ * releaseCoeff_);
    currentPeakR_ = std::max(blockMaxR, currentPeakR_ * releaseCoeff_);

    const float dbL = dsp::gainToDecibels(currentPeakL_);
    const float dbR = dsp::gainToDecibels(currentPeakR_);

    truePeakDbL_.store(dbL, std::memory_order_relaxed);
    truePeakDbR_.store(dbR, std::memory_order_relaxed);

    const float overallPeak = std::max(dbL, dbR);
    float currentMax = maxTruePeakDb_.load(std::memory_order_relaxed);
    if (overallPeak > currentMax) {
        maxTruePeakDb_.store(overallPeak, std::memory_order_relaxed);
    }
}

} // namespace strata::analysis

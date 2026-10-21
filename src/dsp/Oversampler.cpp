#include "Oversampler.h"
#include <algorithm>
#include <cstring>
#include <cassert>

namespace strata::dsp {

namespace {

double besselI0(double x) noexcept {
    double sum = 1.0;
    double u = 1.0;
    double halfX = 0.5 * x;
    double n = 1.0;
    while (u > 1.0e-15 * sum) {
        double temp = halfX / n;
        u *= temp * temp;
        sum += u;
        n += 1.0;
    }
    return sum;
}

} // namespace

// ==============================================================================
// HalfbandStage Implementation
// ==============================================================================

void HalfbandStage::prepare(size_t M, size_t /*maxBlockSize*/) {
    assert(M > 0 && (M % 2 == 0));
    M_ = M;
    computeCoefficients(M_);

    upHistory_.assign(M_, 0.0f);
    downHistoryOdd_.assign(M_, 0.0f);
    downHistoryEven_.assign(M_ / 2, 0.0f);

    upPos_ = 0;
    downOddPos_ = 0;
    downEvenPos_ = 0;
}

void HalfbandStage::reset() noexcept {
    std::fill(upHistory_.begin(), upHistory_.end(), 0.0f);
    std::fill(downHistoryOdd_.begin(), downHistoryOdd_.end(), 0.0f);
    std::fill(downHistoryEven_.begin(), downHistoryEven_.end(), 0.0f);

    upPos_ = 0;
    downOddPos_ = 0;
    downEvenPos_ = 0;
}

void HalfbandStage::computeCoefficients(size_t M) {
    const size_t numUnique = M / 2;
    polyCoeffs_.resize(numUnique);

    // Beta parameter ~ 9.2 for > 92 dB stopband attenuation and minimal passband ripple
    constexpr double beta = 9.2;
    const double denom = besselI0(beta);
    constexpr double pi = 3.14159265358979323846;

    // Filter length is 2*M + 1. Center tap is at index M (h[M] = 0.5).
    // Odd taps are at k = \pm 1, 3, 5, ..., (M - 1).
    // p = 0 corresponds to outermost odd tap k = M - 1
    // p = numUnique - 1 corresponds to innermost odd tap k = 1
    std::vector<double> oddTaps(numUnique, 0.0);
    double sumOdd = 0.0;

    for (size_t p = 0; p < numUnique; ++p) {
        const size_t k = 2 * (numUnique - 1 - p) + 1;
        const double kd = static_cast<double>(k);
        const double sinc = std::sin(0.5 * pi * kd) / (pi * kd);

        const double ratio = kd / static_cast<double>(M);
        const double wArg = (ratio < 1.0) ? (beta * std::sqrt(1.0 - ratio * ratio)) : 0.0;
        const double win = besselI0(wArg) / denom;

        const double tap = sinc * win;
        oddTaps[p] = tap;
        sumOdd += tap; // Since filter is symmetric, total odd sum = 2 * sumOdd
    }

    // Scale so that total odd sum equals 0.5 (meaning DC sum of all taps = 0.5 + 0.5 = 1.0)
    const double scale = 0.5 / (2.0 * sumOdd);
    for (size_t p = 0; p < numUnique; ++p) {
        // In polyphase upsampling, factor of 2 accounts for zero-stuffing
        polyCoeffs_[p] = static_cast<float>(oddTaps[p] * scale * 2.0);
    }
}

void HalfbandStage::upsample(const float* in, float* out, size_t numInputSamples) noexcept {
    const size_t M = M_;
    const size_t halfM = M / 2;
    const auto* coeffs = polyCoeffs_.data();

    for (size_t i = 0; i < numInputSamples; ++i) {
        const float x = in[i];
        upHistory_[upPos_] = x;

        // Even sample: pure delay of halfM samples
        const size_t evenIdx = (upPos_ + M - halfM) % M;
        out[2 * i] = upHistory_[evenIdx];

        // Odd sample: symmetric convolution across past M odd-phase samples
        float oddSum = 0.0f;
        for (size_t p = 0; p < halfM; ++p) {
            const size_t idxA = (upPos_ + M - p) % M;
            const size_t idxB = (upPos_ + M - (M - 1 - p)) % M;
            oddSum += coeffs[p] * (upHistory_[idxA] + upHistory_[idxB]);
        }
        out[2 * i + 1] = oddSum;

        upPos_ = (upPos_ + 1) % M;
    }
}

void HalfbandStage::downsample(const float* in, float* out, size_t numOutputSamples) noexcept {
    const size_t M = M_;
    const size_t halfM = M / 2;
    const auto* coeffs = polyCoeffs_.data();

    for (size_t i = 0; i < numOutputSamples; ++i) {
        const float evenIn = in[2 * i];
        const float oddIn  = in[2 * i + 1];

        // Even path: delay of halfM samples
        const float evenDelayed = downHistoryEven_[downEvenPos_];
        downHistoryEven_[downEvenPos_] = evenIn;
        downEvenPos_ = (downEvenPos_ + 1) % halfM;

        // Odd path: convolution of odd samples
        downHistoryOdd_[downOddPos_] = oddIn;
        float oddSum = 0.0f;
        for (size_t p = 0; p < halfM; ++p) {
            const size_t idxA = (downOddPos_ + M - p) % M;
            const size_t idxB = (downOddPos_ + M - (M - 1 - p)) % M;
            oddSum += coeffs[p] * (downHistoryOdd_[idxA] + downHistoryOdd_[idxB]);
        }
        downOddPos_ = (downOddPos_ + 1) % M;

        out[i] = 0.5f * (evenDelayed + oddSum);
    }
}

// ==============================================================================
// Oversampler Engine Implementation
// ==============================================================================

Oversampler::Oversampler() {
    dryHistory_[0].assign(256, 0.0f);
    dryHistory_[1].assign(256, 0.0f);
}

void Oversampler::prepare(double sampleRate, size_t maxBlockSize, OversamplingFactor factor) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;
    factor_ = factor;

    // Stage 1: M = 48 (97-tap filter, delay 24 samples @ Fs)
    stagesL_[0].prepare(48, maxBlockSize);
    stagesR_[0].prepare(48, maxBlockSize);

    // Stage 2: M = 32 (65-tap filter, delay 8 samples @ Fs)
    stagesL_[1].prepare(32, maxBlockSize * 2);
    stagesR_[1].prepare(32, maxBlockSize * 2);

    // Stage 3: M = 16 (33-tap filter, delay 2 samples @ Fs)
    stagesL_[2].prepare(16, maxBlockSize * 4);
    stagesR_[2].prepare(16, maxBlockSize * 4);

    // Allocate intermediate working buffers
    stage1BufferL_.assign(maxBlockSize * 2, 0.0f);
    stage1BufferR_.assign(maxBlockSize * 2, 0.0f);

    stage2BufferL_.assign(maxBlockSize * 4, 0.0f);
    stage2BufferR_.assign(maxBlockSize * 4, 0.0f);

    stage3BufferL_.assign(maxBlockSize * 8, 0.0f);
    stage3BufferR_.assign(maxBlockSize * 8, 0.0f);

    dryDelayBuffer_[0].assign(maxBlockSize, 0.0f);
    dryDelayBuffer_[1].assign(maxBlockSize, 0.0f);

    const size_t maxLatency = 256;
    dryHistory_[0].assign(maxLatency, 0.0f);
    dryHistory_[1].assign(maxLatency, 0.0f);
    dryWritePos_ = 0;

    reset();
}

void Oversampler::reset() noexcept {
    for (size_t s = 0; s < 3; ++s) {
        stagesL_[s].reset();
        stagesR_[s].reset();
    }
    std::fill(dryHistory_[0].begin(), dryHistory_[0].end(), 0.0f);
    std::fill(dryHistory_[1].begin(), dryHistory_[1].end(), 0.0f);
    dryWritePos_ = 0;
}

void Oversampler::setFactor(OversamplingFactor factor) {
    if (factor_ != factor) {
        factor_ = factor;
        reset();
    }
}

size_t Oversampler::getLatencyInSamples() const noexcept {
    switch (factor_) {
        case OversamplingFactor::None: return 0;
        case OversamplingFactor::X2:   return 48; // Stage 1 (24 up + 24 down)
        case OversamplingFactor::X4:   return 64; // Stage 1 (48) + Stage 2 (16)
        case OversamplingFactor::X8:   return 68; // Stage 1 (48) + Stage 2 (16) + Stage 3 (4)
    }
    return 0;
}

double Oversampler::getOversampledRate() const noexcept {
    return sampleRate_ * static_cast<double>(factor_);
}

void Oversampler::updateDryDelay(const float* const* inputChannels, size_t numSamples) noexcept {
    const size_t latency = getLatencyInSamples();
    const size_t historySize = dryHistory_[0].size();

    if (latency == 0) {
        std::memcpy(dryDelayBuffer_[0].data(), inputChannels[0], numSamples * sizeof(float));
        std::memcpy(dryDelayBuffer_[1].data(), inputChannels[1], numSamples * sizeof(float));
        return;
    }

    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t ch = 0; ch < 2; ++ch) {
            dryHistory_[ch][dryWritePos_] = inputChannels[ch][i];
            const size_t readPos = (dryWritePos_ + historySize - latency) % historySize;
            dryDelayBuffer_[ch][i] = dryHistory_[ch][readPos];
        }
        dryWritePos_ = (dryWritePos_ + 1) % historySize;
    }
}

void Oversampler::upsample(const float* const* inputChannels, size_t numInputSamples,
                           float* const*& oversampledOutputs, size_t& numOversampledSamples) noexcept {
    assert(numInputSamples <= maxBlockSize_);
    updateDryDelay(inputChannels, numInputSamples);

    switch (factor_) {
        case OversamplingFactor::None: {
            // Bypass oversampling
            oversampledPtrs_[0] = const_cast<float*>(inputChannels[0]);
            oversampledPtrs_[1] = const_cast<float*>(inputChannels[1]);
            oversampledOutputs = oversampledPtrs_.data();
            numOversampledSamples = numInputSamples;
            break;
        }

        case OversamplingFactor::X2: {
            stagesL_[0].upsample(inputChannels[0], stage1BufferL_.data(), numInputSamples);
            stagesR_[0].upsample(inputChannels[1], stage1BufferR_.data(), numInputSamples);

            oversampledPtrs_[0] = stage1BufferL_.data();
            oversampledPtrs_[1] = stage1BufferR_.data();
            oversampledOutputs = oversampledPtrs_.data();
            numOversampledSamples = numInputSamples * 2;
            break;
        }

        case OversamplingFactor::X4: {
            stagesL_[0].upsample(inputChannels[0], stage1BufferL_.data(), numInputSamples);
            stagesR_[0].upsample(inputChannels[1], stage1BufferR_.data(), numInputSamples);

            stagesL_[1].upsample(stage1BufferL_.data(), stage2BufferL_.data(), numInputSamples * 2);
            stagesR_[1].upsample(stage1BufferR_.data(), stage2BufferR_.data(), numInputSamples * 2);

            oversampledPtrs_[0] = stage2BufferL_.data();
            oversampledPtrs_[1] = stage2BufferR_.data();
            oversampledOutputs = oversampledPtrs_.data();
            numOversampledSamples = numInputSamples * 4;
            break;
        }

        case OversamplingFactor::X8: {
            stagesL_[0].upsample(inputChannels[0], stage1BufferL_.data(), numInputSamples);
            stagesR_[0].upsample(inputChannels[1], stage1BufferR_.data(), numInputSamples);

            stagesL_[1].upsample(stage1BufferL_.data(), stage2BufferL_.data(), numInputSamples * 2);
            stagesR_[1].upsample(stage1BufferR_.data(), stage2BufferR_.data(), numInputSamples * 2);

            stagesL_[2].upsample(stage2BufferL_.data(), stage3BufferL_.data(), numInputSamples * 4);
            stagesR_[2].upsample(stage2BufferR_.data(), stage3BufferR_.data(), numInputSamples * 4);

            oversampledPtrs_[0] = stage3BufferL_.data();
            oversampledPtrs_[1] = stage3BufferR_.data();
            oversampledOutputs = oversampledPtrs_.data();
            numOversampledSamples = numInputSamples * 8;
            break;
        }
    }
}

void Oversampler::downsample(float* const* outputChannels, size_t numOutputSamples) noexcept {
    switch (factor_) {
        case OversamplingFactor::None: {
            // Nothing to decimate
            break;
        }

        case OversamplingFactor::X2: {
            stagesL_[0].downsample(stage1BufferL_.data(), outputChannels[0], numOutputSamples);
            stagesR_[0].downsample(stage1BufferR_.data(), outputChannels[1], numOutputSamples);
            break;
        }

        case OversamplingFactor::X4: {
            stagesL_[1].downsample(stage2BufferL_.data(), stage1BufferL_.data(), numOutputSamples * 2);
            stagesR_[1].downsample(stage2BufferR_.data(), stage1BufferR_.data(), numOutputSamples * 2);

            stagesL_[0].downsample(stage1BufferL_.data(), outputChannels[0], numOutputSamples);
            stagesR_[0].downsample(stage1BufferR_.data(), outputChannels[1], numOutputSamples);
            break;
        }

        case OversamplingFactor::X8: {
            stagesL_[2].downsample(stage3BufferL_.data(), stage2BufferL_.data(), numOutputSamples * 4);
            stagesR_[2].downsample(stage3BufferR_.data(), stage2BufferR_.data(), numOutputSamples * 4);

            stagesL_[1].downsample(stage2BufferL_.data(), stage1BufferL_.data(), numOutputSamples * 2);
            stagesR_[1].downsample(stage2BufferR_.data(), stage1BufferR_.data(), numOutputSamples * 2);

            stagesL_[0].downsample(stage1BufferL_.data(), outputChannels[0], numOutputSamples);
            stagesR_[0].downsample(stage1BufferR_.data(), outputChannels[1], numOutputSamples);
            break;
        }
    }
}

} // namespace strata::dsp

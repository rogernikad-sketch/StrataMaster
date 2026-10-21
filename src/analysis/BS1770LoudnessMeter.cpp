#include "BS1770LoudnessMeter.h"
#include <algorithm>
#include <numeric>

namespace strata::analysis {

BS1770LoudnessMeter::BS1770LoudnessMeter() {
    prepare(44100.0);
}

BS1770LoudnessMeter::FilterCoeffs BS1770LoudnessMeter::designStage1HighShelf(double sampleRate) noexcept {
    // ITU-R BS.1770-4 Stage 1 High-Shelf Filter (Head modeling)
    // f0 = 1681.974 Hz, Gain = +3.9998438 dB (V0 = 1.5848624), Q = 0.7071752
    constexpr double f0 = 1681.974;
    constexpr double V0 = 1.5848624;
    constexpr double sqrtV0 = 1.2589132;
    constexpr double sqrtTwo = 1.414213562373095;
    constexpr double pi = 3.14159265358979323846;

    const double K = std::tan(pi * f0 / sampleRate);
    const double K2 = K * K;
    const double sqrtTwoK = sqrtTwo * K;
    const double sqrtTwoV0K = sqrtTwo * sqrtV0 * K;

    const double a0 = 1.0 + sqrtTwoK + K2;
    const double b0 = (V0 + sqrtTwoV0K + K2) / a0;
    const double b1 = 2.0 * (K2 - V0) / a0;
    const double b2 = (V0 - sqrtTwoV0K + K2) / a0;
    const double a1 = 2.0 * (K2 - 1.0) / a0;
    const double a2 = (1.0 - sqrtTwoK + K2) / a0;

    return { static_cast<float>(b0), static_cast<float>(b1), static_cast<float>(b2),
             static_cast<float>(a1), static_cast<float>(a2) };
}

BS1770LoudnessMeter::FilterCoeffs BS1770LoudnessMeter::designStage2HighPass(double sampleRate) noexcept {
    // ITU-R BS.1770-4 Stage 2 High-Pass Filter (RLB weighting)
    // f0 = 38.13547 Hz, Q = 0.500327
    constexpr double f0 = 38.13547;
    constexpr double Q = 0.500327;
    constexpr double pi = 3.14159265358979323846;

    const double K = std::tan(pi * f0 / sampleRate);
    const double K2 = K * K;
    const double KQ = K / Q;

    const double a0 = 1.0 + KQ + K2;
    const double b0 = 1.0 / a0;
    const double b1 = -2.0 / a0;
    const double b2 = 1.0 / a0;
    const double a1 = 2.0 * (K2 - 1.0) / a0;
    const double a2 = (1.0 - KQ + K2) / a0;

    return { static_cast<float>(b0), static_cast<float>(b1), static_cast<float>(b2),
             static_cast<float>(a1), static_cast<float>(a2) };
}

void BS1770LoudnessMeter::prepare(double sampleRate) {
    sampleRate_ = sampleRate;

    stage1Coeffs_ = designStage1HighShelf(sampleRate);
    stage2Coeffs_ = designStage2HighPass(sampleRate);

    // 400 ms momentary window
    momentaryWindowSamples_ = std::max(size_t{100}, static_cast<size_t>(0.400 * sampleRate_));
    momentarySqL_.assign(momentaryWindowSamples_, 0.0f);
    momentarySqR_.assign(momentaryWindowSamples_, 0.0f);

    // 3.0 s short-term window
    shortTermWindowSamples_ = std::max(size_t{100}, static_cast<size_t>(3.000 * sampleRate_));
    shortTermSqL_.assign(shortTermWindowSamples_, 0.0f);
    shortTermSqR_.assign(shortTermWindowSamples_, 0.0f);

    // 100 ms gating step for integrated loudness
    gatingBlockSamples_ = std::max(size_t{10}, static_cast<size_t>(0.100 * sampleRate_));

    // Pre-reserve vectors to guarantee zero dynamic memory allocations on audio thread
    integratedBlocks_.reserve(36000); // 1 hour of 100 ms blocks
    shortTermHistory_.reserve(36000);
    absGated_.reserve(36000);
    validShortTerms_.reserve(36000);

    reset();
}

void BS1770LoudnessMeter::reset() noexcept {
    kFilterL1_.reset();
    kFilterL2_.reset();
    kFilterR1_.reset();
    kFilterR2_.reset();

    std::fill(momentarySqL_.begin(), momentarySqL_.end(), 0.0f);
    std::fill(momentarySqR_.begin(), momentarySqR_.end(), 0.0f);
    momentaryPos_ = 0;
    momentarySumL_ = 0.0;
    momentarySumR_ = 0.0;

    std::fill(shortTermSqL_.begin(), shortTermSqL_.end(), 0.0f);
    std::fill(shortTermSqR_.begin(), shortTermSqR_.end(), 0.0f);
    shortTermPos_ = 0;
    shortTermSumL_ = 0.0;
    shortTermSumR_ = 0.0;

    currentBlockCount_ = 0;
    currentBlockSqL_ = 0.0;
    currentBlockSqR_ = 0.0;

    integratedBlocks_.clear();
    shortTermHistory_.clear();

    momentaryLufs_.store(-100.0f, std::memory_order_relaxed);
    shortTermLufs_.store(-100.0f, std::memory_order_relaxed);
    integratedLufs_.store(-100.0f, std::memory_order_relaxed);
    loudnessRangeLu_.store(0.0f, std::memory_order_relaxed);
}

void BS1770LoudnessMeter::processStereo(const float* left, const float* right, size_t numSamples) noexcept {
    const size_t mSize = momentaryWindowSamples_;
    const size_t sSize = shortTermWindowSamples_;
    const size_t gSize = gatingBlockSamples_;

    for (size_t i = 0; i < numSamples; ++i) {
        // 1. Apply K-weighting pre-filter (Stage 1 High-Shelf -> Stage 2 High-Pass)
        const float inL = left[i];
        const float inR = right[i];

        const float kL = kFilterL2_.process(kFilterL1_.process(inL, stage1Coeffs_), stage2Coeffs_);
        const float kR = kFilterR2_.process(kFilterR1_.process(inR, stage1Coeffs_), stage2Coeffs_);

        const float sqL = kL * kL;
        const float sqR = kR * kR;

        // 2. Sliding window accumulation for 400 ms Momentary Loudness
        momentarySumL_ += static_cast<double>(sqL) - static_cast<double>(momentarySqL_[momentaryPos_]);
        momentarySumR_ += static_cast<double>(sqR) - static_cast<double>(momentarySqR_[momentaryPos_]);
        momentarySqL_[momentaryPos_] = sqL;
        momentarySqR_[momentaryPos_] = sqR;
        momentaryPos_ = (momentaryPos_ + 1) % mSize;

        // 3. Sliding window accumulation for 3.0 s Short-Term Loudness
        shortTermSumL_ += static_cast<double>(sqL) - static_cast<double>(shortTermSqL_[shortTermPos_]);
        shortTermSumR_ += static_cast<double>(sqR) - static_cast<double>(shortTermSqR_[shortTermPos_]);
        shortTermSqL_[shortTermPos_] = sqL;
        shortTermSqR_[shortTermPos_] = sqR;
        shortTermPos_ = (shortTermPos_ + 1) % sSize;

        // 4. 100 ms Gating Block Accumulator
        currentBlockSqL_ += sqL;
        currentBlockSqR_ += sqR;
        ++currentBlockCount_;

        if (currentBlockCount_ >= gSize) {
            const float zL = static_cast<float>(currentBlockSqL_ / static_cast<double>(gSize));
            const float zR = static_cast<float>(currentBlockSqR_ / static_cast<double>(gSize));
            integratedBlocks_.push_back({ zL, zR });

            // Record Short-Term reading for LRA computation
            const double sMeanL = std::max(0.0, shortTermSumL_ / static_cast<double>(sSize));
            const double sMeanR = std::max(0.0, shortTermSumR_ / static_cast<double>(sSize));
            const double sLoud = -0.691 + 10.0 * std::log10(std::max(1.0e-10, sMeanL + sMeanR));
            shortTermHistory_.push_back(static_cast<float>(sLoud));

            currentBlockSqL_ = 0.0;
            currentBlockSqR_ = 0.0;
            currentBlockCount_ = 0;

            updateGatedIntegratedLoudness();
        }
    }

    // Update real-time Momentary and Short-Term LUFS
    const double mMean = std::max(0.0, (momentarySumL_ + momentarySumR_) / static_cast<double>(mSize));
    const float mLufs = (mMean > 1.0e-10) ? static_cast<float>(-0.691 + 10.0 * std::log10(mMean)) : -100.0f;
    momentaryLufs_.store(mLufs, std::memory_order_relaxed);

    const double sMean = std::max(0.0, (shortTermSumL_ + shortTermSumR_) / static_cast<double>(sSize));
    const float sLufs = (sMean > 1.0e-10) ? static_cast<float>(-0.691 + 10.0 * std::log10(sMean)) : -100.0f;
    shortTermLufs_.store(sLufs, std::memory_order_relaxed);
}

void BS1770LoudnessMeter::updateGatedIntegratedLoudness() noexcept {
    if (integratedBlocks_.empty()) {
        return;
    }

    // Step 1: Absolute Gating at -70 LKFS
    constexpr double absThreshold = -70.0;
    absGated_.clear();
    double absSum = 0.0;

    for (const auto& b : integratedBlocks_) {
        const double power = static_cast<double>(b.zL) + static_cast<double>(b.zR);
        if (power > 1.0e-10) {
            const double lkfs = -0.691 + 10.0 * std::log10(power);
            if (lkfs > absThreshold) {
                absGated_.push_back(b);
                absSum += power;
            }
        }
    }

    if (absGated_.empty()) {
        integratedLufs_.store(-100.0f, std::memory_order_relaxed);
        return;
    }

    // Step 2: Relative Gating at Gamma_rel = Unweighted_Loudness - 10.0 LU
    const double unweightedMean = absSum / static_cast<double>(absGated_.size());
    const double unweightedLoudness = -0.691 + 10.0 * std::log10(unweightedMean);
    const double relThreshold = unweightedLoudness - 10.0;

    double relSum = 0.0;
    size_t relCount = 0;

    for (const auto& b : absGated_) {
        const double power = static_cast<double>(b.zL) + static_cast<double>(b.zR);
        const double lkfs = -0.691 + 10.0 * std::log10(power);
        if (lkfs > relThreshold) {
            relSum += power;
            ++relCount;
        }
    }

    if (relCount > 0) {
        const double finalMean = relSum / static_cast<double>(relCount);
        const float intLufs = static_cast<float>(-0.691 + 10.0 * std::log10(finalMean));
        integratedLufs_.store(intLufs, std::memory_order_relaxed);
    }

    // Step 3: Loudness Range (LRA) according to EBU Tech 3342
    if (shortTermHistory_.size() >= 10) {
        validShortTerms_.clear();

        for (float val : shortTermHistory_) {
            // Discard values below absolute -70 LUFS and relative (Gamma_rel - 10 LU)
            if (val > -70.0f && val > static_cast<float>(relThreshold - 10.0)) {
                validShortTerms_.push_back(val);
            }
        }

        if (validShortTerms_.size() >= 5) {
            std::sort(validShortTerms_.begin(), validShortTerms_.end());
            const size_t idx10 = static_cast<size_t>(0.10 * static_cast<double>(validShortTerms_.size() - 1));
            const size_t idx95 = static_cast<size_t>(0.95 * static_cast<double>(validShortTerms_.size() - 1));
            const float lra = std::max(0.0f, validShortTerms_[idx95] - validShortTerms_[idx10]);
            loudnessRangeLu_.store(lra, std::memory_order_relaxed);
        }
    }
}

} // namespace strata::analysis

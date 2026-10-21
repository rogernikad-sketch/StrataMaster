#pragma once

#include "../dsp/common/Constants.h"
#include <vector>
#include <deque>
#include <atomic>
#include <cstddef>
#include <cmath>

namespace strata::analysis {

/**
 * ITU-R BS.1770-4 & EBU R128 Compliant Broadcast Loudness Meter.
 * Provides:
 * - K-weighting pre-filter (High-shelf head simulation + High-pass RLB)
 * - Momentary Loudness (400 ms sliding window)
 * - Short-Term Loudness (3.0 s sliding window)
 * - Integrated Loudness with dual-stage gating (Absolute -70 LKFS + Relative -10 LU)
 * - Loudness Range (LRA) according to EBU Tech 3342
 */
class BS1770LoudnessMeter final {
public:
    BS1770LoudnessMeter();
    ~BS1770LoudnessMeter() = default;

    void prepare(double sampleRate);
    void reset() noexcept;

    /** Processes stereo audio block in-place or from input buffers */
    void processStereo(const float* left, const float* right, size_t numSamples) noexcept;

    // Real-time Meter Readings (thread-safe for UI polling)
    [[nodiscard]] float getMomentaryLoudness() const noexcept {
        return momentaryLufs_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] float getShortTermLoudness() const noexcept {
        return shortTermLufs_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] float getIntegratedLoudness() const noexcept {
        return integratedLufs_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] float getLoudnessRange() const noexcept {
        return loudnessRangeLu_.load(std::memory_order_relaxed);
    }

    /** Helper to compute K-weighting coefficients for any sample rate */
    struct FilterCoeffs {
        float b0{1.0f}, b1{0.0f}, b2{0.0f}, a1{0.0f}, a2{0.0f};
    };

    static FilterCoeffs designStage1HighShelf(double sampleRate) noexcept;
    static FilterCoeffs designStage2HighPass(double sampleRate) noexcept;

private:
    struct BiquadState {
        float s1{0.0f};
        float s2{0.0f};

        inline float process(float x, const FilterCoeffs& c) noexcept {
            const float y = c.b0 * x + s1;
            s1 = c.b1 * x - c.a1 * y + s2;
            s2 = c.b2 * x - c.a2 * y;
            return y;
        }

        void reset() noexcept { s1 = 0.0f; s2 = 0.0f; }
    };

    void updateGatedIntegratedLoudness() noexcept;

    double sampleRate_{ 44100.0 };

    FilterCoeffs stage1Coeffs_;
    FilterCoeffs stage2Coeffs_;

    // K-weighting filter states for Left and Right channels
    BiquadState kFilterL1_, kFilterL2_;
    BiquadState kFilterR1_, kFilterR2_;

    // 400 ms sliding window ring buffer for Momentary Loudness
    size_t momentaryWindowSamples_{ 17640 };
    std::vector<float> momentarySqL_;
    std::vector<float> momentarySqR_;
    size_t momentaryPos_{ 0 };
    double momentarySumL_{ 0.0 };
    double momentarySumR_{ 0.0 };

    // 3.0 s sliding window ring buffer for Short-Term Loudness
    size_t shortTermWindowSamples_{ 132300 };
    std::vector<float> shortTermSqL_;
    std::vector<float> shortTermSqR_;
    size_t shortTermPos_{ 0 };
    double shortTermSumL_{ 0.0 };
    double shortTermSumR_{ 0.0 };

    // 100 ms gating block accumulation for Integrated Loudness & LRA
    size_t gatingBlockSamples_{ 4410 };
    size_t currentBlockCount_{ 0 };
    double currentBlockSqL_{ 0.0 };
    double currentBlockSqR_{ 0.0 };

    struct GatedBlock {
        float zL;
        float zR;
    };
    std::vector<GatedBlock> integratedBlocks_;
    std::vector<float> shortTermHistory_;

    // Scratch buffers for gating calculations (avoids allocations during audio callback)
    std::vector<GatedBlock> absGated_;
    std::vector<float> validShortTerms_;

    // Atomic meters for UI
    std::atomic<float> momentaryLufs_{ -100.0f };
    std::atomic<float> shortTermLufs_{ -100.0f };
    std::atomic<float> integratedLufs_{ -100.0f };
    std::atomic<float> loudnessRangeLu_{ 0.0f };
};

} // namespace strata::analysis

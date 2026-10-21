#pragma once

#include "BiquadFilter.h"
#include "common/Constants.h"
#include <array>
#include <cstddef>
#include <vector>

namespace strata::dsp {

inline constexpr size_t NUM_CROSSOVER_BANDS = 4;
inline constexpr size_t NUM_CROSSOVER_SPLITS = 3;

/**
 * 4-Band Phase-Compensated Linkwitz-Riley 4th-Order (24 dB/oct) Crossover Filterbank.
 * Splits full spectrum into 4 bands with an identically flat all-pass summed response.
 */
class CrossoverFilterbank final {
public:
    CrossoverFilterbank();

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    /** Sets the three crossover split frequencies (f1 < f2 < f3) */
    void setCrossoverFrequencies(float f1, float f2, float f3, bool snapImmediately = false) noexcept;

    [[nodiscard]] std::array<float, NUM_CROSSOVER_SPLITS> getCrossoverFrequencies() const noexcept {
        return {f1_, f2_, f3_};
    }

    /**
     * Splits an input channel into 4 distinct frequency bands.
     * bandOutputs must point to an array of 4 buffers, each having at least numSamples capacity.
     */
    void splitChannel(size_t channel, const float* input, float* const* bandOutputs, size_t numSamples) noexcept;

    /**
     * Sums 4 frequency bands back together into a single destination buffer.
     */
    static void sumChannel(const float* const* bandInputs, float* output, size_t numSamples) noexcept;

private:
    void updateFilters(bool snapImmediately) noexcept;

    double sampleRate_{44100.0};
    float f1_{120.0f};   // Low / Low-Mid split
    float f2_{1500.0f};  // Lower / Upper split
    float f3_{6000.0f};  // High-Mid / High split

    // Channel DSP structures (Channel 0 = Left / Mid, Channel 1 = Right / Side)
    struct ChannelCrossover {
        // Stage 1: Split at f2
        BiquadFilter lp2_stage1_a;
        BiquadFilter lp2_stage1_b;
        BiquadFilter hp2_stage1_a;
        BiquadFilter hp2_stage1_b;

        // Stage 2 (Lower half): Split at f1 + Independent Phase Compensation AP(f3)
        BiquadFilter lp1_stage2_a;
        BiquadFilter lp1_stage2_b;
        BiquadFilter hp1_stage2_a;
        BiquadFilter hp1_stage2_b;
        BiquadFilter ap3_stage2_b0; // All-pass at f3 for Band 0
        BiquadFilter ap3_stage2_b1; // All-pass at f3 for Band 1

        // Stage 3 (Upper half): Split at f3 + Independent Phase Compensation AP(f1)
        BiquadFilter lp3_stage3_a;
        BiquadFilter lp3_stage3_b;
        BiquadFilter hp3_stage3_a;
        BiquadFilter hp3_stage3_b;
        BiquadFilter ap1_stage3_b2; // All-pass at f1 for Band 2
        BiquadFilter ap1_stage3_b3; // All-pass at f1 for Band 3

        // Scratch buffers for intermediate splits
        std::vector<float> lowHalfScratch;
        std::vector<float> highHalfScratch;

        void prepare(double sampleRate) {
            lp2_stage1_a.prepare(sampleRate);
            lp2_stage1_b.prepare(sampleRate);
            hp2_stage1_a.prepare(sampleRate);
            hp2_stage1_b.prepare(sampleRate);

            lp1_stage2_a.prepare(sampleRate);
            lp1_stage2_b.prepare(sampleRate);
            hp1_stage2_a.prepare(sampleRate);
            hp1_stage2_b.prepare(sampleRate);
            ap3_stage2_b0.prepare(sampleRate);
            ap3_stage2_b1.prepare(sampleRate);

            lp3_stage3_a.prepare(sampleRate);
            lp3_stage3_b.prepare(sampleRate);
            hp3_stage3_a.prepare(sampleRate);
            hp3_stage3_b.prepare(sampleRate);
            ap1_stage3_b2.prepare(sampleRate);
            ap1_stage3_b3.prepare(sampleRate);
        }

        void reset() {
            lp2_stage1_a.reset();
            lp2_stage1_b.reset();
            hp2_stage1_a.reset();
            hp2_stage1_b.reset();

            lp1_stage2_a.reset();
            lp1_stage2_b.reset();
            hp1_stage2_a.reset();
            hp1_stage2_b.reset();
            ap3_stage2_b0.reset();
            ap3_stage2_b1.reset();

            lp3_stage3_a.reset();
            lp3_stage3_b.reset();
            hp3_stage3_a.reset();
            hp3_stage3_b.reset();
            ap1_stage3_b2.reset();
            ap1_stage3_b3.reset();
        }
    };

    std::array<ChannelCrossover, 2> channels_;
};

} // namespace strata::dsp

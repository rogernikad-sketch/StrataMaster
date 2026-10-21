#include "CrossoverFilterbank.h"
#include <algorithm>

namespace strata::dsp {

CrossoverFilterbank::CrossoverFilterbank() {
    updateFilters(true);
}

void CrossoverFilterbank::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& ch : channels_) {
        ch.prepare(sampleRate);
        ch.lowHalfScratch.resize(2048, 0.0f);
        ch.highHalfScratch.resize(2048, 0.0f);
    }
    updateFilters(true);
}

void CrossoverFilterbank::reset() noexcept {
    for (auto& ch : channels_) {
        ch.reset();
    }
}

void CrossoverFilterbank::setCrossoverFrequencies(float f1, float f2, float f3, bool snapImmediately) noexcept {
    f1_ = std::clamp(f1, 20.0f, 1000.0f);
    f2_ = std::clamp(f2, f1_ * 1.25f, 8000.0f);
    f3_ = std::clamp(f3, f2_ * 1.25f, static_cast<float>(sampleRate_ * 0.45));

    updateFilters(snapImmediately);
}

void CrossoverFilterbank::updateFilters(bool snapImmediately) noexcept {
    constexpr float Q = 0.70710678f; // 1 / sqrt(2) for Butterworth pairs

    for (auto& ch : channels_) {
        // Stage 1: Split at f2
        ch.lp2_stage1_a.setParameters(FilterType::LowPass, f2_, 0.0f, Q, snapImmediately);
        ch.lp2_stage1_b.setParameters(FilterType::LowPass, f2_, 0.0f, Q, snapImmediately);
        ch.hp2_stage1_a.setParameters(FilterType::HighPass, f2_, 0.0f, Q, snapImmediately);
        ch.hp2_stage1_b.setParameters(FilterType::HighPass, f2_, 0.0f, Q, snapImmediately);

        // Stage 2: Split at f1 + Independent Phase Compensation AP(f3) for each band
        ch.lp1_stage2_a.setParameters(FilterType::LowPass, f1_, 0.0f, Q, snapImmediately);
        ch.lp1_stage2_b.setParameters(FilterType::LowPass, f1_, 0.0f, Q, snapImmediately);
        ch.hp1_stage2_a.setParameters(FilterType::HighPass, f1_, 0.0f, Q, snapImmediately);
        ch.hp1_stage2_b.setParameters(FilterType::HighPass, f1_, 0.0f, Q, snapImmediately);
        ch.ap3_stage2_b0.setParameters(FilterType::AllPass, f3_, 0.0f, Q, snapImmediately);
        ch.ap3_stage2_b1.setParameters(FilterType::AllPass, f3_, 0.0f, Q, snapImmediately);

        // Stage 3: Split at f3 + Independent Phase Compensation AP(f1) for each band
        ch.lp3_stage3_a.setParameters(FilterType::LowPass, f3_, 0.0f, Q, snapImmediately);
        ch.lp3_stage3_b.setParameters(FilterType::LowPass, f3_, 0.0f, Q, snapImmediately);
        ch.hp3_stage3_a.setParameters(FilterType::HighPass, f3_, 0.0f, Q, snapImmediately);
        ch.hp3_stage3_b.setParameters(FilterType::HighPass, f3_, 0.0f, Q, snapImmediately);
        ch.ap1_stage3_b2.setParameters(FilterType::AllPass, f1_, 0.0f, Q, snapImmediately);
        ch.ap1_stage3_b3.setParameters(FilterType::AllPass, f1_, 0.0f, Q, snapImmediately);
    }
}

void CrossoverFilterbank::splitChannel(size_t channel, const float* input, float* const* bandOutputs, size_t numSamples) noexcept {
    if (channel >= 2 || numSamples == 0) {
        return;
    }

    auto& ch = channels_[channel];
    if (ch.lowHalfScratch.size() < numSamples) {
        ch.lowHalfScratch.resize(numSamples);
        ch.highHalfScratch.resize(numSamples);
    }

    float* lowHalf = ch.lowHalfScratch.data();
    float* highHalf = ch.highHalfScratch.data();

    // 1. Stage 1: Split at f2
    std::copy_n(input, numSamples, lowHalf);
    std::copy_n(input, numSamples, highHalf);

    ch.lp2_stage1_a.processBlock(lowHalf, numSamples);
    ch.lp2_stage1_b.processBlock(lowHalf, numSamples);

    ch.hp2_stage1_a.processBlock(highHalf, numSamples);
    ch.hp2_stage1_b.processBlock(highHalf, numSamples);

    // 2. Stage 2: Split lowHalf at f1 into Band 0 and Band 1
    float* band0 = bandOutputs[0];
    float* band1 = bandOutputs[1];
    std::copy_n(lowHalf, numSamples, band0);
    std::copy_n(lowHalf, numSamples, band1);

    ch.lp1_stage2_a.processBlock(band0, numSamples);
    ch.lp1_stage2_b.processBlock(band0, numSamples);
    ch.ap3_stage2_b0.processBlock(band0, numSamples); // Phase compensation AP(f3) for Band 0

    ch.hp1_stage2_a.processBlock(band1, numSamples);
    ch.hp1_stage2_b.processBlock(band1, numSamples);
    ch.ap3_stage2_b1.processBlock(band1, numSamples); // Phase compensation AP(f3) for Band 1

    // 3. Stage 3: Split highHalf at f3 into Band 2 and Band 3
    float* band2 = bandOutputs[2];
    float* band3 = bandOutputs[3];
    std::copy_n(highHalf, numSamples, band2);
    std::copy_n(highHalf, numSamples, band3);

    ch.lp3_stage3_a.processBlock(band2, numSamples);
    ch.lp3_stage3_b.processBlock(band2, numSamples);
    ch.ap1_stage3_b2.processBlock(band2, numSamples); // Phase compensation AP(f1) for Band 2

    ch.hp3_stage3_a.processBlock(band3, numSamples);
    ch.hp3_stage3_b.processBlock(band3, numSamples);
    ch.ap1_stage3_b3.processBlock(band3, numSamples); // Phase compensation AP(f1) for Band 3
}

void CrossoverFilterbank::sumChannel(const float* const* bandInputs, float* output, size_t numSamples) noexcept {
    const float* b0 = bandInputs[0];
    const float* b1 = bandInputs[1];
    const float* b2 = bandInputs[2];
    const float* b3 = bandInputs[3];

    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = b0[i] + b1[i] + b2[i] + b3[i];
    }
}

} // namespace strata::dsp

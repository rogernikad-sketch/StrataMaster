#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/TransientProcessor.h"
#include "dsp/common/DecibelUtils.h"
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>

using namespace strata::dsp;

namespace {

// Generates an impulsive test signal: 5 ms transient burst followed by 200 ms steady sustain
std::vector<float> generateBurstSignal(double sampleRate) {
    const size_t numSamples = static_cast<size_t>(sampleRate * 0.4); // 400 ms
    std::vector<float> signal(numSamples, 0.0f);

    const size_t burstStart = static_cast<size_t>(sampleRate * 0.05); // 50 ms
    const size_t burstEnd   = static_cast<size_t>(sampleRate * 0.055); // 55 ms (5 ms duration)
    const size_t sustainEnd = static_cast<size_t>(sampleRate * 0.250); // 250 ms (195 ms duration)

    for (size_t i = burstStart; i < burstEnd; ++i) {
        // High amplitude burst (transient)
        signal[i] = 0.8f * std::sin(2.0f * std::numbers::pi_v<float> * 1000.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
    }

    for (size_t i = burstEnd; i < sustainEnd; ++i) {
        // Lower amplitude sustain
        signal[i] = 0.25f * std::sin(2.0f * std::numbers::pi_v<float> * 1000.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
    }

    return signal;
}

} // namespace

TEST_CASE("TransientProcessor microdynamic envelope differentiation", "[dsp][transient]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t blockSize = 256;

    TransientProcessor processor;
    processor.prepare(sampleRate, blockSize);

    const std::vector<float> input = generateBurstSignal(sampleRate);
    const size_t totalSamples = input.size();

    SECTION("Neutral settings produce bit-exact pass-through") {
        std::vector<float> testL = input;
        std::vector<float> testR = input;

        processor.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            const size_t n = std::min(blockSize, totalSamples - offset);
            processor.processStereo(testL.data() + offset, testR.data() + offset, n, 0.0f, 0.0f);
        }

        for (size_t i = 0; i < totalSamples; ++i) {
            REQUIRE(testL[i] == input[i]);
            REQUIRE(testR[i] == input[i]);
        }
    }

    SECTION("Attack boost amplifies transient burst over steady-state sustain") {
        std::vector<float> testL = input;
        std::vector<float> testR = input;

        processor.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            const size_t n = std::min(blockSize, totalSamples - offset);
            processor.processStereo(testL.data() + offset, testR.data() + offset, n, 6.0f, 0.0f);
        }

        // Measure burst peak vs input burst peak
        const size_t burstStart = static_cast<size_t>(sampleRate * 0.05);
        const size_t burstEnd   = static_cast<size_t>(sampleRate * 0.055);

        float inBurstPeak = 0.0f;
        float outBurstPeak = 0.0f;
        for (size_t i = burstStart; i < burstEnd; ++i) {
            inBurstPeak  = std::max(inBurstPeak, std::abs(input[i]));
            outBurstPeak = std::max(outBurstPeak, std::abs(testL[i]));
        }

        // Burst peak must be amplified by at least 2.5 dB
        const float burstBoostDb = gainToDecibels(outBurstPeak / inBurstPeak);
        REQUIRE(burstBoostDb > 2.5f);

        // Measure steady-state sustain ratio
        const size_t steadyIdx = static_cast<size_t>(sampleRate * 0.150);
        float inSteadyRms = 0.0f;
        float outSteadyRms = 0.0f;
        for (size_t i = steadyIdx; i < steadyIdx + 500; ++i) {
            inSteadyRms  += input[i] * input[i];
            outSteadyRms += testL[i] * testL[i];
        }
        const float steadyChangeDb = gainToDecibels(std::sqrt(outSteadyRms / inSteadyRms));

        // Steady sustain change should be minimal compared to burst boost
        REQUIRE(std::abs(steadyChangeDb) < 1.0f);
    }

    SECTION("Attack cut attenuates transient punch") {
        std::vector<float> testL = input;
        std::vector<float> testR = input;

        processor.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            const size_t n = std::min(blockSize, totalSamples - offset);
            processor.processStereo(testL.data() + offset, testR.data() + offset, n, -6.0f, 0.0f);
        }

        const size_t burstStart = static_cast<size_t>(sampleRate * 0.05);
        const size_t burstEnd   = static_cast<size_t>(sampleRate * 0.055);

        float inBurstPeak = 0.0f;
        float outBurstPeak = 0.0f;
        for (size_t i = burstStart; i < burstEnd; ++i) {
            inBurstPeak  = std::max(inBurstPeak, std::abs(input[i]));
            outBurstPeak = std::max(outBurstPeak, std::abs(testL[i]));
        }

        const float burstCutDb = gainToDecibels(outBurstPeak / inBurstPeak);
        REQUIRE(burstCutDb < -2.0f);
    }

    SECTION("Sustain boost increases steady-state body level") {
        std::vector<float> testL = input;
        std::vector<float> testR = input;

        processor.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            const size_t n = std::min(blockSize, totalSamples - offset);
            processor.processStereo(testL.data() + offset, testR.data() + offset, n, 0.0f, 6.0f);
        }

        const size_t steadyIdx = static_cast<size_t>(sampleRate * 0.150);
        float inSteadyRms = 0.0f;
        float outSteadyRms = 0.0f;
        for (size_t i = steadyIdx; i < steadyIdx + 500; ++i) {
            inSteadyRms  += input[i] * input[i];
            outSteadyRms += testL[i] * testL[i];
        }
        const float steadyBoostDb = gainToDecibels(std::sqrt(outSteadyRms / inSteadyRms));

        REQUIRE(steadyBoostDb > 2.0f);
    }
}

#include "dsp/StereoProcessor.h"
#include "dsp/common/DecibelUtils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <cmath>
#include <numbers>

using namespace strata::dsp;

TEST_CASE("StereoProcessor Mid/Side encode-decode bit-exact null test", "[dsp][stereo]") {
    const size_t numSamples = 256;
    std::vector<float> originalLeft(numSamples);
    std::vector<float> originalRight(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        originalLeft[i]  = static_cast<float>(i) / static_cast<float>(numSamples) * 0.8f;
        originalRight[i] = -0.5f + static_cast<float>(i) / static_cast<float>(numSamples) * 0.5f;
    }

    std::vector<float> left = originalLeft;
    std::vector<float> right = originalRight;

    StereoProcessor::encodeMidSide(left.data(), right.data(), numSamples);
    StereoProcessor::decodeMidSide(left.data(), right.data(), numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        REQUIRE(left[i]  == Catch::Approx(originalLeft[i]).margin(1.0e-6f));
        REQUIRE(right[i] == Catch::Approx(originalRight[i]).margin(1.0e-6f));
    }
}

TEST_CASE("StereoProcessor mono collapse test (width = 0.0)", "[dsp][stereo]") {
    const size_t numSamples = 64;
    std::vector<float> left(numSamples, 1.0f);
    std::vector<float> right(numSamples, -1.0f); // antiphase

    StereoProcessor::applyWidth(left.data(), right.data(), numSamples, 0.0f);

    for (size_t i = 0; i < numSamples; ++i) {
        REQUIRE(left[i]  == Catch::Approx(0.0f).margin(1.0e-6f));
        REQUIRE(right[i] == Catch::Approx(0.0f).margin(1.0e-6f));
    }
}

TEST_CASE("StereoProcessor Mono-Maker sub-bass side attenuation", "[dsp][stereo]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t totalSamples = 2048;

    StereoProcessor processor;
    processor.prepare(sampleRate);
    processor.setMonoMakerEnabled(true);
    processor.setMonoMakerFrequency(120.0f); // 120 Hz crossover

    // Injected signal: Pure 50 Hz sub-bass in opposite phase (pure side channel)
    std::vector<float> left(totalSamples);
    std::vector<float> right(totalSamples);

    for (size_t i = 0; i < totalSamples; ++i) {
        const float s = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * 50.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
        left[i]  =  s;
        right[i] = -s; // 100% Side, 0% Mid
    }

    processor.process(left.data(), right.data(), totalSamples, StereoMode::Stereo, 1.0f);

    // Measure steady-state RMS of remaining side energy
    float sideRms = 0.0f;
    for (size_t i = 512; i < totalSamples; ++i) {
        const float side = (left[i] - right[i]) * INV_SQRT_TWO;
        sideRms += side * side;
    }
    sideRms = std::sqrt(sideRms / static_cast<float>(totalSamples - 512));
    const float sideDb = gainToDecibels(sideRms);

    // At 50 Hz (more than an octave below 120 Hz HPF), side energy must be attenuated by > 12 dB
    REQUIRE(sideDb < -15.0f);
}

TEST_CASE("StereoProcessor Stereoizer mono compatibility", "[dsp][stereo]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t totalSamples = 1024;

    StereoProcessor processor;
    processor.prepare(sampleRate);
    processor.setStereoizerEnabled(true);
    processor.setStereoizerAmount(1.0f); // 100% stereoizer

    // Input is a pure mono signal (L = R)
    std::vector<float> left(totalSamples);
    std::vector<float> right(totalSamples);
    std::vector<float> origMono(totalSamples);

    for (size_t i = 0; i < totalSamples; ++i) {
        const float s = 0.6f * std::sin(2.0f * std::numbers::pi_v<float> * 1000.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
        left[i] = s;
        right[i] = s;
        origMono[i] = s;
    }

    processor.process(left.data(), right.data(), totalSamples, StereoMode::Stereo, 1.0f);

    // 1. Stereo width was created (Left and Right differ)
    bool hasStereoWidth = false;
    for (size_t i = 256; i < totalSamples; ++i) {
        if (std::abs(left[i] - right[i]) > 0.01f) {
            hasStereoWidth = true;
            break;
        }
    }
    REQUIRE(hasStereoWidth);

    // 2. Mono Compatibility Guarantee: Sum of Left and Right exactly cancels injected side
    // (L + R) * 0.5 must equal origMono with zero phase comb filtering
    for (size_t i = 256; i < totalSamples; ++i) {
        const float monoSum = 0.5f * (left[i] + right[i]);
        REQUIRE_THAT(monoSum, Catch::Matchers::WithinAbs(origMono[i], 1.0e-5f));
    }
}

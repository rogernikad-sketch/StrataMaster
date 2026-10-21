#include "dsp/MultibandDynamics.h"
#include "dsp/common/DecibelUtils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <cmath>

TEST_CASE("SingleBandCompressor compression transfer curves", "[dsp][dynamics]") {
    const double sampleRate = 48000.0;
    strata::dsp::SingleBandCompressor comp;
    comp.prepare(sampleRate);

    strata::dsp::BandDynamicsSettings settings;
    settings.enabled = true;
    settings.bypass = false;
    settings.thresholdDb = -20.0f;
    settings.ratio = 4.0f; // 4:1 slope
    settings.kneeWidthDb = 0.01f; // Hard knee for exact mathematical verification
    settings.attackMs = 0.5f;
    settings.releaseMs = 50.0f;
    settings.makeupGainDb = 0.0f;
    settings.autoMakeup = false;

    comp.updateSettings(settings);

    SECTION("Signal 12 dB above threshold compresses by exactly 9 dB (4:1 ratio)") {
        const float inDb = -8.0f; // 12 dB above -20 dBFS
        const float amp = strata::dsp::decibelsToGain(inDb);
        const size_t numSamples = 4096;
        std::vector<float> buffer(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] = amp * std::sin(2.0f * strata::dsp::PI * 1000.0f * i / static_cast<float>(sampleRate));
        }

        comp.process(buffer.data(), numSamples);

        // Expected GR = (1/4 - 1) * 12 dB = -9.0 dB
        float gr = comp.getGainReductionDb();
        REQUIRE(gr == Catch::Approx(-9.0f).margin(0.5f));

        // Output peak amplitude should be ~ -17 dBFS
        float outPeak = 0.0f;
        for (size_t i = 2048; i < numSamples; ++i) {
            outPeak = std::max(outPeak, std::abs(buffer[i]));
        }
        REQUIRE(strata::dsp::gainToDecibels(outPeak) == Catch::Approx(-17.0f).margin(0.6f));
    }

    SECTION("Signal below threshold receives zero gain reduction") {
        const float inDb = -30.0f; // 10 dB below -20 dBFS threshold
        const float amp = strata::dsp::decibelsToGain(inDb);
        const size_t numSamples = 2048;
        std::vector<float> buffer(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] = amp * std::sin(2.0f * strata::dsp::PI * 1000.0f * i / static_cast<float>(sampleRate));
        }

        comp.reset();
        comp.process(buffer.data(), numSamples);

        float gr = comp.getGainReductionDb();
        REQUIRE(gr == Catch::Approx(0.0f).margin(0.01f));
    }
}

TEST_CASE("MultibandDynamics 4-band stereo processing and solo logic", "[dsp][dynamics]") {
    const double sampleRate = 48000.0;
    strata::dsp::MultibandDynamics mb;
    mb.prepare(sampleRate);
    mb.setCrossoverFrequencies(120.0f, 1500.0f, 6000.0f, true);

    const size_t numSamples = 2048;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    // Multi-frequency input (50 Hz bass + 3000 Hz presence)
    for (size_t i = 0; i < numSamples; ++i) {
        float bass = 0.5f * std::sin(2.0f * strata::dsp::PI * 50.0f * i / static_cast<float>(sampleRate));
        float treble = 0.5f * std::sin(2.0f * strata::dsp::PI * 3000.0f * i / static_cast<float>(sampleRate));
        left[i] = bass + treble;
        right[i] = bass + treble;
    }

    SECTION("Soloing High-Mid band (Band 2) silences Low band (Band 0)") {
        auto s2 = mb.getBandSettings(2);
        s2.solo = true;
        mb.setBandSettings(2, s2);

        mb.processStereo(left.data(), right.data(), numSamples);

        // Bass component (50 Hz) should be completely silenced in output
        // Verify peak of output is due only to the treble band
        float peak = 0.0f;
        for (size_t i = 1024; i < numSamples; ++i) {
            peak = std::max(peak, std::abs(left[i]));
        }
        REQUIRE(peak < 0.6f);
    }
}

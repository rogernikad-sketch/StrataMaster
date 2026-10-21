#include "dsp/BiquadFilter.h"
#include "dsp/DynamicEqualizer.h"
#include "dsp/common/DecibelUtils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <cmath>

TEST_CASE("BiquadFilter theoretical frequency response", "[dsp][biquad]") {
    const double sampleRate = 48000.0;

    SECTION("Bell / Peaking EQ at center frequency") {
        auto coeffs = strata::dsp::BiquadFilter::calculateCoefficients(
            strata::dsp::FilterType::Bell, sampleRate, 1000.0f, 6.0f, 1.0f);

        // Center frequency must be exactly +6.0 dB
        REQUIRE(coeffs.getMagnitudeResponseDb(1000.0, sampleRate) == Catch::Approx(6.0).margin(0.05));

        // DC and Nyquist should be close to 0 dB
        REQUIRE(coeffs.getMagnitudeResponseDb(20.0, sampleRate) == Catch::Approx(0.0).margin(0.2));
        REQUIRE(coeffs.getMagnitudeResponseDb(20000.0, sampleRate) == Catch::Approx(0.0).margin(0.2));
    }

    SECTION("Butterworth LowPass -3 dB at cutoff and 12 dB/oct rolloff") {
        auto coeffs = strata::dsp::BiquadFilter::calculateCoefficients(
            strata::dsp::FilterType::LowPass, sampleRate, 1000.0f, 0.0f, 0.7071f);

        // At cutoff, Butterworth is -3.01 dB
        REQUIRE(coeffs.getMagnitudeResponseDb(1000.0, sampleRate) == Catch::Approx(-3.01).margin(0.1));

        // 1 octave above cutoff (2000 Hz), 2nd order is ~ -12 dB down
        REQUIRE(coeffs.getMagnitudeResponseDb(2000.0, sampleRate) == Catch::Approx(-12.3).margin(0.5));

        // Passband (100 Hz) should be close to 0 dB
        REQUIRE(coeffs.getMagnitudeResponseDb(100.0, sampleRate) == Catch::Approx(0.0).margin(0.1));
    }

    SECTION("Butterworth HighPass -3 dB at cutoff") {
        auto coeffs = strata::dsp::BiquadFilter::calculateCoefficients(
            strata::dsp::FilterType::HighPass, sampleRate, 1000.0f, 0.0f, 0.7071f);

        REQUIRE(coeffs.getMagnitudeResponseDb(1000.0, sampleRate) == Catch::Approx(-3.01).margin(0.1));
        REQUIRE(coeffs.getMagnitudeResponseDb(500.0, sampleRate) == Catch::Approx(-12.3).margin(0.5));
        REQUIRE(coeffs.getMagnitudeResponseDb(10000.0, sampleRate) == Catch::Approx(0.0).margin(0.1));
    }

    SECTION("Notch filter deep attenuation at center") {
        auto coeffs = strata::dsp::BiquadFilter::calculateCoefficients(
            strata::dsp::FilterType::Notch, sampleRate, 1000.0f, 0.0f, 5.0f);

        // At center, notch should attenuate by > 40 dB
        REQUIRE(coeffs.getMagnitudeResponseDb(1000.0, sampleRate) < -40.0);
        // Away from notch, gain is unity (0 dB)
        REQUIRE(coeffs.getMagnitudeResponseDb(200.0, sampleRate) == Catch::Approx(0.0).margin(0.1));
    }

    SECTION("Shelving filters") {
        auto lowShelf = strata::dsp::BiquadFilter::calculateCoefficients(
            strata::dsp::FilterType::LowShelf, sampleRate, 200.0f, 8.0f, 1.0f);
        REQUIRE(lowShelf.getMagnitudeResponseDb(30.0, sampleRate) == Catch::Approx(8.0).margin(0.2));
        REQUIRE(lowShelf.getMagnitudeResponseDb(5000.0, sampleRate) == Catch::Approx(0.0).margin(0.2));

        auto highShelf = strata::dsp::BiquadFilter::calculateCoefficients(
            strata::dsp::FilterType::HighShelf, sampleRate, 8000.0f, -6.0f, 1.0f);
        REQUIRE(highShelf.getMagnitudeResponseDb(16000.0, sampleRate) == Catch::Approx(-6.0).margin(0.2));
        REQUIRE(highShelf.getMagnitudeResponseDb(500.0, sampleRate) == Catch::Approx(0.0).margin(0.2));
    }
}

TEST_CASE("BiquadFilter impulse response and stability", "[dsp][biquad]") {
    const double sampleRate = 48000.0;
    strata::dsp::BiquadFilter filter;
    filter.prepare(sampleRate);
    filter.setParameters(strata::dsp::FilterType::Bell, 1000.0f, 6.0f, 2.0f, true);

    // Feed Dirac impulse
    std::vector<float> buffer(512, 0.0f);
    buffer[0] = 1.0f;

    filter.processBlock(buffer.data(), buffer.size());

    // Impulse response should be finite, stable, and decay to zero
    REQUIRE(std::isfinite(buffer[0]));
    REQUIRE(std::abs(buffer.back()) < 1.0e-4f);
}

TEST_CASE("DynamicEqualizer selective dynamic trigger and gain reduction", "[dsp][dynamiceq]") {
    const double sampleRate = 48000.0;
    strata::dsp::DynamicEqualizer eq;
    eq.prepare(sampleRate);

    // Configure Band 3 (Index 2): Bell at 1000 Hz, Dynamic Mode ON, Threshold = -20 dBFS, Range = -6 dB
    strata::dsp::DynamicBandSettings settings;
    settings.enabled = true;
    settings.type = strata::dsp::FilterType::Bell;
    settings.frequencyHz = 1000.0f;
    settings.gainDb = 0.0f;
    settings.q = 2.0f;
    settings.dynamicEnabled = true;
    settings.thresholdDb = -20.0f;
    settings.dynamicRangeDb = -6.0f;
    settings.attackMs = 2.0f;
    settings.releaseMs = 50.0f;

    eq.setBandSettings(2, settings, true);

    SECTION("Signal above threshold triggers dynamic gain reduction") {
        // Generate 1000 Hz sine at -6 dBFS (well above -20 dBFS threshold)
        const float amp = strata::dsp::decibelsToGain(-6.0f);
        const size_t numSamples = 2048;
        std::vector<float> left(numSamples);
        std::vector<float> right(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            float s = amp * std::sin(2.0f * strata::dsp::PI * 1000.0f * i / static_cast<float>(sampleRate));
            left[i] = s;
            right[i] = s;
        }

        eq.processStereo(left.data(), right.data(), numSamples);

        // Dynamic gain reduction must have engaged (negative dB)
        float gr = eq.getGainReductionDb(2, 0);
        REQUIRE(gr < -1.0f);
        REQUIRE(gr >= -6.0f);
    }

    SECTION("Signal below threshold does NOT trigger dynamic gain reduction") {
        // Generate 1000 Hz sine at -30 dBFS (below -20 dBFS threshold)
        const float amp = strata::dsp::decibelsToGain(-30.0f);
        const size_t numSamples = 1024;
        std::vector<float> left(numSamples);
        std::vector<float> right(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            float s = amp * std::sin(2.0f * strata::dsp::PI * 1000.0f * i / static_cast<float>(sampleRate));
            left[i] = s;
            right[i] = s;
        }

        eq.reset();
        eq.processStereo(left.data(), right.data(), numSamples);

        // Dynamic gain reduction remains 0 dB
        float gr = eq.getGainReductionDb(2, 0);
        REQUIRE(gr == Catch::Approx(0.0f).margin(0.01f));
    }

    SECTION("Frequency selectivity: out-of-band energy does not trigger 1000 Hz band") {
        // Generate 80 Hz bass sine at 0 dBFS
        const size_t numSamples = 1024;
        std::vector<float> left(numSamples);
        std::vector<float> right(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            float s = std::sin(2.0f * strata::dsp::PI * 80.0f * i / static_cast<float>(sampleRate));
            left[i] = s;
            right[i] = s;
        }

        eq.reset();
        eq.processStereo(left.data(), right.data(), numSamples);

        // 1000 Hz band detector should reject 80 Hz, GR remains close to 0 dB
        float gr = eq.getGainReductionDb(2, 0);
        REQUIRE(gr > -0.2f);
    }
}

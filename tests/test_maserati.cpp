#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/MaseratiProcessor.h"
#include <vector>
#include <cmath>
#include <numbers>

using namespace strata::dsp;

TEST_CASE("MaseratiProcessor tone shaping and activity response", "[dsp][maserati]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t numSamples = 2048;

    MaseratiProcessor maserati;
    maserati.prepare(sampleRate, 512);

    SECTION("Neutral settings pass audio cleanly") {
        maserati.setBass(0.0f);
        maserati.setTreble(0.0f);
        maserati.setSensitivity(0.0f);
        maserati.setEnabled(true);

        std::vector<float> left(numSamples, 0.2f);
        std::vector<float> right(numSamples, 0.2f);

        maserati.processStereo(left.data(), right.data(), numSamples);

        for (size_t i = 100; i < numSamples; ++i) {
            REQUIRE(std::isfinite(left[i]));
            REQUIRE(std::isfinite(right[i]));
            REQUIRE(std::abs(left[i]) > 0.05f);
        }
    }

    SECTION("Bass boost increases low-frequency amplitude") {
        // 60 Hz sine wave
        std::vector<float> leftFlat(numSamples);
        std::vector<float> rightFlat(numSamples);
        std::vector<float> leftBoost(numSamples);
        std::vector<float> rightBoost(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            const float s = 0.2f * std::sin(2.0f * std::numbers::pi_v<float> * 60.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
            leftFlat[i] = s;
            rightFlat[i] = s;
            leftBoost[i] = s;
            rightBoost[i] = s;
        }

        maserati.reset();
        maserati.setBass(0.0f);
        maserati.setTreble(0.0f);
        maserati.processStereo(leftFlat.data(), rightFlat.data(), numSamples);

        maserati.reset();
        maserati.setBass(6.0f);
        maserati.processStereo(leftBoost.data(), rightBoost.data(), numSamples);

        // Boosted output RMS should be measurably higher than flat RMS
        float rmsFlat = 0.0f;
        float rmsBoost = 0.0f;
        for (size_t i = 512; i < numSamples; ++i) {
            rmsFlat += leftFlat[i] * leftFlat[i];
            rmsBoost += leftBoost[i] * leftBoost[i];
        }

        REQUIRE(rmsBoost > rmsFlat * 1.3f);
    }

    SECTION("Treble boost increases high-frequency amplitude") {
        // 12 kHz sine wave
        std::vector<float> leftFlat(numSamples);
        std::vector<float> rightFlat(numSamples);
        std::vector<float> leftBoost(numSamples);
        std::vector<float> rightBoost(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            const float s = 0.2f * std::sin(2.0f * std::numbers::pi_v<float> * 12000.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
            leftFlat[i] = s;
            rightFlat[i] = s;
            leftBoost[i] = s;
            rightBoost[i] = s;
        }

        maserati.reset();
        maserati.setBass(0.0f);
        maserati.setTreble(0.0f);
        maserati.processStereo(leftFlat.data(), rightFlat.data(), numSamples);

        maserati.reset();
        maserati.setTreble(6.0f);
        maserati.processStereo(leftBoost.data(), rightBoost.data(), numSamples);

        float rmsFlat = 0.0f;
        float rmsBoost = 0.0f;
        for (size_t i = 512; i < numSamples; ++i) {
            rmsFlat += leftFlat[i] * leftFlat[i];
            rmsBoost += leftBoost[i] * leftBoost[i];
        }

        REQUIRE(rmsBoost > rmsFlat * 1.3f);
    }

    SECTION("Sensitivity envelope meter detects active audio") {
        maserati.reset();
        maserati.setEnabled(true);
        maserati.setSensitivity(0.0f);

        std::vector<float> left(numSamples, 0.5f);
        std::vector<float> right(numSamples, 0.5f);

        maserati.processStereo(left.data(), right.data(), numSamples);

        REQUIRE(maserati.getSensitivityActivity() > 0.01f);
    }

    SECTION("Bypassed state leaves buffer untouched") {
        maserati.reset();
        maserati.setEnabled(false);

        std::vector<float> leftOrig(numSamples, 0.35f);
        std::vector<float> rightOrig(numSamples, 0.35f);
        std::vector<float> left = leftOrig;
        std::vector<float> right = rightOrig;

        maserati.processStereo(left.data(), right.data(), numSamples);

        REQUIRE(left == leftOrig);
        REQUIRE(right == rightOrig);
        REQUIRE(maserati.getSensitivityActivity() == 0.0f);
    }
}


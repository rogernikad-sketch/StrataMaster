#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/TruePeakLimiter.h"
#include "dsp/SoftClipper.h"
#include "dsp/common/DecibelUtils.h"
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>

using namespace strata::dsp;

TEST_CASE("SoftClipper transfer curve and ceiling constraint", "[dsp][limiter]") {
    SoftClipper clipper;
    clipper.setEnabled(true);
    clipper.setCeilingDb(-0.5f); // Ceiling ~ 0.9441
    clipper.setKneeDb(2.0f);     // Knee start at -2.5 dB ~ 0.7499

    const float ceilingLinear = std::pow(10.0f, -0.5f / 20.0f);
    const float kneeLinear    = std::pow(10.0f, -2.5f / 20.0f);

    SECTION("Linear region below knee is bit-exact transparent") {
        constexpr float testVals[] = { 0.0f, 0.1f, 0.3f, 0.5f, 0.7f };
        for (float val : testVals) {
            REQUIRE(clipper.processSample(val) == val);
            REQUIRE(clipper.processSample(-val) == -val);
        }
    }

    SECTION("Soft knee smoothly compresses above knee") {
        const float midVal = (kneeLinear + ceilingLinear) * 0.5f;
        const float out = clipper.processSample(midVal);

        REQUIRE(out > kneeLinear);
        REQUIRE(out < midVal); // Compressed
    }

    SECTION("Absolute ceiling clamp on massive peaks") {
        constexpr float massivePeaks[] = { 1.5f, 2.0f, 5.0f, 10.0f };
        for (float p : massivePeaks) {
            const float outPos = clipper.processSample(p);
            const float outNeg = clipper.processSample(-p);

            REQUIRE_THAT(outPos, Catch::Matchers::WithinAbs(ceilingLinear, 1.0e-5f));
            REQUIRE_THAT(outNeg, Catch::Matchers::WithinAbs(-ceilingLinear, 1.0e-5f));
        }
    }
}

TEST_CASE("TruePeakLimiter brickwall ceiling and lookahead protection", "[dsp][limiter]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t blockSize = 256;
    constexpr size_t totalSamples = 8192;

    TruePeakLimiter limiter;
    limiter.prepare(sampleRate, blockSize);
    limiter.setCeilingDb(-0.1f); // -0.1 dBTP target
    const float ceilingLinear = std::pow(10.0f, -0.1f / 20.0f); // ~ 0.98855f

    SECTION("Heavy overdrive (+6 dBFS) never exceeds ceiling") {
        limiter.setThresholdDb(-6.0f); // +6 dB drive

        std::vector<float> inL(totalSamples);
        std::vector<float> inR(totalSamples);
        for (size_t i = 0; i < totalSamples; ++i) {
            const float s = 0.9f * std::sin(2.0f * std::numbers::pi_v<float> * 1000.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
            inL[i] = s;
            inR[i] = s;
        }

        limiter.reset();

        float maxOutputPeak = 0.0f;
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            limiter.processStereo(inL.data() + offset, inR.data() + offset, blockSize);
            for (size_t i = offset; i < offset + blockSize; ++i) {
                maxOutputPeak = std::max(maxOutputPeak, std::max(std::abs(inL[i]), std::abs(inR[i])));
            }
        }

        // Maximum output peak must NEVER exceed the ceiling
        REQUIRE(maxOutputPeak <= ceilingLinear + 1.0e-5f);
        // Limiter must register gain reduction
        REQUIRE(limiter.getGainReductionDb() < -4.0f);
    }

    SECTION("Inter-sample peak detection and containment") {
        // High-frequency alternating samples that peak higher between discrete sample points
        limiter.setThresholdDb(0.0f);
        limiter.setCeilingDb(-0.5f);
        const float targetCeiling = std::pow(10.0f, -0.5f / 20.0f);

        std::vector<float> testL(totalSamples, 0.0f);
        std::vector<float> testR(totalSamples, 0.0f);

        for (size_t i = 512; i < 2048; ++i) {
            // Sinc interpolation of consecutive 0.95 and -0.95 creates inter-sample peaks of ~ 1.3 (+2.3 dBFS)
            testL[i] = (i % 2 == 0) ? 0.95f : -0.95f;
            testR[i] = testL[i];
        }

        limiter.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            limiter.processStereo(testL.data() + offset, testR.data() + offset, blockSize);
        }

        float maxPeak = 0.0f;
        for (size_t i = 1024; i < 2048; ++i) {
            maxPeak = std::max(maxPeak, std::abs(testL[i]));
        }

        REQUIRE(maxPeak <= targetCeiling + 1.0e-5f);
    }

    SECTION("Latency reporting matches lookahead buffer") {
        limiter.setLookaheadMs(3.0f);
        const size_t expectedLatency = static_cast<size_t>(3.0 * 0.001 * sampleRate);
        REQUIRE(limiter.getLatencyInSamples() == expectedLatency);
    }
}

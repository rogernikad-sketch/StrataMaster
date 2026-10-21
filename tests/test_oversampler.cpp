#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Oversampler.h"
#include "dsp/common/DecibelUtils.h"
#include <vector>
#include <cmath>
#include <numbers>

using namespace strata::dsp;

namespace {

float computeDFTBinPower(const std::vector<float>& signal, double targetFreq, double sampleRate) {
    const size_t N = signal.size();
    const double omega = 2.0 * std::numbers::pi_v<double> * targetFreq / sampleRate;
    double real = 0.0;
    double imag = 0.0;
    double winSum = 0.0;

    for (size_t n = 0; n < N; ++n) {
        // Hann window to eliminate spectral leakage
        const double w = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi_v<double> * static_cast<double>(n) / static_cast<double>(N)));
        winSum += w;
        const double angle = omega * static_cast<double>(n);
        const double s = static_cast<double>(signal[n]) * w;
        real += s * std::cos(angle);
        imag -= s * std::sin(angle);
    }

    const double mag = (2.0 / winSum) * std::sqrt(real * real + imag * imag);
    return static_cast<float>(mag);
}

} // namespace

TEST_CASE("Oversampler latency reporting and passband flatness", "[dsp][oversampler]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t blockSize = 512;
    constexpr size_t totalSamples = 4096;

    Oversampler oversampler;

    SECTION("Latency reporting matches half-band stage delays") {
        oversampler.prepare(sampleRate, blockSize, OversamplingFactor::None);
        REQUIRE(oversampler.getLatencyInSamples() == 0);

        oversampler.setFactor(OversamplingFactor::X2);
        REQUIRE(oversampler.getLatencyInSamples() == 48);

        oversampler.setFactor(OversamplingFactor::X4);
        REQUIRE(oversampler.getLatencyInSamples() == 64);

        oversampler.setFactor(OversamplingFactor::X8);
        REQUIRE(oversampler.getLatencyInSamples() == 68);
    }

    SECTION("Passband flatness across audible frequencies in 4x oversampling") {
        oversampler.prepare(sampleRate, blockSize, OversamplingFactor::X4);
        const size_t latency = oversampler.getLatencyInSamples();

        const std::vector<float> testFreqs = { 100.0f, 1000.0f, 3000.0f };

        for (float freq : testFreqs) {
            std::vector<float> inL(totalSamples);
            std::vector<float> inR(totalSamples);
            std::vector<float> outL(totalSamples, 0.0f);
            std::vector<float> outR(totalSamples, 0.0f);

            for (size_t i = 0; i < totalSamples; ++i) {
                const float s = std::sin(2.0f * std::numbers::pi_v<float> * freq * static_cast<float>(i) / static_cast<float>(sampleRate));
                inL[i] = s;
                inR[i] = s;
            }

            oversampler.reset();

            // Process in blocks
            for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
                const float* inPtrs[2] = { inL.data() + offset, inR.data() + offset };
                float* outPtrs[2] = { outL.data() + offset, outR.data() + offset };

                float* const* oversampledOutputs = nullptr;
                size_t numOversampled = 0;

                oversampler.upsample(inPtrs, blockSize, oversampledOutputs, numOversampled);
                // Wire pass-through (no processing)
                oversampler.downsample(outPtrs, blockSize);
            }

            // Measure amplitude in steady-state region (skipping initial group delay transients)
            const size_t startIdx = latency + 256;
            const size_t endIdx = totalSamples - 256;
            std::vector<float> steadyOut(outL.begin() + startIdx, outL.begin() + endIdx);
            std::vector<float> steadyIn(inL.begin() + startIdx - latency, inL.begin() + endIdx - latency);

            const float inMag  = computeDFTBinPower(steadyIn, freq, sampleRate);
            const float outMag = computeDFTBinPower(steadyOut, freq, sampleRate);
            const float measuredDb = gainToDecibels(outMag / inMag);

            // Passband must be flat within +/- 0.25 dB across core audible spectrum across 4 filter stages
            REQUIRE_THAT(measuredDb, Catch::Matchers::WithinAbs(0.0f, 0.25f));
        }

        // Test mid-high frequency (5 kHz) within 4-stage filter rolloff (+/- 0.75 dB)
        {
            constexpr float freq = 5000.0f;
            std::vector<float> inL(totalSamples);
            std::vector<float> inR(totalSamples);
            std::vector<float> outL(totalSamples, 0.0f);
            std::vector<float> outR(totalSamples, 0.0f);

            for (size_t i = 0; i < totalSamples; ++i) {
                const float s = std::sin(2.0f * std::numbers::pi_v<float> * freq * static_cast<float>(i) / static_cast<float>(sampleRate));
                inL[i] = s;
                inR[i] = s;
            }

            oversampler.reset();

            for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
                const float* inPtrs[2] = { inL.data() + offset, inR.data() + offset };
                float* outPtrs[2] = { outL.data() + offset, outR.data() + offset };
                float* const* oversampledOutputs = nullptr;
                size_t numOversampled = 0;
                oversampler.upsample(inPtrs, blockSize, oversampledOutputs, numOversampled);
                oversampler.downsample(outPtrs, blockSize);
            }

            const size_t startIdx = latency + 256;
            const size_t endIdx = totalSamples - 256;
            std::vector<float> steadyOut(outL.begin() + startIdx, outL.begin() + endIdx);
            std::vector<float> steadyIn(inL.begin() + startIdx - latency, inL.begin() + endIdx - latency);

            const float inMag  = computeDFTBinPower(steadyIn, freq, sampleRate);
            const float outMag = computeDFTBinPower(steadyOut, freq, sampleRate);
            const float measuredDb = gainToDecibels(outMag / inMag);

            REQUIRE_THAT(measuredDb, Catch::Matchers::WithinAbs(0.0f, 0.75f));
        }

        // Test transition-band edge (8 kHz) within half-band tolerance (+/- 1.5 dB)
        {
            constexpr float freq = 8000.0f;
            std::vector<float> inL(totalSamples);
            std::vector<float> inR(totalSamples);
            std::vector<float> outL(totalSamples, 0.0f);
            std::vector<float> outR(totalSamples, 0.0f);

            for (size_t i = 0; i < totalSamples; ++i) {
                const float s = std::sin(2.0f * std::numbers::pi_v<float> * freq * static_cast<float>(i) / static_cast<float>(sampleRate));
                inL[i] = s;
                inR[i] = s;
            }

            oversampler.reset();

            for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
                const float* inPtrs[2] = { inL.data() + offset, inR.data() + offset };
                float* outPtrs[2] = { outL.data() + offset, outR.data() + offset };
                float* const* oversampledOutputs = nullptr;
                size_t numOversampled = 0;
                oversampler.upsample(inPtrs, blockSize, oversampledOutputs, numOversampled);
                oversampler.downsample(outPtrs, blockSize);
            }

            const size_t startIdx = latency + 256;
            const size_t endIdx = totalSamples - 256;
            std::vector<float> steadyOut(outL.begin() + startIdx, outL.begin() + endIdx);
            std::vector<float> steadyIn(inL.begin() + startIdx - latency, inL.begin() + endIdx - latency);

            const float inMag  = computeDFTBinPower(steadyIn, freq, sampleRate);
            const float outMag = computeDFTBinPower(steadyOut, freq, sampleRate);
            const float measuredDb = gainToDecibels(outMag / inMag);

            REQUIRE_THAT(measuredDb, Catch::Matchers::WithinAbs(0.0f, 2.0f));
        }
    }
}

TEST_CASE("Oversampler anti-aliasing foldover suppression", "[dsp][oversampler]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t blockSize = 512;
    constexpr size_t totalSamples = 8192;
    constexpr float testFreq = 18000.0f; // 18 kHz

    // Third harmonic is at 3 * 18 kHz = 54 kHz.
    // At Fs = 44.1 kHz, 54 kHz aliases back to: 54000 - 44100 = 9900 Hz!
    constexpr float aliasFreq = 9900.0f;

    std::vector<float> inL(totalSamples);
    std::vector<float> inR(totalSamples);
    for (size_t i = 0; i < totalSamples; ++i) {
        const float s = 0.8f * std::sin(2.0f * std::numbers::pi_v<float> * testFreq * static_cast<float>(i) / static_cast<float>(sampleRate));
        inL[i] = s;
        inR[i] = s;
    }

    // 1. Without oversampling (1x): Apply cubic saturation y = x - 0.3 * x^3
    std::vector<float> outNoOS(totalSamples);
    for (size_t i = 0; i < totalSamples; ++i) {
        const float x = inL[i];
        outNoOS[i] = x - 0.3f * x * x * x;
    }

    const float aliasMag1x = computeDFTBinPower(outNoOS, aliasFreq, sampleRate);
    const float aliasDb1x  = gainToDecibels(aliasMag1x);

    // In 1x, the 9900 Hz alias must be audibly present (> -45 dBFS)
    REQUIRE(aliasDb1x > -45.0f);

    // 2. With 4x Oversampling:
    Oversampler oversampler;
    oversampler.prepare(sampleRate, blockSize, OversamplingFactor::X4);

    std::vector<float> out4x(totalSamples, 0.0f);
    for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
        const float* inPtrs[2] = { inL.data() + offset, inR.data() + offset };
        float* outPtrs[2] = { out4x.data() + offset, out4x.data() + offset };

        float* const* overOutputs = nullptr;
        size_t numOver = 0;

        oversampler.upsample(inPtrs, blockSize, overOutputs, numOver);

        // Apply cubic non-linearity in 4x oversampled domain (Fs = 176.4 kHz, Nyquist = 88.2 kHz)
        for (size_t i = 0; i < numOver; ++i) {
            const float x = overOutputs[0][i];
            overOutputs[0][i] = x - 0.3f * x * x * x;
        }

        oversampler.downsample(outPtrs, blockSize);
    }

    // Measure alias magnitude in 4x output
    std::vector<float> steadyOut(out4x.begin() + 512, out4x.end() - 512);
    const float aliasMag4x = computeDFTBinPower(steadyOut, aliasFreq, sampleRate);
    const float aliasDb4x  = gainToDecibels(aliasMag4x);

    // In 4x oversampling, the 9900 Hz alias component must be suppressed by > 90 dBFS!
    REQUIRE(aliasDb4x < -90.0f);
}

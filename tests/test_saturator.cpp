#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Saturator.h"
#include "dsp/common/DecibelUtils.h"
#include <vector>
#include <cmath>
#include <numbers>

using namespace strata::dsp;

namespace {

float computeHarmonicMag(const std::vector<float>& signal, double freq, double sampleRate) {
    const size_t N = signal.size();
    const double omega = 2.0 * std::numbers::pi_v<double> * freq / sampleRate;
    double real = 0.0;
    double imag = 0.0;
    double winSum = 0.0;

    for (size_t n = 0; n < N; ++n) {
        // Hann window to eliminate spectral leakage of non-integer cycles
        const double w = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi_v<double> * static_cast<double>(n) / static_cast<double>(N)));
        winSum += w;
        const double angle = omega * static_cast<double>(n);
        const double s = static_cast<double>(signal[n]) * w;
        real += s * std::cos(angle);
        imag -= s * std::sin(angle);
    }

    return static_cast<float>((2.0 / winSum) * std::sqrt(real * real + imag * imag));
}

} // namespace

TEST_CASE("Saturator harmonic profile verification", "[dsp][saturator]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t blockSize = 512;
    constexpr size_t totalSamples = 8192;
    constexpr float fundamentalFreq = 1000.0f; // 1 kHz

    Saturator saturator;
    saturator.prepare(sampleRate, blockSize, OversamplingFactor::X4);

    std::vector<float> inL(totalSamples);
    std::vector<float> inR(totalSamples);
    for (size_t i = 0; i < totalSamples; ++i) {
        const float s = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * fundamentalFreq * static_cast<float>(i) / static_cast<float>(sampleRate));
        inL[i] = s;
        inR[i] = s;
    }

    SECTION("Tape mode produces predominantly 3rd harmonic with zero even harmonics") {
        std::vector<float> outL = inL;
        std::vector<float> outR = inR;

        saturator.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            saturator.processStereo(outL.data() + offset, outR.data() + offset, blockSize,
                                    SaturationMode::Tape, 0.7f, 1.0f);
        }

        std::vector<float> steady(outL.begin() + 1024, outL.end() - 1024);
        const float f2Mag = computeHarmonicMag(steady, 2000.0, sampleRate);
        const float f3Mag = computeHarmonicMag(steady, 3000.0, sampleRate);

        const float f2Db = gainToDecibels(f2Mag);
        const float f3Db = gainToDecibels(f3Mag);

        // 3rd harmonic must be generated strongly
        REQUIRE(f3Db > -35.0f);
        // 2nd harmonic must be virtually zero due to symmetric clipping (less than -65 dBFS)
        REQUIRE(f2Db < -65.0f);
    }

    SECTION("Tube mode produces rich asymmetric 2nd harmonic") {
        std::vector<float> outL = inL;
        std::vector<float> outR = inR;

        saturator.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            saturator.processStereo(outL.data() + offset, outR.data() + offset, blockSize,
                                    SaturationMode::Tube, 0.8f, 1.0f);
        }

        std::vector<float> steady(outL.begin() + 1024, outL.end() - 1024);
        const float f2Mag = computeHarmonicMag(steady, 2000.0, sampleRate);
        const float f2Db = gainToDecibels(f2Mag);

        // 2nd harmonic (octave) must be clearly prominent
        REQUIRE(f2Db > -35.0f);
    }

    SECTION("Warm mode produces smooth even-order coloration") {
        std::vector<float> outL = inL;
        std::vector<float> outR = inR;

        saturator.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            saturator.processStereo(outL.data() + offset, outR.data() + offset, blockSize,
                                    SaturationMode::Warm, 0.8f, 1.0f);
        }

        std::vector<float> steady(outL.begin() + 1024, outL.end() - 1024);
        const float f2Mag = computeHarmonicMag(steady, 2000.0, sampleRate);
        const float f2Db = gainToDecibels(f2Mag);

        REQUIRE(f2Db > -40.0f);
    }

    SECTION("Retro mode produces transistor soft saturation") {
        std::vector<float> outL = inL;
        std::vector<float> outR = inR;

        saturator.reset();
        for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
            saturator.processStereo(outL.data() + offset, outR.data() + offset, blockSize,
                                    SaturationMode::Retro, 0.6f, 1.0f);
        }

        std::vector<float> steady(outL.begin() + 1024, outL.end() - 1024);
        const float f3Mag = computeHarmonicMag(steady, 3000.0, sampleRate);
        const float f3Db = gainToDecibels(f3Mag);

        REQUIRE(f3Db > -35.0f);
    }
}

TEST_CASE("Saturator mix and delay compensation", "[dsp][saturator]") {
    constexpr double sampleRate = 44100.0;
    constexpr size_t blockSize = 512;
    constexpr size_t totalSamples = 4096;

    Saturator saturator;
    saturator.prepare(sampleRate, blockSize, OversamplingFactor::X4);
    const size_t latency = saturator.getLatencyInSamples();

    std::vector<float> inL(totalSamples);
    std::vector<float> inR(totalSamples);
    for (size_t i = 0; i < totalSamples; ++i) {
        const float s = std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
        inL[i] = s;
        inR[i] = s;
    }

    std::vector<float> outL = inL;
    std::vector<float> outR = inR;

    saturator.reset();
    for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
        // Wet = 0.0f (pure delay-compensated dry)
        saturator.processStereo(outL.data() + offset, outR.data() + offset, blockSize,
                                SaturationMode::Tape, 0.9f, 0.0f);
    }

    // Verify output matches delayed input
    for (size_t i = latency + 128; i < totalSamples - 128; ++i) {
        const float dryDelayed = inL[i - latency];
        const float wetOutput = outL[i];
        REQUIRE_THAT(wetOutput, Catch::Matchers::WithinAbs(dryDelayed, 1.0e-4f));
    }
}

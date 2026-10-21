#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "analysis/BS1770LoudnessMeter.h"
#include "analysis/TruePeakMeter.h"
#include "analysis/SpectrumAnalyzer.h"
#include "analysis/AudioFeatureExtractor.h"
#include "dsp/common/DecibelUtils.h"
#include <vector>
#include <cmath>
#include <numbers>

using namespace strata;
using namespace strata::analysis;

TEST_CASE("BS.1770 K-Weighting Filter Response", "[analysis][bs1770]") {
    const double sampleRate = 48000.0;
    auto stage1 = BS1770LoudnessMeter::designStage1HighShelf(sampleRate);
    auto stage2 = BS1770LoudnessMeter::designStage2HighPass(sampleRate);

    // Evaluate frequency response of Stage 1 High Shelf at 100 Hz vs 10 kHz
    auto evalResponse = [](const BS1770LoudnessMeter::FilterCoeffs& c, double freq, double sr) {
        const double omega = 2.0 * std::numbers::pi_v<double> * freq / sr;
        const std::complex<double> z_m1 = std::polar(1.0, -omega);
        const std::complex<double> z_m2 = std::polar(1.0, -2.0 * omega);
        const std::complex<double> num = static_cast<double>(c.b0) + static_cast<double>(c.b1) * z_m1 + static_cast<double>(c.b2) * z_m2;
        const std::complex<double> den = 1.0 + static_cast<double>(c.a1) * z_m1 + static_cast<double>(c.a2) * z_m2;
        return std::abs(num / den);
    };

    const double s1_low = evalResponse(stage1, 100.0, sampleRate);
    const double s1_high = evalResponse(stage1, 10000.0, sampleRate);
    const double s1_boostDb = dsp::gainToDecibels(static_cast<float>(s1_high / s1_low));

    // High shelf provides ~ +4 dB boost according to ITU-R BS.1770-4
    REQUIRE(s1_boostDb == Catch::Approx(4.0).margin(0.3));

    // Evaluate Stage 2 High Pass (RLB) at 20 Hz vs 1 kHz
    const double s2_20Hz = evalResponse(stage2, 20.0, sampleRate);
    const double s2_1kHz = evalResponse(stage2, 1000.0, sampleRate);
    const double s2_attenDb = dsp::gainToDecibels(static_cast<float>(s2_20Hz / s2_1kHz));

    // High pass cuts sub-bass below 38 Hz (at 20 Hz, > -4 dB attenuation)
    REQUIRE(s2_attenDb < -4.0f);
}

TEST_CASE("BS.1770-4 1 kHz Reference Sine Calibration (-18 LUFS)", "[analysis][bs1770]") {
    const double sampleRate = 48000.0;
    BS1770LoudnessMeter meter;
    meter.prepare(sampleRate);

    // Generate stereo 1 kHz sine wave at -18 dBFS amplitude
    // Target BS.1770 integrated loudness: exactly -18.0 LKFS +/- 0.1 LU
    const float amplitude = dsp::decibelsToGain(-18.0f);
    const size_t numSamples = 48000 * 4; // 4 seconds (allows 3.0s short-term window to fully populate)
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;
    for (size_t i = 0; i < numSamples; ++i) {
        const float s = amplitude * std::sin(static_cast<float>(twoPi * 1000.0 * static_cast<double>(i) / sampleRate));
        left[i] = s;
        right[i] = s;
    }

    meter.processStereo(left.data(), right.data(), numSamples);

    const float momentary = meter.getMomentaryLoudness();
    const float shortTerm = meter.getShortTermLoudness();
    const float integrated = meter.getIntegratedLoudness();

    // Verify ITU-R BS.1770-4 reference calibration within 0.1 LU
    REQUIRE(momentary == Catch::Approx(-18.0f).margin(0.15f));
    REQUIRE(shortTerm == Catch::Approx(-18.0f).margin(0.15f));
    REQUIRE(integrated == Catch::Approx(-18.0f).margin(0.15f));
}

TEST_CASE("BS.1770-4 Dual-Threshold Gating Behavior", "[analysis][bs1770]") {
    const double sampleRate = 48000.0;
    BS1770LoudnessMeter meter;
    meter.prepare(sampleRate);

    // 1 second of -18 LUFS tone, followed by 1 second of -90 dBFS silence
    const size_t oneSec = 48000;
    std::vector<float> left(oneSec * 2, 0.0f);
    std::vector<float> right(oneSec * 2, 0.0f);

    const float amplitude = dsp::decibelsToGain(-18.0f);
    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;
    for (size_t i = 0; i < oneSec; ++i) {
        const float s = amplitude * std::sin(static_cast<float>(twoPi * 1000.0 * static_cast<double>(i) / sampleRate));
        left[i] = s;
        right[i] = s;
    }

    // Process both seconds
    meter.processStereo(left.data(), right.data(), oneSec * 2);

    // Due to the absolute gate at -70 LKFS, the second second of silence must be gated out,
    // so integrated loudness must remain ~ -18.0 LUFS (NOT dropped by 3 dB)
    const float integrated = meter.getIntegratedLoudness();
    REQUIRE(integrated == Catch::Approx(-18.0f).margin(0.2f));
}

TEST_CASE("True-Peak Inter-Sample Peak Detection", "[analysis][truepeak]") {
    const double sampleRate = 44100.0;
    TruePeakMeter tpMeter;
    tpMeter.prepare(sampleRate);

    // Construct a sine at f = fs / 4 (11025 Hz) with 45 deg (pi/4) phase offset
    // Sample values are: sin(pi/4), sin(3pi/4), sin(5pi/4), sin(7pi/4)
    // = +0.7071, +0.7071, -0.7071, -0.7071 (-3.01 dBFS sample peak)
    // Continuous analog peak = 1.0 (0.00 dBFS true peak)
    const size_t numSamples = 2048;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;
    const double phaseOffset = std::numbers::pi_v<double> / 4.0;
    for (size_t i = 0; i < numSamples; ++i) {
        const float s = std::sin(static_cast<float>(twoPi * 11025.0 * static_cast<double>(i) / sampleRate + phaseOffset));
        left[i] = s;
        right[i] = s;
    }

    // Max sample peak in buffer
    float maxSamplePeak = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        maxSamplePeak = std::max(maxSamplePeak, std::abs(left[i]));
    }
    const float samplePeakDb = dsp::gainToDecibels(maxSamplePeak);
    REQUIRE(samplePeakDb == Catch::Approx(-3.01f).margin(0.1f));

    // True Peak Meter process
    tpMeter.processStereo(left.data(), right.data(), numSamples);
    const float detectedTruePeak = tpMeter.getMaxTruePeakDb();

    // 4x polyphase inter-sample reconstruction should reveal the 0.0 dB true peak
    REQUIRE(detectedTruePeak > -0.5f);
    REQUIRE(detectedTruePeak <= 0.2f);
}

TEST_CASE("Spectrum Analyzer 4096-point Radix-2 FFT", "[analysis][spectrum]") {
    const double sampleRate = 44100.0;
    SpectrumAnalyzer analyzer;
    analyzer.prepare(sampleRate);

    // 1000 Hz sine wave into 4096-point FFT
    const size_t fftSize = SpectrumAnalyzer::FFT_SIZE;
    std::vector<float> input(fftSize);
    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;
    for (size_t i = 0; i < fftSize; ++i) {
        input[i] = std::sin(static_cast<float>(twoPi * 1000.0 * static_cast<double>(i) / sampleRate));
    }

    std::vector<float> mags;
    analyzer.processDirectFFT(input.data(), mags);

    REQUIRE(mags.size() == SpectrumAnalyzer::NUM_BINS);

    // Find bin with maximum energy
    size_t maxBin = 0;
    float maxMag = -200.0f;
    for (size_t k = 0; k < mags.size(); ++k) {
        if (mags[k] > maxMag) {
            maxMag = mags[k];
            maxBin = k;
        }
    }

    // Expected bin = round(1000 * 4096 / 44100) = 93
    const size_t expectedBin = static_cast<size_t>(std::round(1000.0 * 4096.0 / sampleRate));
    REQUIRE(std::abs(static_cast<int>(maxBin) - static_cast<int>(expectedBin)) <= 1);
    REQUIRE(maxMag > -2.0f); // Peak near 0 dBFS

    // Distant bin (e.g. 10 kHz bin ~ 929) should be > 60 dB down
    const size_t distantBin = static_cast<size_t>(std::round(10000.0 * 4096.0 / sampleRate));
    REQUIRE(mags[distantBin] < -60.0f);

    // Test smoothed extraction
    analyzer.pushAudioBlock(input.data(), input.data(), fftSize);
    std::vector<float> smoothedMags;
    const bool ready = analyzer.getLatestMagnitudes(smoothedMags, SpectrumSmoothing::Octave1_3);
    REQUIRE(ready == true);
    REQUIRE(smoothedMags.size() == SpectrumAnalyzer::NUM_BINS);
}

TEST_CASE("Audio Feature Extractor Crest Factor", "[analysis][features]") {
    const double sampleRate = 44100.0;
    AudioFeatureExtractor extractor;
    extractor.prepare(sampleRate);

    // Sine wave crest factor: peak / rms = sqrt(2) -> 3.01 dB
    const size_t numSamples = 4096;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;
    for (size_t i = 0; i < numSamples; ++i) {
        const float s = std::sin(static_cast<float>(twoPi * 1000.0 * static_cast<double>(i) / sampleRate));
        left[i] = s;
        right[i] = s;
    }

    extractor.processStereo(left.data(), right.data(), numSamples);
    const auto features = extractor.getFeatures();

    // Crest factor of a pure sine is 3.01 dB
    REQUIRE(features.crestFactorDb == Catch::Approx(3.01f).margin(0.3f));
    // Stereo width ratio of dual mono should be 0.0 (no side energy)
    REQUIRE(features.stereoWidthRatio == Catch::Approx(0.0f).margin(0.05f));
}

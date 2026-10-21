#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "assistant/MasteringProfiles.h"
#include "assistant/AutoMasterEngine.h"
#include "assistant/ReferenceMatcher.h"
#include "dsp/common/DecibelUtils.h"
#include <numbers>
#include <vector>

using namespace strata;
using namespace strata::assistant;

TEST_CASE("MasteringProfiles Specification Integrity", "[assistant][profiles]") {
    const auto streaming = TargetProfile::getProfile(GenreProfile::StreamingStandard);
    REQUIRE(streaming.targetIntegratedLufs == Catch::Approx(-14.0f));
    REQUIRE(streaming.targetTruePeakCeilingDb == Catch::Approx(-1.0f));
    REQUIRE(streaming.monoMakerFrequencyHz == Catch::Approx(80.0f));
    REQUIRE(streaming.targetSpectralCurveDb.size() == 8);

    const auto edm = TargetProfile::getProfile(GenreProfile::ClubEDM);
    REQUIRE(edm.targetIntegratedLufs == Catch::Approx(-7.0f));
    REQUIRE(edm.targetTruePeakCeilingDb == Catch::Approx(-0.1f));
    REQUIRE(edm.targetStereoWidth > 110.0f);

    const auto acoustic = TargetProfile::getProfile(GenreProfile::AcousticWarm);
    REQUIRE(acoustic.targetIntegratedLufs == Catch::Approx(-16.0f));
    REQUIRE(acoustic.targetCrestFactorDb > 10.0f);
}

TEST_CASE("AutoMasterEngine Spectral Corrections & Limiter Calculation", "[assistant][automaster]") {
    // 1. Spectral Correction Test
    // Suppose measured has heavy bass (+8 dB at 32Hz, +6 dB at 64Hz) and deficient highs (-8 dB at 12kHz, -10 dB at 16kHz)
    // Relative to 1 kHz (0 dB)
    std::array<float, 8> measuredLevels = { +8.0f, +6.0f, +1.0f, 0.0f, -1.0f, -3.0f, -8.0f, -10.0f };
    // Streaming target curve: { +2.0f, +1.5f, +0.5f, 0.0f, -0.5f, -1.0f, -2.0f, -3.5f }
    const auto streaming = TargetProfile::getProfile(GenreProfile::StreamingStandard);

    const auto corrections = AutoMasterEngine::calculateEqCorrections(measuredLevels, streaming.targetSpectralCurveDb);

    // Sub bass (32 Hz): target +2, measured +8 -> diff = -6.0 dB cut
    REQUIRE(corrections[0] == Catch::Approx(-6.0f).margin(0.1f));
    // Bass (64 Hz): target +1.5, measured +6 -> diff = -4.5 dB cut
    REQUIRE(corrections[1] == Catch::Approx(-4.5f).margin(0.1f));
    // Air (12 kHz): target -2.0, measured -8.0 -> diff = +6.0 dB boost (clamped)
    REQUIRE(corrections[6] == Catch::Approx(6.0f).margin(0.1f));

    // 2. Real-Time Listening Simulation
    const double sampleRate = 48000.0;
    AutoMasterEngine engine;
    engine.prepare(sampleRate);
    engine.setTargetGenre(GenreProfile::StreamingStandard);

    REQUIRE(engine.getState() == AssistantState::Idle);
    engine.startListening();
    REQUIRE(engine.getState() == AssistantState::Listening);

    // Generate stereo 1 kHz sine at -20 dBFS amplitude (~ -20 LUFS)
    const size_t blockSize = 480; // 10 ms blocks
    const size_t totalSamples = 48000 * 5; // 5 seconds
    std::vector<float> left(blockSize);
    std::vector<float> right(blockSize);
    const float amp = dsp::decibelsToGain(-20.0f);
    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;

    for (size_t s = 0; s < totalSamples; s += blockSize) {
        for (size_t i = 0; i < blockSize; ++i) {
            const double t = static_cast<double>(s + i) / sampleRate;
            const float val = amp * std::sin(static_cast<float>(twoPi * 1000.0 * t));
            left[i] = val;
            right[i] = val;
        }
        engine.processBlock(left.data(), right.data(), blockSize);
    }

    REQUIRE(engine.getState() == AssistantState::Ready);
    REQUIRE(engine.getListeningProgressPercent() == Catch::Approx(100.0f));

    const auto& rec = engine.getRecommendations();
    // Measured LUFS was ~ -20. Target is -14. Delta = +6 dB.
    // Recommended limiter threshold should be around -6.0 dB
    REQUIRE(rec.limiterThresholdDb == Catch::Approx(-6.0f).margin(0.5f));
    REQUIRE(rec.limiterCeilingDb == Catch::Approx(-1.0f));
    REQUIRE(rec.monoMakerFreqHz == Catch::Approx(80.0f));
}

TEST_CASE("ReferenceMatcher Match Curve Computation", "[assistant][matcher]") {
    // Master track spectrum (8 bands, normalized at 1 kHz index 3 = 0.0)
    std::array<float, 8> masterSpectrum    = { +1.0f, +1.0f, 0.0f, 0.0f, -1.0f, -2.0f, -4.0f, -6.0f };
    // Reference track spectrum (more sub bass, brighter top end)
    std::array<float, 8> referenceSpectrum = { +4.0f, +3.0f, 0.0f, 0.0f,  0.0f, +1.0f, -1.0f, -2.0f };

    // 100% Match
    const auto fullMatch = ReferenceMatcher::computeMatchCurve(masterSpectrum, referenceSpectrum, 1.0f);
    // Band 0 (32 Hz): +4 - (+1) = +3.0 dB
    REQUIRE(fullMatch[0] == Catch::Approx(3.0f).margin(0.05f));
    // Band 1 (64 Hz): +3 - (+1) = +2.0 dB
    REQUIRE(fullMatch[1] == Catch::Approx(2.0f).margin(0.05f));
    // Band 5 (6 kHz): +1 - (-2) = +3.0 dB
    REQUIRE(fullMatch[5] == Catch::Approx(3.0f).margin(0.05f));
    // Band 6 (12 kHz): -1 - (-4) = +3.0 dB
    REQUIRE(fullMatch[6] == Catch::Approx(3.0f).margin(0.05f));

    // 50% Match (Gentler mastering touch)
    const auto halfMatch = ReferenceMatcher::computeMatchCurve(masterSpectrum, referenceSpectrum, 0.5f);
    REQUIRE(halfMatch[0] == Catch::Approx(1.5f).margin(0.05f));
    REQUIRE(halfMatch[1] == Catch::Approx(1.0f).margin(0.05f));
    REQUIRE(halfMatch[5] == Catch::Approx(1.5f).margin(0.05f));
}

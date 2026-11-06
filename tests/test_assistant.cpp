#if __has_include("catch2/catch_test_macros.hpp")
#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_approx.hpp"
#else
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#endif
#include "assistant/MasteringProfiles.h"
#include "assistant/AutoMasterEngine.h"
#include "assistant/ReferenceMatcher.h"
#include "engine/StrataAudioProcessor.h"
#include "engine/ParameterIDs.h"
#include "dsp/common/DecibelUtils.h"
#include <numbers>
#include <vector>
#include <set>
#include <cmath>

using namespace strata;
using namespace strata::assistant;

TEST_CASE("MasteringProfiles Complete 35-Target Library Integrity", "[assistant][profiles]") {
    const auto& all = TargetProfile::getAllProfiles();
    // At least 35 targets (plus optional legacy aliases)
    REQUIRE(all.size() >= 35);

    std::set<std::string> names;
    std::set<GenreProfile> genres;

    for (const auto& p : all) {
        REQUIRE_FALSE(p.name.empty());
        REQUIRE_FALSE(p.category.empty());
        names.insert(p.name);
        genres.insert(p.genre);

        // Sanity checks on mastering acoustics
        REQUIRE(p.targetIntegratedLufs >= -24.0f);
        REQUIRE(p.targetIntegratedLufs <= -5.0f);
        REQUIRE(p.targetTruePeakCeilingDb >= -2.0f);
        REQUIRE(p.targetTruePeakCeilingDb <= -0.1f);
        REQUIRE(p.targetCrestFactorDb >= 4.0f);
        REQUIRE(p.targetCrestFactorDb <= 16.0f);
        REQUIRE(p.targetStereoWidth >= 100.0f);
        REQUIRE(p.targetStereoWidth <= 140.0f);
        REQUIRE(p.monoMakerFrequencyHz >= 40.0f);
        REQUIRE(p.monoMakerFrequencyHz <= 140.0f);

        REQUIRE(p.targetSpectralCurveDb.size() == 8);
        REQUIRE(p.dynThresholdsDb.size() == 4);
        REQUIRE(p.dynRatios.size() == 4);
        REQUIRE(p.exciterModes.size() == 4);
        REQUIRE(p.exciterDrives.size() == 4);
        REQUIRE(p.transientAttacks.size() == 4);
        REQUIRE(p.transientSustains.size() == 4);
    }

    // Every profile has a unique name and unique genre enum
    REQUIRE(names.size() == all.size());
    REQUIRE(genres.size() == all.size());

    // Mandatory specific genre targets from prompt
    const auto folk = TargetProfile::getProfile(GenreProfile::Folk);
    REQUIRE(folk.targetIntegratedLufs == Catch::Approx(-15.0f));
    REQUIRE(folk.targetTruePeakCeilingDb == Catch::Approx(-1.0f));
    REQUIRE(folk.monoMakerFrequencyHz == Catch::Approx(70.0f));
    REQUIRE(folk.targetCrestFactorDb == Catch::Approx(11.5f));
    REQUIRE(folk.targetStereoWidth == Catch::Approx(105.0f));

    const auto hiphop = TargetProfile::getProfile(GenreProfile::HipHop);
    REQUIRE(hiphop.targetIntegratedLufs == Catch::Approx(-8.0f));
    REQUIRE(hiphop.targetTruePeakCeilingDb == Catch::Approx(-0.2f));
    REQUIRE(hiphop.targetSpectralCurveDb[0] > 5.0f); // Sub-bass emphasis

    const auto edm = TargetProfile::getProfile(GenreProfile::EDM);
    REQUIRE(edm.targetIntegratedLufs == Catch::Approx(-7.0f));
    REQUIRE(edm.targetStereoWidth == Catch::Approx(125.0f));

    const auto rock = TargetProfile::getProfile(GenreProfile::Rock);
    REQUIRE(rock.targetIntegratedLufs == Catch::Approx(-9.5f));
    REQUIRE(rock.targetSpectralCurveDb[4] == Catch::Approx(2.0f)); // Midrange guitar bite

    const auto classical = TargetProfile::getProfile(GenreProfile::Orchestral);
    REQUIRE(classical.targetIntegratedLufs == Catch::Approx(-18.0f));
    REQUIRE(classical.targetCrestFactorDb == Catch::Approx(14.0f));
}

TEST_CASE("AutoMasterEngine Spectral Corrections & Limiter Calculation", "[assistant][automaster]") {
    // 1. Spectral Correction Test
    std::array<float, 8> measuredLevels = { +8.0f, +6.0f, +1.0f, 0.0f, -1.0f, -3.0f, -8.0f, -10.0f };
    const auto streaming = TargetProfile::getProfile(GenreProfile::AllPurpose);

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
    engine.setTargetGenre(GenreProfile::AllPurpose);

    REQUIRE(engine.getState() == AssistantState::Idle);
    engine.startListening();
    REQUIRE(engine.getState() == AssistantState::Listening);

    // Generate stereo 1 kHz sine at -20 dBFS amplitude (~ -20 LUFS)
    const size_t blockSize = 480;
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
    REQUIRE(rec.limiterThresholdDb == Catch::Approx(-6.0f).margin(0.5f));
    REQUIRE(rec.limiterCeilingDb == Catch::Approx(-1.0f));
    REQUIRE(rec.monoMakerFreqHz == Catch::Approx(80.0f));
}

TEST_CASE("AutoMasterEngine Ozone 12 Macros Scale DSP Parameters", "[assistant][macros]") {
    engine::StrataAudioProcessor processor;
    auto& apvts = processor.getAPVTS();
    auto& engine = processor.getAutoMasterEngine();

    engine.setTargetGenre(GenreProfile::AllPurpose);
    engine.applyGenreDefaults(apvts);

    // Check base EQ gain of band 0
    auto* pEq0 = apvts.getParameter(param::getEQBandGainID(0));
    REQUIRE(pEq0 != nullptr);
    const float baseEq0 = pEq0->convertFrom0to1(pEq0->getValue());

    // 1. Tonal Balance Macro: set to 50%
    engine.setMacroTonalBalance(50.0f);
    engine.applyMacroAdjustments(apvts);
    const float scaledEq0 = pEq0->convertFrom0to1(pEq0->getValue());
    REQUIRE(scaledEq0 == Catch::Approx(baseEq0 * 0.5f).margin(0.1f));

    // Set to 0% -> flat EQ
    engine.setMacroTonalBalance(0.0f);
    engine.applyMacroAdjustments(apvts);
    REQUIRE(pEq0->convertFrom0to1(pEq0->getValue()) == Catch::Approx(0.0f).margin(0.05f));

    // Reset to 100%
    engine.setMacroTonalBalance(100.0f);

    // 2. Loudness Macro: +3.0 dB offset
    auto* pLimThresh = apvts.getParameter(juce::String(param::LimiterThreshold.data()));
    REQUIRE(pLimThresh != nullptr);
    const float baseThresh = pLimThresh->convertFrom0to1(pLimThresh->getValue());

    engine.setMacroLoudness(3.0f);
    engine.applyMacroAdjustments(apvts);
    const float adjustedThresh = pLimThresh->convertFrom0to1(pLimThresh->getValue());
    // Lower threshold by 3.0 dB to drive limiter harder
    REQUIRE(adjustedThresh == Catch::Approx(baseThresh - 3.0f).margin(0.1f));

    // 3. Width Match Macro: set to 150%
    auto* pWidth = apvts.getParameter(juce::String(param::StereoWidth.data()));
    REQUIRE(pWidth != nullptr);
    engine.setMacroWidthMatch(150.0f);
    engine.applyMacroAdjustments(apvts);
    const float scaledWidth = pWidth->convertFrom0to1(pWidth->getValue());
    REQUIRE(scaledWidth > 1.05f); // Expanded beyond baseline

    // 4. Stabilizer Macro: 80% should engage Dynamic EQ resonance control on Band 3
    auto* pDynEqEnable = apvts.getParameter(param::getEQBandDynEnableID(3));
    auto* pDynEqRange  = apvts.getParameter(param::getEQBandDynRangeID(3));
    REQUIRE(pDynEqEnable != nullptr);
    REQUIRE(pDynEqRange != nullptr);

    engine.setMacroStabilizer(80.0f);
    engine.applyMacroAdjustments(apvts);
    REQUIRE(pDynEqEnable->convertFrom0to1(pDynEqEnable->getValue()) == Catch::Approx(1.0f));
    // Negative dynamic range to tame resonance
    REQUIRE(pDynEqRange->convertFrom0to1(pDynEqRange->getValue()) < -2.0f);
}

TEST_CASE("Genre Target Switching Creates Measurable Render Deltas", "[assistant][render]") {
    engine::StrataAudioProcessor processor;
    constexpr int blockSize = 512;
    constexpr int numBlocks = 8;

    processor.prepareToPlay(48000.0, blockSize);

    auto renderWithGenre = [&](GenreProfile profile) -> float {
        processor.getAutoMasterEngine().setTargetGenre(profile);
        processor.getAutoMasterEngine().applyGenreDefaults(processor.getAPVTS());

        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::MidiBuffer midi;
        constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;

        float totalRmsSum = 0.0f;
        for (int b = 0; b < numBlocks; ++b) {
            for (int i = 0; i < blockSize; ++i) {
                const int globalIdx = b * blockSize + i;
                const double t = static_cast<double>(globalIdx) / 48000.0;
                const float s50  = 0.35f * std::sin(static_cast<float>(twoPi * 50.0 * t));
                const float s500 = 0.35f * std::sin(static_cast<float>(twoPi * 500.0 * t));
                const float s5k  = 0.20f * std::sin(static_cast<float>(twoPi * 5000.0 * t));
                const float sample = s50 + s500 + s5k;
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample * 0.9f);
            }
            processor.processBlock(buffer, midi);
            totalRmsSum += buffer.getRMSLevel(0, 0, blockSize);
        }
        return totalRmsSum / static_cast<float>(numBlocks);
    };

    const float folkRms = renderWithGenre(GenreProfile::Folk);
    const float hiphopRms = renderWithGenre(GenreProfile::HipHop);
    const float classicalRms = renderWithGenre(GenreProfile::Orchestral);

    // HipHop (-8.0 LUFS) with heavy sub-bass drive > Folk (-15.0 LUFS) > Classical (-18.0 LUFS)
    REQUIRE(hiphopRms > folkRms);
    REQUIRE(folkRms > classicalRms);
}

TEST_CASE("ReferenceMatcher Match Curve Computation", "[assistant][matcher]") {
    std::array<float, 8> masterSpectrum    = { +1.0f, +1.0f, 0.0f, 0.0f, -1.0f, -2.0f, -4.0f, -6.0f };
    std::array<float, 8> referenceSpectrum = { +4.0f, +3.0f, 0.0f, 0.0f,  0.0f, +1.0f, -1.0f, -2.0f };

    // 100% Match
    const auto fullMatch = ReferenceMatcher::computeMatchCurve(masterSpectrum, referenceSpectrum, 1.0f);
    REQUIRE(fullMatch[0] == Catch::Approx(3.0f).margin(0.05f));
    REQUIRE(fullMatch[1] == Catch::Approx(2.0f).margin(0.05f));
    REQUIRE(fullMatch[5] == Catch::Approx(3.0f).margin(0.05f));
    REQUIRE(fullMatch[6] == Catch::Approx(3.0f).margin(0.05f));

    // 50% Match
    const auto halfMatch = ReferenceMatcher::computeMatchCurve(masterSpectrum, referenceSpectrum, 0.5f);
    REQUIRE(halfMatch[0] == Catch::Approx(1.5f).margin(0.05f));
    REQUIRE(halfMatch[1] == Catch::Approx(1.0f).margin(0.05f));
    REQUIRE(halfMatch[5] == Catch::Approx(1.5f).margin(0.05f));
}

#include "AutoMasterEngine.h"
#include "../engine/ParameterIDs.h"
#include "../dsp/common/DecibelUtils.h"
#include <algorithm>

namespace strata::assistant {

static constexpr std::array<double, 8> TARGET_FREQUENCIES_HZ = {
    32.0, 64.0, 250.0, 1000.0, 3000.0, 6000.0, 12000.0, 16000.0
};

AutoMasterEngine::AutoMasterEngine() {
    targetProfile_ = TargetProfile::getProfile(GenreProfile::AllPurpose);
    prepare(44100.0);
}

void AutoMasterEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    loudnessMeter_.prepare(sampleRate);
    featureExtractor_.prepare(sampleRate);
    spectrumAnalyzer_.prepare(sampleRate);
    targetSamplesToListen_ = static_cast<size_t>(sampleRate * 5.0); // 5s analysis window
    reset();
}

void AutoMasterEngine::reset() noexcept {
    state_.store(AssistantState::Idle, std::memory_order_relaxed);
    samplesAccumulated_ = 0;
    fftFrameCount_ = 0;
    spectralBinAccumulator_.fill(0.0);
    loudnessMeter_.reset();
    featureExtractor_.reset();
    spectrumAnalyzer_.reset();
}

void AutoMasterEngine::setTargetGenre(GenreProfile genre) noexcept {
    targetProfile_ = TargetProfile::getProfile(genre);
}

void AutoMasterEngine::startListening() noexcept {
    reset();
    state_.store(AssistantState::Listening, std::memory_order_relaxed);
}

void AutoMasterEngine::cancelListening() noexcept {
    state_.store(AssistantState::Idle, std::memory_order_relaxed);
}

float AutoMasterEngine::getListeningProgressPercent() const noexcept {
    if (state_.load(std::memory_order_relaxed) != AssistantState::Listening) {
        return (state_.load(std::memory_order_relaxed) == AssistantState::Ready) ? 100.0f : 0.0f;
    }
    if (targetSamplesToListen_ == 0) {
        return 0.0f;
    }
    return std::min(100.0f, 100.0f * static_cast<float>(samplesAccumulated_) / static_cast<float>(targetSamplesToListen_));
}

void AutoMasterEngine::processBlock(const float* left, const float* right, size_t numSamples) noexcept {
    if (state_.load(std::memory_order_relaxed) != AssistantState::Listening) {
        return;
    }

    loudnessMeter_.processStereo(left, right, numSamples);
    featureExtractor_.processStereo(left, right, numSamples);
    spectrumAnalyzer_.pushAudioBlock(left, right, numSamples);

    std::vector<float> mags;
    while (spectrumAnalyzer_.getLatestMagnitudes(mags, analysis::SpectrumSmoothing::None)) {
        for (size_t b = 0; b < 8; ++b) {
            const double freq = TARGET_FREQUENCIES_HZ[b];
            const size_t bin = std::clamp(static_cast<size_t>(std::round(freq * static_cast<double>(analysis::SpectrumAnalyzer::FFT_SIZE) / sampleRate_)),
                                          size_t{0},
                                          mags.size() - 1);
            spectralBinAccumulator_[b] += static_cast<double>(mags[bin]);
        }
        ++fftFrameCount_;
    }

    samplesAccumulated_ += numSamples;
    if (samplesAccumulated_ >= targetSamplesToListen_) {
        finishListeningAndAnalyze();
    }
}

bool AutoMasterEngine::finishListeningAndAnalyze() noexcept {
    computeRecommendations();
    state_.store(AssistantState::Ready, std::memory_order_relaxed);
    return true;
}

std::array<float, 8> AutoMasterEngine::calculateEqCorrections(
    const std::array<float, 8>& measuredLevelsDb,
    const std::array<float, 8>& targetCurveDb) noexcept {

    std::array<float, 8> corrections{};
    // Normalize both relative to 1000 Hz (index 3)
    const float measuredRef = measuredLevelsDb[3];
    const float targetRef   = targetCurveDb[3];

    for (size_t i = 0; i < 8; ++i) {
        const float measuredRel = measuredLevelsDb[i] - measuredRef;
        const float targetRel   = targetCurveDb[i] - targetRef;
        // Correction is target minus measured, clamped to +/- 6.0 dB for musical transparency
        const float diff = targetRel - measuredRel;
        corrections[i] = std::clamp(diff, -6.0f, 6.0f);
    }
    return corrections;
}

void AutoMasterEngine::computeRecommendations() noexcept {
    // 1. Spectral Envelope
    std::array<float, 8> measuredLevelsDb{};
    if (fftFrameCount_ > 0) {
        for (size_t b = 0; b < 8; ++b) {
            measuredLevelsDb[b] = static_cast<float>(spectralBinAccumulator_[b] / static_cast<double>(fftFrameCount_));
        }
    } else {
        measuredLevelsDb.fill(0.0f);
    }

    recommendations_.eqGainsDb = calculateEqCorrections(measuredLevelsDb, targetProfile_.targetSpectralCurveDb);

    // 2. Loudness & Limiter Threshold
    const float measuredLufs = loudnessMeter_.getIntegratedLoudness();
    recommendations_.measuredLufs = (measuredLufs > -70.0f) ? measuredLufs : -24.0f;

    const float deltaLufs = targetProfile_.targetIntegratedLufs - recommendations_.measuredLufs;
    // If delta > 0, track is quieter than target, drive limiter
    if (deltaLufs > 0.0f) {
        recommendations_.limiterThresholdDb = std::clamp(-deltaLufs, -15.0f, 0.0f);
    } else {
        recommendations_.limiterThresholdDb = 0.0f;
    }
    recommendations_.limiterCeilingDb = targetProfile_.targetTruePeakCeilingDb;

    // 3. Stereo Field
    recommendations_.monoMakerFreqHz = targetProfile_.monoMakerFrequencyHz;
    recommendations_.stereoWidth     = targetProfile_.targetStereoWidth;

    // 4. Dynamics, Exciter & Transients based on Genre Target Profile
    recommendations_.dynThresholdsDb   = targetProfile_.dynThresholdsDb;
    recommendations_.dynRatios         = targetProfile_.dynRatios;
    recommendations_.exciterModes      = targetProfile_.exciterModes;
    recommendations_.exciterDrives     = targetProfile_.exciterDrives;
    recommendations_.transientAttacks  = targetProfile_.transientAttacks;
    recommendations_.transientSustains = targetProfile_.transientSustains;

    const auto feat = featureExtractor_.getFeatures();
    recommendations_.measuredCrestFactorDb = feat.crestFactorDb;

    const float crestDelta = feat.crestFactorDb - targetProfile_.targetCrestFactorDb;
    if (crestDelta > 2.0f) {
        // High crest factor: tighten dynamics slightly
        for (size_t b = 0; b < 4; ++b) {
            recommendations_.dynThresholdsDb[b] -= 2.0f;
            recommendations_.dynRatios[b]       = std::min(4.0f, recommendations_.dynRatios[b] + 0.5f);
        }
    }
}

void AutoMasterEngine::applyMacroAdjustments(juce::AudioProcessorValueTreeState& apvts) {
    auto setParam = [&](const juce::String& paramId, float value) {
        if (auto* p = apvts.getParameter(paramId)) {
            p->setValueNotifyingHost(p->convertTo0to1(value));
        }
    };

    // 1. Tonal Balance Macro: Scales EQ Gain corrections (0% to 200%)
    const float tonalScale = tonalBalanceMacro_ / 100.0f;
    for (size_t b = 0; b < 8; ++b) {
        const float scaledEq = recommendations_.eqGainsDb[b] * tonalScale;
        setParam(param::getEQBandGainID(b), scaledEq);
    }

    // 2. Loudness Macro: Modulates Limiter Threshold (-6 to +6 dB offset)
    const float adjustedLimiterThresh = std::clamp(recommendations_.limiterThresholdDb - loudnessMacroDb_, -20.0f, 0.0f);
    setParam(juce::String(param::LimiterThreshold.data()), adjustedLimiterThresh);

    // 3. Dynamics Match Macro: Scales Dynamics Thresholds and Ratios
    const float dynScale = dynamicsMatchMacro_ / 100.0f;
    for (size_t b = 0; b < 4; ++b) {
        const float baseThresh = recommendations_.dynThresholdsDb[b];
        const float scaledThresh = -6.0f + (baseThresh - (-6.0f)) * dynScale;
        const float baseRatio = recommendations_.dynRatios[b];
        const float scaledRatio = 1.0f + (baseRatio - 1.0f) * dynScale;
        setParam(param::getDynBandThreshID(b), scaledThresh);
        setParam(param::getDynBandRatioID(b),  scaledRatio);
    }

    // 4. Width Match Macro: Scales Master Stereo Width
    const float baseWidth = recommendations_.stereoWidth / 100.0f;
    const float scaledWidth = std::clamp(1.0f + (baseWidth - 1.0f) * (widthMatchMacro_ / 100.0f), 0.0f, 2.0f);
    setParam(juce::String(param::StereoWidth.data()), scaledWidth);

    // 5. Clarity Macro: Modulates high-band exciter drive and upper transient attacks
    const float clarityBoost = clarityMacro_ / 100.0f;
    const float exc3Drive = std::clamp(recommendations_.exciterDrives[2] + clarityBoost * 15.0f, 0.0f, 100.0f);
    const float exc4Drive = std::clamp(recommendations_.exciterDrives[3] + clarityBoost * 20.0f, 0.0f, 100.0f);
    setParam(param::getExciterBandDriveID(2), exc3Drive);
    setParam(param::getExciterBandDriveID(3), exc4Drive);
    const float trans3Attack = std::clamp(recommendations_.transientAttacks[2] + clarityBoost * 1.5f, -6.0f, 6.0f);
    const float trans4Attack = std::clamp(recommendations_.transientAttacks[3] + clarityBoost * 1.5f, -6.0f, 6.0f);
    setParam(param::getTransientBandAttackID(2), trans3Attack);
    setParam(param::getTransientBandAttackID(3), trans4Attack);

    // 6. Stabilizer Macro: Dynamic resonance suppression on sensitive midrange bands (Bands 2, 3, 4)
    const float stabAmount = stabilizerMacro_ / 100.0f;
    const bool stabEnable = (stabAmount > 0.05f);
    const float dynRange = -stabAmount * 4.0f; // up to -4.0 dB dynamic smoothing
    for (size_t b = 2; b <= 4; ++b) {
        setParam(param::getEQBandDynEnableID(b), stabEnable ? 1.0f : 0.0f);
        setParam(param::getEQBandDynThreshID(b), -16.0f);
        setParam(param::getEQBandDynRangeID(b),  dynRange);
    }
}

void AutoMasterEngine::applyRecommendations(juce::AudioProcessorValueTreeState& apvts) {
    auto setParam = [&](const juce::String& paramId, float value) {
        if (auto* p = apvts.getParameter(paramId)) {
            p->setValueNotifyingHost(p->convertTo0to1(value));
        }
    };

    // 1. EQ Band Enables & initial gains
    setParam(juce::String(param::EQBypass.data()), 0.0f);
    for (size_t b = 0; b < 8; ++b) {
        setParam(param::getEQBandEnableID(b), 1.0f);
    }

    // 2. Limiter Parameters
    setParam(juce::String(param::LimiterCeiling.data()),   recommendations_.limiterCeilingDb);
    setParam(juce::String(param::LimiterBypass.data()),    0.0f);

    // 3. Stereo Imager
    setParam(juce::String(param::StereoMonoMakerEnable.data()), 1.0f);
    setParam(juce::String(param::StereoMonoMakerFreq.data()),   recommendations_.monoMakerFreqHz);

    // 4. Multiband Dynamics
    setParam(juce::String(param::DynamicsBypass.data()), 0.0f);
    for (size_t b = 0; b < 4; ++b) {
        setParam(param::getDynBandEnableID(b), 1.0f);
        setParam(param::getDynBandBypassID(b), 0.0f);
    }

    // 5. Multiband Exciter
    setParam(juce::String(param::ExciterBypass.data()), 0.0f);
    for (size_t b = 0; b < 4; ++b) {
        setParam(param::getExciterBandEnableID(b), 1.0f);
        setParam(param::getExciterBandModeID(b),   static_cast<float>(recommendations_.exciterModes[b]));
        setParam(param::getExciterBandDriveID(b),  recommendations_.exciterDrives[b]);
        setParam(param::getExciterBandMixID(b),    100.0f);
    }

    // 6. Multiband Transient
    setParam(juce::String(param::TransientBypass.data()), 0.0f);
    for (size_t b = 0; b < 4; ++b) {
        setParam(param::getTransientBandEnableID(b),  1.0f);
        setParam(param::getTransientBandAttackID(b),  recommendations_.transientAttacks[b]);
        setParam(param::getTransientBandSustainID(b), recommendations_.transientSustains[b]);
    }

    // Apply macro scalings (Tonal Balance, Loudness, Dynamics Match, Width Match, Clarity, Stabilizer)
    applyMacroAdjustments(apvts);
}

void AutoMasterEngine::applyGenreDefaults(juce::AudioProcessorValueTreeState& apvts) {
    auto setParam = [&](const juce::String& paramId, float value) {
        if (auto* p = apvts.getParameter(paramId)) {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
                *choice = static_cast<int>(std::round(value));
            } else if (auto* bp = dynamic_cast<juce::AudioParameterBool*>(p)) {
                *bp = (value > 0.5f);
            } else if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(p)) {
                *fp = value;
            } else {
                p->setValueNotifyingHost(p->convertTo0to1(value));
            }
        }
    };

    // 1. Equalizer: Configure frequencies, types, Q and gains for genuine mastering curves
    // 8 distinct acoustic bands: Sub, Bass, Low-Mid, Mid, Upper-Mid, Presence, Air, Brilliance
    const std::array<float, 8> eqFreqs = { 35.0f, 75.0f, 250.0f, 1000.0f, 3000.0f, 6000.0f, 11000.0f, 16000.0f };
    // Filter types: 1 = LowShelf, 0 = Bell, 2 = HighShelf
    const std::array<int, 8> eqTypes = { 1, 0, 0, 0, 0, 0, 2, 2 };
    const std::array<float, 8> eqQ = { 0.71f, 1.0f, 1.2f, 1.0f, 1.0f, 1.0f, 0.71f, 0.71f };

    setParam(juce::String(param::EQBypass.data()), 0.0f);
    for (size_t b = 0; b < 8; ++b) {
        setParam(param::getEQBandFreqID(b), eqFreqs[b]);
        setParam(param::getEQBandTypeID(b), static_cast<float>(eqTypes[b]));
        setParam(param::getEQBandQID(b), eqQ[b]);
        setParam(param::getEQBandGainID(b), targetProfile_.targetSpectralCurveDb[b]);
        setParam(param::getEQBandEnableID(b), 1.0f);
    }

    // 2. Multiband Dynamics: Set Crossover Splits, thresholds, ratios, attacks, and releases
    setParam(juce::String(param::DynamicsBypass.data()), 0.0f);
    setParam(juce::String(param::CrossoverSplit1.data()), 180.0f);
    setParam(juce::String(param::CrossoverSplit2.data()), 2200.0f);
    setParam(juce::String(param::CrossoverSplit3.data()), 6800.0f);

    const std::array<float, 4> defaultAttacks = { 20.0f, 14.0f, 8.0f, 4.0f };
    const std::array<float, 4> defaultReleases = { 35.0f, 45.0f, 25.0f, 12.0f };
    const std::array<float, 4> defaultMakeups = { 0.8f, 0.5f, 0.4f, 0.0f };

    for (size_t b = 0; b < 4; ++b) {
        setParam(param::getDynBandEnableID(b), 1.0f);
        setParam(param::getDynBandBypassID(b), 0.0f);
        setParam(param::getDynBandThreshID(b), targetProfile_.dynThresholdsDb[b]);
        setParam(param::getDynBandRatioID(b),  targetProfile_.dynRatios[b]);
        setParam(param::getDynBandAttackID(b), defaultAttacks[b]);
        setParam(param::getDynBandReleaseID(b), defaultReleases[b]);
        setParam(param::getDynBandMakeupID(b), defaultMakeups[b]);
    }

    // 3. Multiband Exciter
    setParam(juce::String(param::ExciterBypass.data()), 0.0f);
    for (size_t b = 0; b < 4; ++b) {
        setParam(param::getExciterBandEnableID(b), 1.0f);
        setParam(param::getExciterBandModeID(b),   static_cast<float>(targetProfile_.exciterModes[b]));
        setParam(param::getExciterBandDriveID(b),  targetProfile_.exciterDrives[b]);
        setParam(param::getExciterBandMixID(b),    (targetProfile_.exciterDrives[b] > 0.0f) ? 35.0f : 0.0f);
    }

    // 4. Stereo Imager
    setParam(juce::String(param::StereoMonoMakerEnable.data()), 1.0f);
    setParam(juce::String(param::StereoMonoMakerFreq.data()),   targetProfile_.monoMakerFrequencyHz);
    setParam(juce::String(param::StereoWidth.data()),           targetProfile_.targetStereoWidth / 100.0f);
    const bool stereoizer = (targetProfile_.targetStereoWidth > 110.0f);
    setParam(juce::String(param::StereoStereoizerEnable.data()), stereoizer ? 1.0f : 0.0f);
    setParam(juce::String(param::StereoStereoizerAmount.data()), stereoizer ? 20.0f : 0.0f);

    // 5. Pre-Limiter Soft Clipper
    // Ozone Maximizer uses soft clipping for aggressive loudness styles
    const bool enableClipper = (targetProfile_.targetIntegratedLufs >= -11.0f);
    setParam(juce::String(param::ClipperEnable.data()), enableClipper ? 1.0f : 0.0f);
    setParam(juce::String(param::ClipperCeiling.data()), -0.5f);
    setParam(juce::String(param::ClipperKnee.data()), 3.0f);

    // 6. Maximizer (True Peak Limiter)
    // Map target integrated LUFS to realistic mastering limiter threshold
    float thresholdDb = -2.0f;
    if (targetProfile_.targetIntegratedLufs >= -8.0f) {
        thresholdDb = -5.5f; // Heavy modern loudness (Trap, EDM, Hip-Hop)
    } else if (targetProfile_.targetIntegratedLufs >= -10.0f) {
        thresholdDb = -4.5f; // Rock, Dance Pop, Club
    } else if (targetProfile_.targetIntegratedLufs >= -12.0f) {
        thresholdDb = -3.0f; // Indie, Classic Rock
    } else if (targetProfile_.targetIntegratedLufs >= -14.0f) {
        thresholdDb = -1.5f; // Streaming Universal, Country
    } else {
        thresholdDb = -0.5f; // Folk, Acoustic, Ambient
    }

    setParam(juce::String(param::LimiterThreshold.data()), thresholdDb);
    setParam(juce::String(param::LimiterCeiling.data()),   targetProfile_.targetTruePeakCeilingDb);
    setParam(juce::String(param::LimiterRelease.data()),   (targetProfile_.targetIntegratedLufs >= -9.0f) ? 60.0f : 120.0f);
    setParam(juce::String(param::LimiterBypass.data()),    0.0f);

    // 7. Multiband Transient
    setParam(juce::String(param::TransientBypass.data()), 0.0f);
    for (size_t b = 0; b < 4; ++b) {
        setParam(param::getTransientBandEnableID(b),  1.0f);
        setParam(param::getTransientBandAttackID(b),  targetProfile_.transientAttacks[b]);
        setParam(param::getTransientBandSustainID(b), targetProfile_.transientSustains[b]);
    }

    // Copy to recommendations so macros can tweak them subsequently
    recommendations_.eqGainsDb = targetProfile_.targetSpectralCurveDb;
    recommendations_.limiterThresholdDb = thresholdDb;
    recommendations_.limiterCeilingDb = targetProfile_.targetTruePeakCeilingDb;
    recommendations_.monoMakerFreqHz = targetProfile_.monoMakerFrequencyHz;
    recommendations_.stereoWidth = targetProfile_.targetStereoWidth;
    recommendations_.dynThresholdsDb = targetProfile_.dynThresholdsDb;
    recommendations_.dynRatios = targetProfile_.dynRatios;
    recommendations_.exciterModes = targetProfile_.exciterModes;
    recommendations_.exciterDrives = targetProfile_.exciterDrives;
    recommendations_.transientAttacks = targetProfile_.transientAttacks;
    recommendations_.transientSustains = targetProfile_.transientSustains;
}

} // namespace strata::assistant

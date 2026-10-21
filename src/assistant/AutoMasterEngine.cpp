#include "AutoMasterEngine.h"
#include "../engine/ParameterIDs.h"
#include "../dsp/common/DecibelUtils.h"
#include <algorithm>

namespace strata::assistant {

static constexpr std::array<double, 8> TARGET_FREQUENCIES_HZ = {
    32.0, 64.0, 250.0, 1000.0, 3000.0, 6000.0, 12000.0, 16000.0
};

AutoMasterEngine::AutoMasterEngine() {
    targetProfile_ = TargetProfile::getProfile(GenreProfile::StreamingStandard);
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

    // 4. Dynamics & Crest Factor
    const auto feat = featureExtractor_.getFeatures();
    recommendations_.measuredCrestFactorDb = feat.crestFactorDb;

    const float crestDelta = feat.crestFactorDb - targetProfile_.targetCrestFactorDb;
    for (size_t b = 0; b < 4; ++b) {
        if (crestDelta > 2.0f) {
            // High crest factor: tighten dynamics
            recommendations_.dynThresholdsDb[b] = -14.0f;
            recommendations_.dynRatios[b]       = 2.5f;
        } else {
            recommendations_.dynThresholdsDb[b] = -10.0f;
            recommendations_.dynRatios[b]       = 1.8f;
        }
    }
}

void AutoMasterEngine::applyRecommendations(juce::AudioProcessorValueTreeState& apvts) {
    auto setParam = [&](const juce::String& paramId, float value) {
        if (auto* p = apvts.getParameter(paramId)) {
            p->setValueNotifyingHost(p->convertTo0to1(value));
        }
    };

    // Apply EQ band gains
    for (size_t b = 0; b < 8; ++b) {
        setParam(param::getEQBandGainID(b), recommendations_.eqGainsDb[b]);
        setParam(param::getEQBandEnableID(b), 1.0f);
    }

    // Apply Limiter parameters
    setParam(juce::String(param::LimiterThreshold.data()), recommendations_.limiterThresholdDb);
    setParam(juce::String(param::LimiterCeiling.data()),   recommendations_.limiterCeilingDb);
    setParam(juce::String(param::LimiterBypass.data()),    0.0f);

    // Apply Stereo Imager
    setParam(juce::String(param::StereoMonoMakerEnable.data()), 1.0f);
    setParam(juce::String(param::StereoMonoMakerFreq.data()),   recommendations_.monoMakerFreqHz);
    setParam(juce::String(param::StereoWidth.data()),           recommendations_.stereoWidth);
}

} // namespace strata::assistant

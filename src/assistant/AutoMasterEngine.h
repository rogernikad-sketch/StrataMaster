#pragma once

#include "MasteringProfiles.h"
#include "../analysis/SpectrumAnalyzer.h"
#include "../analysis/BS1770LoudnessMeter.h"
#include "../analysis/AudioFeatureExtractor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <array>
#include <atomic>
#include <memory>

namespace strata::assistant {

enum class AssistantState {
    Idle,
    Listening,
    Ready
};

struct MasteringRecommendations {
    std::array<float, 8> eqGainsDb{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float limiterThresholdDb{ -1.0f };
    float limiterCeilingDb{ -1.0f };
    float monoMakerFreqHz{ 80.0f };
    float stereoWidth{ 100.0f };
    std::array<float, 4> dynThresholdsDb{ -12.0f, -12.0f, -12.0f, -12.0f };
    std::array<float, 4> dynRatios{ 2.0f, 1.8f, 1.8f, 1.5f };
    float measuredLufs{ -20.0f };
    float measuredCrestFactorDb{ 10.0f };
};

/**
 * Intelligent Analysis-Based Auto-Mastering Engine.
 * Analyzes spectral tilt, dynamic crest factor, stereo field, and LUFS loudness
 * against curated reference target profiles, generating professional mastering decisions.
 */
class AutoMasterEngine final {
public:
    AutoMasterEngine();
    ~AutoMasterEngine() = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Workflow Control
    void setTargetGenre(GenreProfile genre) noexcept;
    [[nodiscard]] GenreProfile getTargetGenre() const noexcept { return targetProfile_.genre; }

    void startListening() noexcept;
    void cancelListening() noexcept;
    bool finishListeningAndAnalyze() noexcept;

    [[nodiscard]] AssistantState getState() const noexcept {
        return state_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] float getListeningProgressPercent() const noexcept;

    /** Audio thread accumulation callback */
    void processBlock(const float* left, const float* right, size_t numSamples) noexcept;

    // Results & Application
    [[nodiscard]] const MasteringRecommendations& getRecommendations() const noexcept {
        return recommendations_;
    }

    void applyRecommendations(juce::AudioProcessorValueTreeState& apvts);

    // Static helper to calculate EQ corrections for testing and direct inspection
    static std::array<float, 8> calculateEqCorrections(
        const std::array<float, 8>& measuredLevelsDb,
        const std::array<float, 8>& targetCurveDb) noexcept;

private:
    void computeRecommendations() noexcept;

    double sampleRate_{ 44100.0 };
    std::atomic<AssistantState> state_{ AssistantState::Idle };

    TargetProfile targetProfile_;
    MasteringRecommendations recommendations_;

    // Analysis components
    analysis::BS1770LoudnessMeter loudnessMeter_;
    analysis::AudioFeatureExtractor featureExtractor_;
    analysis::SpectrumAnalyzer spectrumAnalyzer_;

    // Listening statistics
    size_t samplesAccumulated_{ 0 };
    size_t targetSamplesToListen_{ 44100 * 5 }; // 5 seconds default listening duration
    std::array<double, 8> spectralBinAccumulator_{ 0.0 };
    size_t fftFrameCount_{ 0 };
};

} // namespace strata::assistant

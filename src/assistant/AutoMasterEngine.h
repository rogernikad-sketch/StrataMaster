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
#include <algorithm>

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
    float stereoWidth{ 105.0f };
    std::array<float, 4> dynThresholdsDb{ -16.0f, -18.0f, -20.0f, -22.0f };
    std::array<float, 4> dynRatios{ 2.0f, 2.0f, 2.0f, 2.0f };
    std::array<int, 4> exciterModes{ 0, 0, 0, 0 };
    std::array<float, 4> exciterDrives{ 10.0f, 8.0f, 12.0f, 15.0f };
    std::array<float, 4> transientAttacks{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> transientSustains{ 0.0f, 0.0f, 0.0f, 0.0f };
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
    [[nodiscard]] const TargetProfile& getTargetProfile() const noexcept { return targetProfile_; }

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
    void applyGenreDefaults(juce::AudioProcessorValueTreeState& apvts);

    // Assistant Macros (Ozone 12 Reference Macros)
    void setMacroTonalBalance(float percent) noexcept { tonalBalanceMacro_ = std::clamp(percent, 0.0f, 200.0f); }
    [[nodiscard]] float getMacroTonalBalance() const noexcept { return tonalBalanceMacro_; }

    void setMacroLoudness(float offsetDb) noexcept { loudnessMacroDb_ = std::clamp(offsetDb, -6.0f, 6.0f); }
    [[nodiscard]] float getMacroLoudness() const noexcept { return loudnessMacroDb_; }

    void setMacroDynamicsMatch(float percent) noexcept { dynamicsMatchMacro_ = std::clamp(percent, 0.0f, 200.0f); }
    [[nodiscard]] float getMacroDynamicsMatch() const noexcept { return dynamicsMatchMacro_; }

    void setMacroWidthMatch(float percent) noexcept { widthMatchMacro_ = std::clamp(percent, 0.0f, 200.0f); }
    [[nodiscard]] float getMacroWidthMatch() const noexcept { return widthMatchMacro_; }

    void setMacroClarity(float percent) noexcept { clarityMacro_ = std::clamp(percent, 0.0f, 100.0f); }
    [[nodiscard]] float getMacroClarity() const noexcept { return clarityMacro_; }

    void setMacroStabilizer(float percent) noexcept { stabilizerMacro_ = std::clamp(percent, 0.0f, 100.0f); }
    [[nodiscard]] float getMacroStabilizer() const noexcept { return stabilizerMacro_; }

    void applyMacroAdjustments(juce::AudioProcessorValueTreeState& apvts);

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

    // Macro states
    float tonalBalanceMacro_{ 100.0f };
    float loudnessMacroDb_{ 0.0f };
    float dynamicsMatchMacro_{ 100.0f };
    float widthMatchMacro_{ 100.0f };
    float clarityMacro_{ 25.0f };
    float stabilizerMacro_{ 30.0f };

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

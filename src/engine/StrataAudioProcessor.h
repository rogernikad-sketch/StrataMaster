#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../dsp/StereoProcessor.h"
#include "../dsp/DynamicEqualizer.h"
#include "../dsp/MultibandDynamics.h"
#include "../dsp/CrossoverFilterbank.h"
#include "../dsp/Saturator.h"
#include "../dsp/TransientProcessor.h"
#include "../dsp/TruePeakLimiter.h"
#include "../dsp/MaseratiProcessor.h"
#include "../dsp/common/SmoothedValue.h"
#include "../analysis/CorrelationMeter.h"
#include "../analysis/BS1770LoudnessMeter.h"
#include "../analysis/TruePeakMeter.h"
#include "../analysis/SpectrumAnalyzer.h"
#include "../analysis/AudioFeatureExtractor.h"
#include "PresetManager.h"
#include "ReferencePlayer.h"
#include "../assistant/AutoMasterEngine.h"
#include "../assistant/ReferenceMatcher.h"

namespace strata::engine {

class StrataAudioProcessor final : public juce::AudioProcessor {
public:
    StrataAudioProcessor();
    ~StrataAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "StrataMaster"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts_; }
    [[nodiscard]] analysis::CorrelationMeter& getCorrelationMeter() noexcept { return correlationMeter_; }
    [[nodiscard]] analysis::BS1770LoudnessMeter& getBS1770Meter() noexcept { return bs1770Meter_; }
    [[nodiscard]] analysis::TruePeakMeter& getTruePeakMeter() noexcept { return truePeakMeter_; }
    [[nodiscard]] analysis::SpectrumAnalyzer& getSpectrumAnalyzer() noexcept { return spectrumAnalyzer_; }
    [[nodiscard]] analysis::AudioFeatureExtractor& getFeatureExtractor() noexcept { return featureExtractor_; }
    [[nodiscard]] PresetManager& getPresetManager() noexcept { return presetManager_; }
    [[nodiscard]] ReferencePlayer& getReferencePlayer() noexcept { return referencePlayer_; }
    [[nodiscard]] assistant::AutoMasterEngine& getAutoMasterEngine() noexcept { return autoMasterEngine_; }
    [[nodiscard]] dsp::DynamicEqualizer& getDynamicEqualizer() noexcept { return dynamicEqualizer_; }
    [[nodiscard]] dsp::MultibandDynamics& getMultibandDynamics() noexcept { return multibandDynamics_; }
    [[nodiscard]] dsp::Saturator& getSaturator() noexcept { return saturator_; }
    [[nodiscard]] dsp::TransientProcessor& getTransientProcessor() noexcept { return transientProcessor_; }
    [[nodiscard]] dsp::TruePeakLimiter& getTruePeakLimiter() noexcept { return truePeakLimiter_; }
    [[nodiscard]] dsp::MaseratiProcessor& getMaseratiProcessor() noexcept { return maseratiProcessor_; }

    static constexpr size_t LISSAJOUS_SAMPLES = 256;
    void getLissajousPoints(std::array<std::pair<float, float>, LISSAJOUS_SAMPLES>& dest) const noexcept;

    void getMeterLevels(float& inL, float& inR, float& outL, float& outR) const noexcept {
        inL = inputPeakL_.load(std::memory_order_relaxed);
        inR = inputPeakR_.load(std::memory_order_relaxed);
        outL = outputPeakL_.load(std::memory_order_relaxed);
        outR = outputPeakR_.load(std::memory_order_relaxed);
    }

    // Audition track and test signal playback (for Standalone and Auditioning)
    bool loadAuditionAudio(const juce::File& file);
    void setAuditionPlaying(bool playing) noexcept { auditionPlaying_.store(playing, std::memory_order_relaxed); }
    [[nodiscard]] bool isAuditionPlaying() const noexcept { return auditionPlaying_.load(std::memory_order_relaxed); }
    void setAuditionTestTone(int tone) noexcept { auditionTestTone_.store(tone, std::memory_order_relaxed); }
    [[nodiscard]] int getAuditionTestTone() const noexcept { return auditionTestTone_.load(std::memory_order_relaxed); }
    [[nodiscard]] bool hasAuditionTrack() const noexcept { return auditionBuffer_.getNumSamples() > 0; }
    [[nodiscard]] juce::String getAuditionFileName() const { return auditionFileName_; }

private:
    void updateEQParametersFromState() noexcept;
    void updateDynamicsParametersFromState() noexcept;
    void updateExciterParametersFromState() noexcept;
    void updateTransientParametersFromState() noexcept;
    void updateStereoParametersFromState() noexcept;
    void updateLimiterParametersFromState() noexcept;

    juce::AudioProcessorValueTreeState apvts_;

    // Master parameter value pointers
    std::atomic<float>* inputGainParam_{nullptr};
    std::atomic<float>* outputGainParam_{nullptr};
    std::atomic<float>* bypassParam_{nullptr};
    std::atomic<float>* eqBypassParam_{nullptr};
    std::atomic<float>* dynamicsBypassParam_{nullptr};
    std::atomic<float>* exciterBypassParam_{nullptr};
    std::atomic<float>* exciterOversamplingParam_{nullptr};
    std::atomic<float>* transientBypassParam_{nullptr};
    std::atomic<float>* stereoModeParam_{nullptr};
    std::atomic<float>* stereoWidthParam_{nullptr};

    // Advanced Stereo Imager parameter pointers
    std::atomic<float>* stereoMonoMakerEnableParam_{nullptr};
    std::atomic<float>* stereoMonoMakerFreqParam_{nullptr};
    std::atomic<float>* stereoStereoizerEnableParam_{nullptr};
    std::atomic<float>* stereoStereoizerAmountParam_{nullptr};

    // Pre-Limiter Soft Clipper parameter pointers
    std::atomic<float>* clipperEnableParam_{nullptr};
    std::atomic<float>* clipperCeilingParam_{nullptr};
    std::atomic<float>* clipperKneeParam_{nullptr};

    // Lookahead True-Peak Limiter parameter pointers
    std::atomic<float>* limiterBypassParam_{nullptr};
    std::atomic<float>* limiterCeilingParam_{nullptr};
    std::atomic<float>* limiterThresholdParam_{nullptr};
    std::atomic<float>* limiterReleaseParam_{nullptr};
    std::atomic<float>* limiterLookaheadParam_{nullptr};

    // Maserati Analog Tone Processor parameter pointers
    std::atomic<float>* maseratiBypassParam_{nullptr};
    std::atomic<float>* maseratiBassParam_{nullptr};
    std::atomic<float>* maseratiTrebleParam_{nullptr};
    std::atomic<float>* maseratiSensitivityParam_{nullptr};

    // Crossover split parameter pointers
    std::atomic<float>* crossoverSplit1Param_{nullptr};
    std::atomic<float>* crossoverSplit2Param_{nullptr};
    std::atomic<float>* crossoverSplit3Param_{nullptr};

    // EQ band parameter pointers
    struct EQBandParamPointers {
        std::atomic<float>* enable{nullptr};
        std::atomic<float>* type{nullptr};
        std::atomic<float>* freq{nullptr};
        std::atomic<float>* gain{nullptr};
        std::atomic<float>* q{nullptr};
        std::atomic<float>* dynEnable{nullptr};
        std::atomic<float>* dynThresh{nullptr};
        std::atomic<float>* dynRange{nullptr};
    };
    std::array<EQBandParamPointers, dsp::NUM_EQ_BANDS> eqBandParams_;

    // Multiband Dynamics band parameter pointers
    struct DynBandParamPointers {
        std::atomic<float>* enable{nullptr};
        std::atomic<float>* bypass{nullptr};
        std::atomic<float>* solo{nullptr};
        std::atomic<float>* mute{nullptr};
        std::atomic<float>* thresh{nullptr};
        std::atomic<float>* ratio{nullptr};
        std::atomic<float>* attack{nullptr};
        std::atomic<float>* release{nullptr};
        std::atomic<float>* makeup{nullptr};
        std::atomic<float>* autoMakeup{nullptr};
    };
    std::array<DynBandParamPointers, dsp::NUM_CROSSOVER_BANDS> dynBandParams_;

    // Exciter band parameter pointers
    struct ExciterBandParamPointers {
        std::atomic<float>* enable{nullptr};
        std::atomic<float>* mode{nullptr};
        std::atomic<float>* drive{nullptr};
        std::atomic<float>* mix{nullptr};
    };
    std::array<ExciterBandParamPointers, dsp::NUM_CROSSOVER_BANDS> exciterBandParams_;

    // Transient band parameter pointers
    struct TransientBandParamPointers {
        std::atomic<float>* enable{nullptr};
        std::atomic<float>* attack{nullptr};
        std::atomic<float>* sustain{nullptr};
    };
    std::array<TransientBandParamPointers, dsp::NUM_CROSSOVER_BANDS> transientBandParams_;

    // Smoothers
    dsp::SmoothedValue<float> inputGainSmoother_;
    dsp::SmoothedValue<float> outputGainSmoother_;
    dsp::SmoothedValue<float> stereoWidthSmoother_;

    // DSP components
    dsp::StereoProcessor stereoProcessor_;
    dsp::DynamicEqualizer dynamicEqualizer_;
    dsp::MultibandDynamics multibandDynamics_;
    dsp::CrossoverFilterbank exciterCrossover_;
    dsp::CrossoverFilterbank transientCrossover_;
    dsp::Saturator saturator_;
    dsp::TransientProcessor transientProcessor_;
    dsp::TruePeakLimiter truePeakLimiter_;
    dsp::MaseratiProcessor maseratiProcessor_;
    mutable std::array<std::atomic<float>, LISSAJOUS_SAMPLES> lissajousL_{};
    mutable std::array<std::atomic<float>, LISSAJOUS_SAMPLES> lissajousR_{};
    size_t lissajousWritePos_{0};
    analysis::CorrelationMeter correlationMeter_;
    analysis::BS1770LoudnessMeter bs1770Meter_;
    analysis::TruePeakMeter truePeakMeter_;
    analysis::SpectrumAnalyzer spectrumAnalyzer_;
    analysis::AudioFeatureExtractor featureExtractor_;
    PresetManager presetManager_;
    ReferencePlayer referencePlayer_;
    assistant::AutoMasterEngine autoMasterEngine_;

    // Pre-allocated Multiband Scratch Buffers (Zero heap allocations in processBlock)
    std::array<std::vector<float>, dsp::NUM_CROSSOVER_BANDS> exciterBandBuffersL_;
    std::array<std::vector<float>, dsp::NUM_CROSSOVER_BANDS> exciterBandBuffersR_;
    std::array<std::vector<float>, dsp::NUM_CROSSOVER_BANDS> transientBandBuffersL_;
    std::array<std::vector<float>, dsp::NUM_CROSSOVER_BANDS> transientBandBuffersR_;

    // Meter levels
    std::atomic<float> inputPeakL_{0.0f};
    std::atomic<float> inputPeakR_{0.0f};
    std::atomic<float> outputPeakL_{0.0f};
    std::atomic<float> outputPeakR_{0.0f};

    // Cached latency-affecting parameter states
    int lastOversamplingIdx_{ -1 };
    float lastLookaheadMs_{ -1.0f };

    // Audition track and test signal state
    juce::AudioFormatManager formatManager_;
    juce::AudioBuffer<float> auditionBuffer_;
    juce::String auditionFileName_;
    std::atomic<bool> auditionPlaying_{ false };
    std::atomic<int> auditionTestTone_{ 0 };
    std::atomic<int64_t> auditionPlayhead_{ 0 };
    std::atomic<bool> auditionLoop_{ true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrataAudioProcessor)
};

} // namespace strata::engine

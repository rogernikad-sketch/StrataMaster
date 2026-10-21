#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"
#include "components/Knob.h"
#include "components/LevelMeter.h"
#include "components/LoudnessMeterBar.h"
#include "components/ModuleRackBar.h"
#include "views/EqualizerView.h"
#include "views/DynamicsView.h"
#include "views/ExciterView.h"
#include "views/ImagerView.h"
#include "views/MaximizerView.h"
#include "views/MasterAssistantView.h"

namespace strata::engine {
    class StrataAudioProcessor;
}

namespace strata::gui {

class StrataAudioEditor final : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit StrataAudioEditor(engine::StrataAudioProcessor& processor);
    ~StrataAudioEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void showModule(ModuleID id);

private:
    void populatePresetBox();
    void selectPrevPreset();
    void selectNextPreset();

    engine::StrataAudioProcessor& processor_;
    StrataLookAndFeel lookAndFeel_;

    // 1. Header Bar Controls
    juce::Label titleLabel_;
    juce::ComboBox presetBox_;
    juce::TextButton prevPresetBtn_{ "<" };
    juce::TextButton nextPresetBtn_{ ">" };
    juce::TextButton assistantHeaderBtn_{ "MASTER ASSISTANT" };
    juce::TextButton abButton_{ "TRACK A" };
    juce::ToggleButton gainMatchButton_{ "GAIN MATCH" };
    juce::ToggleButton bypassButton_{ "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    // 2. Module Rack Bar (Modular mastering chain navigation)
    ModuleRackBar moduleRackBar_;

    // 3. Modular Active Views
    EqualizerView equalizerView_;
    DynamicsView dynamicsView_;
    ExciterView exciterView_;
    ImagerView imagerView_;
    MaximizerView maximizerView_;
    MasterAssistantView assistantView_;

    // 4. Right Master I/O & Broadcast Metering Stage
    LevelMeter inputMeter_;
    std::unique_ptr<Knob> inputGainKnob_;
    LoudnessMeterBar loudnessMeterBar_;
    LevelMeter outputMeter_;
    std::unique_ptr<Knob> outputGainKnob_;

    // Phase Correlation Indicator
    juce::Label correlationLabel_;
    float correlationValue_{ 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrataAudioEditor)
};

} // namespace strata::gui

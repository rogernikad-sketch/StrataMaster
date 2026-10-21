#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"
#include "components/Knob.h"
#include "components/LevelMeter.h"
#include "components/SpectrumDisplay.h"
#include "components/EQNodeGraph.h"
#include "components/LoudnessMeterBar.h"

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

private:
    void populatePresetBox();

    engine::StrataAudioProcessor& processor_;
    StrataLookAndFeel lookAndFeel_;

    // Header Controls
    juce::Label titleLabel_;
    juce::ComboBox presetBox_;
    juce::TextButton assistantButton_{ "LEARN MASTER" };
    juce::TextButton abButton_{ "TRACK A" };
    juce::ToggleButton gainMatchButton_{ "GAIN MATCH" };
    juce::ToggleButton bypassButton_{ "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    // Left Input Stage
    LevelMeter inputMeter_;
    std::unique_ptr<Knob> inputGainKnob_;

    // Center Visualizer & Processing Stage
    SpectrumDisplay spectrumDisplay_;
    std::unique_ptr<EQNodeGraph> eqGraph_;

    // Center Controls (Width, Mode, Limiter)
    std::unique_ptr<Knob> stereoWidthKnob_;
    juce::ComboBox stereoModeBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoModeAttachment_;
    std::unique_ptr<Knob> limiterThresholdKnob_;
    std::unique_ptr<Knob> limiterCeilingKnob_;

    // Phase Correlation
    juce::Label correlationLabel_;
    float correlationValue_{ 1.0f };

    // Right Meter & Output Stage
    LoudnessMeterBar loudnessMeterBar_;
    LevelMeter outputMeter_;
    std::unique_ptr<Knob> outputGainKnob_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrataAudioEditor)
};

} // namespace strata::gui

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../components/Knob.h"

namespace strata::gui {

class ImagerView final : public juce::Component {
public:
    explicit ImagerView(juce::AudioProcessorValueTreeState& apvts);
    ~ImagerView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setCorrelation(float correlation);

private:
    juce::AudioProcessorValueTreeState& apvts_;

    // Master Width
    std::unique_ptr<Knob> masterWidthKnob_;

    // Mono Maker (Sub-bass mono anchor)
    juce::GroupComponent monoMakerGroup_{ "MONO_MAKER_GRP", "MONO MAKER (BASS ANCHOR)" };
    juce::ToggleButton monoMakerEnableButton_{ "ENABLE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoMakerEnableAttachment_;
    std::unique_ptr<Knob> monoMakerFreqKnob_;

    // Stereoizer (Side-injected Haas decorrelator)
    juce::GroupComponent stereoizerGroup_{ "STEREOIZER_GRP", "STEREOIZER (HAAS DECORRELATOR)" };
    juce::ToggleButton stereoizerEnableButton_{ "ENABLE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stereoizerEnableAttachment_;
    std::unique_ptr<Knob> stereoizerAmountKnob_;

    // Routing Mode
    juce::ComboBox stereoModeBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoModeAttachment_;

    // Phase Correlation Readout
    float correlation_{ 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImagerView)
};

} // namespace strata::gui

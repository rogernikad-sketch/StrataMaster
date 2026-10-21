#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../components/CrossoverGraph.h"
#include "../components/Knob.h"
#include <array>

namespace strata::gui {

class DynamicsView final : public juce::Component {
public:
    explicit DynamicsView(juce::AudioProcessorValueTreeState& apvts);
    ~DynamicsView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void selectBand(int bandIndex);
    void updateGainReduction(int bandIndex, float grDb);

private:
    void rebuildBandControls(int bandIndex);

    juce::AudioProcessorValueTreeState& apvts_;

    CrossoverGraph crossoverGraph_;

    int selectedBand_{ 0 };
    std::array<juce::TextButton, 4> bandTabs_;

    juce::ToggleButton bandEnableButton_{ "ENABLE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandEnableAttachment_;

    juce::ToggleButton bandBypassButton_{ "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandBypassAttachment_;

    juce::ToggleButton bandSoloButton_{ "SOLO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandSoloAttachment_;

    juce::ToggleButton bandMuteButton_{ "MUTE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandMuteAttachment_;

    std::unique_ptr<Knob> threshKnob_;
    std::unique_ptr<Knob> ratioKnob_;
    std::unique_ptr<Knob> attackKnob_;
    std::unique_ptr<Knob> releaseKnob_;
    std::unique_ptr<Knob> makeupKnob_;

    juce::ToggleButton autoMakeupButton_{ "AUTO MAKEUP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoMakeupAttachment_;

    // Real-time Gain Reduction Meter
    float currentGainReductionDb_{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicsView)
};

} // namespace strata::gui

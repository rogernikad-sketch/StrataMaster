#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../components/Knob.h"
#include <memory>

namespace strata::gui {

class MaseratiView final : public juce::Component {
public:
    explicit MaseratiView(juce::AudioProcessorValueTreeState& apvts);
    ~MaseratiView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateActivity(float activity);

private:
    juce::AudioProcessorValueTreeState& apvts_;

    std::unique_ptr<Knob> bassKnob_;
    std::unique_ptr<Knob> trebleKnob_;
    std::unique_ptr<Knob> sensitivityKnob_;

    juce::ToggleButton bypassButton_{ "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    float activityMeter_{ 0.0f };
};

} // namespace strata::gui

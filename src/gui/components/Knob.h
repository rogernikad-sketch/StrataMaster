#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace strata::gui {

class Knob final : public juce::Component {
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& labelText);
    ~Knob() override = default;

    void resized() override;

    [[nodiscard]] juce::Slider& getSlider() noexcept { return slider_; }

private:
    juce::Slider slider_;
    juce::Label label_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;
};

} // namespace strata::gui

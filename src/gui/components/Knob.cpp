#include "Knob.h"
#include "../LookAndFeel.h"

namespace strata::gui {

Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& labelText) {
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    slider_.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider_.setColour(juce::Slider::textBoxTextColourId, StrataLookAndFeel::textLight);
    addAndMakeVisible(slider_);

    label_.setText(labelText, juce::dontSendNotification);
    label_.setJustificationType(juce::Justification::centred);
    label_.setColour(juce::Label::textColourId, StrataLookAndFeel::textDim);
    label_.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(label_);

    attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider_);
}

void Knob::resized() {
    auto bounds = getLocalBounds();
    label_.setBounds(bounds.removeFromTop(18));
    slider_.setBounds(bounds);
}

} // namespace strata::gui

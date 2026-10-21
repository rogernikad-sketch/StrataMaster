#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace strata::gui {

class StrataLookAndFeel : public juce::LookAndFeel_V4 {
public:
    StrataLookAndFeel();
    ~StrataLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // Palette Colors
    static const juce::Colour bgDark;
    static const juce::Colour panelBg;
    static const juce::Colour accentCyan;
    static const juce::Colour accentGold;
    static const juce::Colour textLight;
    static const juce::Colour textDim;
    static const juce::Colour borderSubtle;
};

} // namespace strata::gui

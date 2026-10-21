#include "LookAndFeel.h"
#include <numbers>

namespace strata::gui {

const juce::Colour StrataLookAndFeel::bgDark       = juce::Colour::fromRGB(18, 20, 24);
const juce::Colour StrataLookAndFeel::panelBg      = juce::Colour::fromRGB(26, 29, 36);
const juce::Colour StrataLookAndFeel::accentCyan   = juce::Colour::fromRGB(0, 229, 255);
const juce::Colour StrataLookAndFeel::accentGold   = juce::Colour::fromRGB(255, 170, 0);
const juce::Colour StrataLookAndFeel::textLight    = juce::Colour::fromRGB(240, 242, 245);
const juce::Colour StrataLookAndFeel::textDim      = juce::Colour::fromRGB(130, 138, 150);
const juce::Colour StrataLookAndFeel::borderSubtle = juce::Colour::fromRGB(45, 50, 62);

StrataLookAndFeel::StrataLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, bgDark);
    setColour(juce::Label::textColourId, textLight);
}

void StrataLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& /*slider*/) {
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto center = bounds.getCentre();

    // Background track
    juce::Path track;
    track.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(borderSubtle);
    g.strokePath(track, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active modulated arc
    if (sliderPosProportional > 0.0f) {
        juce::Path valueArc;
        valueArc.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, toAngle, true);
        g.setColour(accentCyan);
        g.strokePath(valueArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Dial body
    const float innerRadius = radius * 0.78f;
    g.setColour(panelBg);
    g.fillEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
    g.setColour(borderSubtle.brighter(0.1f));
    g.drawEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.2f);

    // Indicator pointer dot
    const float pointerDistance = innerRadius * 0.65f;
    const float px = center.x + pointerDistance * std::sin(toAngle);
    const float py = center.y - pointerDistance * std::cos(toAngle);
    g.setColour(accentCyan);
    g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);
}

void StrataLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto baseCol = backgroundColour;

    if (button.getToggleState()) {
        baseCol = accentCyan.withAlpha(0.25f);
    } else if (shouldDrawButtonAsDown) {
        baseCol = baseCol.darker(0.2f);
    } else if (shouldDrawButtonAsHighlighted) {
        baseCol = baseCol.brighter(0.1f);
    }

    g.setColour(baseCol);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(button.getToggleState() ? accentCyan : borderSubtle);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

} // namespace strata::gui

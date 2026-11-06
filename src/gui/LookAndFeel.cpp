#include "LookAndFeel.h"
#include <numbers>
#include <cmath>

namespace strata::gui {

// 4K 3D Skeuomorphic & Luxury Obsidian Palette
const juce::Colour StrataLookAndFeel::bgDark          = juce::Colour::fromRGB(14, 16, 21);
const juce::Colour StrataLookAndFeel::panelBg         = juce::Colour::fromRGB(22, 26, 34);
const juce::Colour StrataLookAndFeel::panelBgLight    = juce::Colour::fromRGB(32, 38, 50);
const juce::Colour StrataLookAndFeel::accentCyan      = juce::Colour::fromRGB(0, 229, 255);
const juce::Colour StrataLookAndFeel::accentCyanGlow  = juce::Colour::fromRGBA(0, 229, 255, 60);
const juce::Colour StrataLookAndFeel::accentGold      = juce::Colour::fromRGB(255, 180, 20);
const juce::Colour StrataLookAndFeel::accentGoldGlow  = juce::Colour::fromRGBA(255, 180, 20, 60);
const juce::Colour StrataLookAndFeel::textLight       = juce::Colour::fromRGB(244, 246, 250);
const juce::Colour StrataLookAndFeel::textDim         = juce::Colour::fromRGB(132, 142, 158);
const juce::Colour StrataLookAndFeel::borderSubtle    = juce::Colour::fromRGB(42, 48, 62);
const juce::Colour StrataLookAndFeel::bevelHighlight  = juce::Colour::fromRGBA(255, 255, 255, 38);
const juce::Colour StrataLookAndFeel::bevelShadow     = juce::Colour::fromRGBA(0, 0, 0, 140);

StrataLookAndFeel::StrataLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, bgDark);
    setColour(juce::Label::textColourId, textLight);
    setColour(juce::PopupMenu::backgroundColourId, panelBg);
    setColour(juce::PopupMenu::textColourId, textLight);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentCyan.withAlpha(0.2f));
    setColour(juce::PopupMenu::highlightedTextColourId, textLight);
    setColour(juce::ComboBox::backgroundColourId, panelBg);
    setColour(juce::ComboBox::textColourId, textLight);
    setColour(juce::ComboBox::outlineColourId, borderSubtle);
    setColour(juce::TextButton::buttonColourId, panelBg);
    setColour(juce::TextButton::textColourOffId, textLight);
}

void StrataLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& /*slider*/) {
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    if (radius <= 2.0f) return;

    const auto center = bounds.getCentre();
    const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. 3D Outer Cast Shadow
    g.setColour(juce::Colour::fromRGBA(0, 0, 0, 90));
    g.fillEllipse(center.x - radius, center.y - radius + 2.5f, radius * 2.0f, radius * 2.0f);

    // 2. 3D Sunken Outer Track (Recessed Groove)
    juce::Path track;
    track.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    
    // Groove base shadow
    g.setColour(juce::Colour::fromRGB(8, 10, 14));
    g.strokePath(track, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Subtle outer track rim
    g.setColour(borderSubtle.withAlpha(0.6f));
    g.strokePath(track, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Modulated Neon Glowing Arc
    if (sliderPosProportional > 0.001f) {
        juce::Path valueArc;
        valueArc.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, toAngle, true);

        // Soft Neon Glow Underneath
        g.setColour(accentCyanGlow);
        g.strokePath(valueArc, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Precision Laser Arc on top with dynamic gradient
        juce::ColourGradient arcGrad(
            accentCyan, center.x, center.y - radius,
            accentCyan.brighter(0.2f), center.x, center.y + radius, false);
        g.setGradientFill(arcGrad);
        g.strokePath(valueArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. 3D Knurled Metallic Bezel Ring
    const float bezelRadius = radius * 0.78f;
    juce::ColourGradient bezelGrad(
        juce::Colour::fromRGB(48, 54, 68), center.x - bezelRadius, center.y - bezelRadius,
        juce::Colour::fromRGB(18, 20, 26), center.x + bezelRadius, center.y + bezelRadius, false);
    g.setGradientFill(bezelGrad);
    g.fillEllipse(center.x - bezelRadius, center.y - bezelRadius, bezelRadius * 2.0f, bezelRadius * 2.0f);

    // 3D Bezel Rim Chamfer Highlights
    g.setColour(bevelHighlight);
    g.drawEllipse(center.x - bezelRadius, center.y - bezelRadius, bezelRadius * 2.0f, bezelRadius * 2.0f, 1.2f);
    g.setColour(bevelShadow);
    g.drawEllipse(center.x - bezelRadius + 1.0f, center.y - bezelRadius + 1.0f, (bezelRadius - 1.0f) * 2.0f, (bezelRadius - 1.0f) * 2.0f, 1.0f);

    // Fine Knurled Grip Notches (Tactile Machined Texture)
    constexpr int numKnurlTicks = 20;
    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 18));
    for (int i = 0; i < numKnurlTicks; ++i) {
        const float angle = static_cast<float>(i) * (2.0f * std::numbers::pi_v<float> / numKnurlTicks);
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        const float r1 = bezelRadius * 0.88f;
        const float r2 = bezelRadius * 0.98f;
        g.drawLine(center.x + r1 * sinA, center.y - r1 * cosA,
                   center.x + r2 * sinA, center.y - r2 * cosA, 1.0f);
    }

    // 5. 3D Brushed Anodized Dial Cap Face
    const float capRadius = bezelRadius * 0.82f;
    juce::ColourGradient capGrad(
        juce::Colour::fromRGB(34, 38, 48), center.x - capRadius * 0.6f, center.y - capRadius * 0.8f,
        juce::Colour::fromRGB(14, 16, 22), center.x + capRadius * 0.6f, center.y + capRadius * 0.8f, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(center.x - capRadius, center.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // 3D Specular Light Gleam across Face (Simulating Studio Product Lighting)
    juce::ColourGradient gleamGrad(
        juce::Colour::fromRGBA(255, 255, 255, 30), center.x - capRadius * 0.5f, center.y - capRadius * 0.5f,
        juce::Colour::fromRGBA(255, 255, 255, 0), center.x + capRadius * 0.3f, center.y + capRadius * 0.3f, false);
    g.setGradientFill(gleamGrad);
    g.fillEllipse(center.x - capRadius, center.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Center Concave Dimple
    const float dimpleRadius = capRadius * 0.28f;
    juce::ColourGradient dimpleGrad(
        juce::Colour::fromRGB(10, 12, 16), center.x - dimpleRadius, center.y - dimpleRadius,
        juce::Colour::fromRGB(28, 32, 40), center.x + dimpleRadius, center.y + dimpleRadius, false);
    g.setGradientFill(dimpleGrad);
    g.fillEllipse(center.x - dimpleRadius, center.y - dimpleRadius, dimpleRadius * 2.0f, dimpleRadius * 2.0f);
    g.setColour(bevelShadow);
    g.drawEllipse(center.x - dimpleRadius, center.y - dimpleRadius, dimpleRadius * 2.0f, dimpleRadius * 2.0f, 0.8f);

    // 6. Illuminated 3D Neon Indicator Needle
    const float ptrR1 = capRadius * 0.35f;
    const float ptrR2 = capRadius * 0.92f;
    const float sinT = std::sin(toAngle);
    const float cosT = std::cos(toAngle);

    const float x1 = center.x + ptrR1 * sinT;
    const float y1 = center.y - ptrR1 * cosT;
    const float x2 = center.x + ptrR2 * sinT;
    const float y2 = center.y - ptrR2 * cosT;

    // Needle drop shadow
    g.setColour(juce::Colour::fromRGBA(0, 0, 0, 120));
    g.drawLine(x1, y1 + 1.5f, x2, y2 + 1.5f, 2.8f);

    // Needle neon glow
    g.setColour(accentCyanGlow);
    g.drawLine(x1, y1, x2, y2, 4.0f);

    // Needle laser line
    g.setColour(accentCyan.brighter(0.4f));
    g.drawLine(x1, y1, x2, y2, 2.2f);

    // Glowing tip bead
    g.setColour(juce::Colours::white);
    g.fillEllipse(x2 - 1.5f, y2 - 1.5f, 3.0f, 3.0f);
}

void StrataLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const float cornerSize = 4.0f;

    // 1. Button Drop Shadow
    g.setColour(juce::Colour::fromRGBA(0, 0, 0, 70));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), cornerSize);

    // 2. 3D Body Gradient
    juce::Colour topCol = backgroundColour.brighter(shouldDrawButtonAsHighlighted ? 0.15f : 0.08f);
    juce::Colour botCol = backgroundColour.darker(0.12f);

    if (shouldDrawButtonAsDown) {
        topCol = backgroundColour.darker(0.25f);
        botCol = backgroundColour.darker(0.10f);
    } else if (button.getToggleState()) {
        topCol = accentCyan.withAlpha(0.28f);
        botCol = accentCyan.withAlpha(0.12f);
    }

    juce::ColourGradient bodyGrad(topCol, bounds.getX(), bounds.getY(),
                                 botCol, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bodyGrad);
    g.fillRoundedRectangle(bounds, cornerSize);

    // 3. 3D Top Specular Highlight Edge (1px)
    if (!shouldDrawButtonAsDown) {
        g.setColour(bevelHighlight);
        g.drawLine(bounds.getX() + cornerSize, bounds.getY() + 0.5f,
                   bounds.getRight() - cornerSize, bounds.getY() + 0.5f, 1.0f);
    }

    // 4. Border / Neon Glow Outline
    if (button.getToggleState()) {
        g.setColour(accentCyanGlow);
        g.drawRoundedRectangle(bounds, cornerSize, 2.0f);
        g.setColour(accentCyan);
        g.drawRoundedRectangle(bounds, cornerSize, 1.2f);
    } else {
        g.setColour(shouldDrawButtonAsHighlighted ? borderSubtle.brighter(0.2f) : borderSubtle);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
}

void StrataLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat();
    const float pillH = 16.0f;
    const float pillW = 32.0f;
    const float pillY = bounds.getCentreY() - pillH * 0.5f;
    const float pillX = 2.0f;

    auto pillBounds = juce::Rectangle<float>(pillX, pillY, pillW, pillH);
    const bool isOn = button.getToggleState();

    // 1. Recessed Switch Well
    g.setColour(juce::Colour::fromRGB(10, 12, 16));
    g.fillRoundedRectangle(pillBounds, pillH * 0.5f);
    g.setColour(borderSubtle);
    g.drawRoundedRectangle(pillBounds, pillH * 0.5f, 1.0f);

    // 2. 3D Sliding Thumb Orb
    const float thumbDiam = pillH - 4.0f;
    const float thumbX = isOn ? (pillBounds.getRight() - thumbDiam - 2.0f) : (pillBounds.getX() + 2.0f);
    const float thumbY = pillBounds.getY() + 2.0f;

    // Thumb Glow if Active
    if (isOn) {
        g.setColour(accentCyanGlow);
        g.fillEllipse(thumbX - 2.0f, thumbY - 2.0f, thumbDiam + 4.0f, thumbDiam + 4.0f);
    }

    // Thumb Body Gradient
    juce::ColourGradient thumbGrad(
        isOn ? accentCyan.brighter(0.3f) : juce::Colour::fromRGB(70, 78, 96), thumbX, thumbY,
        isOn ? accentCyan.darker(0.2f) : juce::Colour::fromRGB(36, 42, 54), thumbX + thumbDiam, thumbY + thumbDiam, false);
    g.setGradientFill(thumbGrad);
    g.fillEllipse(thumbX, thumbY, thumbDiam, thumbDiam);

    // Specular Highlight on Thumb
    g.setColour(juce::Colour::fromRGBA(255, 255, 255, isOn ? 160 : 70));
    g.drawEllipse(thumbX, thumbY, thumbDiam, thumbDiam, 0.8f);

    // 3. Label Text
    auto textArea = bounds;
    textArea.removeFromLeft(pillW + 8.0f);
    g.setColour(isOn ? textLight : textDim);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), textArea, juce::Justification::centredLeft);
}

void StrataLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                    juce::ComboBox& box) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    // 3D Glassmorphic Background
    juce::ColourGradient bgGrad(
        panelBg.brighter(0.08f), 0.0f, 0.0f,
        panelBg.darker(0.12f), 0.0f, static_cast<float>(height), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Top Highlight Edge
    g.setColour(bevelHighlight);
    g.drawLine(bounds.getX() + 4.0f, bounds.getY() + 0.5f,
               bounds.getRight() - 4.0f, bounds.getY() + 0.5f, 1.0f);

    // Border
    g.setColour(borderSubtle);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // 3D Chevron Arrow on Right
    const float arrowX = bounds.getRight() - 18.0f;
    const float arrowY = bounds.getCentreY();
    juce::Path arrow;
    arrow.startNewSubPath(arrowX, arrowY - 2.5f);
    arrow.lineTo(arrowX + 4.5f, arrowY + 2.5f);
    arrow.lineTo(arrowX + 9.0f, arrowY - 2.5f);

    g.setColour(accentCyan);
    g.strokePath(arrow, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void StrataLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    // Dark Smoked Frosted Glass
    g.setColour(juce::Colour::fromRGB(18, 21, 28));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Neon Accent Border
    g.setColour(borderSubtle.brighter(0.15f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
}

void StrataLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool /*hasSubMenu*/,
                                         const juce::String& text, const juce::String& shortcutKeyText,
                                         const juce::Drawable* /*icon*/, const juce::Colour* textColour) {
    if (isSeparator) {
        auto r = area.reduced(5, 0);
        g.setColour(borderSubtle);
        g.fillRect(r.removeFromTop(1));
        return;
    }

    auto r = area.reduced(2, 1);
    if (isHighlighted && isActive) {
        // Glowing Neon Hover Bar
        g.setColour(accentCyan.withAlpha(0.22f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(accentCyan.withAlpha(0.45f));
        g.drawRoundedRectangle(r.toFloat(), 4.0f, 1.0f);
    }

    // Text Label
    g.setFont(juce::Font(13.0f, isHighlighted ? juce::Font::bold : juce::Font::plain));
    g.setColour(isHighlighted ? textLight : (isActive ? textLight.darker(0.1f) : textDim));

    auto textArea = r.reduced(8, 0);
    if (isTicked) {
        g.setColour(accentGold);
        g.fillEllipse(textArea.removeFromLeft(12).reduced(3).toFloat());
        textArea.removeFromLeft(4);
    }

    g.drawText(text, textArea, juce::Justification::centredLeft, true);

    if (shortcutKeyText.isNotEmpty()) {
        g.setColour(textDim);
        g.setFont(juce::Font(11.0f));
        g.drawText(shortcutKeyText, r.reduced(8, 0), juce::Justification::centredRight, true);
    }
}

} // namespace strata::gui

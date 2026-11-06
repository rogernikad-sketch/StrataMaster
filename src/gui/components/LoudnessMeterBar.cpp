#include "LoudnessMeterBar.h"
#include "../LookAndFeel.h"
#include <algorithm>

namespace strata::gui {

LoudnessMeterBar::LoudnessMeterBar() {
    setOpaque(false);
}

void LoudnessMeterBar::updateLevels(float momentaryLufs, float shortTermLufs, float integratedLufs, float lraLu, float truePeakDb) {
    momentaryLufs_  = momentaryLufs;
    shortTermLufs_  = shortTermLufs;
    integratedLufs_ = integratedLufs;
    lraLu_          = lraLu;
    truePeakDb_     = truePeakDb;

    if (truePeakDb > maxTruePeakHoldDb_) {
        maxTruePeakHoldDb_ = truePeakDb;
    } else {
        maxTruePeakHoldDb_ = maxTruePeakHoldDb_ * 0.995f + truePeakDb * 0.005f;
    }

    repaint();
}

void LoudnessMeterBar::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty()) {
        return;
    }

    // 1. OLED Glass Meter Housing
    juce::ColourGradient housingGrad(
        juce::Colour::fromRGB(18, 22, 30), 0.0f, bounds.getY(),
        juce::Colour::fromRGB(8, 10, 14), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(housingGrad);
    g.fillRoundedRectangle(bounds, 6.0f);

    // 3D Inner Bevel Shadow
    g.setColour(juce::Colour::fromRGBA(0, 0, 0, 150));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    // Top Header: True-Peak Clip indicator & Numeric
    auto topArea = bounds.removeFromTop(24.0f).reduced(4.0f, 2.0f);
    const bool isOverload = (maxTruePeakHoldDb_ > -0.1f);
    
    // OLED Overload Glass Capsule
    if (isOverload) {
        g.setColour(juce::Colour::fromRGBA(255, 30, 30, 45));
        g.fillRoundedRectangle(topArea, 3.0f);
    }
    
    g.setColour(isOverload ? juce::Colour::fromRGB(255, 60, 60) : (maxTruePeakHoldDb_ > -1.0f ? StrataLookAndFeel::accentGold : StrataLookAndFeel::accentCyan));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    if (maxTruePeakHoldDb_ <= -70.0f) {
        g.drawText("--- dBTP", topArea, juce::Justification::centred);
    } else {
        g.drawText(juce::String::formatted("%.1f dBTP", maxTruePeakHoldDb_), topArea, juce::Justification::centred);
    }

    // Bottom Footer: Integrated LUFS & LRA
    auto bottomArea = bounds.removeFromBottom(36.0f).reduced(4.0f, 2.0f);
    g.setColour(StrataLookAndFeel::textLight);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    if (integratedLufs_ <= -70.0f) {
        g.drawText("--- LUFS", bottomArea.removeFromTop(16.0f), juce::Justification::centred);
    } else {
        g.drawText(juce::String::formatted("%.1f LUFS", integratedLufs_), bottomArea.removeFromTop(16.0f), juce::Justification::centred);
    }
    g.setColour(StrataLookAndFeel::textDim);
    g.setFont(juce::Font(9.0f));
    if (integratedLufs_ <= -70.0f) {
        g.drawText("LRA: ---", bottomArea, juce::Justification::centred);
    } else {
        g.drawText(juce::String::formatted("LRA: %.1f", lraLu_), bottomArea, juce::Justification::centred);
    }

    // Center Meter Columns: 2 bars (Momentary LUFS on left, True Peak on right)
    auto meterArea = bounds.reduced(6.0f, 4.0f);
    const float barW = (meterArea.getWidth() - 4.0f) * 0.5f;
    const float meterH = meterArea.getHeight();

    // Bar 1: Momentary LUFS Recessed Screen
    auto lufsRect = juce::Rectangle<float>(meterArea.getX(), meterArea.getY(), barW, meterH);
    g.setColour(juce::Colour(0xFF06080C));
    g.fillRoundedRectangle(lufsRect, 3.0f);

    const float normLufs = std::clamp((momentaryLufs_ - minLufs) / (maxLufs - minLufs), 0.0f, 1.0f);
    const float fillHLufs = normLufs * meterH;
    auto lufsFill = juce::Rectangle<float>(lufsRect.getX(), lufsRect.getBottom() - fillHLufs, barW, fillHLufs);

    juce::ColourGradient lufsGrad(
        StrataLookAndFeel::accentCyan.brighter(0.2f), lufsRect.getX(), lufsRect.getY(),
        juce::Colour(0xFF003852), lufsRect.getX(), lufsRect.getBottom(),
        false);
    g.setGradientFill(lufsGrad);
    g.fillRoundedRectangle(lufsFill, 2.0f);

    // Target -14 LUFS marker line
    const float targetY = lufsRect.getY() + (maxLufs - (-14.0f)) / (maxLufs - minLufs) * meterH;
    g.setColour(StrataLookAndFeel::accentGold);
    g.drawHorizontalLine(static_cast<int>(targetY), lufsRect.getX(), lufsRect.getRight());

    // Bar 2: True Peak dBTP Recessed Screen
    auto tpRect = juce::Rectangle<float>(lufsRect.getRight() + 4.0f, meterArea.getY(), barW, meterH);
    g.setColour(juce::Colour(0xFF06080C));
    g.fillRoundedRectangle(tpRect, 3.0f);

    const float normTp = std::clamp((truePeakDb_ - minDbTP) / (maxDbTP - minDbTP), 0.0f, 1.0f);
    const float fillHTp = normTp * meterH;
    auto tpFill = juce::Rectangle<float>(tpRect.getX(), tpRect.getBottom() - fillHTp, barW, fillHTp);

    juce::ColourGradient tpGrad(
        isOverload ? juce::Colour::fromRGB(255, 60, 60) : StrataLookAndFeel::accentCyan.brighter(0.2f), tpRect.getX(), tpRect.getY(),
        juce::Colour(0xFF002A44), tpRect.getX(), tpRect.getBottom(),
        false);
    g.setGradientFill(tpGrad);
    g.fillRoundedRectangle(tpFill, 2.0f);

    // Floating Peak Hold Indicator Pip
    const float normHold = std::clamp((maxTruePeakHoldDb_ - minDbTP) / (maxDbTP - minDbTP), 0.0f, 1.0f);
    const float holdY = tpRect.getBottom() - normHold * meterH;
    g.setColour(isOverload ? juce::Colour::fromRGB(255, 80, 80) : juce::Colours::white);
    g.drawHorizontalLine(static_cast<int>(holdY), tpRect.getX() + 1.0f, tpRect.getRight() - 1.0f);

    // 0 dB True-Peak reference line
    const float zeroTpY = tpRect.getY() + (maxDbTP - 0.0f) / (maxDbTP - minDbTP) * meterH;
    g.setColour(juce::Colours::red.withAlpha(0.85f));
    g.drawHorizontalLine(static_cast<int>(zeroTpY), tpRect.getX(), tpRect.getRight());

    // Outer 3D Bevel Rim
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 1.0f);
}

} // namespace strata::gui

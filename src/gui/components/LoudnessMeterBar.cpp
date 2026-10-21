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

    g.setColour(juce::Colour(0xFF0F131C));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Top Header: True-Peak Clip indicator & Numeric
    auto topArea = bounds.removeFromTop(24.0f).reduced(4.0f, 2.0f);
    const bool isOverload = (maxTruePeakHoldDb_ > -0.1f);
    g.setColour(isOverload ? juce::Colours::red : (maxTruePeakHoldDb_ > -1.0f ? StrataLookAndFeel::accentGold : StrataLookAndFeel::accentCyan));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(juce::String::formatted("%.1f dBTP", maxTruePeakHoldDb_), topArea, juce::Justification::centred);

    // Bottom Footer: Integrated LUFS & LRA
    auto bottomArea = bounds.removeFromBottom(36.0f).reduced(4.0f, 2.0f);
    g.setColour(StrataLookAndFeel::textLight);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(juce::String::formatted("%.1f LUFS", integratedLufs_), bottomArea.removeFromTop(16.0f), juce::Justification::centred);
    g.setColour(StrataLookAndFeel::textDim);
    g.setFont(juce::Font(9.0f));
    g.drawText(juce::String::formatted("LRA: %.1f", lraLu_), bottomArea, juce::Justification::centred);

    // Center Meter Columns: 2 bars (Momentary LUFS on left, True Peak on right)
    auto meterArea = bounds.reduced(6.0f, 4.0f);
    const float barW = (meterArea.getWidth() - 4.0f) * 0.5f;
    const float meterH = meterArea.getHeight();

    // Bar 1: Momentary LUFS
    auto lufsRect = juce::Rectangle<float>(meterArea.getX(), meterArea.getY(), barW, meterH);
    g.setColour(juce::Colour(0xFF07090D));
    g.fillRoundedRectangle(lufsRect, 3.0f);

    const float normLufs = std::clamp((momentaryLufs_ - minLufs) / (maxLufs - minLufs), 0.0f, 1.0f);
    const float fillHLufs = normLufs * meterH;
    auto lufsFill = juce::Rectangle<float>(lufsRect.getX(), lufsRect.getBottom() - fillHLufs, barW, fillHLufs);

    juce::ColourGradient lufsGrad(
        StrataLookAndFeel::accentCyan, lufsRect.getX(), lufsRect.getY(),
        juce::Colour(0xFF005577), lufsRect.getX(), lufsRect.getBottom(),
        false);
    g.setGradientFill(lufsGrad);
    g.fillRoundedRectangle(lufsFill, 3.0f);

    // Target -14 LUFS marker line
    const float targetY = lufsRect.getY() + (maxLufs - (-14.0f)) / (maxLufs - minLufs) * meterH;
    g.setColour(StrataLookAndFeel::accentGold);
    g.drawHorizontalLine(static_cast<int>(targetY), lufsRect.getX(), lufsRect.getRight());

    // Bar 2: True Peak dBTP
    auto tpRect = juce::Rectangle<float>(lufsRect.getRight() + 4.0f, meterArea.getY(), barW, meterH);
    g.setColour(juce::Colour(0xFF07090D));
    g.fillRoundedRectangle(tpRect, 3.0f);

    const float normTp = std::clamp((truePeakDb_ - minDbTP) / (maxDbTP - minDbTP), 0.0f, 1.0f);
    const float fillHTp = normTp * meterH;
    auto tpFill = juce::Rectangle<float>(tpRect.getX(), tpRect.getBottom() - fillHTp, barW, fillHTp);

    juce::ColourGradient tpGrad(
        isOverload ? juce::Colours::red : StrataLookAndFeel::accentCyan, tpRect.getX(), tpRect.getY(),
        juce::Colour(0xFF003355), tpRect.getX(), tpRect.getBottom(),
        false);
    g.setGradientFill(tpGrad);
    g.fillRoundedRectangle(tpFill, 3.0f);

    // 0 dB True-Peak line
    const float zeroTpY = tpRect.getY() + (maxDbTP - 0.0f) / (maxDbTP - minDbTP) * meterH;
    g.setColour(juce::Colours::red.withAlpha(0.7f));
    g.drawHorizontalLine(static_cast<int>(zeroTpY), tpRect.getX(), tpRect.getRight());

    // Outer border
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 1.0f);
}

} // namespace strata::gui

#include "MaseratiView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>
#include <algorithm>

namespace strata::gui {

MaseratiView::MaseratiView(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {

    bassKnob_ = std::make_unique<Knob>(apvts_, param::MaseratiBass.data(), "BASS");
    addAndMakeVisible(*bassKnob_);

    trebleKnob_ = std::make_unique<Knob>(apvts_, param::MaseratiTreble.data(), "TREBLE");
    addAndMakeVisible(*trebleKnob_);

    sensitivityKnob_ = std::make_unique<Knob>(apvts_, param::MaseratiSensitivity.data(), "SENSITIVITY");
    addAndMakeVisible(*sensitivityKnob_);

    bypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::MaseratiBypass.data(), bypassButton_);
    addAndMakeVisible(bypassButton_);
}

void MaseratiView::updateActivity(float activity) {
    activityMeter_ = activity;
    repaint();
}

void MaseratiView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    auto bounds = getLocalBounds().reduced(24, 20);

    // Main Vintage Chassis Box
    g.setColour(StrataLookAndFeel::panelBg.darker(0.15f));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.5f);

    // Chassis Header
    auto headerArea = bounds.removeFromTop(60).reduced(12, 8);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.setColour(StrataLookAndFeel::accentGold);
    g.drawText("MASERATI TONE PROCESSOR", headerArea.removeFromTop(26), juce::Justification::centred);

    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.setColour(StrataLookAndFeel::textDim);
    g.drawText("Signature Master-Bus Acoustic Contouring: 60Hz Punch, 12kHz Air & Harmonic Sensitivity",
               headerArea, juce::Justification::centred);

    // Center Dynamic Activity Meter Panel
    auto centerArea = bounds.removeFromTop(120).reduced(20, 10);
    g.setColour(juce::Colour(0xFF0F131C));
    g.fillRoundedRectangle(centerArea.toFloat(), 6.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(centerArea.toFloat(), 6.0f, 1.0f);

    // Sensitivity VU/LED Response Bar
    const float meterW = centerArea.getWidth() * 0.7f;
    const float meterH = 16.0f;
    const float meterX = centerArea.getCentreX() - meterW * 0.5f;
    const float meterY = centerArea.getCentreY() - meterH * 0.5f + 10.0f;

    g.setColour(juce::Colour(0xFF161B24));
    g.fillRoundedRectangle(meterX, meterY, meterW, meterH, 4.0f);

    const float normAct = std::clamp(activityMeter_, 0.0f, 1.0f);
    if (normAct > 0.01f) {
        const float fillW = normAct * meterW;
        juce::ColourGradient grad(StrataLookAndFeel::accentCyan, meterX, meterY,
                                  (normAct > 0.7f) ? juce::Colours::red : StrataLookAndFeel::accentGold, meterX + fillW, meterY, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(meterX + 1.0f, meterY + 1.0f, fillW - 2.0f, meterH - 2.0f, 3.0f);
    }

    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(StrataLookAndFeel::textLight);
    g.drawText("DYNAMIC HARMONIC SENSITIVITY DRIVE", static_cast<int>(meterX), static_cast<int>(meterY - 22.0f), static_cast<int>(meterW), 16, juce::Justification::centred);

    // Bottom Decorative Label
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.setColour(StrataLookAndFeel::textDim);
    g.drawText("Analog Console Transformer & Triode Harmonic Saturation Stage",
               bounds.removeFromBottom(24), juce::Justification::centred);
}

void MaseratiView::resized() {
    auto bounds = getLocalBounds().reduced(24, 20);

    // Top Header & Meter
    bounds.removeFromTop(180);

    // Bypass in top right
    bypassButton_.setBounds(getWidth() - 150, 35, 100, 24);

    // Knobs Row
    auto knobArea = bounds.removeFromTop(160).reduced(40, 10);
    const int numKnobs = 3;
    const int knobSpacing = 60;
    const int totalSpacing = knobSpacing * (numKnobs - 1);
    const int knobWidth = (knobArea.getWidth() - totalSpacing) / numKnobs;

    if (bassKnob_) {
        bassKnob_->setBounds(knobArea.removeFromLeft(knobWidth));
        knobArea.removeFromLeft(knobSpacing);
    }
    if (sensitivityKnob_) {
        sensitivityKnob_->setBounds(knobArea.removeFromLeft(knobWidth));
        knobArea.removeFromLeft(knobSpacing);
    }
    if (trebleKnob_) {
        trebleKnob_->setBounds(knobArea.removeFromLeft(knobWidth));
    }
}

} // namespace strata::gui

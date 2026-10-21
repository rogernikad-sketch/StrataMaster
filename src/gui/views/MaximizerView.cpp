#include "MaximizerView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>
#include <algorithm>

namespace strata::gui {

MaximizerView::MaximizerView(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {

    // Threshold (-24 to 0 dB)
    thresholdKnob_ = std::make_unique<Knob>(apvts_, param::LimiterThreshold.data(), "THRESHOLD");
    addAndMakeVisible(*thresholdKnob_);

    // Ceiling (-12 to 0 dBTP)
    ceilingKnob_ = std::make_unique<Knob>(apvts_, param::LimiterCeiling.data(), "CEILING (TP)");
    addAndMakeVisible(*ceilingKnob_);

    // Release (10 to 1000 ms)
    releaseKnob_ = std::make_unique<Knob>(apvts_, param::LimiterRelease.data(), "RELEASE");
    addAndMakeVisible(*releaseKnob_);

    // Lookahead
    lookaheadButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::accentCyan);
    lookaheadAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::LimiterLookahead.data(), lookaheadButton_);
    addAndMakeVisible(lookaheadButton_);

    // Limiter Bypass
    limiterBypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
    limiterBypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::LimiterBypass.data(), limiterBypassButton_);
    addAndMakeVisible(limiterBypassButton_);

    // Pre-Limiter Soft Clipper
    clipperGroup_.setColour(juce::GroupComponent::outlineColourId, StrataLookAndFeel::borderSubtle);
    clipperGroup_.setColour(juce::GroupComponent::textColourId, StrataLookAndFeel::accentGold);
    addAndMakeVisible(clipperGroup_);

    clipperEnableButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    clipperEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::ClipperEnable.data(), clipperEnableButton_);
    addAndMakeVisible(clipperEnableButton_);

    clipperCeilingKnob_ = std::make_unique<Knob>(apvts_, param::ClipperCeiling.data(), "CLIP CEILING");
    addAndMakeVisible(*clipperCeilingKnob_);

    clipperKneeKnob_ = std::make_unique<Knob>(apvts_, param::ClipperKnee.data(), "CLIP KNEE");
    addAndMakeVisible(*clipperKneeKnob_);
}

void MaximizerView::updateLimiterState(float gainReductionDb, float truePeakDb) {
    gainReductionDb_ = gainReductionDb;
    truePeakDb_ = truePeakDb;
    repaint();
}

void MaximizerView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    // Large Gain Reduction & True Peak Display (Top)
    const auto topBounds = getLocalBounds().removeFromTop(getHeight() - 170).reduced(16).toFloat();
    g.setColour(StrataLookAndFeel::panelBg.darker(0.2f));
    g.fillRoundedRectangle(topBounds, 6.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(topBounds, 6.0f, 1.0f);

    // Center Limiter Gain Reduction Meter Bar
    const float meterW = 32.0f;
    const float meterH = topBounds.getHeight() - 60.0f;
    const float meterX = topBounds.getCentreX() - meterW * 0.5f;
    const float meterY = topBounds.getY() + 30.0f;

    g.setColour(juce::Colour(0xFF0F131C));
    g.fillRoundedRectangle(meterX, meterY, meterW, meterH, 4.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(meterX, meterY, meterW, meterH, 4.0f, 1.0f);

    // Downward Gain Reduction Fill
    const float normGR = std::clamp(-gainReductionDb_ / 20.0f, 0.0f, 1.0f);
    if (normGR > 0.01f) {
        const float fillH = normGR * meterH;
        g.setColour(StrataLookAndFeel::accentGold);
        g.fillRoundedRectangle(meterX + 1.0f, meterY + 1.0f, meterW - 2.0f, fillH, 3.0f);
    }

    // Scale marks: 0, -3, -6, -10, -15, -20 dB
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.setColour(StrataLookAndFeel::textDim);
    const float dbs[] = { 0.0f, -3.0f, -6.0f, -10.0f, -15.0f, -20.0f };
    for (float db : dbs) {
        const float y = meterY + (-db / 20.0f) * meterH;
        g.drawHorizontalLine(static_cast<int>(y), meterX - 6.0f, meterX);
        g.drawText(juce::String::formatted("%.0f", db), static_cast<int>(meterX - 34.0f), static_cast<int>(y - 6.0f), 26, 12, juce::Justification::right);
    }

    // Numerical Readouts
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.setColour(StrataLookAndFeel::accentGold);
    g.drawText(juce::String::formatted("GAIN REDUCTION: %+.1f dB", gainReductionDb_),
               static_cast<int>(topBounds.getX() + 20), static_cast<int>(topBounds.getY() + 20), 280, 24,
               juce::Justification::left);

    g.setColour(StrataLookAndFeel::accentCyan);
    g.drawText(juce::String::formatted("MAX TRUE PEAK: %+.2f dBTP", truePeakDb_),
               static_cast<int>(topBounds.getRight() - 300), static_cast<int>(topBounds.getY() + 20), 280, 24,
               juce::Justification::right);

    // Bottom controls panel background
    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 170, getWidth(), 170);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 170, 0.0f, static_cast<float>(getWidth()));
}

void MaximizerView::resized() {
    auto bounds = getLocalBounds();

    auto bottom = bounds.removeFromBottom(170).reduced(16, 12);

    const int knobW = 85;
    const int knobH = 100;

    // Threshold, Ceiling, Release
    thresholdKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(12);
    ceilingKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(12);
    releaseKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));

    bottom.removeFromLeft(20);

    // Lookahead & Bypass
    auto optArea = bottom.removeFromLeft(130);
    lookaheadButton_.setBounds(optArea.removeFromTop(26));
    optArea.removeFromTop(8);
    limiterBypassButton_.setBounds(optArea.removeFromTop(26));

    bottom.removeFromLeft(20);

    // Soft Clipper Group
    auto clipArea = bottom.removeFromLeft(240);
    clipperGroup_.setBounds(clipArea);
    auto clipInner = clipArea.reduced(10, 18);
    clipperEnableButton_.setBounds(clipInner.removeFromTop(24));
    clipperCeilingKnob_->setBounds(clipInner.removeFromLeft(75).withHeight(knobH - 24));
    clipInner.removeFromLeft(10);
    clipperKneeKnob_->setBounds(clipInner.removeFromLeft(75).withHeight(knobH - 24));
}

} // namespace strata::gui

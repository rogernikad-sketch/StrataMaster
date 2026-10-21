#include "DynamicsView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"

namespace strata::gui {

DynamicsView::DynamicsView(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts), crossoverGraph_(apvts) {

    addAndMakeVisible(crossoverGraph_);
    crossoverGraph_.onBandSelected = [this](int b) {
        selectBand(b);
    };

    const juce::String bandNames[] = { "LOW (1)", "LOW-MID (2)", "HIGH-MID (3)", "HIGH (4)" };
    for (int b = 0; b < 4; ++b) {
        bandTabs_[b].setButtonText(bandNames[b]);
        bandTabs_[b].setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
        bandTabs_[b].setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
        bandTabs_[b].onClick = [this, b]() {
            selectBand(b);
        };
        addAndMakeVisible(bandTabs_[b]);
    }

    bandEnableButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    addAndMakeVisible(bandEnableButton_);

    bandBypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
    addAndMakeVisible(bandBypassButton_);

    bandSoloButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::accentGold);
    addAndMakeVisible(bandSoloButton_);

    bandMuteButton_.setColour(juce::ToggleButton::textColourId, juce::Colours::red.brighter(0.2f));
    addAndMakeVisible(bandMuteButton_);

    autoMakeupButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::accentCyan);
    addAndMakeVisible(autoMakeupButton_);

    selectBand(0);
}

void DynamicsView::selectBand(int bandIndex) {
    if (bandIndex < 0 || bandIndex >= 4) return;
    selectedBand_ = bandIndex;
    crossoverGraph_.setSelectedBand(selectedBand_);

    for (int b = 0; b < 4; ++b) {
        if (b == selectedBand_) {
            bandTabs_[b].setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.18f));
            bandTabs_[b].setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
        } else {
            bandTabs_[b].setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
            bandTabs_[b].setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
        }
    }

    rebuildBandControls(selectedBand_);
    resized();
}

void DynamicsView::rebuildBandControls(int bandIndex) {
    const size_t b = static_cast<size_t>(bandIndex);

    bandEnableAttachment_.reset();
    bandEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getDynBandEnableID(b), bandEnableButton_);

    bandBypassAttachment_.reset();
    bandBypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getDynBandBypassID(b), bandBypassButton_);

    bandSoloAttachment_.reset();
    bandSoloAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getDynBandSoloID(b), bandSoloButton_);

    bandMuteAttachment_.reset();
    bandMuteAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getDynBandMuteID(b), bandMuteButton_);

    threshKnob_ = std::make_unique<Knob>(apvts_, param::getDynBandThreshID(b), "THRESH");
    addAndMakeVisible(*threshKnob_);

    ratioKnob_ = std::make_unique<Knob>(apvts_, param::getDynBandRatioID(b), "RATIO");
    addAndMakeVisible(*ratioKnob_);

    attackKnob_ = std::make_unique<Knob>(apvts_, param::getDynBandAttackID(b), "ATTACK");
    addAndMakeVisible(*attackKnob_);

    releaseKnob_ = std::make_unique<Knob>(apvts_, param::getDynBandReleaseID(b), "RELEASE");
    addAndMakeVisible(*releaseKnob_);

    makeupKnob_ = std::make_unique<Knob>(apvts_, param::getDynBandMakeupID(b), "MAKEUP");
    addAndMakeVisible(*makeupKnob_);

    autoMakeupAttachment_.reset();
    autoMakeupAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getDynBandAutoMakeupID(b), autoMakeupButton_);
}

void DynamicsView::updateGainReduction(int bandIndex, float grDb) {
    if (bandIndex == selectedBand_) {
        currentGainReductionDb_ = grDb;
        repaint();
    }
}

void DynamicsView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    // Bottom controls panel background
    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 150, getWidth(), 150);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 150, 0.0f, static_cast<float>(getWidth()));

    // Draw Gain Reduction Meter Bar next to threshold
    const int grX = getWidth() - 75;
    const int grY = getHeight() - 130;
    const int grW = 14;
    const int grH = 110;

    g.setColour(juce::Colour(0xFF0F131C));
    g.fillRect(grX, grY, grW, grH);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRect(grX, grY, grW, grH, 1);

    // GR fill from top downwards (0 to -24 dB)
    const float normGR = std::clamp(-currentGainReductionDb_ / 24.0f, 0.0f, 1.0f);
    if (normGR > 0.01f) {
        const float fillH = normGR * grH;
        g.setColour(StrataLookAndFeel::accentGold);
        g.fillRect(static_cast<float>(grX + 1), static_cast<float>(grY + 1),
                   static_cast<float>(grW - 2), fillH);
    }

    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(StrataLookAndFeel::textDim);
    g.drawText("GR", grX - 10, grY + grH + 2, 34, 12, juce::Justification::centred);
}

void DynamicsView::resized() {
    auto bounds = getLocalBounds();

    // Bottom Panel (150px)
    auto bottom = bounds.removeFromBottom(150).reduced(12, 8);

    // Top row of bottom panel: Band Tabs & Solo/Mute
    auto tabRow = bottom.removeFromTop(30);
    const int tabW = 100;
    for (int b = 0; b < 4; ++b) {
        bandTabs_[b].setBounds(tabRow.removeFromLeft(tabW));
        tabRow.removeFromLeft(4);
    }

    tabRow.removeFromLeft(16);
    bandEnableButton_.setBounds(tabRow.removeFromLeft(80));
    bandBypassButton_.setBounds(tabRow.removeFromLeft(75));
    bandSoloButton_.setBounds(tabRow.removeFromLeft(65));
    bandMuteButton_.setBounds(tabRow.removeFromLeft(65));
    autoMakeupButton_.setBounds(tabRow.removeFromLeft(110));

    bottom.removeFromTop(8);

    // Knobs Row
    const int knobW = 75;
    const int knobH = 90;
    if (threshKnob_) threshKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(10);
    if (ratioKnob_) ratioKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(10);
    if (attackKnob_) attackKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(10);
    if (releaseKnob_) releaseKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(10);
    if (makeupKnob_) makeupKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));

    // Top Area: Crossover Graph
    crossoverGraph_.setBounds(bounds.reduced(6, 6));
}

} // namespace strata::gui

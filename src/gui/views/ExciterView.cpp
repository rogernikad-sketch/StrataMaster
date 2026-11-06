#include "ExciterView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"

namespace strata::gui {

ExciterView::ExciterView(juce::AudioProcessorValueTreeState& apvts)
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

    // Mode Box (Exact 1:1 mapping with SaturationMode: Tape, Tube, Warm, Retro)
    modeBox_.addItem("Tape (Odd Harmonics)", 1);
    modeBox_.addItem("Tube (Triode Even/Odd)", 2);
    modeBox_.addItem("Warm (Even Harmonics)", 3);
    modeBox_.addItem("Retro (Transistor Warmth)", 4);
    modeBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    modeBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    addAndMakeVisible(modeBox_);

    // Oversampling
    oversamplingLabel_.setFont(juce::Font(12.0f, juce::Font::bold));
    oversamplingLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::textDim);
    addAndMakeVisible(oversamplingLabel_);

    oversamplingBox_.addItem("1x (Off)", 1);
    oversamplingBox_.addItem("2x Oversampling", 2);
    oversamplingBox_.addItem("4x Polyphase", 3);
    oversamplingBox_.addItem("8x Linear-Phase", 4);
    oversamplingBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    oversamplingBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    oversamplingAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, param::ExciterOversampling.data(), oversamplingBox_);
    addAndMakeVisible(oversamplingBox_);

    // Bypass
    exciterBypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
    exciterBypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::ExciterBypass.data(), exciterBypassButton_);
    addAndMakeVisible(exciterBypassButton_);

    selectBand(0);
}

void ExciterView::selectBand(int bandIndex) {
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

void ExciterView::rebuildBandControls(int bandIndex) {
    const size_t b = static_cast<size_t>(bandIndex);

    bandEnableAttachment_.reset();
    bandEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getExciterBandEnableID(b), bandEnableButton_);

    modeAttachment_.reset();
    modeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, param::getExciterBandModeID(b), modeBox_);

    driveKnob_ = std::make_unique<Knob>(apvts_, param::getExciterBandDriveID(b), "DRIVE");
    addAndMakeVisible(*driveKnob_);

    mixKnob_ = std::make_unique<Knob>(apvts_, param::getExciterBandMixID(b), "MIX");
    addAndMakeVisible(*mixKnob_);
}

void ExciterView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 140, getWidth(), 140);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 140, 0.0f, static_cast<float>(getWidth()));
}

void ExciterView::resized() {
    auto bounds = getLocalBounds();

    auto bottom = bounds.removeFromBottom(140).reduced(14, 8);

    // Top row: Band tabs & enable
    auto tabRow = bottom.removeFromTop(30);
    const int tabW = 100;
    for (int b = 0; b < 4; ++b) {
        bandTabs_[b].setBounds(tabRow.removeFromLeft(tabW));
        tabRow.removeFromLeft(4);
    }

    tabRow.removeFromLeft(16);
    bandEnableButton_.setBounds(tabRow.removeFromLeft(90));
    tabRow.removeFromLeft(10);
    modeBox_.setBounds(tabRow.removeFromLeft(140));

    // Right of tab row: Oversampling & Bypass
    auto rightOptions = tabRow.removeFromRight(260);
    exciterBypassButton_.setBounds(rightOptions.removeFromRight(120));
    oversamplingBox_.setBounds(rightOptions.removeFromRight(130));

    bottom.removeFromTop(10);

    // Knobs Row
    const int knobW = 80;
    const int knobH = 85;
    if (driveKnob_) driveKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));
    bottom.removeFromLeft(20);
    if (mixKnob_) mixKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));

    crossoverGraph_.setBounds(bounds.reduced(6, 6));
}

} // namespace strata::gui

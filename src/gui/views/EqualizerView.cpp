#include "EqualizerView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"

namespace strata::gui {

EqualizerView::EqualizerView(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {

    addAndMakeVisible(spectrumDisplay_);
    eqGraph_ = std::make_unique<EQNodeGraph>(apvts_);
    addAndMakeVisible(*eqGraph_);

    // Band select buttons 1..8
    for (int b = 0; b < 8; ++b) {
        bandSelectButtons_[b].setButtonText(juce::String(b + 1));
        bandSelectButtons_[b].setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
        bandSelectButtons_[b].setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
        bandSelectButtons_[b].onClick = [this, b]() {
            selectBand(b);
        };
        addAndMakeVisible(bandSelectButtons_[b]);
    }

    // Filter Type Box
    filterTypeBox_.addItem("High-Pass", 1);
    filterTypeBox_.addItem("Low-Shelf", 2);
    filterTypeBox_.addItem("Bell", 3);
    filterTypeBox_.addItem("High-Shelf", 4);
    filterTypeBox_.addItem("Low-Pass", 5);
    filterTypeBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    filterTypeBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    addAndMakeVisible(filterTypeBox_);

    // Band Enable & Dynamic Toggle
    bandEnableButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    addAndMakeVisible(bandEnableButton_);

    dynEnableButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::accentGold);
    addAndMakeVisible(dynEnableButton_);

    // Stereo Mode
    stereoModeBox_.addItemList(juce::StringArray{ "Stereo", "Mid/Side", "Mid Solo", "Side Solo" }, 1);
    stereoModeBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    stereoModeBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    stereoModeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, param::StereoMode.data(), stereoModeBox_);
    addAndMakeVisible(stereoModeBox_);

    // EQ Bypass
    eqBypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
    eqBypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::EQBypass.data(), eqBypassButton_);
    addAndMakeVisible(eqBypassButton_);

    selectBand(0);
}

void EqualizerView::selectBand(int bandIndex) {
    if (bandIndex < 0 || bandIndex >= 8) return;
    selectedBand_ = bandIndex;

    for (int b = 0; b < 8; ++b) {
        if (b == selectedBand_) {
            bandSelectButtons_[b].setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.2f));
            bandSelectButtons_[b].setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
        } else {
            bandSelectButtons_[b].setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
            bandSelectButtons_[b].setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
        }
    }

    rebuildBandControls(selectedBand_);
    resized();
}

void EqualizerView::rebuildBandControls(int bandIndex) {
    const size_t b = static_cast<size_t>(bandIndex);

    bandEnableAttachment_.reset();
    bandEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getEQBandEnableID(b), bandEnableButton_);

    filterTypeAttachment_.reset();
    filterTypeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, param::getEQBandTypeID(b), filterTypeBox_);

    freqKnob_ = std::make_unique<Knob>(apvts_, param::getEQBandFreqID(b), "FREQ");
    addAndMakeVisible(*freqKnob_);

    gainKnob_ = std::make_unique<Knob>(apvts_, param::getEQBandGainID(b), "GAIN");
    addAndMakeVisible(*gainKnob_);

    qKnob_ = std::make_unique<Knob>(apvts_, param::getEQBandQID(b), "Q");
    addAndMakeVisible(*qKnob_);

    dynEnableAttachment_.reset();
    dynEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::getEQBandDynEnableID(b), dynEnableButton_);

    dynThreshKnob_ = std::make_unique<Knob>(apvts_, param::getEQBandDynThreshID(b), "THRESH");
    addAndMakeVisible(*dynThreshKnob_);

    dynRangeKnob_ = std::make_unique<Knob>(apvts_, param::getEQBandDynRangeID(b), "RANGE");
    addAndMakeVisible(*dynRangeKnob_);
}

void EqualizerView::updateSpectrum(analysis::SpectrumAnalyzer& analyzer) {
    spectrumDisplay_.updateSpectrum(analyzer);
}

void EqualizerView::updateCurvesFromAPVTS() {
    if (eqGraph_) {
        eqGraph_->updateCurvesFromAPVTS();
    }
}

void EqualizerView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    // Bottom strip surface background
    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 100, getWidth(), 100);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 100, 0.0f, static_cast<float>(getWidth()));
}

void EqualizerView::resized() {
    auto bounds = getLocalBounds();

    // Bottom Controls Strip (100px)
    auto bottomArea = bounds.removeFromBottom(100).reduced(12, 6);

    // Left band selector buttons (8 buttons, 24px wide each)
    auto bandSelectArea = bottomArea.removeFromLeft(220);
    const int btnW = 24;
    for (int b = 0; b < 8; ++b) {
        bandSelectButtons_[b].setBounds(bandSelectArea.removeFromLeft(btnW));
        bandSelectArea.removeFromLeft(3);
    }

    bottomArea.removeFromLeft(12);

    // Filter Type & Enable
    auto typeArea = bottomArea.removeFromLeft(110);
    bandEnableButton_.setBounds(typeArea.removeFromTop(24));
    typeArea.removeFromTop(4);
    filterTypeBox_.setBounds(typeArea.removeFromTop(24));

    bottomArea.removeFromLeft(16);

    // Freq, Gain, Q Knobs
    const int knobW = 64;
    const int knobH = 80;
    if (freqKnob_) freqKnob_->setBounds(bottomArea.removeFromLeft(knobW).withHeight(knobH));
    if (gainKnob_) gainKnob_->setBounds(bottomArea.removeFromLeft(knobW).withHeight(knobH));
    if (qKnob_) qKnob_->setBounds(bottomArea.removeFromLeft(knobW).withHeight(knobH));

    bottomArea.removeFromLeft(16);

    // Dynamic EQ Controls
    auto dynArea = bottomArea.removeFromLeft(160);
    dynEnableButton_.setBounds(dynArea.removeFromTop(24));
    if (dynThreshKnob_) dynThreshKnob_->setBounds(dynArea.removeFromLeft(64).withHeight(knobH));
    if (dynRangeKnob_) dynRangeKnob_->setBounds(dynArea.removeFromLeft(64).withHeight(knobH));

    // Right side: Stereo mode & EQ bypass
    auto rightOptions = bottomArea.removeFromRight(120);
    stereoModeBox_.setBounds(rightOptions.removeFromTop(24));
    rightOptions.removeFromTop(8);
    eqBypassButton_.setBounds(rightOptions.removeFromTop(24));

    // Center Display Area (Spectrum + Node Graph overlaid)
    auto centerArea = bounds.reduced(6, 6);
    spectrumDisplay_.setBounds(centerArea);
    if (eqGraph_) {
        eqGraph_->setBounds(centerArea);
    }
}

} // namespace strata::gui

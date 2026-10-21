#include "StrataAudioEditor.h"
#include "../engine/StrataAudioProcessor.h"
#include "../engine/ParameterIDs.h"

namespace strata::gui {

StrataAudioEditor::StrataAudioEditor(engine::StrataAudioProcessor& processor)
    : AudioProcessorEditor(processor),
      processor_(processor),
      moduleRackBar_(processor.getAPVTS()),
      equalizerView_(processor.getAPVTS()),
      dynamicsView_(processor.getAPVTS()),
      exciterView_(processor.getAPVTS()),
      imagerView_(processor.getAPVTS()),
      maximizerView_(processor.getAPVTS()),
      assistantView_(processor.getAPVTS(), processor.getAutoMasterEngine()) {

    setLookAndFeel(&lookAndFeel_);

    // 1. Header Banner & Branding
    titleLabel_.setText("STRATA MASTER", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(15.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentCyan);
    addAndMakeVisible(titleLabel_);

    // 2. Preset Management with Step Buttons
    prevPresetBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    prevPresetBtn_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textLight);
    prevPresetBtn_.onClick = [this]() { selectPrevPreset(); };
    addAndMakeVisible(prevPresetBtn_);

    presetBox_.setTextWhenNothingSelected("Select Preset");
    presetBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    presetBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    populatePresetBox();
    presetBox_.onChange = [this]() {
        const int selectedId = presetBox_.getSelectedId();
        if (selectedId > 0) {
            processor_.getPresetManager().loadFactoryPreset(static_cast<size_t>(selectedId - 1));
            equalizerView_.updateCurvesFromAPVTS();
        }
    };
    addAndMakeVisible(presetBox_);

    nextPresetBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    nextPresetBtn_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textLight);
    nextPresetBtn_.onClick = [this]() { selectNextPreset(); };
    addAndMakeVisible(nextPresetBtn_);

    // 3. Master Assistant Header Trigger
    assistantHeaderBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.12f));
    assistantHeaderBtn_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
    assistantHeaderBtn_.onClick = [this]() {
        moduleRackBar_.setSelectedModule(ModuleID::Assistant);
        showModule(ModuleID::Assistant);
    };
    addAndMakeVisible(assistantHeaderBtn_);

    // 4. Reference A/B Button & Gain Match
    abButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    abButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
    abButton_.onClick = [this]() {
        auto& ref = processor_.getReferencePlayer();
        if (ref.getABMode() == engine::ABMode::TrackA) {
            ref.setABMode(engine::ABMode::TrackB);
            abButton_.setButtonText("TRACK B (REF)");
            abButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::accentCyan.darker(0.6f));
        } else {
            ref.setABMode(engine::ABMode::TrackA);
            abButton_.setButtonText("TRACK A (MASTER)");
            abButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
        }
    };
    addAndMakeVisible(abButton_);

    gainMatchButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
    gainMatchButton_.onClick = [this]() {
        processor_.getReferencePlayer().setGainMatchEnabled(gainMatchButton_.getToggleState());
    };
    addAndMakeVisible(gainMatchButton_);

    // 5. Global Bypass Toggle
    bypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getAPVTS(), param::Bypass.data(), bypassButton_);
    addAndMakeVisible(bypassButton_);

    // 6. Module Rack Bar
    moduleRackBar_.onModuleSelected = [this](ModuleID id) {
        showModule(id);
    };
    addAndMakeVisible(moduleRackBar_);

    // 7. Active Views
    addAndMakeVisible(equalizerView_);
    addChildComponent(dynamicsView_);
    addChildComponent(exciterView_);
    addChildComponent(imagerView_);
    addChildComponent(maximizerView_);
    addChildComponent(assistantView_);

    assistantView_.onMasterApplied = [this]() {
        equalizerView_.updateCurvesFromAPVTS();
    };

    // 8. Right Master I/O & Metering Panel
    inputGainKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::InputGain.data(), "IN GAIN");
    addAndMakeVisible(*inputGainKnob_);
    addAndMakeVisible(inputMeter_);

    addAndMakeVisible(loudnessMeterBar_);

    correlationLabel_.setText("CORR: +1.00", juce::dontSendNotification);
    correlationLabel_.setFont(juce::Font(11.0f, juce::Font::bold));
    correlationLabel_.setJustificationType(juce::Justification::centred);
    correlationLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::textDim);
    addAndMakeVisible(correlationLabel_);

    addAndMakeVisible(outputMeter_);
    outputGainKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::OutputGain.data(), "OUT GAIN");
    addAndMakeVisible(*outputGainKnob_);

    // Resizable Window Support
    setResizable(true, true);
    setResizeLimits(960, 580, 1600, 1000);
    setSize(1040, 640);

    showModule(ModuleID::Equalizer);
    startTimerHz(60);
}

StrataAudioEditor::~StrataAudioEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void StrataAudioEditor::populatePresetBox() {
    presetBox_.clear();
    const auto& factoryPresets = processor_.getPresetManager().getFactoryPresets();
    for (size_t i = 0; i < factoryPresets.size(); ++i) {
        presetBox_.addItem(factoryPresets[i].name, static_cast<int>(i + 1));
    }
}

void StrataAudioEditor::selectPrevPreset() {
    int currentId = presetBox_.getSelectedId();
    if (currentId <= 1) {
        currentId = presetBox_.getNumItems();
    } else {
        currentId--;
    }
    presetBox_.setSelectedId(currentId);
}

void StrataAudioEditor::selectNextPreset() {
    int currentId = presetBox_.getSelectedId();
    if (currentId >= presetBox_.getNumItems()) {
        currentId = 1;
    } else {
        currentId++;
    }
    presetBox_.setSelectedId(currentId);
}

void StrataAudioEditor::showModule(ModuleID id) {
    equalizerView_.setVisible(id == ModuleID::Equalizer);
    dynamicsView_.setVisible(id == ModuleID::Dynamics);
    exciterView_.setVisible(id == ModuleID::Exciter);
    imagerView_.setVisible(id == ModuleID::Imager);
    maximizerView_.setVisible(id == ModuleID::Maximizer);
    assistantView_.setVisible(id == ModuleID::Assistant);

    resized();
}

void StrataAudioEditor::timerCallback() {
    // 1. Peak I/O Meters
    float inL = 0.0f, inR = 0.0f, outL = 0.0f, outR = 0.0f;
    processor_.getMeterLevels(inL, inR, outL, outR);
    inputMeter_.setLevel(inL, inR);
    outputMeter_.setLevel(outL, outR);

    // 2. Correlation
    correlationValue_ = processor_.getCorrelationMeter().getCorrelation();
    correlationLabel_.setText(juce::String::formatted("CORR: %+.2f", correlationValue_), juce::dontSendNotification);
    if (correlationValue_ >= 0.2f) {
        correlationLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentCyan);
    } else if (correlationValue_ >= -0.2f) {
        correlationLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentGold);
    } else {
        correlationLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
    }

    if (imagerView_.isVisible()) {
        imagerView_.setCorrelation(correlationValue_);
    }

    // 3. Equalizer 60 FPS Spectrum
    if (equalizerView_.isVisible()) {
        equalizerView_.updateSpectrum(processor_.getSpectrumAnalyzer());
    }

    // 4. Loudness & True Peak
    const auto& bs1770 = processor_.getBS1770Meter();
    const auto& tp = processor_.getTruePeakMeter();
    loudnessMeterBar_.updateLevels(
        bs1770.getMomentaryLoudness(),
        bs1770.getShortTermLoudness(),
        bs1770.getIntegratedLoudness(),
        bs1770.getLoudnessRange(),
        tp.getMaxTruePeakDb());

    // 5. Maximizer View Telemetry
    if (maximizerView_.isVisible()) {
        maximizerView_.updateLimiterState(
            processor_.getTruePeakLimiter().getGainReductionDb(),
            tp.getMaxTruePeakDb());
    }

    // 6. Dynamics View Telemetry
    if (dynamicsView_.isVisible()) {
        dynamicsView_.updateGainReduction(0, processor_.getMultibandDynamics().getGainReductionDb(0));
    }

    // 7. Assistant View Telemetry
    if (assistantView_.isVisible()) {
        assistantView_.updateTelemetry();
    }
}

void StrataAudioEditor::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    // Top Header Banner
    g.setColour(StrataLookAndFeel::panelBg);
    g.fillRect(0, 0, getWidth(), 44);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(44, 0.0f, static_cast<float>(getWidth()));

    // Right Metering Panel Background
    const int rightWidth = 145;
    const int rightX = getWidth() - rightWidth;
    g.setColour(StrataLookAndFeel::panelBg.darker(0.12f));
    g.fillRect(rightX, 44, rightWidth, getHeight() - 44);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawVerticalLine(rightX, 44.0f, static_cast<float>(getHeight()));
}

void StrataAudioEditor::resized() {
    auto bounds = getLocalBounds();

    // 1. Top Header Bar (44px)
    auto header = bounds.removeFromTop(44).reduced(10, 6);
    titleLabel_.setBounds(header.removeFromLeft(150));
    prevPresetBtn_.setBounds(header.removeFromLeft(24));
    header.removeFromLeft(2);
    presetBox_.setBounds(header.removeFromLeft(160));
    header.removeFromLeft(2);
    nextPresetBtn_.setBounds(header.removeFromLeft(24));

    header.removeFromLeft(14);
    assistantHeaderBtn_.setBounds(header.removeFromLeft(145));

    header.removeFromLeft(14);
    abButton_.setBounds(header.removeFromLeft(130));
    gainMatchButton_.setBounds(header.removeFromLeft(105));
    bypassButton_.setBounds(header.removeFromRight(85));

    // 2. Right Metering Strip (145px)
    auto right = bounds.removeFromRight(145).reduced(8, 8);
    auto topMeters = right.removeFromTop(75);
    inputGainKnob_->setBounds(topMeters.removeFromLeft(60));
    inputMeter_.setBounds(topMeters.removeFromLeft(22));

    right.removeFromTop(6);
    correlationLabel_.setBounds(right.removeFromTop(18));
    right.removeFromTop(4);
    loudnessMeterBar_.setBounds(right.removeFromTop(getHeight() - 250));

    right.removeFromTop(6);
    auto bottomMeters = right.removeFromBottom(75);
    outputMeter_.setBounds(bottomMeters.removeFromLeft(22));
    outputGainKnob_->setBounds(bottomMeters.removeFromLeft(60));

    // 3. Module Rack Bar (38px)
    moduleRackBar_.setBounds(bounds.removeFromTop(38));

    // 4. Center Workspace for Active View
    auto workspace = bounds.reduced(4, 4);
    equalizerView_.setBounds(workspace);
    dynamicsView_.setBounds(workspace);
    exciterView_.setBounds(workspace);
    imagerView_.setBounds(workspace);
    maximizerView_.setBounds(workspace);
    assistantView_.setBounds(workspace);
}

} // namespace strata::gui

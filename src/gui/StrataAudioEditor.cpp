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
      maseratiView_(processor.getAPVTS()),
      assistantView_(processor.getAPVTS(), processor.getAutoMasterEngine(), processor.getReferencePlayer()) {

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

    // Audition Track & Test Tone Generators (for Standalone and Auditioning)
    loadAudioBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    loadAudioBtn_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
    loadAudioBtn_.onClick = [this]() {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Select Audio File for Mastering Audition...",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.existsAsFile()) {
                processor_.loadAuditionAudio(result);
                processor_.getReferencePlayer().loadAudioFile(result);
                loadAudioBtn_.setButtonText(result.getFileName().substring(0, 10));
                auditionPlayBtn_.setButtonText("AUDITION: ON");
                auditionPlayBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::accentCyan.darker(0.6f));
            }
        });
    };
    addAndMakeVisible(loadAudioBtn_);

    auditionPlayBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    auditionPlayBtn_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textLight);
    auditionPlayBtn_.onClick = [this]() {
        const bool playing = !processor_.isAuditionPlaying();
        processor_.setAuditionPlaying(playing);
        if (playing) {
            auditionPlayBtn_.setButtonText("AUDITION: ON");
            auditionPlayBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::accentCyan.darker(0.6f));
        } else {
            auditionPlayBtn_.setButtonText("AUDITION: OFF");
            auditionPlayBtn_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
        }
    };
    addAndMakeVisible(auditionPlayBtn_);

    testToneBox_.addItem("Host Input", 1);
    testToneBox_.addItem("Pink Noise", 2);
    testToneBox_.addItem("1kHz Sine", 3);
    testToneBox_.setSelectedId(1, juce::dontSendNotification);
    testToneBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    testToneBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    testToneBox_.onChange = [this]() {
        const int id = testToneBox_.getSelectedId();
        processor_.setAuditionTestTone(id - 1);
    };
    addAndMakeVisible(testToneBox_);

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
    addChildComponent(maseratiView_);
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

    // Resizable Window Support (Up to 4K UHD 3840x2160)
    setResizable(true, true);
    setResizeLimits(960, 580, 3840, 2160);
    setSize(1200, 750);

    const auto cmd = juce::JUCEApplicationBase::getCommandLineParameters();
    if (cmd.containsIgnoreCase("dynamics")) {
        moduleRackBar_.setSelectedModule(ModuleID::Dynamics);
        showModule(ModuleID::Dynamics);
    } else if (cmd.containsIgnoreCase("exciter")) {
        moduleRackBar_.setSelectedModule(ModuleID::Exciter);
        showModule(ModuleID::Exciter);
    } else if (cmd.containsIgnoreCase("imager")) {
        moduleRackBar_.setSelectedModule(ModuleID::Imager);
        showModule(ModuleID::Imager);
    } else if (cmd.containsIgnoreCase("maserati")) {
        moduleRackBar_.setSelectedModule(ModuleID::Maserati);
        showModule(ModuleID::Maserati);
    } else if (cmd.containsIgnoreCase("maximizer")) {
        moduleRackBar_.setSelectedModule(ModuleID::Maximizer);
        showModule(ModuleID::Maximizer);
    } else if (cmd.containsIgnoreCase("assistant")) {
        moduleRackBar_.setSelectedModule(ModuleID::Assistant);
        showModule(ModuleID::Assistant);
    } else {
        showModule(ModuleID::Equalizer);
    }
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
    maseratiView_.setVisible(id == ModuleID::Maserati);
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
        std::array<std::pair<float, float>, engine::StrataAudioProcessor::LISSAJOUS_SAMPLES> lissPoints;
        processor_.getLissajousPoints(lissPoints);
        imagerView_.updateLissajous(lissPoints);
        imagerView_.setCorrelation(correlationValue_);
    }

    if (maseratiView_.isVisible()) {
        maseratiView_.updateActivity(processor_.getMaseratiProcessor().getSensitivityActivity());
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
    auto bounds = getLocalBounds().toFloat();

    // 1. 4K Deep Obsidian Titanium Chassis Background
    juce::ColourGradient chassisGrad(
        juce::Colour::fromRGB(13, 15, 20), 0.0f, 0.0f,
        juce::Colour::fromRGB(20, 24, 32), bounds.getWidth(), bounds.getHeight(), false);
    g.setGradientFill(chassisGrad);
    g.fillAll();

    // 2. 3D Machined Header Strip (44px)
    auto headerRect = juce::Rectangle<float>(0.0f, 0.0f, bounds.getWidth(), 44.0f);
    juce::ColourGradient headerGrad(
        juce::Colour::fromRGB(30, 35, 47), 0.0f, 0.0f,
        juce::Colour::fromRGB(18, 21, 28), 0.0f, 44.0f, false);
    g.setGradientFill(headerGrad);
    g.fillRect(headerRect);

    // 3D Top Specular Chamfer Highlight (1px)
    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 38));
    g.drawLine(0.0f, 0.5f, bounds.getWidth(), 0.5f, 1.0f);

    // Header Bottom Recessed Groove & Drop Shadow
    g.setColour(juce::Colour::fromRGB(8, 10, 14));
    g.drawHorizontalLine(43, 0.0f, bounds.getWidth());
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(44, 0.0f, bounds.getWidth());

    // 3. 3D Recessed Right Metering Dock (145px)
    const float rightWidth = 145.0f;
    const float rightX = bounds.getWidth() - rightWidth;
    auto dockRect = juce::Rectangle<float>(rightX, 44.0f, rightWidth, bounds.getHeight() - 44.0f);

    juce::ColourGradient dockGrad(
        juce::Colour::fromRGB(16, 18, 24), rightX, 44.0f,
        juce::Colour::fromRGB(10, 12, 16), bounds.getWidth(), bounds.getHeight(), false);
    g.setGradientFill(dockGrad);
    g.fillRect(dockRect);

    // Dock Left Bevel Shadow & Specular Lip
    g.setColour(juce::Colour::fromRGBA(0, 0, 0, 130));
    g.drawVerticalLine(static_cast<int>(rightX), 44.0f, bounds.getHeight());
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawVerticalLine(static_cast<int>(rightX + 1.0f), 44.0f, bounds.getHeight());

    // Subtle 4K Neon Dock Corner Indicator
    g.setColour(StrataLookAndFeel::accentCyan.withAlpha(0.7f));
    g.fillRect(rightX + 1.0f, 44.0f, 2.0f, 28.0f);
}

void StrataAudioEditor::resized() {
    auto bounds = getLocalBounds();

    // 1. Top Header Bar (44px)
    auto header = bounds.removeFromTop(44).reduced(8, 6);
    titleLabel_.setBounds(header.removeFromLeft(125));
    prevPresetBtn_.setBounds(header.removeFromLeft(20));
    header.removeFromLeft(2);
    presetBox_.setBounds(header.removeFromLeft(130));
    header.removeFromLeft(2);
    nextPresetBtn_.setBounds(header.removeFromLeft(20));

    header.removeFromLeft(10);
    loadAudioBtn_.setBounds(header.removeFromLeft(75));
    header.removeFromLeft(4);
    auditionPlayBtn_.setBounds(header.removeFromLeft(85));
    header.removeFromLeft(4);
    testToneBox_.setBounds(header.removeFromLeft(95));

    header.removeFromLeft(10);
    assistantHeaderBtn_.setBounds(header.removeFromLeft(135));

    header.removeFromLeft(10);
    abButton_.setBounds(header.removeFromLeft(95));
    header.removeFromLeft(4);
    gainMatchButton_.setBounds(header.removeFromLeft(85));
    bypassButton_.setBounds(header.removeFromRight(75));

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
    maseratiView_.setBounds(workspace);
    assistantView_.setBounds(workspace);
}

} // namespace strata::gui

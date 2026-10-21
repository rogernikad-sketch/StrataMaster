#include "StrataAudioEditor.h"
#include "../engine/StrataAudioProcessor.h"
#include "../engine/ParameterIDs.h"

namespace strata::gui {

StrataAudioEditor::StrataAudioEditor(engine::StrataAudioProcessor& processor)
    : AudioProcessorEditor(processor), processor_(processor) {

    setLookAndFeel(&lookAndFeel_);

    // 1. Header Banner
    titleLabel_.setText("STRATA MASTER", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentCyan);
    addAndMakeVisible(titleLabel_);

    // 2. Preset Selector
    presetBox_.setTextWhenNothingSelected("Select Preset");
    presetBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    presetBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    populatePresetBox();
    presetBox_.onChange = [this]() {
        const int selectedId = presetBox_.getSelectedId();
        if (selectedId > 0) {
            processor_.getPresetManager().loadFactoryPreset(static_cast<size_t>(selectedId - 1));
            if (eqGraph_) {
                eqGraph_->updateCurvesFromAPVTS();
            }
        }
    };
    addAndMakeVisible(presetBox_);

    // 3. Assistant Button
    assistantButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.1f));
    assistantButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
    assistantButton_.onClick = [this]() {
        auto& engine = processor_.getAutoMasterEngine();
        if (engine.getState() == assistant::AssistantState::Idle) {
            engine.startListening();
            assistantButton_.setButtonText("LISTENING (5s)...");
        } else if (engine.getState() == assistant::AssistantState::Ready) {
            engine.applyRecommendations(processor_.getAPVTS());
            assistantButton_.setButtonText("MASTER APPLIED!");
            if (eqGraph_) {
                eqGraph_->updateCurvesFromAPVTS();
            }
        }
    };
    addAndMakeVisible(assistantButton_);

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

    // 5. Bypass Button
    bypassButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.getAPVTS(), param::Bypass.data(), bypassButton_);
    addAndMakeVisible(bypassButton_);

    // 6. Left Input Stage
    addAndMakeVisible(inputMeter_);
    inputGainKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::InputGain.data(), "IN GAIN");
    addAndMakeVisible(*inputGainKnob_);

    // 7. Center Spectrum & Interactive EQ Graph (Layered)
    addAndMakeVisible(spectrumDisplay_);
    eqGraph_ = std::make_unique<EQNodeGraph>(processor_.getAPVTS());
    addAndMakeVisible(*eqGraph_);

    // 8. Center Knobs: Width, Stereo Mode, Limiter Threshold, Limiter Ceiling
    stereoWidthKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::StereoWidth.data(), "WIDTH");
    addAndMakeVisible(*stereoWidthKnob_);

    stereoModeBox_.addItemList(juce::StringArray{"Stereo", "Mid/Side", "Mid Solo", "Side Solo"}, 1);
    stereoModeBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    stereoModeBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    stereoModeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor_.getAPVTS(), param::StereoMode.data(), stereoModeBox_);
    addAndMakeVisible(stereoModeBox_);

    limiterThresholdKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::LimiterThreshold.data(), "LIMIT THRESH");
    addAndMakeVisible(*limiterThresholdKnob_);

    limiterCeilingKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::LimiterCeiling.data(), "CEILING");
    addAndMakeVisible(*limiterCeilingKnob_);

    // 9. Correlation Label
    correlationLabel_.setText("CORR: +1.00", juce::dontSendNotification);
    correlationLabel_.setFont(juce::Font(11.0f, juce::Font::plain));
    correlationLabel_.setJustificationType(juce::Justification::centred);
    correlationLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::textDim);
    addAndMakeVisible(correlationLabel_);

    // 10. Right Stage: Loudness Bar & Output
    addAndMakeVisible(loudnessMeterBar_);
    addAndMakeVisible(outputMeter_);
    outputGainKnob_ = std::make_unique<Knob>(processor_.getAPVTS(), param::OutputGain.data(), "OUT GAIN");
    addAndMakeVisible(*outputGainKnob_);

    setSize(960, 560);
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

void StrataAudioEditor::timerCallback() {
    // 1. Input / Output Peak Meters
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

    // 3. 60 FPS Spectrum Update
    spectrumDisplay_.updateSpectrum(processor_.getSpectrumAnalyzer());

    // 4. Loudness & True Peak Meters
    const auto& bs1770 = processor_.getBS1770Meter();
    const auto& tp = processor_.getTruePeakMeter();
    loudnessMeterBar_.updateLevels(
        bs1770.getMomentaryLoudness(),
        bs1770.getShortTermLoudness(),
        bs1770.getIntegratedLoudness(),
        bs1770.getLoudnessRange(),
        tp.getMaxTruePeakDb());

    // 5. Assistant State Updates
    auto& engine = processor_.getAutoMasterEngine();
    if (engine.getState() == assistant::AssistantState::Listening) {
        const float prog = engine.getListeningProgressPercent();
        assistantButton_.setButtonText(juce::String::formatted("ANALYZING (%.0f%%)", prog));
    } else if (engine.getState() == assistant::AssistantState::Ready && assistantButton_.getButtonText().startsWith("ANALYZING")) {
        assistantButton_.setButtonText("APPLY MASTER");
    }
}

void StrataAudioEditor::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    // Top Header Banner
    g.setColour(StrataLookAndFeel::panelBg);
    g.fillRect(0, 0, getWidth(), 46);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(46, 0.0f, static_cast<float>(getWidth()));

    // Bottom Controls Banner
    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 110, getWidth(), 110);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 110, 0.0f, static_cast<float>(getWidth()));

    // Phase Correlation Mini Bar in bottom center
    const float barW = 160.0f;
    const float barH = 6.0f;
    const float barX = (getWidth() - barW) * 0.5f;
    const float barY = static_cast<float>(getHeight() - 24);

    g.setColour(juce::Colour(0xFF0F131C));
    g.fillRoundedRectangle(barX, barY, barW, barH, 3.0f);

    const float normCorr = (correlationValue_ + 1.0f) * 0.5f;
    const float needleX = barX + normCorr * barW;
    g.setColour(StrataLookAndFeel::accentCyan);
    g.fillEllipse(needleX - 3.0f, barY - 2.0f, 6.0f, 10.0f);
}

void StrataAudioEditor::resized() {
    auto bounds = getLocalBounds();

    // 1. Top Header (46px)
    auto header = bounds.removeFromTop(46).reduced(12, 6);
    titleLabel_.setBounds(header.removeFromLeft(160));
    presetBox_.setBounds(header.removeFromLeft(180));
    header.removeFromLeft(16);
    assistantButton_.setBounds(header.removeFromLeft(140));
    header.removeFromLeft(16);
    abButton_.setBounds(header.removeFromLeft(130));
    gainMatchButton_.setBounds(header.removeFromLeft(110));
    bypassButton_.setBounds(header.removeFromRight(90));

    // 2. Bottom Controls Strip (110px)
    auto bottom = bounds.removeFromBottom(110).reduced(16, 8);
    const int knobW = 80;
    const int knobH = 90;

    auto bottomCenter = bottom.withSizeKeepingCentre(500, 90);
    stereoWidthKnob_->setBounds(bottomCenter.getX(), bottomCenter.getY(), knobW, knobH);
    stereoModeBox_.setBounds(bottomCenter.getX() + knobW + 10, bottomCenter.getY() + 32, 110, 24);
    correlationLabel_.setBounds(bottomCenter.getX() + knobW + 10, bottomCenter.getY() + 60, 110, 16);

    limiterThresholdKnob_->setBounds(bottomCenter.getX() + 240, bottomCenter.getY(), knobW, knobH);
    limiterCeilingKnob_->setBounds(bottomCenter.getX() + 340, bottomCenter.getY(), knobW, knobH);

    // 3. Left Input Strip (90px)
    auto leftArea = bounds.removeFromLeft(90).reduced(10, 8);
    inputGainKnob_->setBounds(leftArea.removeFromTop(80));
    inputMeter_.setBounds(leftArea.reduced(14, 4));

    // 4. Right Output & Loudness Strip (150px)
    auto rightArea = bounds.removeFromRight(150).reduced(10, 8);
    auto rightTop = rightArea.removeFromTop(80);
    outputGainKnob_->setBounds(rightTop.removeFromRight(70));
    outputMeter_.setBounds(rightTop.reduced(10, 0));
    loudnessMeterBar_.setBounds(rightArea.reduced(4, 2));

    // 5. Center Visualizer & Interactive EQ Graph
    auto centerArea = bounds.reduced(8, 8);
    spectrumDisplay_.setBounds(centerArea);
    eqGraph_->setBounds(centerArea); // Overlaid directly on top of spectrum
}

} // namespace strata::gui

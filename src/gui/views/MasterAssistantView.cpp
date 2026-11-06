#include "../../assistant/ReferenceMatcher.h"
#include "MasterAssistantView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <algorithm>

namespace strata::gui {

MasterAssistantView::MasterAssistantView(juce::AudioProcessorValueTreeState& apvts,
                                         assistant::AutoMasterEngine& assistantEngine,
                                         engine::ReferencePlayer& referencePlayer)
    : apvts_(apvts), assistantEngine_(assistantEngine), refPlayer_(referencePlayer) {

    // 1. Left Sidebar: Header & Search
    sidebarTitleLabel_.setFont(juce::Font(12.0f, juce::Font::bold));
    sidebarTitleLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentGold);
    addAndMakeVisible(sidebarTitleLabel_);

    searchBox_.setTextToShowWhenEmpty("Search genres...", StrataLookAndFeel::textDim);
    searchBox_.setColour(juce::TextEditor::backgroundColourId, StrataLookAndFeel::panelBg.darker(0.2f));
    searchBox_.setColour(juce::TextEditor::textColourId, StrataLookAndFeel::textLight);
    searchBox_.setColour(juce::TextEditor::outlineColourId, StrataLookAndFeel::borderSubtle);
    searchBox_.onTextChange = [this]() {
        populateGenreProfiles();
    };
    addAndMakeVisible(searchBox_);

    // Genre ListBox
    genreListBox_.setRowHeight(36);
    genreListBox_.setColour(juce::ListBox::backgroundColourId, StrataLookAndFeel::panelBg.darker(0.3f));
    genreListBox_.setColour(juce::ListBox::outlineColourId, StrataLookAndFeel::borderSubtle);
    addAndMakeVisible(genreListBox_);

    // 2. Action Buttons in Right Panel Header
    listenButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.15f));
    listenButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
    listenButton_.onClick = [this]() {
        assistantEngine_.startListening();
        updateTelemetry();
    };
    addAndMakeVisible(listenButton_);

    applyButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::accentCyan.darker(0.5f));
    applyButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
    applyButton_.onClick = [this]() {
        assistantEngine_.applyRecommendations(apvts_);
        if (onMasterApplied) onMasterApplied();
        repaint();
    };
    addAndMakeVisible(applyButton_);

    applyGenreDefaultsButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.1f));
    applyGenreDefaultsButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
    applyGenreDefaultsButton_.onClick = [this]() {
        assistantEngine_.applyGenreDefaults(apvts_);
        if (onMasterApplied) onMasterApplied();
        repaint();
    };
    addAndMakeVisible(applyGenreDefaultsButton_);

    // 3. Macro Controls Setup
    auto setupMacroSlider = [this](juce::Slider& slider, juce::Label& label,
                                   double minVal, double maxVal, double defaultVal,
                                   const juce::String& suffix, auto updateFn) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 16);
        slider.setRange(minVal, maxVal, 1.0);
        slider.setValue(defaultVal, juce::dontSendNotification);
        slider.setTextValueSuffix(suffix);
        slider.setColour(juce::Slider::rotarySliderFillColourId, StrataLookAndFeel::accentCyan);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, StrataLookAndFeel::borderSubtle);
        slider.setColour(juce::Slider::textBoxTextColourId, StrataLookAndFeel::textLight);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, StrataLookAndFeel::panelBg);
        slider.setColour(juce::Slider::textBoxOutlineColourId, StrataLookAndFeel::borderSubtle);
        slider.onValueChange = [this, &slider, updateFn]() {
            updateFn(static_cast<float>(slider.getValue()));
            assistantEngine_.applyMacroAdjustments(apvts_);
            if (onMasterApplied) onMasterApplied();
        };
        addAndMakeVisible(slider);

        label.setFont(juce::Font(11.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, StrataLookAndFeel::textLight);
        addAndMakeVisible(label);
    };

    macroSectionLabel_.setFont(juce::Font(11.0f, juce::Font::bold));
    macroSectionLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentGold);
    addAndMakeVisible(macroSectionLabel_);

    setupMacroSlider(tonalSlider_, tonalLabel_, 0.0, 200.0, 100.0, "%",
                     [this](float val) { assistantEngine_.setMacroTonalBalance(val); });

    setupMacroSlider(loudnessSlider_, loudnessLabel_, -6.0, 6.0, 0.0, " dB",
                     [this](float val) { assistantEngine_.setMacroLoudness(val); });

    setupMacroSlider(dynamicsSlider_, dynamicsLabel_, 0.0, 200.0, 100.0, "%",
                     [this](float val) { assistantEngine_.setMacroDynamicsMatch(val); });

    setupMacroSlider(widthSlider_, widthLabel_, 0.0, 200.0, 100.0, "%",
                     [this](float val) { assistantEngine_.setMacroWidthMatch(val); });

    setupMacroSlider(claritySlider_, clarityLabel_, 0.0, 100.0, 25.0, "%",
                     [this](float val) { assistantEngine_.setMacroClarity(val); });

    setupMacroSlider(stabilizerSlider_, stabilizerLabel_, 0.0, 100.0, 30.0, "%",
                     [this](float val) { assistantEngine_.setMacroStabilizer(val); });

    // 4. Reference Track Audition Section
    loadRefButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    loadRefButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textLight);
    loadRefButton_.onClick = [this]() {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Select Reference Track (WAV, AIFF, FLAC, MP3)...",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3");

        constexpr auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (file.existsAsFile()) {
                if (refPlayer_.loadAudioFile(file)) {
                    refTrackLabel_.setText("REF: " + file.getFileName(), juce::dontSendNotification);
                    refTrackLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::accentCyan);
                }
            }
        });
    };
    addAndMakeVisible(loadRefButton_);

    refTrackLabel_.setFont(juce::Font(11.0f, juce::Font::plain));
    refTrackLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::textDim);
    addAndMakeVisible(refTrackLabel_);

    abButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    abButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
    abButton_.onClick = [this]() {
        if (refPlayer_.getABMode() == engine::ABMode::TrackA) {
            refPlayer_.setABMode(engine::ABMode::TrackB);
            abButton_.setButtonText("TRACK B (REF)");
            abButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
        } else {
            refPlayer_.setABMode(engine::ABMode::TrackA);
            abButton_.setButtonText("TRACK A (MASTER)");
            abButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
        }
    };
    addAndMakeVisible(abButton_);

    gainMatchToggle_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    gainMatchToggle_.setColour(juce::ToggleButton::tickColourId, StrataLookAndFeel::accentCyan);
    gainMatchToggle_.onClick = [this]() {
        refPlayer_.setGainMatchEnabled(gainMatchToggle_.getToggleState());
    };
    addAndMakeVisible(gainMatchToggle_);

    matchRefEqButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    matchRefEqButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
    matchRefEqButton_.onClick = [this]() {
        const auto& rec = assistantEngine_.getRecommendations();
        const auto target = assistantEngine_.getTargetProfile();
        const auto matchCurve = assistant::ReferenceMatcher::computeMatchCurve(
            rec.eqGainsDb, target.targetSpectralCurveDb, 1.0f);
        assistant::ReferenceMatcher::applyMatchCurveToAPVTS(apvts_, matchCurve);
        if (onMasterApplied) onMasterApplied();
        repaint();
    };
    addAndMakeVisible(matchRefEqButton_);

    populateGenreProfiles();
    selectGenreIndex(0);
}

void MasterAssistantView::populateGenreProfiles() {
    const auto& all = assistant::TargetProfile::getAllProfiles();
    const auto query = searchBox_.getText().toLowerCase();

    displayProfiles_.clear();
    for (const auto& p : all) {
        if (query.isEmpty() ||
            juce::String(p.name).toLowerCase().contains(query) ||
            juce::String(p.category).toLowerCase().contains(query)) {
            displayProfiles_.push_back(p);
        }
    }
    genreListBox_.updateContent();
    genreListBox_.repaint();
}

int MasterAssistantView::getNumRows() {
    return static_cast<int>(displayProfiles_.size());
}

void MasterAssistantView::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    if (rowNumber < 0 || rowNumber >= static_cast<int>(displayProfiles_.size())) return;

    const auto& p = displayProfiles_[static_cast<size_t>(rowNumber)];

    if (rowIsSelected) {
        g.setColour(StrataLookAndFeel::panelBg.brighter(0.2f));
        g.fillRect(0, 0, width, height);

        // Active cyan indicator on left edge
        g.setColour(StrataLookAndFeel::accentCyan);
        g.fillRect(0, 0, 4, height);
    } else {
        if (rowNumber % 2 == 1) {
            g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
            g.fillRect(0, 0, width, height);
        }
    }

    g.setFont(juce::Font(12.0f, rowIsSelected ? juce::Font::bold : juce::Font::plain));
    g.setColour(rowIsSelected ? StrataLookAndFeel::accentCyan : StrataLookAndFeel::textLight);
    g.drawText(p.name, 12, 2, width - 85, height - 4, juce::Justification::centredLeft);

    // LUFS Target badge on right
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(rowIsSelected ? StrataLookAndFeel::accentGold : StrataLookAndFeel::textDim);
    g.drawText(juce::String(p.targetIntegratedLufs, 1) + " LUFS", width - 80, 2, 70, height - 4, juce::Justification::centredRight);
}

void MasterAssistantView::selectedRowsChanged(int lastRowSelected) {
    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(displayProfiles_.size())) {
        selectGenreIndex(lastRowSelected);
    }
}

void MasterAssistantView::selectGenreIndex(int index) {
    if (index < 0 || index >= static_cast<int>(displayProfiles_.size())) return;
    selectedProfileIndex_ = index;
    const auto& p = displayProfiles_[static_cast<size_t>(index)];

    assistantEngine_.setTargetGenre(p.genre);
    assistantEngine_.applyGenreDefaults(apvts_);
    if (onMasterApplied) onMasterApplied();
    repaint();
}

void MasterAssistantView::updateTelemetry() {
    state_ = assistantEngine_.getState();
    progressPercent_ = assistantEngine_.getListeningProgressPercent();
    recommendations_ = assistantEngine_.getRecommendations();

    if (state_ == assistant::AssistantState::Listening) {
        listenButton_.setButtonText(juce::String::formatted("LISTENING (%.0f%%)...", progressPercent_));
        applyButton_.setEnabled(false);
    } else if (state_ == assistant::AssistantState::Ready) {
        listenButton_.setButtonText("LISTEN AGAIN");
        applyButton_.setEnabled(true);
    } else {
        listenButton_.setButtonText("START LISTENING (5s)");
        applyButton_.setEnabled(false);
    }

    repaint();
}

void MasterAssistantView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    auto bounds = getLocalBounds();

    // Left Sidebar Area Background
    const auto sidebarArea = bounds.removeFromLeft(240);
    g.setColour(StrataLookAndFeel::panelBg.darker(0.25f));
    g.fillRect(sidebarArea);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawVerticalLine(sidebarArea.getRight() - 1, 0.0f, static_cast<float>(getHeight()));

    // Right Content Area
    auto rightBounds = bounds.reduced(14, 10);

    // Selected Target Banner
    if (selectedProfileIndex_ >= 0 && selectedProfileIndex_ < static_cast<int>(displayProfiles_.size())) {
        const auto& p = displayProfiles_[static_cast<size_t>(selectedProfileIndex_)];
        auto banner = rightBounds.removeFromTop(48);
        g.setColour(StrataLookAndFeel::panelBg.darker(0.12f));
        g.fillRoundedRectangle(banner.toFloat(), 6.0f);
        g.setColour(StrataLookAndFeel::borderSubtle);
        g.drawRoundedRectangle(banner.toFloat(), 6.0f, 1.0f);

        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::accentGold);
        g.drawText("TARGET: " + juce::String(p.name).toUpperCase(), banner.removeFromLeft(300).reduced(12, 4), juce::Justification::centredLeft);

        g.setFont(juce::Font(12.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText("Target: " + juce::String(p.targetIntegratedLufs, 1) + " LUFS | Peak: " + juce::String(p.targetTruePeakCeilingDb, 1) + " dBTP | Crest: " + juce::String(p.targetCrestFactorDb, 1) + " dB",
                   banner.reduced(8, 4), juce::Justification::centredRight);
    }

    // Center Status Display
    auto centerArea = rightBounds.removeFromTop(rightBounds.getHeight() - 170).reduced(0, 10);
    g.setColour(StrataLookAndFeel::panelBg.darker(0.15f));
    g.fillRoundedRectangle(centerArea.toFloat(), 6.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(centerArea.toFloat(), 6.0f, 1.0f);

    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.setColour(StrataLookAndFeel::textLight);
    g.drawText("INTELLIGENT ACOUSTIC MASTERING ASSISTANT", centerArea.removeFromTop(50), juce::Justification::centred);

    g.setFont(juce::Font(13.0f, juce::Font::plain));
    g.setColour(StrataLookAndFeel::textDim);
    g.drawText("Select a Target Genre on the left sidebar, play your track in your DAW, and click Start Listening.",
               centerArea.removeFromTop(30), juce::Justification::centred);

    if (state_ == assistant::AssistantState::Listening) {
        const float barW = 320.0f;
        const float barH = 14.0f;
        const float barX = centerArea.getCentreX() - barW * 0.5f;
        const float barY = centerArea.getCentreY() - barH * 0.5f;

        g.setColour(juce::Colour(0xFF161B24));
        g.fillRoundedRectangle(barX, barY, barW, barH, 4.0f);

        const float fillW = std::clamp(progressPercent_ / 100.0f, 0.0f, 1.0f) * barW;
        g.setColour(StrataLookAndFeel::accentGold);
        g.fillRoundedRectangle(barX + 1.0f, barY + 1.0f, fillW - 2.0f, barH - 2.0f, 3.0f);
    }

    // Macro section header line
    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 170, getWidth(), 170);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 170, static_cast<float>(sidebarArea.getRight()), static_cast<float>(getWidth()));
}

void MasterAssistantView::resized() {
    auto bounds = getLocalBounds();

    // 1. Left Sidebar
    auto sidebar = bounds.removeFromLeft(240).reduced(10, 8);
    sidebarTitleLabel_.setBounds(sidebar.removeFromTop(24));
    sidebar.removeFromTop(4);
    searchBox_.setBounds(sidebar.removeFromTop(26));
    sidebar.removeFromTop(8);
    genreListBox_.setBounds(sidebar);

    // 2. Right Content Area
    auto rightBounds = bounds.reduced(14, 10);

    // Top action buttons row (Banner is 48px, buttons sit in banner right area)
    auto topRow = rightBounds.removeFromTop(48).reduced(6, 6);
    topRow.removeFromLeft(380); // Leave space for target banner title
    applyButton_.setBounds(topRow.removeFromRight(130).reduced(0, 2));
    topRow.removeFromRight(8);
    listenButton_.setBounds(topRow.removeFromRight(150).reduced(0, 2));
    topRow.removeFromRight(8);
    applyGenreDefaultsButton_.setBounds(topRow.removeFromRight(170).reduced(0, 2));

    // Bottom 170px for Macros and Reference Track
    auto bottom = rightBounds.removeFromBottom(160);

    // Reference track row
    auto refRow = bottom.removeFromBottom(34);
    loadRefButton_.setBounds(refRow.removeFromLeft(120).reduced(0, 3));
    refRow.removeFromLeft(8);
    refTrackLabel_.setBounds(refRow.removeFromLeft(180));
    refRow.removeFromLeft(8);
    abButton_.setBounds(refRow.removeFromLeft(140).reduced(0, 3));
    refRow.removeFromLeft(10);
    gainMatchToggle_.setBounds(refRow.removeFromLeft(110));
    refRow.removeFromLeft(10);
    matchRefEqButton_.setBounds(refRow.removeFromLeft(130).reduced(0, 3));

    bottom.removeFromBottom(8);

    // Macro Knobs
    macroSectionLabel_.setBounds(bottom.removeFromTop(18));
    auto macroArea = bottom;
    const int numMacros = 6;
    const int macroW = macroArea.getWidth() / numMacros;

    tonalLabel_.setBounds(macroArea.getX(), macroArea.getY(), macroW, 16);
    tonalSlider_.setBounds(macroArea.getX(), macroArea.getY() + 16, macroW, macroArea.getHeight() - 16);
    macroArea.removeFromLeft(macroW);

    loudnessLabel_.setBounds(macroArea.getX(), macroArea.getY(), macroW, 16);
    loudnessSlider_.setBounds(macroArea.getX(), macroArea.getY() + 16, macroW, macroArea.getHeight() - 16);
    macroArea.removeFromLeft(macroW);

    dynamicsLabel_.setBounds(macroArea.getX(), macroArea.getY(), macroW, 16);
    dynamicsSlider_.setBounds(macroArea.getX(), macroArea.getY() + 16, macroW, macroArea.getHeight() - 16);
    macroArea.removeFromLeft(macroW);

    widthLabel_.setBounds(macroArea.getX(), macroArea.getY(), macroW, 16);
    widthSlider_.setBounds(macroArea.getX(), macroArea.getY() + 16, macroW, macroArea.getHeight() - 16);
    macroArea.removeFromLeft(macroW);

    clarityLabel_.setBounds(macroArea.getX(), macroArea.getY(), macroW, 16);
    claritySlider_.setBounds(macroArea.getX(), macroArea.getY() + 16, macroW, macroArea.getHeight() - 16);
    macroArea.removeFromLeft(macroW);

    stabilizerLabel_.setBounds(macroArea.getX(), macroArea.getY(), macroW, 16);
    stabilizerSlider_.setBounds(macroArea.getX(), macroArea.getY() + 16, macroW, macroArea.getHeight() - 16);
}

} // namespace strata::gui

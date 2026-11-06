#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../assistant/AutoMasterEngine.h"
#include "../../engine/ReferencePlayer.h"
#include <functional>
#include <memory>
#include <vector>

namespace strata::gui {

class MasterAssistantView final : public juce::Component,
                                  public juce::ListBoxModel {
public:
    MasterAssistantView(juce::AudioProcessorValueTreeState& apvts,
                        assistant::AutoMasterEngine& assistantEngine,
                        engine::ReferencePlayer& referencePlayer);
    ~MasterAssistantView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateTelemetry();

    // ListBoxModel implementation for left sidebar genre list
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    std::function<void()> onMasterApplied;

private:
    void populateGenreProfiles();
    void selectGenreIndex(int index);

    juce::AudioProcessorValueTreeState& apvts_;
    assistant::AutoMasterEngine& assistantEngine_;
    engine::ReferencePlayer& refPlayer_;

    // Left Sidebar Target Genre List
    juce::Label sidebarTitleLabel_{ "SIDEBAR_TITLE", "TARGET MASTERING GENRES" };
    juce::TextEditor searchBox_;
    juce::ListBox genreListBox_{ "GENRE_LISTBOX", this };
    std::vector<assistant::TargetProfile> displayProfiles_;
    int selectedProfileIndex_{ 0 };

    // Macro Sliders (Ozone 12 Reference Macros)
    juce::Label macroSectionLabel_{ "MACRO_SECTION", "MASTER ASSISTANT MACRO ADJUSTMENTS" };
    juce::Slider tonalSlider_;
    juce::Label tonalLabel_{ "TONAL_LBL", "Tonal Balance" };

    juce::Slider loudnessSlider_;
    juce::Label loudnessLabel_{ "LOUD_LBL", "Loudness" };

    juce::Slider dynamicsSlider_;
    juce::Label dynamicsLabel_{ "DYN_LBL", "Dynamics Match" };

    juce::Slider widthSlider_;
    juce::Label widthLabel_{ "WIDTH_LBL", "Width Match" };

    juce::Slider claritySlider_;
    juce::Label clarityLabel_{ "CLARITY_LBL", "Clarity" };

    juce::Slider stabilizerSlider_;
    juce::Label stabilizerLabel_{ "STAB_LBL", "Stabilizer" };

    // Action Buttons
    juce::TextButton listenButton_{ "START LISTENING (5s)" };
    juce::TextButton applyButton_{ "APPLY AI MASTER" };
    juce::TextButton applyGenreDefaultsButton_{ "APPLY TARGET DEFAULTS" };

    // Reference Track Integration
    juce::TextButton loadRefButton_{ "LOAD REFERENCE" };
    juce::Label refTrackLabel_{ "REF_NAME", "No Reference Loaded" };
    juce::TextButton abButton_{ "TRACK A (MASTER)" };
    juce::ToggleButton gainMatchToggle_{ "MATCH GAIN" };
    juce::TextButton matchRefEqButton_{ "MATCH REF EQ" };
    std::unique_ptr<juce::FileChooser> fileChooser_;

    // Telemetry cache
    assistant::AssistantState state_{ assistant::AssistantState::Idle };
    float progressPercent_{ 0.0f };
    assistant::MasteringRecommendations recommendations_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterAssistantView)
};

} // namespace strata::gui

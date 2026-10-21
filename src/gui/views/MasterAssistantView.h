#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../assistant/AutoMasterEngine.h"
#include <functional>

namespace strata::gui {

class MasterAssistantView final : public juce::Component {
public:
    MasterAssistantView(juce::AudioProcessorValueTreeState& apvts, assistant::AutoMasterEngine& assistantEngine);
    ~MasterAssistantView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateTelemetry();

    std::function<void()> onMasterApplied;

private:
    juce::AudioProcessorValueTreeState& apvts_;
    assistant::AutoMasterEngine& assistantEngine_;

    // Target Profile Selection
    juce::Label profileLabel_{ "PROFILE_LBL", "TARGET MASTERING PROFILE:" };
    juce::ComboBox profileBox_;

    // Action Buttons
    juce::TextButton listenButton_{ "START LISTENING (5s)" };
    juce::TextButton applyButton_{ "APPLY MASTER" };
    juce::TextButton reanalyzeButton_{ "RE-ANALYZE" };

    // Telemetry cache
    assistant::AssistantState state_{ assistant::AssistantState::Idle };
    float progressPercent_{ 0.0f };
    assistant::MasteringRecommendations recommendations_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterAssistantView)
};

} // namespace strata::gui

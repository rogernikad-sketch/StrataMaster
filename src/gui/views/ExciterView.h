#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../components/CrossoverGraph.h"
#include "../components/Knob.h"
#include <array>

namespace strata::gui {

class ExciterView final : public juce::Component {
public:
    explicit ExciterView(juce::AudioProcessorValueTreeState& apvts);
    ~ExciterView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void selectBand(int bandIndex);

private:
    void rebuildBandControls(int bandIndex);

    juce::AudioProcessorValueTreeState& apvts_;

    CrossoverGraph crossoverGraph_;

    int selectedBand_{ 0 };
    std::array<juce::TextButton, 4> bandTabs_;

    juce::ToggleButton bandEnableButton_{ "ENABLE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandEnableAttachment_;

    juce::ComboBox modeBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment_;

    std::unique_ptr<Knob> driveKnob_;
    std::unique_ptr<Knob> mixKnob_;

    // Global Oversampling & Bypass
    juce::Label oversamplingLabel_{ "OVERSAMPLING", "OVERSAMPLING:" };
    juce::ComboBox oversamplingBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttachment_;

    juce::ToggleButton exciterBypassButton_{ "EXCITER BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> exciterBypassAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExciterView)
};

} // namespace strata::gui

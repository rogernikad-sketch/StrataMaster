#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../components/EQNodeGraph.h"
#include "../components/SpectrumDisplay.h"
#include "../components/Knob.h"
#include <array>

namespace strata::gui {

class EqualizerView final : public juce::Component {
public:
    explicit EqualizerView(juce::AudioProcessorValueTreeState& apvts);
    ~EqualizerView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateSpectrum(analysis::SpectrumAnalyzer& analyzer);
    void updateCurvesFromAPVTS();
    void selectBand(int bandIndex);

private:
    void rebuildBandControls(int bandIndex);

    juce::AudioProcessorValueTreeState& apvts_;

    SpectrumDisplay spectrumDisplay_;
    std::unique_ptr<EQNodeGraph> eqGraph_;

    // Bottom Band Strip
    int selectedBand_{ 0 };
    std::array<juce::TextButton, 8> bandSelectButtons_;

    juce::ToggleButton bandEnableButton_{ "ENABLE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandEnableAttachment_;

    juce::ComboBox filterTypeBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment_;

    std::unique_ptr<Knob> freqKnob_;
    std::unique_ptr<Knob> gainKnob_;
    std::unique_ptr<Knob> qKnob_;

    // Dynamic EQ Controls
    juce::ToggleButton dynEnableButton_{ "DYNAMIC" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynEnableAttachment_;
    std::unique_ptr<Knob> dynThreshKnob_;
    std::unique_ptr<Knob> dynRangeKnob_;

    // Global Mode & Bypass
    juce::ComboBox stereoModeBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoModeAttachment_;

    juce::ToggleButton eqBypassButton_{ "EQ BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqBypassAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerView)
};

} // namespace strata::gui

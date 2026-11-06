#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../components/Knob.h"

namespace strata::gui {

class MaximizerView final : public juce::Component {
public:
    explicit MaximizerView(juce::AudioProcessorValueTreeState& apvts);
    ~MaximizerView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateLimiterState(float gainReductionDb, float truePeakDb);

private:
    juce::AudioProcessorValueTreeState& apvts_;

    // Main Brickwall Limiter Controls
    std::unique_ptr<Knob> thresholdKnob_;
    std::unique_ptr<Knob> ceilingKnob_;
    std::unique_ptr<Knob> releaseKnob_;
    std::unique_ptr<Knob> lookaheadKnob_;

    juce::ToggleButton limiterBypassButton_{ "LIMITER BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterBypassAttachment_;

    // Pre-Limiter Soft Clipper
    juce::GroupComponent clipperGroup_{ "CLIPPER_GRP", "PRE-LIMITER SOFT CLIPPER" };
    juce::ToggleButton clipperEnableButton_{ "ENABLE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clipperEnableAttachment_;
    std::unique_ptr<Knob> clipperCeilingKnob_;
    std::unique_ptr<Knob> clipperKneeKnob_;

    // Metering telemetry
    float gainReductionDb_{ 0.0f };
    float truePeakDb_{ -100.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MaximizerView)
};

} // namespace strata::gui

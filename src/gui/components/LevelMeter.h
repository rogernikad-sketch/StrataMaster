#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

namespace strata::gui {

class LevelMeter final : public juce::Component, public juce::Timer {
public:
    LevelMeter();
    ~LevelMeter() override;

    void setLevel(float peakL, float peakR) noexcept;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    std::atomic<float> currentPeakL_{0.0f};
    std::atomic<float> currentPeakR_{0.0f};
    float displayPeakL_{0.0f};
    float displayPeakR_{0.0f};
    float peakHoldL_{0.0f};
    float peakHoldR_{0.0f};
};

} // namespace strata::gui

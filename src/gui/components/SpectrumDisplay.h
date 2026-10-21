#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace strata::analysis {
    class SpectrumAnalyzer;
}

namespace strata::gui {

/**
 * 60 FPS Real-Time Vector RTA Spectrum Visualizer.
 * Renders logarithmic 20 Hz - 20 kHz frequency axis, dBFS scale,
 * and glowing vector path with decay ballistics.
 */
class SpectrumDisplay : public juce::Component {
public:
    SpectrumDisplay();
    ~SpectrumDisplay() override = default;

    void updateSpectrum(analysis::SpectrumAnalyzer& analyzer);
    void paint(juce::Graphics& g) override;

    [[nodiscard]] float frequencyToX(float freqHz) const noexcept;
    [[nodiscard]] float xToFrequency(float x) const noexcept;
    [[nodiscard]] float decibelsToY(float db) const noexcept;
    [[nodiscard]] float yToDecibels(float y) const noexcept;

private:
    void drawGrid(juce::Graphics& g);

    std::vector<float> currentMagsDb_;
    std::vector<float> displayedMagsDb_;

    static constexpr float minFreqHz = 20.0f;
    static constexpr float maxFreqHz = 20000.0f;
    static constexpr float minDb = -90.0f;
    static constexpr float maxDb = +6.0f;
};

} // namespace strata::gui

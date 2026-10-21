#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <array>

namespace strata::gui {

class CrossoverGraph final : public juce::Component {
public:
    explicit CrossoverGraph(juce::AudioProcessorValueTreeState& apvts);
    ~CrossoverGraph() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setSelectedBand(int bandIndex);
    [[nodiscard]] int getSelectedBand() const noexcept { return selectedBand_; }

    void updateFromAPVTS();

    std::function<void(int)> onBandSelected;
    std::function<void()> onCrossoverChanged;

private:
    [[nodiscard]] float freqToX(float freqHz) const noexcept;
    [[nodiscard]] float xToFreq(float x) const noexcept;

    juce::AudioProcessorValueTreeState& apvts_;

    std::array<float, 3> splitFrequencies_{ 120.0f, 1500.0f, 6000.0f };
    std::array<float, 3> splitX_{ 0.0f, 0.0f, 0.0f };

    int selectedBand_{ 0 };
    int draggedSplitIndex_{ -1 };
    int hoveredSplitIndex_{ -1 };

    static constexpr float minFreqHz = 20.0f;
    static constexpr float maxFreqHz = 20000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossoverGraph)
};

} // namespace strata::gui

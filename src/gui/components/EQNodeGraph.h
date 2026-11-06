#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectrumDisplay.h"
#include <array>

namespace strata::gui {

/**
 * Interactive 8-Band Parametric EQ Vector Node Graph.
 * Real-time composite filter curve calculation and rendering for Bell, Shelves, and Cuts.
 * Automatically synchronizes with APVTS parameter changes and GUI knobs.
 */
class EQNodeGraph final : public juce::Component,
                          public juce::AudioProcessorValueTreeState::Listener {
public:
    explicit EQNodeGraph(juce::AudioProcessorValueTreeState& apvts);
    ~EQNodeGraph() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateCurvesFromAPVTS();

    std::function<void(int)> onBandSelected;

private:
    [[nodiscard]] int getBandUnderMouse(juce::Point<float> pos) const noexcept;
    void computeCompositeEQCurve(juce::Path& path) const;

    juce::AudioProcessorValueTreeState& apvts_;

    struct NodeData {
        float freqHz{ 1000.0f };
        float gainDb{ 0.0f };
        float q{ 0.707f };
        bool enabled{ true };
        int type{ 0 }; // 0: Bell, 1: LowShelf, 2: HighShelf, 3: LowPass, 4: HighPass, etc.
        juce::Point<float> screenPos;
    };
    std::array<NodeData, 8> nodes_;

    int draggedBandIndex_{ -1 };
    int selectedBandIndex_{ 0 };

    static constexpr float minFreqHz = 20.0f;
    static constexpr float maxFreqHz = 20000.0f;
    static constexpr float minDb = -18.0f;
    static constexpr float maxDb = +18.0f;
};

} // namespace strata::gui

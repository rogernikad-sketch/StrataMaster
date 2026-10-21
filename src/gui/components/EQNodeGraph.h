#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectrumDisplay.h"
#include <array>

namespace strata::gui {

/**
 * Interactive 8-Band Parametric EQ Vector Node Graph.
 * Supports:
 * - Mouse drag for Frequency and Gain
 * - Mouse wheel for Q
 * - Double click to toggle band enable/bypass
 * - Real-time composite filter curve calculation and rendering
 */
class EQNodeGraph final : public juce::Component {
public:
    explicit EQNodeGraph(juce::AudioProcessorValueTreeState& apvts);
    ~EQNodeGraph() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void updateCurvesFromAPVTS();

private:
    [[nodiscard]] int getBandUnderMouse(juce::Point<float> pos) const noexcept;
    void computeCompositeEQCurve(juce::Path& path) const;

    juce::AudioProcessorValueTreeState& apvts_;

    struct NodeData {
        float freqHz{ 1000.0f };
        float gainDb{ 0.0f };
        float q{ 0.707f };
        bool enabled{ true };
        int type{ 0 }; // 0: Bell, etc.
        juce::Point<float> screenPos;
    };
    std::array<NodeData, 8> nodes_;

    int draggedBandIndex_{ -1 };
    int hoveredBandIndex_{ -1 };

    static constexpr float minFreqHz = 20.0f;
    static constexpr float maxFreqHz = 20000.0f;
    static constexpr float minDb = -18.0f;
    static constexpr float maxDb = +18.0f;
};

} // namespace strata::gui

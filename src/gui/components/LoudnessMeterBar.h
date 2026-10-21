#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace strata::gui {

/**
 * Broadcast-Grade Loudness & True-Peak Precision Meter.
 * Displays:
 * - Momentary & Short-Term LUFS
 * - Integrated LUFS numerical readout
 * - EBU R128 LRA (Loudness Range)
 * - 4x Polyphase True-Peak (dBTP) with inter-sample clip hold
 */
class LoudnessMeterBar final : public juce::Component {
public:
    LoudnessMeterBar();
    ~LoudnessMeterBar() override = default;

    void updateLevels(float momentaryLufs, float shortTermLufs, float integratedLufs, float lraLu, float truePeakDb);
    void paint(juce::Graphics& g) override;

private:
    float momentaryLufs_{ -70.0f };
    float shortTermLufs_{ -70.0f };
    float integratedLufs_{ -70.0f };
    float lraLu_{ 0.0f };
    float truePeakDb_{ -70.0f };
    float maxTruePeakHoldDb_{ -70.0f };

    static constexpr float minLufs = -60.0f;
    static constexpr float maxLufs = 0.0f;
    static constexpr float minDbTP = -60.0f;
    static constexpr float maxDbTP = +3.0f;
};

} // namespace strata::gui

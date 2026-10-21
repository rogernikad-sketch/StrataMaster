#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

namespace strata::assistant {

/**
 * Intelligent Reference Spectrum Matcher.
 * Compares the spectral response of the incoming mix against the loaded Reference Track,
 * computing a precise 8-band Match EQ curve to align tonal balance with the reference.
 */
class ReferenceMatcher final {
public:
    ReferenceMatcher() = default;
    ~ReferenceMatcher() = default;

    /**
     * Computes the 8-band match EQ gain adjustments required to sculpt the master
     * toward the reference track's tonal profile.
     * matchAmount: 0.0 (no change) to 1.0 (100% full match)
     */
    static std::array<float, 8> computeMatchCurve(
        const std::array<float, 8>& masterSpectrumDb,
        const std::array<float, 8>& referenceSpectrumDb,
        float matchAmount = 1.0f) noexcept;

    /**
     * Applies computed match gains directly into the APVTS Parametric EQ bands.
     */
    static void applyMatchCurveToAPVTS(
        juce::AudioProcessorValueTreeState& apvts,
        const std::array<float, 8>& matchGainsDb);
};

} // namespace strata::assistant

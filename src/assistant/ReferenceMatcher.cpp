#include "ReferenceMatcher.h"
#include "../engine/ParameterIDs.h"
#include <algorithm>

namespace strata::assistant {

std::array<float, 8> ReferenceMatcher::computeMatchCurve(
    const std::array<float, 8>& masterSpectrumDb,
    const std::array<float, 8>& referenceSpectrumDb,
    float matchAmount) noexcept {

    std::array<float, 8> matchCurve{};
    const float amount = std::clamp(matchAmount, 0.0f, 1.0f);

    // Normalize relative to 1000 Hz (band 3)
    const float masterRef = masterSpectrumDb[3];
    const float refRef    = referenceSpectrumDb[3];

    for (size_t b = 0; b < 8; ++b) {
        const float masterNorm = masterSpectrumDb[b] - masterRef;
        const float refNorm    = referenceSpectrumDb[b] - refRef;

        // Match difference: reference minus master
        const float diff = (refNorm - masterNorm) * amount;
        // Limit to +/- 8 dB to keep EQ within musical mastering boundaries
        matchCurve[b] = std::clamp(diff, -8.0f, 8.0f);
    }

    return matchCurve;
}

void ReferenceMatcher::applyMatchCurveToAPVTS(
    juce::AudioProcessorValueTreeState& apvts,
    const std::array<float, 8>& matchGainsDb) {

    for (size_t b = 0; b < 8; ++b) {
        if (auto* gainParam = apvts.getParameter(param::getEQBandGainID(b))) {
            gainParam->setValueNotifyingHost(gainParam->convertTo0to1(matchGainsDb[b]));
        }
        if (auto* enableParam = apvts.getParameter(param::getEQBandEnableID(b))) {
            enableParam->setValueNotifyingHost(enableParam->convertTo0to1(1.0f));
        }
    }
}

} // namespace strata::assistant

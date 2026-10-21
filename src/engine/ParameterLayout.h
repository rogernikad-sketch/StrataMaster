#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace strata::engine {

class ParameterLayout final {
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
};

} // namespace strata::engine

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>

namespace strata::engine {

struct PresetInfo {
    juce::String name;
    juce::String category;
    bool isFactory{ false };
};

/**
 * High-performance, atomic preset management system.
 * Handles:
 * - Built-in factory presets designed for modern mastering workflows
 * - User preset creation, recall, deletion, and directory discovery
 * - Atomic XML serialization & state restoration
 */
class PresetManager final {
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);
    ~PresetManager() = default;

    // Factory Presets
    [[nodiscard]] const std::vector<PresetInfo>& getFactoryPresets() const noexcept { return factoryPresets_; }
    void loadFactoryPreset(size_t index);
    void loadFactoryPreset(const juce::String& name);

    // User Presets
    [[nodiscard]] juce::StringArray getUserPresetNames() const;
    bool saveUserPreset(const juce::String& presetName);
    bool loadUserPreset(const juce::String& presetName);
    bool deleteUserPreset(const juce::String& presetName);

    // Direct XML serialization
    [[nodiscard]] juce::String exportCurrentStateToXml() const;
    bool restoreStateFromXml(const juce::String& xmlString);

    // File system paths
    [[nodiscard]] static juce::File getPresetsDirectory();

private:
    void initializeFactoryPresets();
    void applyParameter(const juce::String& paramId, float value);

    juce::AudioProcessorValueTreeState& apvts_;
    std::vector<PresetInfo> factoryPresets_;
    std::vector<juce::String> factoryPresetXmls_;
};

} // namespace strata::engine

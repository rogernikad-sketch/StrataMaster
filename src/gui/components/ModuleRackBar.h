#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <array>
#include <memory>

namespace strata::gui {

enum class ModuleID {
    Equalizer = 0,
    Dynamics,
    Exciter,
    Imager,
    Maximizer,
    Assistant
};

class ModuleRackBar final : public juce::Component {
public:
    explicit ModuleRackBar(juce::AudioProcessorValueTreeState& apvts);
    ~ModuleRackBar() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedModule(ModuleID id);
    [[nodiscard]] ModuleID getSelectedModule() const noexcept { return activeModule_; }

    std::function<void(ModuleID)> onModuleSelected;

private:
    struct ModuleTab {
        ModuleID id{ ModuleID::Equalizer };
        juce::String name;
        juce::String acronym;
        juce::TextButton selectButton;
        juce::ToggleButton bypassButton;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    };

    void setupTab(size_t index, ModuleID id, const juce::String& name, const juce::String& acronym, const juce::String& bypassParam);

    juce::AudioProcessorValueTreeState& apvts_;
    std::array<std::unique_ptr<ModuleTab>, 6> tabs_;
    ModuleID activeModule_{ ModuleID::Equalizer };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleRackBar)
};

} // namespace strata::gui

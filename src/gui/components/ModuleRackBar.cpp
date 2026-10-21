#include "ModuleRackBar.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"

namespace strata::gui {

ModuleRackBar::ModuleRackBar(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {

    for (size_t i = 0; i < tabs_.size(); ++i) {
        tabs_[i] = std::make_unique<ModuleTab>();
    }

    setupTab(0, ModuleID::Equalizer, "EQUALIZER", "EQ", param::EQBypass.data());
    setupTab(1, ModuleID::Dynamics, "DYNAMICS", "DYN", param::DynamicsBypass.data());
    setupTab(2, ModuleID::Exciter, "EXCITER", "EXC", param::ExciterBypass.data());
    setupTab(3, ModuleID::Imager, "IMAGER", "IMG", "");
    setupTab(4, ModuleID::Maximizer, "MAXIMIZER", "MAX", param::LimiterBypass.data());
    setupTab(5, ModuleID::Assistant, "ASSISTANT", "AI", "");

    setSelectedModule(ModuleID::Equalizer);
}

void ModuleRackBar::setupTab(size_t index, ModuleID id, const juce::String& name, const juce::String& acronym, const juce::String& bypassParam) {
    auto& tab = *tabs_[index];
    tab.id = id;
    tab.name = name;
    tab.acronym = acronym;

    tab.selectButton.setButtonText(name);
    tab.selectButton.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    tab.selectButton.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
    tab.selectButton.onClick = [this, id]() {
        setSelectedModule(id);
        if (onModuleSelected) {
            onModuleSelected(id);
        }
    };
    addAndMakeVisible(tab.selectButton);

    if (!bypassParam.isEmpty()) {
        tab.bypassButton.setTooltip("Bypass " + name);
        tab.bypassButton.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textDim);
        tab.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, bypassParam, tab.bypassButton);
        addAndMakeVisible(tab.bypassButton);
    }
}

void ModuleRackBar::setSelectedModule(ModuleID id) {
    activeModule_ = id;
    for (auto& tabPtr : tabs_) {
        if (!tabPtr) continue;
        auto& tab = *tabPtr;
        const bool isActive = (tab.id == activeModule_);
        if (isActive) {
            tab.selectButton.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.12f));
            tab.selectButton.setColour(juce::TextButton::textColourOffId,
                (tab.id == ModuleID::Assistant) ? StrataLookAndFeel::accentGold : StrataLookAndFeel::accentCyan);
        } else {
            tab.selectButton.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.darker(0.1f));
            tab.selectButton.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
        }
    }
    repaint();
}

void ModuleRackBar::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

    // Draw active highlight underline beneath the active module tab
    for (const auto& tabPtr : tabs_) {
        if (!tabPtr) continue;
        const auto& tab = *tabPtr;
        if (tab.id == activeModule_) {
            const auto bounds = tab.selectButton.getBounds().toFloat();
            g.setColour((tab.id == ModuleID::Assistant) ? StrataLookAndFeel::accentGold : StrataLookAndFeel::accentCyan);
            g.fillRect(bounds.getX(), static_cast<float>(getHeight() - 3), bounds.getWidth(), 3.0f);
            break;
        }
    }
}

void ModuleRackBar::resized() {
    auto bounds = getLocalBounds().reduced(8, 3);
    const int numTabs = static_cast<int>(tabs_.size());
    if (numTabs == 0) return;

    const int spacing = 6;
    const int totalSpacing = spacing * (numTabs - 1);
    const int tabWidth = (bounds.getWidth() - totalSpacing) / numTabs;

    for (auto& tabPtr : tabs_) {
        if (!tabPtr) continue;
        auto& tab = *tabPtr;
        auto tabBounds = bounds.removeFromLeft(tabWidth);
        bounds.removeFromLeft(spacing);

        if (tab.bypassButton.isVisible()) {
            tab.bypassButton.setBounds(tabBounds.removeFromLeft(22).reduced(2));
        }
        tab.selectButton.setBounds(tabBounds);
    }
}

} // namespace strata::gui

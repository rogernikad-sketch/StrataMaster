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
    setupTab(5, ModuleID::Maserati, "MASERATI", "MST", param::MaseratiBypass.data());
    setupTab(6, ModuleID::Assistant, "ASSISTANT", "AI", "");

    addModuleButton_.setButtonText("+ ADD MODULE");
    addModuleButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.08f));
    addModuleButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
    addModuleButton_.onClick = [this]() { showAddModuleMenu(); };
    addAndMakeVisible(addModuleButton_);

    setSelectedModule(ModuleID::Equalizer);
}

void ModuleRackBar::showAddModuleMenu() {
    juce::PopupMenu menu;
    menu.addSectionHeader("INSERT MASTERING MODULE");

    auto isParamBypassed = [this](const juce::String& paramId) -> bool {
        if (auto* p = apvts_.getRawParameterValue(paramId)) {
            return p->load() > 0.5f;
        }
        return false;
    };

    auto toggleParam = [this](const juce::String& paramId) {
        if (auto* p = apvts_.getParameter(paramId)) {
            const float cur = p->getValue();
            p->setValueNotifyingHost(cur > 0.5f ? 0.0f : 1.0f);
        }
    };

    menu.addItem(1, "Dynamic Parametric EQ", true, !isParamBypassed(juce::String(param::EQBypass.data())));
    menu.addItem(2, "Multiband Dynamics Compressor", true, !isParamBypassed(juce::String(param::DynamicsBypass.data())));
    menu.addItem(3, "Harmonic Exciter & Tape Saturation", true, !isParamBypassed(juce::String(param::ExciterBypass.data())));
    menu.addItem(4, "Stereo Imager & Mono-Maker", true, true);
    menu.addItem(5, "IRC True-Peak Maximizer", true, !isParamBypassed(juce::String(param::LimiterBypass.data())));
    menu.addItem(6, "Maserati Analog Tone Processor", true, !isParamBypassed(juce::String(param::MaseratiBypass.data())));
    menu.addSeparator();
    menu.addSectionHeader("ANALOG INSERTS & PROCESSORS");
    menu.addItem(7, "Pre-Limiter Soft Clipper", true, isParamBypassed(juce::String(param::ClipperEnable.data())));
    menu.addItem(8, "Stereoizer Spatial Decorrelation", true, isParamBypassed(juce::String(param::StereoStereoizerEnable.data())));
    menu.addItem(9, "Multiband Transient Processor", true, !isParamBypassed(juce::String(param::TransientBypass.data())));

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addModuleButton_),
        [this, toggleParam](int result) {
            switch (result) {
                case 1: setSelectedModule(ModuleID::Equalizer); if (onModuleSelected) onModuleSelected(ModuleID::Equalizer); break;
                case 2: setSelectedModule(ModuleID::Dynamics); if (onModuleSelected) onModuleSelected(ModuleID::Dynamics); break;
                case 3: setSelectedModule(ModuleID::Exciter); if (onModuleSelected) onModuleSelected(ModuleID::Exciter); break;
                case 4: setSelectedModule(ModuleID::Imager); if (onModuleSelected) onModuleSelected(ModuleID::Imager); break;
                case 5: setSelectedModule(ModuleID::Maximizer); if (onModuleSelected) onModuleSelected(ModuleID::Maximizer); break;
                case 6: setSelectedModule(ModuleID::Maserati); if (onModuleSelected) onModuleSelected(ModuleID::Maserati); break;
                case 7: toggleParam(juce::String(param::ClipperEnable.data())); break;
                case 8: toggleParam(juce::String(param::StereoStereoizerEnable.data())); break;
                case 9: toggleParam(juce::String(param::TransientBypass.data())); break;
                default: break;
            }
        });
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
    
    // Add Module Button on the right
    auto addBounds = bounds.removeFromRight(115);
    bounds.removeFromRight(8);
    addModuleButton_.setBounds(addBounds.reduced(0, 2));

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
            tab.bypassButton.setBounds(tabBounds.removeFromLeft(20).reduced(1));
        }
        tab.selectButton.setBounds(tabBounds);
    }
}

} // namespace strata::gui

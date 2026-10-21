#include "PresetManager.h"
#include "ParameterIDs.h"

namespace strata::engine {

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {
    initializeFactoryPresets();
}

juce::File PresetManager::getPresetsDirectory() {
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("StrataAudio")
                   .getChildFile("StrataMaster")
                   .getChildFile("Presets");
    if (!dir.exists()) {
        dir.createDirectory();
    }
    return dir;
}

void PresetManager::applyParameter(const juce::String& paramId, float value) {
    if (auto* param = apvts_.getParameter(paramId)) {
        param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

void PresetManager::initializeFactoryPresets() {
    factoryPresets_.clear();
    factoryPresetXmls_.clear();

    // 1. Clean Master
    {
        factoryPresets_.push_back({ "Clean Master", "General", true });
        juce::ValueTree vt("StrataMasterPreset");
        vt.setProperty(juce::String(param::InputGain.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::OutputGain.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 100.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 75.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 100.0f, nullptr);
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        factoryPresetXmls_.push_back(xml->toString());
    }

    // 2. EDM Punch & Weight
    {
        factoryPresets_.push_back({ "EDM Punch & Weight", "Electronic", true });
        juce::ValueTree vt("StrataMasterPreset");
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 90.0f, nullptr);
        vt.setProperty(juce::String(param::StereoStereoizerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoStereoizerAmount.data()), 15.0f, nullptr);
        vt.setProperty(juce::String(param::ExciterBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::getExciterBandEnableID(0).c_str()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::getExciterBandDriveID(0).c_str()), 25.0f, nullptr); // Warm low drive
        vt.setProperty(juce::String(param::getTransientBandAttackID(0).c_str()), 2.5f, nullptr); // +2.5 dB kick punch
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.1f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -3.0f, nullptr);
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        factoryPresetXmls_.push_back(xml->toString());
    }

    // 3. Warm Acoustic Clarity
    {
        factoryPresets_.push_back({ "Warm Acoustic Clarity", "Acoustic", true });
        juce::ValueTree vt("StrataMasterPreset");
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 60.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 108.0f, nullptr);
        vt.setProperty(juce::String(param::ExciterBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::getExciterBandEnableID(3).c_str()), 1.0f, nullptr); // High tape sheen
        vt.setProperty(juce::String(param::getExciterBandDriveID(3).c_str()), 15.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.8f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -1.5f, nullptr);
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        factoryPresetXmls_.push_back(xml->toString());
    }

    // 4. Broadcast R128 (-1.0 dBTP)
    {
        factoryPresets_.push_back({ "Broadcast R128 (-1.0 dBTP)", "Broadcast", true });
        juce::ValueTree vt("StrataMasterPreset");
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 80.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -2.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.5f, nullptr);
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        factoryPresetXmls_.push_back(xml->toString());
    }

    // 5. Modern Pop Master
    {
        factoryPresets_.push_back({ "Modern Pop Master", "Pop", true });
        juce::ValueTree vt("StrataMasterPreset");
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 85.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 112.0f, nullptr);
        vt.setProperty(juce::String(param::getTransientBandAttackID(1).c_str()), 1.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.2f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -2.5f, nullptr);
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        factoryPresetXmls_.push_back(xml->toString());
    }
}

void PresetManager::loadFactoryPreset(size_t index) {
    if (index >= factoryPresetXmls_.size()) {
        return;
    }
    restoreStateFromXml(factoryPresetXmls_[index]);
}

void PresetManager::loadFactoryPreset(const juce::String& name) {
    for (size_t i = 0; i < factoryPresets_.size(); ++i) {
        if (factoryPresets_[i].name == name) {
            loadFactoryPreset(i);
            return;
        }
    }
}

juce::StringArray PresetManager::getUserPresetNames() const {
    juce::StringArray names;
    const auto dir = getPresetsDirectory();
    if (!dir.exists()) {
        return names;
    }

    const auto files = dir.findChildFiles(juce::File::findFiles, false, "*.strata");
    for (const auto& f : files) {
        names.add(f.getFileNameWithoutExtension());
    }
    return names;
}

bool PresetManager::saveUserPreset(const juce::String& presetName) {
    if (presetName.trim().isEmpty()) {
        return false;
    }

    const auto xmlStr = exportCurrentStateToXml();
    const auto file = getPresetsDirectory().getChildFile(presetName.trim() + ".strata");
    return file.replaceWithText(xmlStr);
}

bool PresetManager::loadUserPreset(const juce::String& presetName) {
    const auto file = getPresetsDirectory().getChildFile(presetName.trim() + ".strata");
    if (!file.existsAsFile()) {
        return false;
    }
    const auto xmlStr = file.loadFileAsString();
    return restoreStateFromXml(xmlStr);
}

bool PresetManager::deleteUserPreset(const juce::String& presetName) {
    const auto file = getPresetsDirectory().getChildFile(presetName.trim() + ".strata");
    if (file.existsAsFile()) {
        return file.deleteFile();
    }
    return false;
}

juce::String PresetManager::exportCurrentStateToXml() const {
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    return xml ? xml->toString() : juce::String();
}

bool PresetManager::restoreStateFromXml(const juce::String& xmlString) {
    if (xmlString.isEmpty()) {
        return false;
    }

    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(xmlString);
    if (!xml) {
        return false;
    }

    // Support both full APVTS value tree export and simplified preset properties
    if (xml->hasTagName(apvts_.state.getType())) {
        apvts_.replaceState(juce::ValueTree::fromXml(*xml));
        return true;
    }

    // If it's a property-based preset
    for (int i = 0; i < xml->getNumAttributes(); ++i) {
        const auto paramName = xml->getAttributeName(i);
        const float val = static_cast<float>(xml->getDoubleAttribute(paramName));
        applyParameter(paramName, val);
    }
    return true;
}

} // namespace strata::engine

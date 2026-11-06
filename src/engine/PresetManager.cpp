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
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param)) {
            *choice = static_cast<int>(std::round(value));
        } else if (auto* bp = dynamic_cast<juce::AudioParameterBool*>(param)) {
            *bp = (value > 0.5f);
        } else if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(param)) {
            *fp = value;
        } else {
            param->setValueNotifyingHost(param->convertTo0to1(value));
        }
    }
}

void PresetManager::initializeFactoryPresets() {
    factoryPresets_.clear();
    factoryPresetXmls_.clear();

    auto addPreset = [this](const char* name, const char* category, auto builder) {
        factoryPresets_.push_back({ name, category, true });
        juce::ValueTree vt("StrataMasterPreset");
        builder(vt);
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        factoryPresetXmls_.push_back(xml->toString());
    };

    auto setEQ = [](juce::ValueTree& vt, const std::array<float, 8>& gains, const std::array<float, 8>& freqs, const std::array<int, 8>& types) {
        vt.setProperty(juce::String(param::EQBypass.data()), 0.0f, nullptr);
        for (size_t b = 0; b < 8; ++b) {
            vt.setProperty(juce::String(param::getEQBandFreqID(b).c_str()), freqs[b], nullptr);
            vt.setProperty(juce::String(param::getEQBandTypeID(b).c_str()), static_cast<float>(types[b]), nullptr);
            vt.setProperty(juce::String(param::getEQBandGainID(b).c_str()), gains[b], nullptr);
            vt.setProperty(juce::String(param::getEQBandEnableID(b).c_str()), 1.0f, nullptr);
            vt.setProperty(juce::String(param::getEQBandQID(b).c_str()), (b == 0 || b == 6 || b == 7) ? 0.71f : 1.0f, nullptr);
        }
    };

    auto setDyn = [](juce::ValueTree& vt, const std::array<float, 4>& thresh, const std::array<float, 4>& ratios) {
        vt.setProperty(juce::String(param::DynamicsBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::CrossoverSplit1.data()), 180.0f, nullptr);
        vt.setProperty(juce::String(param::CrossoverSplit2.data()), 2200.0f, nullptr);
        vt.setProperty(juce::String(param::CrossoverSplit3.data()), 6800.0f, nullptr);
        for (size_t b = 0; b < 4; ++b) {
            vt.setProperty(juce::String(param::getDynBandEnableID(b).c_str()), 1.0f, nullptr);
            vt.setProperty(juce::String(param::getDynBandBypassID(b).c_str()), 0.0f, nullptr);
            vt.setProperty(juce::String(param::getDynBandThreshID(b).c_str()), thresh[b], nullptr);
            vt.setProperty(juce::String(param::getDynBandRatioID(b).c_str()), ratios[b], nullptr);
            vt.setProperty(juce::String(param::getDynBandAttackID(b).c_str()), (b == 0) ? 20.0f : (b == 1) ? 14.0f : (b == 2) ? 8.0f : 4.0f, nullptr);
            vt.setProperty(juce::String(param::getDynBandReleaseID(b).c_str()), (b == 0) ? 35.0f : (b == 1) ? 45.0f : (b == 2) ? 25.0f : 12.0f, nullptr);
        }
    };

    auto setExciter = [](juce::ValueTree& vt, const std::array<int, 4>& modes, const std::array<float, 4>& drives) {
        vt.setProperty(juce::String(param::ExciterBypass.data()), 0.0f, nullptr);
        for (size_t b = 0; b < 4; ++b) {
            vt.setProperty(juce::String(param::getExciterBandEnableID(b).c_str()), 1.0f, nullptr);
            vt.setProperty(juce::String(param::getExciterBandModeID(b).c_str()), static_cast<float>(modes[b]), nullptr);
            vt.setProperty(juce::String(param::getExciterBandDriveID(b).c_str()), drives[b], nullptr);
            vt.setProperty(juce::String(param::getExciterBandMixID(b).c_str()), (drives[b] > 0.0f) ? 35.0f : 0.0f, nullptr);
        }
    };

    const std::array<float, 8> stdFreqs = { 35.0f, 75.0f, 250.0f, 1000.0f, 3000.0f, 6000.0f, 11000.0f, 16000.0f };
    const std::array<int, 8> stdTypes = { 1, 0, 0, 0, 0, 0, 2, 2 }; // LowShelf, Bell x 5, HighShelf x 2

    // 1. Clean Master
    addPreset("Clean Master (-14 LUFS)", "Universal", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::InputGain.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::OutputGain.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.05f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 80.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -1.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 120.0f, nullptr);
        setEQ(vt, { +1.0f, +0.5f, 0.0f, 0.0f, 0.0f, +0.5f, +1.0f, 0.0f }, stdFreqs, stdTypes);
        setDyn(vt, { -18.0f, -20.0f, -22.0f, -24.0f }, { 1.5f, 1.5f, 1.4f, 1.4f });
        setExciter(vt, { 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f });
    });

    // 2. Classic Hip-Hop & Boom Bap
    addPreset("Classic Hip-Hop & Boom Bap (-9.0 LUFS)", "Hip-Hop & Urban", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 100.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.06f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.3f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -4.8f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 65.0f, nullptr);
        setEQ(vt, { +4.5f, +3.5f, -1.0f, -1.5f, +0.5f, +1.5f, +1.0f, -1.5f }, stdFreqs, stdTypes);
        setDyn(vt, { -13.0f, -15.0f, -17.0f, -19.0f }, { 2.5f, 2.0f, 1.8f, 1.6f });
        setExciter(vt, { 0, 1, 0, 3 }, { 20.0f, 18.0f, 15.0f, 16.0f });
    });

    // 3. Hip-Hop & Modern Trap Weight
    addPreset("Hip-Hop & Modern Trap Weight (-8.0 LUFS)", "Hip-Hop & Urban", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 120.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.10f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.4f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.2f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -5.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 50.0f, nullptr);
        setEQ(vt, { +5.5f, +4.0f, -1.5f, -1.8f, +1.0f, +2.0f, +2.5f, +1.0f }, stdFreqs, stdTypes);
        setDyn(vt, { -11.0f, -13.0f, -15.0f, -18.0f }, { 3.0f, 2.4f, 2.0f, 1.8f });
        setExciter(vt, { 1, 0, 3, 3 }, { 25.0f, 15.0f, 22.0f, 26.0f });
    });

    // 4. Folk & Acoustic Detail
    addPreset("Folk & Acoustic Detail (-15.0 LUFS)", "Acoustic & Classical", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 60.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.08f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -1.2f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 150.0f, nullptr);
        setEQ(vt, { 0.0f, +1.0f, +1.5f, 0.0f, +0.5f, -0.5f, +1.5f, +0.5f }, stdFreqs, stdTypes);
        setDyn(vt, { -18.0f, -20.0f, -22.0f, -24.0f }, { 1.4f, 1.4f, 1.3f, 1.3f });
        setExciter(vt, { 2, 2, 0, 0 }, { 10.0f, 10.0f, 12.0f, 15.0f });
    });

    // 5. EDM Punch & Weight
    addPreset("EDM Punch & Weight (-7.0 LUFS)", "Electronic & Dance", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 100.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.22f, nullptr);
        vt.setProperty(juce::String(param::StereoStereoizerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoStereoizerAmount.data()), 25.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.3f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.1f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -6.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 50.0f, nullptr);
        setEQ(vt, { +5.0f, +3.5f, -1.5f, -0.5f, +1.5f, +2.5f, +3.0f, +1.5f }, stdFreqs, stdTypes);
        setDyn(vt, { -10.0f, -12.0f, -14.0f, -16.0f }, { 3.2f, 2.6f, 2.4f, 2.0f });
        setExciter(vt, { 1, 0, 3, 3 }, { 28.0f, 20.0f, 25.0f, 30.0f });
    });

    // 6. Classic Rock & Vintage Warmth
    addPreset("Classic Rock & Vintage Warmth (-10.5 LUFS)", "Rock & Metal", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 80.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.10f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.6f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -4.2f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 85.0f, nullptr);
        setEQ(vt, { +1.5f, +2.5f, +1.5f, 0.0f, +1.8f, +1.2f, -0.5f, -2.5f }, stdFreqs, stdTypes);
        setDyn(vt, { -14.0f, -16.0f, -18.0f, -20.0f }, { 2.2f, 2.0f, 1.8f, 1.6f });
        setExciter(vt, { 0, 2, 0, 0 }, { 20.0f, 16.0f, 14.0f, 12.0f });
    });

    // 7. Dance Pop & Commercial Polish
    addPreset("Dance Pop & Commercial Polish (-8.5 LUFS)", "Pop & Commercial", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 90.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.15f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.4f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.2f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -5.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 60.0f, nullptr);
        setEQ(vt, { +3.5f, +2.5f, 0.0f, +0.5f, +1.5f, +2.2f, +2.5f, +1.0f }, stdFreqs, stdTypes);
        setDyn(vt, { -12.0f, -14.0f, -16.0f, -18.0f }, { 2.6f, 2.2f, 2.0f, 1.8f });
        setExciter(vt, { 0, 1, 3, 3 }, { 18.0f, 15.0f, 22.0f, 25.0f });
    });

    // 8. Ambient & Cinematic Immersion
    addPreset("Ambient & Cinematic Immersion (-16.0 LUFS)", "Acoustic & Classical", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 70.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.30f, nullptr);
        vt.setProperty(juce::String(param::StereoStereoizerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoStereoizerAmount.data()), 35.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 200.0f, nullptr);
        setEQ(vt, { +2.0f, +1.0f, 0.0f, 0.0f, -0.5f, -0.5f, +1.5f, +0.5f }, stdFreqs, stdTypes);
        setDyn(vt, { -18.0f, -20.0f, -22.0f, -24.0f }, { 1.5f, 1.5f, 1.4f, 1.4f });
        setExciter(vt, { 2, 0, 2, 0 }, { 12.0f, 10.0f, 15.0f, 15.0f });
    });

    // 9. Techno & Industrial Impact
    addPreset("Techno & Industrial Impact (-7.0 LUFS)", "Electronic & Dance", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 110.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.20f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.3f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.1f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -5.8f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 45.0f, nullptr);
        setEQ(vt, { +5.0f, +3.0f, -2.0f, -0.5f, +1.5f, +2.5f, +2.0f, +0.5f }, stdFreqs, stdTypes);
        setDyn(vt, { -10.0f, -12.0f, -14.0f, -17.0f }, { 3.0f, 2.5f, 2.2f, 2.0f });
        setExciter(vt, { 1, 0, 1, 3 }, { 25.0f, 18.0f, 22.0f, 24.0f });
    });

    // 10. RnB & Modern Soul
    addPreset("RnB & Modern Soul (-9.0 LUFS)", "Hip-Hop & Urban", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 90.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.10f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -0.3f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -4.5f, nullptr);
        vt.setProperty(juce::String(param::LimiterRelease.data()), 70.0f, nullptr);
        setEQ(vt, { +3.5f, +2.5f, -0.5f, 0.0f, +1.2f, +1.8f, +1.5f, 0.0f }, stdFreqs, stdTypes);
        setDyn(vt, { -13.0f, -15.0f, -17.0f, -19.0f }, { 2.2f, 2.0f, 1.8f, 1.6f });
        setExciter(vt, { 0, 2, 0, 3 }, { 16.0f, 15.0f, 14.0f, 18.0f });
    });

    // 11. Broadcast R128 (-1.0 dBTP)
    addPreset("Broadcast R128 (-1.0 dBTP)", "Broadcast", [&](juce::ValueTree& vt) {
        vt.setProperty(juce::String(param::StereoMonoMakerEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::StereoMonoMakerFreq.data()), 80.0f, nullptr);
        vt.setProperty(juce::String(param::StereoWidth.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterBypass.data()), 0.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterCeiling.data()), -1.0f, nullptr);
        vt.setProperty(juce::String(param::LimiterThreshold.data()), -1.5f, nullptr);
        vt.setProperty(juce::String(param::ClipperEnable.data()), 1.0f, nullptr);
        vt.setProperty(juce::String(param::ClipperCeiling.data()), -0.5f, nullptr);
        setEQ(vt, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, stdFreqs, stdTypes);
        setDyn(vt, { -18.0f, -20.0f, -22.0f, -24.0f }, { 1.5f, 1.5f, 1.4f, 1.4f });
        setExciter(vt, { 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f });
    });
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
        float val = static_cast<float>(xml->getDoubleAttribute(paramName));
        if (paramName == param::StereoWidth.data() && val > 2.0f) {
            val /= 100.0f; // Scale legacy percentage values (e.g. 100% -> 1.0x)
        }
        applyParameter(paramName, val);
    }
    return true;
}

} // namespace strata::engine

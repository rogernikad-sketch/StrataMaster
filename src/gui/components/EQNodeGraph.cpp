#include "EQNodeGraph.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>
#include <algorithm>

namespace strata::gui {

EQNodeGraph::EQNodeGraph(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {
    setOpaque(false);

    // Register APVTS listeners for instant real-time synchronization with knobs & presets
    for (size_t b = 0; b < 8; ++b) {
        apvts_.addParameterListener(param::getEQBandFreqID(b), this);
        apvts_.addParameterListener(param::getEQBandGainID(b), this);
        apvts_.addParameterListener(param::getEQBandQID(b), this);
        apvts_.addParameterListener(param::getEQBandTypeID(b), this);
        apvts_.addParameterListener(param::getEQBandEnableID(b), this);
    }

    updateCurvesFromAPVTS();
}

EQNodeGraph::~EQNodeGraph() {
    for (size_t b = 0; b < 8; ++b) {
        apvts_.removeParameterListener(param::getEQBandFreqID(b), this);
        apvts_.removeParameterListener(param::getEQBandGainID(b), this);
        apvts_.removeParameterListener(param::getEQBandQID(b), this);
        apvts_.removeParameterListener(param::getEQBandTypeID(b), this);
        apvts_.removeParameterListener(param::getEQBandEnableID(b), this);
    }
}

void EQNodeGraph::parameterChanged(const juce::String& /*parameterID*/, float /*newValue*/) {
    juce::MessageManager::callAsync([this]() {
        updateCurvesFromAPVTS();
    });
}

void EQNodeGraph::updateCurvesFromAPVTS() {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    for (size_t b = 0; b < 8; ++b) {
        if (auto* freqParam = apvts_.getRawParameterValue(param::getEQBandFreqID(b))) {
            nodes_[b].freqHz = freqParam->load();
        }
        if (auto* gainParam = apvts_.getRawParameterValue(param::getEQBandGainID(b))) {
            nodes_[b].gainDb = gainParam->load();
        }
        if (auto* qParam = apvts_.getRawParameterValue(param::getEQBandQID(b))) {
            nodes_[b].q = qParam->load();
        }
        if (auto* typeParam = apvts_.getRawParameterValue(param::getEQBandTypeID(b))) {
            nodes_[b].type = static_cast<int>(typeParam->load());
        }
        if (auto* enableParam = apvts_.getRawParameterValue(param::getEQBandEnableID(b))) {
            nodes_[b].enabled = (enableParam->load() > 0.5f);
        }

        if (w > 0.0f && h > 0.0f) {
            const float normX = std::log10(std::clamp(nodes_[b].freqHz, minFreqHz, maxFreqHz) / minFreqHz) / std::log10(maxFreqHz / minFreqHz);
            // If cut filter (LowPass/HighPass), position node at 0dB line
            float dispGain = nodes_[b].gainDb;
            if (nodes_[b].type == 3 || nodes_[b].type == 4) { // LP or HP
                dispGain = 0.0f;
            }
            const float normY = (maxDb - std::clamp(dispGain, minDb, maxDb)) / (maxDb - minDb);
            nodes_[b].screenPos = { normX * w, normY * h };
        }
    }
    repaint();
}

void EQNodeGraph::resized() {
    updateCurvesFromAPVTS();
}

int EQNodeGraph::getBandUnderMouse(juce::Point<float> pos) const noexcept {
    constexpr float hitRadius = 16.0f;
    for (int b = 0; b < 8; ++b) {
        if (pos.getDistanceFrom(nodes_[b].screenPos) <= hitRadius) {
            return b;
        }
    }
    return -1;
}

void EQNodeGraph::mouseDown(const juce::MouseEvent& e) {
    draggedBandIndex_ = getBandUnderMouse(e.position);
    if (draggedBandIndex_ >= 0) {
        selectedBandIndex_ = draggedBandIndex_;
        if (onBandSelected) onBandSelected(selectedBandIndex_);
    }
    repaint();
}

void EQNodeGraph::mouseDrag(const juce::MouseEvent& e) {
    if (draggedBandIndex_ < 0 || draggedBandIndex_ >= 8) {
        return;
    }

    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }

    const float normX = std::clamp(e.position.x / w, 0.0f, 1.0f);
    const float newFreq = minFreqHz * std::pow(maxFreqHz / minFreqHz, normX);

    const float normY = std::clamp(e.position.y / h, 0.0f, 1.0f);
    const float newGain = maxDb - normY * (maxDb - minDb);

    if (auto* param = apvts_.getParameter(param::getEQBandFreqID(static_cast<size_t>(draggedBandIndex_)))) {
        param->setValueNotifyingHost(param->convertTo0to1(newFreq));
    }
    if (auto* param = apvts_.getParameter(param::getEQBandGainID(static_cast<size_t>(draggedBandIndex_)))) {
        param->setValueNotifyingHost(param->convertTo0to1(newGain));
    }

    updateCurvesFromAPVTS();
}

void EQNodeGraph::mouseUp(const juce::MouseEvent&) {
    draggedBandIndex_ = -1;
    repaint();
}

void EQNodeGraph::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    const int band = getBandUnderMouse(e.position);
    if (band < 0) {
        return;
    }

    float currentQ = nodes_[band].q;
    currentQ = std::clamp(currentQ + wheel.deltaY * 0.5f, 0.2f, 10.0f);

    if (auto* param = apvts_.getParameter(param::getEQBandQID(static_cast<size_t>(band)))) {
        param->setValueNotifyingHost(param->convertTo0to1(currentQ));
    }
    updateCurvesFromAPVTS();
}

void EQNodeGraph::mouseDoubleClick(const juce::MouseEvent& e) {
    const int band = getBandUnderMouse(e.position);
    if (band < 0) {
        return;
    }

    const float newEnable = nodes_[band].enabled ? 0.0f : 1.0f;
    if (auto* param = apvts_.getParameter(param::getEQBandEnableID(static_cast<size_t>(band)))) {
        param->setValueNotifyingHost(param->convertTo0to1(newEnable));
    }
    updateCurvesFromAPVTS();
}

void EQNodeGraph::computeCompositeEQCurve(juce::Path& path) const {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }

    constexpr int numSteps = 200;
    path.clear();

    for (int step = 0; step <= numSteps; ++step) {
        const float normX = static_cast<float>(step) / static_cast<float>(numSteps);
        const float freq = minFreqHz * std::pow(maxFreqHz / minFreqHz, normX);
        const float x = normX * w;

        float totalDb = 0.0f;
        for (size_t b = 0; b < 8; ++b) {
            if (!nodes_[b].enabled) {
                continue;
            }
            const float f0 = std::max(10.0f, nodes_[b].freqHz);
            const float g0 = nodes_[b].gainDb;
            const float q0 = std::max(0.1f, nodes_[b].q);
            const int type = nodes_[b].type;

            float bandDb = 0.0f;
            if (type == 0) {
                // Bell (Parametric peak/dip)
                const float ratio = freq / f0;
                const float bw = (ratio - 1.0f / ratio) * q0;
                bandDb = g0 / (1.0f + bw * bw);
            } else if (type == 1) {
                // Low-Shelf
                const float r = freq / f0;
                bandDb = g0 / (1.0f + std::pow(r, 2.0f * q0));
            } else if (type == 2) {
                // High-Shelf
                const float r = f0 / freq;
                bandDb = g0 / (1.0f + std::pow(r, 2.0f * q0));
            } else if (type == 3) {
                // Low-Pass (12 dB/oct)
                const float r = freq / f0;
                bandDb = -10.0f * std::log10(1.0f + std::pow(r, 4.0f));
            } else if (type == 4) {
                // High-Pass (12 dB/oct)
                const float r = f0 / freq;
                bandDb = -10.0f * std::log10(1.0f + std::pow(r, 4.0f));
            } else if (type == 5) {
                // Notch
                const float ratio = freq / f0;
                const float bw = std::abs(ratio - 1.0f / ratio) * q0;
                bandDb = -24.0f / (1.0f + bw * bw * 10.0f);
            } else if (type == 6) {
                // Band-Pass
                const float ratio = freq / f0;
                const float bw = (ratio - 1.0f / ratio) * q0;
                bandDb = -18.0f * (bw * bw) / (1.0f + bw * bw);
            }

            totalDb += bandDb;
        }

        const float normY = (maxDb - std::clamp(totalDb, minDb, maxDb)) / (maxDb - minDb);
        const float y = normY * h;

        if (step == 0) {
            path.startNewSubPath(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
}

void EQNodeGraph::paint(juce::Graphics& g) {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }

    // 1. Draw 0 dB baseline
    const float zeroY = (maxDb / (maxDb - minDb)) * h;
    g.setColour(juce::Colour(0x30FFFFFF));
    g.drawHorizontalLine(static_cast<int>(zeroY), 0.0f, w);

    // 2. Composite Curve
    juce::Path curvePath;
    computeCompositeEQCurve(curvePath);

    // Subtle fill below curve to baseline
    juce::Path fillPath = curvePath;
    fillPath.lineTo(w, zeroY);
    fillPath.lineTo(0.0f, zeroY);
    fillPath.closeSubPath();
    juce::ColourGradient fillGrad(StrataLookAndFeel::accentGold.withAlpha(0.12f), 0.0f, zeroY - 40.0f,
                                  StrataLookAndFeel::accentCyan.withAlpha(0.04f), 0.0f, zeroY + 40.0f, false);
    g.setGradientFill(fillGrad);
    g.fillPath(fillPath);

    // Glow under curve
    g.setColour(StrataLookAndFeel::accentGold.withAlpha(0.35f));
    g.strokePath(curvePath, juce::PathStrokeType(4.5f));

    // Sharp glowing curve line
    g.setColour(StrataLookAndFeel::accentGold);
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // 3. Draw 8 Node handles directly on/near their frequency line
    for (int b = 0; b < 8; ++b) {
        const auto& node = nodes_[b];
        const float x = node.screenPos.x;
        const float y = node.screenPos.y;

        const bool isSelected = (draggedBandIndex_ == b || selectedBandIndex_ == b);
        const float radius = isSelected ? 13.0f : 10.0f;

        // Outer glow on active/selected node
        if (isSelected) {
            g.setColour(StrataLookAndFeel::accentCyan.withAlpha(0.5f));
            g.fillEllipse(x - radius - 5.0f, y - radius - 5.0f, (radius + 5.0f) * 2.0f, (radius + 5.0f) * 2.0f);
        }

        // Connecting vertical guideline to 0dB baseline
        g.setColour(node.enabled ? StrataLookAndFeel::accentGold.withAlpha(0.3f) : juce::Colour(0x20FFFFFF));
        g.drawLine(x, zeroY, x, y, 1.0f);

        // Outer rim circle
        if (node.enabled) {
            g.setColour(isSelected ? StrataLookAndFeel::accentCyan : StrataLookAndFeel::accentGold);
        } else {
            g.setColour(StrataLookAndFeel::textDim);
        }
        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

        // Dark center
        g.setColour(StrataLookAndFeel::bgDark);
        g.fillEllipse(x - radius + 2.5f, y - radius + 2.5f, (radius - 2.5f) * 2.0f, (radius - 2.5f) * 2.0f);

        // Band number text
        g.setColour(node.enabled ? StrataLookAndFeel::textLight : StrataLookAndFeel::textDim);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(juce::String(b + 1), static_cast<int>(x - radius), static_cast<int>(y - radius),
                   static_cast<int>(radius * 2.0f), static_cast<int>(radius * 2.0f), juce::Justification::centred);
    }
}

} // namespace strata::gui

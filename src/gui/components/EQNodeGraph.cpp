#include "EQNodeGraph.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>
#include <algorithm>

namespace strata::gui {

EQNodeGraph::EQNodeGraph(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {
    setOpaque(false);
    updateCurvesFromAPVTS();
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
        if (auto* enableParam = apvts_.getRawParameterValue(param::getEQBandEnableID(b))) {
            nodes_[b].enabled = (enableParam->load() > 0.5f);
        }

        if (w > 0.0f && h > 0.0f) {
            const float normX = std::log10(std::clamp(nodes_[b].freqHz, minFreqHz, maxFreqHz) / minFreqHz) / std::log10(maxFreqHz / minFreqHz);
            const float normY = (maxDb - std::clamp(nodes_[b].gainDb, minDb, maxDb)) / (maxDb - minDb);
            nodes_[b].screenPos = { normX * w, normY * h };
        }
    }
    repaint();
}

void EQNodeGraph::resized() {
    updateCurvesFromAPVTS();
}

int EQNodeGraph::getBandUnderMouse(juce::Point<float> pos) const noexcept {
    constexpr float hitRadius = 14.0f;
    for (int b = 0; b < 8; ++b) {
        if (pos.getDistanceFrom(nodes_[b].screenPos) <= hitRadius) {
            return b;
        }
    }
    return -1;
}

void EQNodeGraph::mouseDown(const juce::MouseEvent& e) {
    draggedBandIndex_ = getBandUnderMouse(e.position);
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

    constexpr int numSteps = 150;
    path.clear();

    for (int step = 0; step <= numSteps; ++step) {
        const float normX = static_cast<float>(step) / static_cast<float>(numSteps);
        const float freq = minFreqHz * std::pow(maxFreqHz / minFreqHz, normX);
        const float x = normX * w;

        // Sum responses of 8 parametric bands
        float totalDb = 0.0f;
        for (size_t b = 0; b < 8; ++b) {
            if (!nodes_[b].enabled) {
                continue;
            }
            const float f0 = nodes_[b].freqHz;
            const float g0 = nodes_[b].gainDb;
            const float q0 = std::max(0.1f, nodes_[b].q);

            const float ratio = freq / std::max(1.0f, f0);
            const float bandwidthTerm = (ratio - 1.0f / ratio) * q0;
            const float bandDb = g0 / (1.0f + bandwidthTerm * bandwidthTerm);
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

    // Glow under curve
    g.setColour(StrataLookAndFeel::accentGold.withAlpha(0.25f));
    g.strokePath(curvePath, juce::PathStrokeType(4.0f));

    // Sharp curve line
    g.setColour(StrataLookAndFeel::accentGold);
    g.strokePath(curvePath, juce::PathStrokeType(1.8f));

    // 3. Draw 8 Node handles
    for (int b = 0; b < 8; ++b) {
        const auto& node = nodes_[b];
        const float x = node.screenPos.x;
        const float y = node.screenPos.y;

        const bool isSelected = (draggedBandIndex_ == b);
        const float radius = isSelected ? 12.0f : 9.0f;

        // Outer glow on active/dragged
        if (isSelected) {
            g.setColour(StrataLookAndFeel::accentCyan.withAlpha(0.4f));
            g.fillEllipse(x - radius - 4.0f, y - radius - 4.0f, (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f);
        }

        // Inner circle
        if (node.enabled) {
            g.setColour(isSelected ? StrataLookAndFeel::accentCyan : StrataLookAndFeel::accentGold);
        } else {
            g.setColour(StrataLookAndFeel::textDim);
        }
        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

        // Dark center
        g.setColour(StrataLookAndFeel::bgDark);
        g.fillEllipse(x - radius + 2.0f, y - radius + 2.0f, (radius - 2.0f) * 2.0f, (radius - 2.0f) * 2.0f);

        // Band number text
        g.setColour(node.enabled ? StrataLookAndFeel::textLight : StrataLookAndFeel::textDim);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(juce::String(b + 1), static_cast<int>(x - radius), static_cast<int>(y - radius), static_cast<int>(radius * 2.0f), static_cast<int>(radius * 2.0f), juce::Justification::centred);
    }
}

} // namespace strata::gui

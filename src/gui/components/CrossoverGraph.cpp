#include "CrossoverGraph.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>
#include <algorithm>

namespace strata::gui {

CrossoverGraph::CrossoverGraph(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {
    updateFromAPVTS();
}

float CrossoverGraph::freqToX(float freqHz) const noexcept {
    const float w = static_cast<float>(getWidth());
    if (w <= 0.0f) return 0.0f;
    const float clampedFreq = std::clamp(freqHz, minFreqHz, maxFreqHz);
    const float norm = std::log10(clampedFreq / minFreqHz) / std::log10(maxFreqHz / minFreqHz);
    return norm * w;
}

float CrossoverGraph::xToFreq(float x) const noexcept {
    const float w = static_cast<float>(getWidth());
    if (w <= 0.0f) return minFreqHz;
    const float norm = std::clamp(x / w, 0.0f, 1.0f);
    return minFreqHz * std::pow(maxFreqHz / minFreqHz, norm);
}

void CrossoverGraph::updateFromAPVTS() {
    if (auto* s1 = apvts_.getRawParameterValue(param::CrossoverSplit1.data())) {
        splitFrequencies_[0] = s1->load();
    }
    if (auto* s2 = apvts_.getRawParameterValue(param::CrossoverSplit2.data())) {
        splitFrequencies_[1] = s2->load();
    }
    if (auto* s3 = apvts_.getRawParameterValue(param::CrossoverSplit3.data())) {
        splitFrequencies_[2] = s3->load();
    }

    for (size_t i = 0; i < 3; ++i) {
        splitX_[i] = freqToX(splitFrequencies_[i]);
    }
    repaint();
}

void CrossoverGraph::resized() {
    updateFromAPVTS();
}

void CrossoverGraph::setSelectedBand(int bandIndex) {
    if (bandIndex >= 0 && bandIndex < 4 && selectedBand_ != bandIndex) {
        selectedBand_ = bandIndex;
        repaint();
    }
}

void CrossoverGraph::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Background gradient
    g.fillAll(StrataLookAndFeel::panelBg.darker(0.2f));

    // Calculate band bounds
    const float x0 = 0.0f;
    const float x1 = splitX_[0];
    const float x2 = splitX_[1];
    const float x3 = splitX_[2];
    const float x4 = w;

    const std::array<float, 5> bandEdges{ x0, x1, x2, x3, x4 };

    // Fill selected band with highlighted accent tint
    if (selectedBand_ >= 0 && selectedBand_ < 4) {
        const float bxStart = bandEdges[selectedBand_];
        const float bxEnd = bandEdges[selectedBand_ + 1];
        g.setColour(StrataLookAndFeel::accentCyan.withAlpha(0.12f));
        g.fillRect(bxStart, 0.0f, bxEnd - bxStart, h);
    }

    // Grid lines for standard octaves
    g.setColour(StrataLookAndFeel::borderSubtle.withAlpha(0.4f));
    const float gridFreqs[] = { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f };
    for (float f : gridFreqs) {
        const float gx = freqToX(f);
        g.drawVerticalLine(static_cast<int>(gx), 0.0f, h);
    }

    // Draw Crossover Split Lines & Drag Handles
    for (size_t i = 0; i < 3; ++i) {
        const float sx = splitX_[i];
        const bool isDragged = (draggedSplitIndex_ == static_cast<int>(i));

        // Vertical line
        g.setColour(isDragged ? StrataLookAndFeel::accentCyan : StrataLookAndFeel::textDim);
        g.drawVerticalLine(static_cast<int>(sx), 0.0f, h);

        // Circular handle at center
        const float handleY = h * 0.5f;
        g.setColour(isDragged ? StrataLookAndFeel::accentCyan : StrataLookAndFeel::panelBg.brighter(0.2f));
        g.fillEllipse(sx - 5.0f, handleY - 12.0f, 10.0f, 24.0f);
        g.setColour(StrataLookAndFeel::accentCyan);
        g.drawEllipse(sx - 5.0f, handleY - 12.0f, 10.0f, 24.0f, 1.2f);

        // Frequency Label
        juce::String freqText;
        if (splitFrequencies_[i] >= 1000.0f) {
            freqText = juce::String(splitFrequencies_[i] / 1000.0f, 1) + " kHz";
        } else {
            freqText = juce::String(static_cast<int>(splitFrequencies_[i])) + " Hz";
        }

        g.setFont(juce::Font(10.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText(freqText, static_cast<int>(sx - 35.0f), static_cast<int>(h - 20.0f), 70, 16,
                   juce::Justification::centred);
    }

    // Band Names (Low, Low-Mid, High-Mid, High)
    const juce::String bandNames[] = { "LOW", "LOW-MID", "HIGH-MID", "HIGH" };
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    for (int b = 0; b < 4; ++b) {
        const float bxStart = bandEdges[b];
        const float bxEnd = bandEdges[b + 1];
        const float bw = bxEnd - bxStart;
        const bool isSelected = (b == selectedBand_);

        g.setColour(isSelected ? StrataLookAndFeel::accentCyan : StrataLookAndFeel::textDim);
        g.drawText(bandNames[b], static_cast<int>(bxStart), 6, static_cast<int>(bw), 20,
                   juce::Justification::centred);
    }

    // Border
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRect(bounds, 1.0f);
}

void CrossoverGraph::mouseDown(const juce::MouseEvent& e) {
    constexpr float hitRadius = 14.0f;
    draggedSplitIndex_ = -1;

    for (size_t i = 0; i < 3; ++i) {
        if (std::abs(e.position.x - splitX_[i]) <= hitRadius) {
            draggedSplitIndex_ = static_cast<int>(i);
            repaint();
            return;
        }
    }

    // If not dragging split line, select corresponding band
    for (int b = 0; b < 4; ++b) {
        const float xStart = (b == 0) ? 0.0f : splitX_[b - 1];
        const float xEnd = (b == 3) ? static_cast<float>(getWidth()) : splitX_[b];
        if (e.position.x >= xStart && e.position.x < xEnd) {
            setSelectedBand(b);
            if (onBandSelected) {
                onBandSelected(b);
            }
            break;
        }
    }
}

void CrossoverGraph::mouseDrag(const juce::MouseEvent& e) {
    if (draggedSplitIndex_ < 0 || draggedSplitIndex_ >= 3) return;

    float newFreq = xToFreq(e.position.x);

    // Apply strict crossover ordering constraints
    if (draggedSplitIndex_ == 0) {
        newFreq = std::clamp(newFreq, 40.0f, splitFrequencies_[1] - 30.0f);
        if (auto* param = apvts_.getParameter(param::CrossoverSplit1.data())) {
            param->setValueNotifyingHost(param->convertTo0to1(newFreq));
        }
    } else if (draggedSplitIndex_ == 1) {
        newFreq = std::clamp(newFreq, splitFrequencies_[0] + 30.0f, splitFrequencies_[2] - 100.0f);
        if (auto* param = apvts_.getParameter(param::CrossoverSplit2.data())) {
            param->setValueNotifyingHost(param->convertTo0to1(newFreq));
        }
    } else if (draggedSplitIndex_ == 2) {
        newFreq = std::clamp(newFreq, splitFrequencies_[1] + 100.0f, 18000.0f);
        if (auto* param = apvts_.getParameter(param::CrossoverSplit3.data())) {
            param->setValueNotifyingHost(param->convertTo0to1(newFreq));
        }
    }

    updateFromAPVTS();
    if (onCrossoverChanged) {
        onCrossoverChanged();
    }
}

void CrossoverGraph::mouseUp(const juce::MouseEvent&) {
    draggedSplitIndex_ = -1;
    repaint();
}

} // namespace strata::gui

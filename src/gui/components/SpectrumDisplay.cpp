#include "SpectrumDisplay.h"
#include "../../analysis/SpectrumAnalyzer.h"
#include "../LookAndFeel.h"
#include <cmath>
#include <algorithm>

namespace strata::gui {

SpectrumDisplay::SpectrumDisplay() {
    setOpaque(false);
    displayedMagsDb_.assign(analysis::SpectrumAnalyzer::NUM_BINS, minDb);
}

float SpectrumDisplay::frequencyToX(float freqHz) const noexcept {
    const float w = static_cast<float>(getWidth());
    if (w <= 0.0f) {
        return 0.0f;
    }
    const float clampedFreq = std::clamp(freqHz, minFreqHz, maxFreqHz);
    const float norm = std::log10(clampedFreq / minFreqHz) / std::log10(maxFreqHz / minFreqHz);
    return norm * w;
}

float SpectrumDisplay::xToFrequency(float x) const noexcept {
    const float w = static_cast<float>(getWidth());
    if (w <= 0.0f) {
        return minFreqHz;
    }
    const float norm = std::clamp(x / w, 0.0f, 1.0f);
    return minFreqHz * std::pow(maxFreqHz / minFreqHz, norm);
}

float SpectrumDisplay::decibelsToY(float db) const noexcept {
    const float h = static_cast<float>(getHeight());
    if (h <= 0.0f) {
        return 0.0f;
    }
    const float clampedDb = std::clamp(db, minDb, maxDb);
    const float norm = (maxDb - clampedDb) / (maxDb - minDb);
    return norm * h;
}

float SpectrumDisplay::yToDecibels(float y) const noexcept {
    const float h = static_cast<float>(getHeight());
    if (h <= 0.0f) {
        return minDb;
    }
    const float norm = std::clamp(y / h, 0.0f, 1.0f);
    return maxDb - norm * (maxDb - minDb);
}

void SpectrumDisplay::updateSpectrum(analysis::SpectrumAnalyzer& analyzer) {
    if (analyzer.getLatestMagnitudes(currentMagsDb_, analysis::SpectrumSmoothing::Octave1_6)) {
        if (displayedMagsDb_.size() != currentMagsDb_.size()) {
            displayedMagsDb_.assign(currentMagsDb_.size(), minDb);
        }

        // Fast attack, smooth decay ballistics
        for (size_t i = 0; i < currentMagsDb_.size(); ++i) {
            const float target = currentMagsDb_[i];
            if (target > displayedMagsDb_[i]) {
                displayedMagsDb_[i] = target; // Instant peak attack
            } else {
                displayedMagsDb_[i] = displayedMagsDb_[i] * 0.88f + target * 0.12f; // Smooth 60 FPS falloff
            }
        }
        repaint();
    }
}

void SpectrumDisplay::drawGrid(juce::Graphics& g) {
    const auto w = static_cast<float>(getWidth());
    const auto h = static_cast<float>(getHeight());

    // 1. Horizontal dB grid lines & labels
    const std::array<float, 6> dbLines = { 0.0f, -18.0f, -36.0f, -54.0f, -72.0f, -90.0f };
    g.setFont(juce::Font(9.0f));
    for (float db : dbLines) {
        const float y = decibelsToY(db);
        g.setColour(juce::Colour(0x18FFFFFF));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, w);

        g.setColour(StrataLookAndFeel::textDim);
        g.drawText(juce::String(static_cast<int>(db)) + " dB", 4, static_cast<int>(y) - 10, 40, 10, juce::Justification::left);
    }

    // 2. Vertical Frequency grid lines & labels
    const std::vector<std::pair<float, juce::String>> freqLines = {
        { 50.0f, "50" }, { 100.0f, "100" }, { 250.0f, "250" }, { 500.0f, "500" },
        { 1000.0f, "1k" }, { 2500.0f, "2.5k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" }, { 20000.0f, "20k" }
    };

    for (const auto& [f, label] : freqLines) {
        const float x = frequencyToX(f);
        g.setColour(juce::Colour(0x14FFFFFF));
        g.drawVerticalLine(static_cast<int>(x), 0.0f, h);

        g.setColour(StrataLookAndFeel::textDim);
        g.drawText(label, static_cast<int>(x) - 15, static_cast<int>(h) - 14, 30, 12, juce::Justification::centred);
    }
}

void SpectrumDisplay::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty()) {
        return;
    }

    // Background gradient box
    g.setColour(juce::Colour(0xFF0B0E14));
    g.fillRoundedRectangle(bounds, 6.0f);

    drawGrid(g);

    if (displayedMagsDb_.empty()) {
        return;
    }

    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    constexpr double sampleRate = 44100.0;
    constexpr double binWidth = sampleRate / static_cast<double>(analysis::SpectrumAnalyzer::FFT_SIZE);

    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(0.0f, h);

    bool firstPoint = true;
    for (size_t bin = 1; bin < displayedMagsDb_.size(); ++bin) {
        const float freq = static_cast<float>(static_cast<double>(bin) * binWidth);
        if (freq < minFreqHz) {
            continue;
        }
        if (freq > maxFreqHz) {
            break;
        }

        const float x = frequencyToX(freq);
        const float y = decibelsToY(displayedMagsDb_[bin]);

        if (firstPoint) {
            spectrumPath.lineTo(x, y);
            firstPoint = false;
        } else {
            spectrumPath.lineTo(x, y);
        }
    }

    spectrumPath.lineTo(w, h);
    spectrumPath.closeSubPath();

    // Fill spectrum gradient
    juce::ColourGradient grad(
        juce::Colour(0x5500E5FF), 0.0f, 0.0f,
        juce::Colour(0x05001525), 0.0f, h,
        false);
    g.setGradientFill(grad);
    g.fillPath(spectrumPath);

    // Stroke top spectrum outline
    g.setColour(StrataLookAndFeel::accentCyan.withAlpha(0.85f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.2f));

    // Outer subtle border
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

} // namespace strata::gui

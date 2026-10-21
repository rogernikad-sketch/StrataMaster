#include "LevelMeter.h"
#include "../LookAndFeel.h"
#include "../../dsp/common/DecibelUtils.h"
#include <algorithm>

namespace strata::gui {

LevelMeter::LevelMeter() {
    startTimerHz(60); // 60 FPS smooth meter refresh
}

LevelMeter::~LevelMeter() {
    stopTimer();
}

void LevelMeter::setLevel(float peakL, float peakR) noexcept {
    currentPeakL_.store(peakL, std::memory_order_relaxed);
    currentPeakR_.store(peakR, std::memory_order_relaxed);
}

void LevelMeter::timerCallback() {
    const float targetL = currentPeakL_.load(std::memory_order_relaxed);
    const float targetR = currentPeakR_.load(std::memory_order_relaxed);

    // Ballistics: Instant attack, exponential decay (~300ms)
    displayPeakL_ = (targetL > displayPeakL_) ? targetL : (displayPeakL_ * 0.90f);
    displayPeakR_ = (targetR > displayPeakR_) ? targetR : (displayPeakR_ * 0.90f);

    // Peak hold decay
    peakHoldL_ = std::max(peakHoldL_ * 0.985f, displayPeakL_);
    peakHoldR_ = std::max(peakHoldR_ * 0.985f, displayPeakR_);

    repaint();
}

void LevelMeter::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const float colWidth = (bounds.getWidth() - 2.0f) * 0.5f;

    auto drawBar = [&](float x, float level, float hold) {
        const float db = dsp::gainToDecibels(level, -60.0f);
        // Map -60 dB .. 0 dB to 0.0 .. 1.0
        const float norm = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
        const float barH = bounds.getHeight() * norm;

        // Background groove
        g.setColour(StrataLookAndFeel::panelBg.darker(0.3f));
        g.fillRect(x, bounds.getY(), colWidth, bounds.getHeight());

        // Level bar gradient (green/cyan -> gold -> red if clipping)
        if (barH > 0.0f) {
            juce::ColourGradient grad(StrataLookAndFeel::accentCyan, x, bounds.getBottom(),
                                      (db >= 0.0f ? juce::Colours::red : StrataLookAndFeel::accentGold),
                                      x, bounds.getY(), false);
            g.setGradientFill(grad);
            g.fillRect(x, bounds.getBottom() - barH, colWidth, barH);
        }

        // Peak hold marker
        const float holdDb = dsp::gainToDecibels(hold, -60.0f);
        const float holdNorm = std::clamp((holdDb + 60.0f) / 60.0f, 0.0f, 1.0f);
        const float holdY = bounds.getBottom() - (bounds.getHeight() * holdNorm);
        g.setColour(holdDb >= 0.0f ? juce::Colours::red : StrataLookAndFeel::textLight);
        g.drawHorizontalLine(static_cast<int>(holdY), x, x + colWidth);
    };

    drawBar(bounds.getX(), displayPeakL_, peakHoldL_);
    drawBar(bounds.getX() + colWidth + 2.0f, displayPeakR_, peakHoldR_);
}

} // namespace strata::gui

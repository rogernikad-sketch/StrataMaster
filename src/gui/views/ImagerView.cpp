#include "ImagerView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>

namespace strata::gui {

ImagerView::ImagerView(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts) {

    // Master Width
    masterWidthKnob_ = std::make_unique<Knob>(apvts_, param::StereoWidth.data(), "STEREO WIDTH");
    addAndMakeVisible(*masterWidthKnob_);

    // Mono Maker Group
    monoMakerGroup_.setColour(juce::GroupComponent::outlineColourId, StrataLookAndFeel::borderSubtle);
    monoMakerGroup_.setColour(juce::GroupComponent::textColourId, StrataLookAndFeel::accentCyan);
    addAndMakeVisible(monoMakerGroup_);

    monoMakerEnableButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    monoMakerEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::StereoMonoMakerEnable.data(), monoMakerEnableButton_);
    addAndMakeVisible(monoMakerEnableButton_);

    monoMakerFreqKnob_ = std::make_unique<Knob>(apvts_, param::StereoMonoMakerFreq.data(), "CUTOFF FREQ");
    addAndMakeVisible(*monoMakerFreqKnob_);

    // Stereoizer Group
    stereoizerGroup_.setColour(juce::GroupComponent::outlineColourId, StrataLookAndFeel::borderSubtle);
    stereoizerGroup_.setColour(juce::GroupComponent::textColourId, StrataLookAndFeel::accentGold);
    addAndMakeVisible(stereoizerGroup_);

    stereoizerEnableButton_.setColour(juce::ToggleButton::textColourId, StrataLookAndFeel::textLight);
    stereoizerEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, param::StereoStereoizerEnable.data(), stereoizerEnableButton_);
    addAndMakeVisible(stereoizerEnableButton_);

    stereoizerAmountKnob_ = std::make_unique<Knob>(apvts_, param::StereoStereoizerAmount.data(), "AMOUNT");
    addAndMakeVisible(*stereoizerAmountKnob_);

    // Stereo Mode
    stereoModeBox_.addItemList(juce::StringArray{ "Stereo", "Mid/Side", "Mid Solo", "Side Solo" }, 1);
    stereoModeBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    stereoModeBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    stereoModeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, param::StereoMode.data(), stereoModeBox_);
    addAndMakeVisible(stereoModeBox_);
}

void ImagerView::setCorrelation(float correlation) {
    correlation_ = correlation;
    repaint();
}

void ImagerView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    // Visualizer Area (Top)
    const auto visBounds = getLocalBounds().removeFromTop(getHeight() - 170).reduced(16).toFloat();
    g.setColour(StrataLookAndFeel::panelBg.darker(0.2f));
    g.fillRoundedRectangle(visBounds, 6.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(visBounds, 6.0f, 1.0f);

    const auto center = visBounds.getCentre();
    const float radius = juce::jmin(visBounds.getWidth(), visBounds.getHeight()) * 0.42f;

    // Polar Grid Arcs (Stereo Field)
    g.setColour(StrataLookAndFeel::borderSubtle.withAlpha(0.5f));
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    g.drawEllipse(center.x - radius * 0.5f, center.y - radius * 0.5f, radius, radius, 1.0f);

    // Diagonal 45-degree axis lines (L and R)
    const float d = radius * 0.7071f;
    g.drawLine(center.x - d, center.y - d, center.x + d, center.y + d, 1.0f);
    g.drawLine(center.x - d, center.y + d, center.x + d, center.y - d, 1.0f);
    g.drawLine(center.x, center.y - radius, center.x, center.y + radius, 1.0f); // Center Mid
    g.drawLine(center.x - radius, center.y, center.x + radius, center.y, 1.0f); // Side

    // Axis Labels
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(StrataLookAndFeel::textDim);
    g.drawText("L", static_cast<int>(center.x - d - 18), static_cast<int>(center.y - d - 18), 16, 16, juce::Justification::centred);
    g.drawText("R", static_cast<int>(center.x + d + 2), static_cast<int>(center.y - d - 18), 16, 16, juce::Justification::centred);
    g.drawText("M", static_cast<int>(center.x - 8), static_cast<int>(center.y - radius - 18), 16, 16, juce::Justification::centred);
    g.drawText("+S", static_cast<int>(center.x + radius + 4), static_cast<int>(center.y - 8), 24, 16, juce::Justification::centred);
    g.drawText("-S", static_cast<int>(center.x - radius - 26), static_cast<int>(center.y - 8), 24, 16, juce::Justification::centred);

    // Correlation Meter Bar at bottom of visualizer
    const float corrW = 240.0f;
    const float corrH = 8.0f;
    const float corrX = center.x - corrW * 0.5f;
    const float corrY = visBounds.getBottom() - 30.0f;

    g.setColour(juce::Colour(0xFF0F131C));
    g.fillRoundedRectangle(corrX, corrY, corrW, corrH, 4.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(corrX, corrY, corrW, corrH, 4.0f, 1.0f);

    // Indicator Dot
    const float normCorr = (correlation_ + 1.0f) * 0.5f;
    const float dotX = corrX + normCorr * corrW;
    const juce::Colour corrCol = (correlation_ >= 0.2f) ? StrataLookAndFeel::accentCyan
                               : (correlation_ >= -0.2f) ? StrataLookAndFeel::accentGold
                               : juce::Colours::red;

    g.setColour(corrCol);
    g.fillEllipse(dotX - 4.0f, corrY - 3.0f, 8.0f, 14.0f);

    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(corrCol);
    g.drawText(juce::String::formatted("CORRELATION: %+.2f", correlation_),
               static_cast<int>(corrX), static_cast<int>(corrY - 20.0f), static_cast<int>(corrW), 16,
               juce::Justification::centred);

    // Bottom controls panel background
    g.setColour(StrataLookAndFeel::panelBg.darker(0.1f));
    g.fillRect(0, getHeight() - 170, getWidth(), 170);

    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawHorizontalLine(getHeight() - 170, 0.0f, static_cast<float>(getWidth()));
}

void ImagerView::resized() {
    auto bounds = getLocalBounds();

    auto bottom = bounds.removeFromBottom(170).reduced(16, 12);

    // Master Width Knob
    const int knobW = 90;
    const int knobH = 100;
    masterWidthKnob_->setBounds(bottom.removeFromLeft(knobW).withHeight(knobH));

    bottom.removeFromLeft(20);

    // Mono Maker Section
    auto mmArea = bottom.removeFromLeft(200);
    monoMakerGroup_.setBounds(mmArea);
    auto mmInner = mmArea.reduced(10, 20);
    monoMakerEnableButton_.setBounds(mmInner.removeFromTop(24));
    monoMakerFreqKnob_->setBounds(mmInner.removeFromLeft(80).withHeight(knobH - 20));

    bottom.removeFromLeft(20);

    // Stereoizer Section
    auto stArea = bottom.removeFromLeft(200);
    stereoizerGroup_.setBounds(stArea);
    auto stInner = stArea.reduced(10, 20);
    stereoizerEnableButton_.setBounds(stInner.removeFromTop(24));
    stereoizerAmountKnob_->setBounds(stInner.removeFromLeft(80).withHeight(knobH - 20));

    bottom.removeFromLeft(20);

    // Mode Box
    auto modeArea = bottom.removeFromLeft(120);
    stereoModeBox_.setBounds(modeArea.removeFromTop(26));
}

} // namespace strata::gui

#include "MasterAssistantView.h"
#include "../../engine/ParameterIDs.h"
#include "../LookAndFeel.h"
#include <cmath>

namespace strata::gui {

MasterAssistantView::MasterAssistantView(juce::AudioProcessorValueTreeState& apvts, assistant::AutoMasterEngine& assistantEngine)
    : apvts_(apvts), assistantEngine_(assistantEngine) {

    // Profile Selector
    profileLabel_.setFont(juce::Font(13.0f, juce::Font::bold));
    profileLabel_.setColour(juce::Label::textColourId, StrataLookAndFeel::textLight);
    addAndMakeVisible(profileLabel_);

    profileBox_.addItem("Streaming (-14.0 LUFS, -1.0 dBTP)", 1);
    profileBox_.addItem("CD / Commercial (-9.0 LUFS, -0.3 dBTP)", 2);
    profileBox_.addItem("Club / Electronic (-8.0 LUFS, -0.2 dBTP)", 3);
    profileBox_.addItem("High Dynamic / Classical (-18.0 LUFS, -1.5 dBTP)", 4);
    profileBox_.setSelectedId(1, juce::dontSendNotification);
    profileBox_.setColour(juce::ComboBox::backgroundColourId, StrataLookAndFeel::panelBg);
    profileBox_.setColour(juce::ComboBox::textColourId, StrataLookAndFeel::textLight);
    profileBox_.onChange = [this]() {
        const int id = profileBox_.getSelectedId();
        switch (id) {
            case 1: assistantEngine_.setTargetGenre(assistant::GenreProfile::StreamingStandard); break;
            case 2: assistantEngine_.setTargetGenre(assistant::GenreProfile::CDPop); break;
            case 3: assistantEngine_.setTargetGenre(assistant::GenreProfile::ClubEDM); break;
            case 4: assistantEngine_.setTargetGenre(assistant::GenreProfile::AcousticWarm); break;
            default: break;
        }
        repaint();
    };
    addAndMakeVisible(profileBox_);

    // Action Buttons
    listenButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg.brighter(0.15f));
    listenButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentGold);
    listenButton_.onClick = [this]() {
        assistantEngine_.startListening();
        updateTelemetry();
    };
    addAndMakeVisible(listenButton_);

    applyButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::accentCyan.darker(0.6f));
    applyButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::accentCyan);
    applyButton_.onClick = [this]() {
        assistantEngine_.applyRecommendations(apvts_);
        if (onMasterApplied) {
            onMasterApplied();
        }
        repaint();
    };
    addAndMakeVisible(applyButton_);

    reanalyzeButton_.setColour(juce::TextButton::buttonColourId, StrataLookAndFeel::panelBg);
    reanalyzeButton_.setColour(juce::TextButton::textColourOffId, StrataLookAndFeel::textDim);
    reanalyzeButton_.onClick = [this]() {
        assistantEngine_.startListening();
        updateTelemetry();
    };
    addAndMakeVisible(reanalyzeButton_);

    updateTelemetry();
}

void MasterAssistantView::updateTelemetry() {
    state_ = assistantEngine_.getState();
    progressPercent_ = assistantEngine_.getListeningProgressPercent();
    recommendations_ = assistantEngine_.getRecommendations();

    if (state_ == assistant::AssistantState::Listening) {
        listenButton_.setButtonText(juce::String::formatted("LISTENING (%.0f%%)...", progressPercent_));
        applyButton_.setEnabled(false);
    } else if (state_ == assistant::AssistantState::Ready) {
        listenButton_.setButtonText("LISTEN AGAIN");
        applyButton_.setEnabled(true);
    } else {
        listenButton_.setButtonText("START LISTENING (5s)");
        applyButton_.setEnabled(false);
    }

    repaint();
}

void MasterAssistantView::paint(juce::Graphics& g) {
    g.fillAll(StrataLookAndFeel::bgDark);

    const auto bounds = getLocalBounds().toFloat();

    // Visualizer Area
    const auto visBounds = bounds.withTrimmedTop(60.0f).withTrimmedBottom(80.0f).reduced(24.0f);
    g.setColour(StrataLookAndFeel::panelBg.darker(0.2f));
    g.fillRoundedRectangle(visBounds, 8.0f);
    g.setColour(StrataLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(visBounds, 8.0f, 1.0f);

    const auto center = visBounds.getCentre();

    if (state_ == assistant::AssistantState::Listening) {
        // Listening Circular Radar / Progress
        const float radius = 70.0f;
        g.setColour(StrataLookAndFeel::borderSubtle);
        g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 4.0f);

        juce::Path arc;
        const float startAngle = 0.0f;
        const float endAngle = (progressPercent_ / 100.0f) * juce::MathConstants<float>::twoPi;
        arc.addCentredArc(center.x, center.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour(StrataLookAndFeel::accentGold);
        g.strokePath(arc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont(juce::Font(22.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText(juce::String::formatted("%.0f%%", progressPercent_),
                   static_cast<int>(center.x - 50.0f), static_cast<int>(center.y - 14.0f), 100, 28,
                   juce::Justification::centred);

        g.setFont(juce::Font(13.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textDim);
        g.drawText("ANALYZING REAL-TIME AUDIO CHARACTERISTICS...",
                   static_cast<int>(visBounds.getX()), static_cast<int>(center.y + radius + 20.0f),
                   static_cast<int>(visBounds.getWidth()), 20, juce::Justification::centred);

    } else if (state_ == assistant::AssistantState::Ready) {
        // Recommendations Summary
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::accentGold);
        g.drawText("ACOUSTIC EVALUATION COMPLETE",
                   static_cast<int>(visBounds.getX() + 30), static_cast<int>(visBounds.getY() + 20),
                   static_cast<int>(visBounds.getWidth() - 60), 26, juce::Justification::left);

        // Cards for metrics
        const float cardW = (visBounds.getWidth() - 80.0f) / 3.0f;
        const float cardH = visBounds.getHeight() - 80.0f;
        const float cardY = visBounds.getY() + 60.0f;

        // Card 1: Loudness & Limiter
        const auto c1 = juce::Rectangle<float>(visBounds.getX() + 20.0f, cardY, cardW, cardH);
        g.setColour(StrataLookAndFeel::panelBg);
        g.fillRoundedRectangle(c1, 6.0f);
        g.setColour(StrataLookAndFeel::borderSubtle);
        g.drawRoundedRectangle(c1, 6.0f, 1.0f);

        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::accentCyan);
        g.drawText("LOUDNESS & LIMITING", static_cast<int>(c1.getX() + 10), static_cast<int>(c1.getY() + 12), static_cast<int>(c1.getWidth() - 20), 20, juce::Justification::left);
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText(juce::String::formatted("Measured: %.1f LUFS", recommendations_.measuredLufs), static_cast<int>(c1.getX() + 10), static_cast<int>(c1.getY() + 42), static_cast<int>(c1.getWidth() - 20), 18, juce::Justification::left);
        g.drawText(juce::String::formatted("Limiter Thresh: %.1f dB", recommendations_.limiterThresholdDb), static_cast<int>(c1.getX() + 10), static_cast<int>(c1.getY() + 64), static_cast<int>(c1.getWidth() - 20), 18, juce::Justification::left);
        g.drawText(juce::String::formatted("Limiter Ceiling: %.1f dBTP", recommendations_.limiterCeilingDb), static_cast<int>(c1.getX() + 10), static_cast<int>(c1.getY() + 86), static_cast<int>(c1.getWidth() - 20), 18, juce::Justification::left);

        // Card 2: Dynamic Balance
        const auto c2 = juce::Rectangle<float>(c1.getRight() + 20.0f, cardY, cardW, cardH);
        g.setColour(StrataLookAndFeel::panelBg);
        g.fillRoundedRectangle(c2, 6.0f);
        g.setColour(StrataLookAndFeel::borderSubtle);
        g.drawRoundedRectangle(c2, 6.0f, 1.0f);

        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::accentGold);
        g.drawText("DYNAMIC BALANCE", static_cast<int>(c2.getX() + 10), static_cast<int>(c2.getY() + 12), static_cast<int>(c2.getWidth() - 20), 20, juce::Justification::left);
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText(juce::String::formatted("Crest Factor: %.1f dB", recommendations_.measuredCrestFactorDb), static_cast<int>(c2.getX() + 10), static_cast<int>(c2.getY() + 42), static_cast<int>(c2.getWidth() - 20), 18, juce::Justification::left);
        g.drawText(juce::String::formatted("Low Thresh: %.1f dB", recommendations_.dynThresholdsDb[0]), static_cast<int>(c2.getX() + 10), static_cast<int>(c2.getY() + 64), static_cast<int>(c2.getWidth() - 20), 18, juce::Justification::left);
        g.drawText(juce::String::formatted("Mid Thresh: %.1f dB", recommendations_.dynThresholdsDb[1]), static_cast<int>(c2.getX() + 10), static_cast<int>(c2.getY() + 86), static_cast<int>(c2.getWidth() - 20), 18, juce::Justification::left);

        // Card 3: Stereo Field & Bass
        const auto c3 = juce::Rectangle<float>(c2.getRight() + 20.0f, cardY, cardW, cardH);
        g.setColour(StrataLookAndFeel::panelBg);
        g.fillRoundedRectangle(c3, 6.0f);
        g.setColour(StrataLookAndFeel::borderSubtle);
        g.drawRoundedRectangle(c3, 6.0f, 1.0f);

        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::accentCyan);
        g.drawText("STEREO & BASS ANCHOR", static_cast<int>(c3.getX() + 10), static_cast<int>(c3.getY() + 12), static_cast<int>(c3.getWidth() - 20), 20, juce::Justification::left);
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText(juce::String::formatted("Mono Anchor: %.0f Hz", recommendations_.monoMakerFreqHz), static_cast<int>(c3.getX() + 10), static_cast<int>(c3.getY() + 42), static_cast<int>(c3.getWidth() - 20), 18, juce::Justification::left);
        g.drawText(juce::String::formatted("Master Width: %.0f%%", recommendations_.stereoWidth), static_cast<int>(c3.getX() + 10), static_cast<int>(c3.getY() + 64), static_cast<int>(c3.getWidth() - 20), 18, juce::Justification::left);
        g.drawText("Match EQ: 8 Bands Calculated", static_cast<int>(c3.getX() + 10), static_cast<int>(c3.getY() + 86), static_cast<int>(c3.getWidth() - 20), 18, juce::Justification::left);

    } else {
        // Idle State Prompt
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.setColour(StrataLookAndFeel::textLight);
        g.drawText("INTELLIGENT ACOUSTIC MASTERING ASSISTANT",
                   static_cast<int>(visBounds.getX()), static_cast<int>(center.y - 30.0f),
                   static_cast<int>(visBounds.getWidth()), 26, juce::Justification::centred);

        g.setFont(juce::Font(13.0f, juce::Font::plain));
        g.setColour(StrataLookAndFeel::textDim);
        g.drawText("Play the loudest section of your track in your DAW, then click Start Listening below.",
                   static_cast<int>(visBounds.getX()), static_cast<int>(center.y + 6.0f),
                   static_cast<int>(visBounds.getWidth()), 20, juce::Justification::centred);
    }
}

void MasterAssistantView::resized() {
    auto bounds = getLocalBounds();

    // Top Header: Profile Selection
    auto topArea = bounds.removeFromTop(54).reduced(24, 10);
    profileLabel_.setBounds(topArea.removeFromLeft(220));
    profileBox_.setBounds(topArea.removeFromLeft(320));

    // Bottom Action Buttons
    auto bottomArea = bounds.removeFromBottom(70).reduced(24, 16);
    listenButton_.setBounds(bottomArea.removeFromLeft(200));
    bottomArea.removeFromLeft(20);
    applyButton_.setBounds(bottomArea.removeFromLeft(160));
    bottomArea.removeFromLeft(20);
    reanalyzeButton_.setBounds(bottomArea.removeFromLeft(140));
}

} // namespace strata::gui

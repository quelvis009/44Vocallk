#pragma once
#include "PluginProcessor.h"

class VocalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

class VocalAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit VocalAudioProcessorEditor (VocalAudioProcessor&);
    ~VocalAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum Fmt { fmtHz, fmtDb, fmtDbSigned, fmtMs, fmtPlain, fmtRatio, fmtPct };

    struct KnobSpec
    {
        const char* id;
        juce::String name;
        juce::String section;
        int fmt;
        bool gainType;
        int cx, cy;
        float scaleRadius;
        std::vector<std::pair<float, juce::String>> labels;
    };

    void timerCallback() override;
    void stepPreset (int delta);
    juce::String formatValue (const KnobSpec&, double v) const;
    void drawKnobScale (juce::Graphics&, const KnobSpec&);
    void drawSeal (juce::Graphics&, juce::Point<float> centre, float r);
    void drawReadout (juce::Graphics&);

    VocalAudioProcessor& proc;
    VocalLookAndFeel lnf;

    std::vector<KnobSpec> specs;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    juce::TextButton prevButton { "<" }, nextButton { ">" };

    const juce::Rectangle<int> readoutArea  { 262, 533, 366, 64 };
    const juce::Rectangle<int> sealArea     { 640, 512, 112, 106 };
    const juce::Rectangle<int> meterArea    { 384, 357, 70, 18 };
    const juce::Rectangle<int> eqScreenArea { 36, 178, 688, 30 };

    int shownPreset = -1;
    int lastKnob = -1;
    double lastReadValue = 0.0;
    float ringRot = 0.0f;
    float pulsePhase = 0.0f;
    float grShown = 0.0f;
    float shownEqF[4] = { -1, -1, -1, -1 };
    float shownEqG[4] = { -99, -99, -99, -99 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalAudioProcessorEditor)
};

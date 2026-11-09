#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include "Dsp.h"

namespace ids
{
inline constexpr const char* eqF[4] = { "eq1_freq", "eq2_freq", "eq3_freq", "eq4_freq" };
inline constexpr const char* eqG[4] = { "eq1_gain", "eq2_gain", "eq3_gain", "eq4_gain" };
inline constexpr const char* thr     = "comp_thr";
inline constexpr const char* atk     = "comp_atk";
inline constexpr const char* rel     = "comp_rel";
inline constexpr const char* ratio   = "comp_ratio";
inline constexpr const char* makeup  = "comp_gain";
inline constexpr const char* deF     = "de_freq";
inline constexpr const char* deThr   = "de_thr";
inline constexpr const char* deRed   = "de_red";
inline constexpr const char* sat     = "sat";
inline constexpr const char* satTone = "sat_tone";
inline constexpr const char* satMix  = "sat_mix";
inline constexpr const char* aire    = "aire";
inline constexpr const char* out     = "salida";
}

struct VPreset
{
    const char* name;
    float eqF[4];
    float eqG[4];
    float thr, atk, rel, ratio, makeup;
    float deF, deThr, deRed;
    float aire, sat, satTone, satMix, out;
};

class VocalAudioProcessor : public juce::AudioProcessor
{
public:
    VocalAudioProcessor();
    ~VocalAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "44vocal"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static const std::vector<VPreset>& getPresets();
    void applyPreset (int index);
    int getCurrentPresetIndex() const { return currentPreset.load(); }
    float getGainReduction() const { return grMeter.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float realValue);
    void updateEq (int band, float freq, float gainDb);
    void updateAir (float aireValue);

    std::atomic<int> currentPreset { 0 };
    std::atomic<float> grMeter { 0.0f };

    double sampleRateHz = 44100.0;

    juce::dsp::IIR::Filter<float> eq[4][2];
    juce::dsp::IIR::Filter<float> airFilter[2];
    float lastEqF[4] = { -1, -1, -1, -1 };
    float lastEqG[4] = { -99, -99, -99, -99 };
    float lastAire = -99.0f;

    vfx::Compressor comp;
    vfx::DeEsser deess;
    vfx::OnePoleLP satLp[2];
    juce::SmoothedValue<float> outGain;
    juce::dsp::Limiter<float> limiter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalAudioProcessor)
};

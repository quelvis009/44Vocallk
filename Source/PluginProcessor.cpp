#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

const std::vector<VPreset>& VocalAudioProcessor::getPresets()
{
    //   nombre        eq freqs (Hz)               eq gains (dB)              comp: thr atk rel ratio makeup   de-esser: f thr red   aire sat tono mezcla out
    static const std::vector<VPreset> presets =
    {
        { "Limpio",    { 100, 500, 3000, 10000 }, { 0, 0, 0, 0 },           -20, 10, 120, 2.0f, 2, 7000, -20, 3,   1.0f, 0.5f, 14000, 100,  0 },
        { "Voz pop",   { 100, 400, 3500, 12000 }, { -2, -2.5f, 3, 3 },      -24, 8,  100, 3.5f, 5, 7500, -24, 5,   4.0f, 2.0f, 12000, 100,  0 },
        { "Rap",       { 90, 350, 3000, 10000 },  { 1.5f, -3, 3.5f, 2 },    -28, 3,  80,  5.0f, 8, 7000, -26, 6,   3.0f, 3.5f,  9000, 100, -1 },
        { "Voz grave", { 100, 300, 2500, 8000 },  { 2, -2, 2, 2.5f },       -22, 15, 150, 3.0f, 4, 6500, -22, 4,   3.0f, 2.0f, 10000, 100,  0 },
        { "Voz aguda", { 120, 500, 4000, 14000 }, { -4, -2, 2, 3 },         -24, 6,  90,  3.0f, 4, 8500, -26, 7,   5.0f, 1.5f, 14000, 100,  0 },
        { "Coros",     { 120, 450, 2500, 9000 },  { -3, -1, -1, 2 },        -20, 12, 140, 4.0f, 5, 7000, -24, 5,   4.0f, 1.0f, 11000,  80, -2 },
        { "Susurro",   { 150, 600, 4500, 12000 }, { -4, 0, 3, 4 },          -34, 4,  120, 4.0f, 9, 8000, -30, 8,   6.0f, 0.5f, 16000, 100,  0 },
        { "Radio",     { 200, 800, 3000, 8000 },  { -8, 3, 4, -6 },         -26, 5,  60,  6.0f, 8, 6000, -20, 3,   0.0f, 5.0f,  5000, 100, -2 },
        { "Brillante", { 80, 400, 5000, 16000 },  { -2, -2, 4, 5 },         -22, 7,  100, 3.0f, 4, 8000, -26, 6,   7.0f, 3.0f, 16000, 100, -1 },
        { "Calido",    { 120, 300, 2500, 6000 },  { 2.5f, 1, -1, -2 },      -22, 12, 140, 3.0f, 4, 6000, -20, 3,   1.0f, 4.0f,  7000, 100,  0 }
    };
    return presets;
}

static juce::NormalisableRange<float> rangeWithCentre (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addF = [&p] (const char* id, const juce::String& name, juce::NormalisableRange<float> range, float def)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def));
    };

    const char* bandNames[4] = { "Graves", "Medios", "Presencia", "Agudos" };
    const float lo[4]  = { 20.0f, 200.0f, 1000.0f, 5000.0f };
    const float hi[4]  = { 250.0f, 1000.0f, 8000.0f, 20000.0f };
    const float mid[4] = { 70.0f, 450.0f, 2800.0f, 10000.0f };
    const float def[4] = { 100.0f, 400.0f, 3000.0f, 10000.0f };

    for (int b = 0; b < 4; ++b)
    {
        addF (ids::eqF[b], juce::String (bandNames[b]) + " freq", rangeWithCentre (lo[b], hi[b], mid[b]), def[b]);
        addF (ids::eqG[b], juce::String (bandNames[b]) + " gain", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f);
    }

    addF (ids::thr,    "Threshold",      juce::NormalisableRange<float> (-60.0f, 0.0f), -20.0f);
    addF (ids::atk,    "Ataque",         rangeWithCentre (0.1f, 100.0f, 5.0f), 10.0f);
    addF (ids::rel,    "Release",        rangeWithCentre (10.0f, 1000.0f, 100.0f), 120.0f);
    addF (ids::ratio,  "Ratio",          juce::NormalisableRange<float> (1.0f, 10.0f), 3.0f);
    addF (ids::makeup, "Compresor gain", juce::NormalisableRange<float> (0.0f, 18.0f), 0.0f);

    addF (ids::deF,    "De-esser frecuencia", rangeWithCentre (4000.0f, 10000.0f, 6300.0f), 7000.0f);
    addF (ids::deThr,  "De-esser umbral",     juce::NormalisableRange<float> (-40.0f, 0.0f), -20.0f);
    addF (ids::deRed,  "De-esser reduccion",  juce::NormalisableRange<float> (0.0f, 12.0f), 0.0f);

    addF (ids::sat,     "Saturacion drive", juce::NormalisableRange<float> (0.0f, 10.0f), 0.0f);
    addF (ids::satTone, "Saturacion tono",  rangeWithCentre (1000.0f, 20000.0f, 6000.0f), 12000.0f);
    addF (ids::satMix,  "Saturacion mezcla", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f);

    addF (ids::aire,   "Aire",   juce::NormalisableRange<float> (0.0f, 10.0f), 0.0f);
    addF (ids::out,    "Salida", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f);

    return { p.begin(), p.end() };
}

VocalAudioProcessor::VocalAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOCAL44", createLayout())
{
}

void VocalAudioProcessor::setParam (const char* id, float realValue)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

void VocalAudioProcessor::applyPreset (int index)
{
    auto& presets = getPresets();
    index = juce::jlimit (0, (int) presets.size() - 1, index);
    const auto& pr = presets[(size_t) index];

    for (int b = 0; b < 4; ++b)
    {
        setParam (ids::eqF[b], pr.eqF[b]);
        setParam (ids::eqG[b], pr.eqG[b]);
    }
    setParam (ids::thr, pr.thr);
    setParam (ids::atk, pr.atk);
    setParam (ids::rel, pr.rel);
    setParam (ids::ratio, pr.ratio);
    setParam (ids::makeup, pr.makeup);
    setParam (ids::deF, pr.deF);
    setParam (ids::deThr, pr.deThr);
    setParam (ids::deRed, pr.deRed);
    setParam (ids::sat, pr.sat);
    setParam (ids::satTone, pr.satTone);
    setParam (ids::satMix, pr.satMix);
    setParam (ids::aire, pr.aire);
    setParam (ids::out, pr.out);

    currentPreset.store (index);
    apvts.state.setProperty ("presetIndex", index, nullptr);
}

void VocalAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;

    juce::dsp::ProcessSpec mono { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    juce::dsp::ProcessSpec stereo { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    for (int b = 0; b < 4; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            eq[b][ch].prepare (mono);
            eq[b][ch].reset();
        }
        lastEqF[b] = -1.0f;
        lastEqG[b] = -99.0f;
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        airFilter[ch].prepare (mono);
        airFilter[ch].reset();
        satLp[ch].reset();
    }
    lastAire = -99.0f;

    comp.prepare (sampleRate);
    deess.prepare (sampleRate, samplesPerBlock);

    limiter.prepare (stereo);
    limiter.setThreshold (-1.0f);
    limiter.setRelease (60.0f);

    outGain.reset (sampleRate, 0.03);
    outGain.setCurrentAndTargetValue (1.0f);
}

bool VocalAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void VocalAudioProcessor::updateEq (int band, float freq, float gainDb)
{
    freq = juce::jlimit (20.0f, (float) (0.45 * sampleRateHz), freq);
    const float gain = vfx::dbToLin (gainDb);

    juce::dsp::IIR::Coefficients<float>::Ptr c;
    if (band == 0)
        c = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRateHz, freq, 0.7f, gain);
    else if (band == 3)
        c = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRateHz, freq, 0.7f, gain);
    else
        c = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRateHz, freq, band == 1 ? 0.9f : 1.1f, gain);

    for (int ch = 0; ch < 2; ++ch)
        eq[band][ch].coefficients = c;
}

void VocalAudioProcessor::updateAir (float aireValue)
{
    const float gain = vfx::dbToLin (aireValue * 0.6f);
    auto c = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRateHz, juce::jmin (12000.0f, (float) (0.45 * sampleRateHz)), 0.7f, gain);
    for (int ch = 0; ch < 2; ++ch)
        airFilter[ch].coefficients = c;
}

void VocalAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // 1) Ecualizador de 4 bandas
    for (int b = 0; b < 4; ++b)
    {
        const float f = get (ids::eqF[b]);
        const float g = get (ids::eqG[b]);
        if (f != lastEqF[b] || g != lastEqG[b])
        {
            updateEq (b, f, g);
            lastEqF[b] = f;
            lastEqG[b] = g;
        }
        for (int i = 0; i < n; ++i)
        {
            L[i] = eq[b][0].processSample (L[i]);
            R[i] = eq[b][1].processSample (R[i]);
        }
    }

    // 2) Compresor
    {
        const float thr = get (ids::thr);
        const float ratio = get (ids::ratio);
        const float makeupDb = get (ids::makeup);
        comp.setTimes (get (ids::atk), get (ids::rel));

        float minGr = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float lvl = std::max (std::fabs (L[i]), std::fabs (R[i]));
            const float gr = comp.process (lvl, thr, ratio);
            minGr = std::min (minGr, gr);
            const float g = vfx::dbToLin (gr + makeupDb);
            L[i] *= g;
            R[i] *= g;
        }
        grMeter.store (-minGr);
    }

    // 3) De-esser
    {
        const float red = get (ids::deRed);
        if (red > 0.05f)
        {
            deess.setFreq (get (ids::deF));
            const float thr = get (ids::deThr);
            for (int i = 0; i < n; ++i)
                deess.process (L[i], R[i], thr, red);
        }
    }

    // 4) Saturacion: drive + tono (pasa-bajos sobre la señal saturada) + mezcla seco/humedo
    {
        const float s = get (ids::sat) / 10.0f;
        if (s > 0.001f)
        {
            const float drive = 1.0f + 7.0f * s;
            const float bias = 0.10f * s;
            const float tb = std::tanh (bias);
            const float slope = drive * (1.0f - tb * tb);   // ganancia unitaria a bajo nivel
            const float mix = juce::jlimit (0.0f, 1.0f, get (ids::satMix) / 100.0f);
            const float tone = get (ids::satTone);
            const float sr = (float) sampleRateHz;

            satLp[0].setCutoff (tone, sr);
            satLp[1].setCutoff (tone, sr);

            for (int i = 0; i < n; ++i)
            {
                float wl = (std::tanh (drive * L[i] + bias) - tb) / slope;
                float wr = (std::tanh (drive * R[i] + bias) - tb) / slope;
                wl = satLp[0].process (wl);
                wr = satLp[1].process (wr);
                L[i] = L[i] * (1.0f - mix) + wl * mix;
                R[i] = R[i] * (1.0f - mix) + wr * mix;
            }
        }
        else
        {
            satLp[0].reset();
            satLp[1].reset();
        }
    }

    // 5) Aire (realce suave de agudos)
    {
        const float a = get (ids::aire);
        if (a != lastAire)
        {
            updateAir (a);
            lastAire = a;
        }
        if (a > 0.01f)
        {
            for (int i = 0; i < n; ++i)
            {
                L[i] = airFilter[0].processSample (L[i]);
                R[i] = airFilter[1].processSample (R[i]);
            }
        }
    }

    // 6) Salida y limitador de seguridad
    outGain.setTargetValue (vfx::dbToLin (get (ids::out)));
    for (int i = 0; i < n; ++i)
    {
        const float g = outGain.getNextValue();
        L[i] *= g;
        R[i] *= g;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    limiter.process (ctx);
}

juce::AudioProcessorEditor* VocalAudioProcessor::createEditor()
{
    return new VocalAudioProcessorEditor (*this);
}

void VocalAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VocalAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        currentPreset.store ((int) apvts.state.getProperty ("presetIndex", 0));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalAudioProcessor();
}

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace vfx
{
inline float dbToLin (float db) { return std::pow (10.0f, db * 0.05f); }
inline float linToDb (float x)  { return 20.0f * std::log10 (std::max (x, 1.0e-6f)); }

// Compresor con rodilla suave. Devuelve la reduccion de ganancia en dB (<= 0).
struct Compressor
{
    float sr = 44100.0f;
    float grState = 0.0f;
    float atkCoef = 0.0f, relCoef = 0.0f;

    void prepare (double s) { sr = (float) s; grState = 0.0f; setTimes (10.0f, 100.0f); }

    void setTimes (float atkMs, float relMs)
    {
        atkCoef = std::exp (-1.0f / (0.001f * std::max (atkMs, 0.05f) * sr));
        relCoef = std::exp (-1.0f / (0.001f * std::max (relMs, 1.0f) * sr));
    }

    inline float process (float level, float thrDb, float ratio)
    {
        const float lvlDb = linToDb (level);
        const float over = lvlDb - thrDb;
        const float knee = 6.0f;
        const float slope = 1.0f / ratio - 1.0f;
        float target = 0.0f;

        if (2.0f * over < -knee)
            target = 0.0f;
        else if (2.0f * std::fabs (over) <= knee)
        {
            const float t = over + knee * 0.5f;
            target = slope * t * t / (2.0f * knee);
        }
        else
            target = slope * over;

        const float coef = (target < grState) ? atkCoef : relCoef;
        grState = coef * grState + (1.0f - coef) * target;
        return grState;
    }
};

// De-esser: filtro de banda como cadena lateral y reduccion solo de esa banda.
struct DeEsser
{
    juce::dsp::StateVariableTPTFilter<float> band[2];
    float sr = 44100.0f;
    float gr = 0.0f;
    float atk = 0.0f, rel = 0.0f;

    void prepare (double s, int blockSize)
    {
        sr = (float) s;
        juce::dsp::ProcessSpec spec { s, (juce::uint32) blockSize, 1 };
        for (auto& b : band)
        {
            b.prepare (spec);
            b.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            b.setResonance (1.2f);
            b.setCutoffFrequency (7000.0f);
        }
        atk = std::exp (-1.0f / (0.0005f * sr));
        rel = std::exp (-1.0f / (0.05f * sr));
        gr = 0.0f;
    }

    void setFreq (float f)
    {
        f = juce::jlimit (1000.0f, 0.45f * sr, f);
        for (auto& b : band)
            b.setCutoffFrequency (f);
    }

    inline void process (float& l, float& r, float thrDb, float maxRedDb)
    {
        const float bl = band[0].processSample (0, l);
        const float br = band[1].processSample (0, r);
        const float lvl = std::max (std::fabs (bl), std::fabs (br));
        const float over = linToDb (lvl) - thrDb;
        const float target = over > 0.0f ? -std::min (maxRedDb, over) : 0.0f;
        const float coef = target < gr ? atk : rel;
        gr = coef * gr + (1.0f - coef) * target;
        const float g = dbToLin (gr);
        l -= bl * (1.0f - g);
        r -= br * (1.0f - g);
    }
};

// Filtro pasa-bajos de un polo (se usa como control de "Tono" de la saturacion).
struct OnePoleLP
{
    float a = 1.0f;
    float z = 0.0f;

    void setCutoff (float fc, float sr)
    {
        fc = juce::jlimit (20.0f, 0.45f * sr, fc);
        a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / sr);
    }

    void reset() { z = 0.0f; }

    inline float process (float x)
    {
        z += a * (x - z);
        return z;
    }
};
} // namespace vfx

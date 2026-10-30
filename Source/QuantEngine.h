#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace qp
{
    struct InstrumentPreset
    {
        const char* name;
        float strength;     // 0..1
        float swing;        // 0..1
        float humanize;     // 0..1
        float sensitivity;  // 0..1
        float windowMs;     // maxima distancia que puede mover un golpe
        float fadeMs;       // crossfade entre golpes
        float preMs;        // pre-ataque que viaja con cada golpe
        float minGapMs;     // separacion minima entre golpes detectados
        int   grid;         // indice de rejilla sugerida
    };

    inline const InstrumentPreset* presets (int& n)
    {
        static const InstrumentPreset p[] =
        {
            { "Bachata en vivo (general)",     0.80f, 0.00f, 0.25f, 0.50f, 110.f, 6.f, 8.f, 55.f, 3 },
            { "Segunda guitarra bachata",      0.88f, 0.00f, 0.15f, 0.55f, 120.f, 6.f, 10.f, 60.f, 3 },
            { "Requinto",                      0.72f, 0.00f, 0.30f, 0.60f,  90.f, 4.f, 6.f, 45.f, 3 },
            { "Adornos / arpegios",            0.62f, 0.00f, 0.35f, 0.62f,  70.f, 4.f, 5.f, 35.f, 5 },
            { "Guitarra ritmo (strum)",        0.85f, 0.00f, 0.20f, 0.50f, 120.f, 6.f, 10.f, 70.f, 3 },
            { "Bajo",                          0.90f, 0.00f, 0.15f, 0.45f, 120.f, 8.f, 8.f, 80.f, 2 },
            { "Guira",                         0.82f, 0.05f, 0.20f, 0.70f,  60.f, 3.f, 4.f, 28.f, 3 },
            { "Bongo",                         0.80f, 0.00f, 0.25f, 0.65f,  70.f, 3.f, 4.f, 35.f, 3 },
            { "Conga",                         0.80f, 0.00f, 0.25f, 0.65f,  70.f, 3.f, 4.f, 40.f, 3 },
            { "Tambora (merengue)",            0.85f, 0.10f, 0.20f, 0.60f,  70.f, 3.f, 4.f, 35.f, 3 },
            { "Piano / teclado",               0.75f, 0.00f, 0.30f, 0.50f, 100.f, 8.f, 8.f, 70.f, 3 },
        };
        n = (int) (sizeof (p) / sizeof (p[0]));
        return p;
    }

    inline const char* const* momentNames (int& n)
    {
        static const char* const m[] = { "En vivo (natural)", "Intro", "Verso", "Estribillo", "Mambo / cierre" };
        n = 5; return m;
    }
    inline float momentFactor (int i)
    {
        static const float f[] = { 0.85f, 0.95f, 1.00f, 1.05f, 1.15f };
        return f[juce::jlimit (0, 4, i)];
    }

    inline const char* const* gridNames (int& n)
    {
        static const char* const g[] = { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
        n = 6; return g;
    }
    inline double gridBeats (int i)   // en negras
    {
        static const double b[] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 5, i)];
    }

    struct Params
    {
        double bpm = 120.0;
        int    grid = 3;
        int    moment = 0;
        float  strength = 0.8f, swing = 0.f, humanize = 0.25f, sensitivity = 0.5f;
        float  windowMs = 110.f, fadeMs = 6.f, preMs = 8.f, minGapMs = 55.f;
        double offsetMs = 0.0;      // desplazamiento del origen de la rejilla
        double clipStartSamples = 0.0; // posicion del clip en la linea de tiempo del host
    };

    struct Onset { int src = 0; int dst = 0; float level = 0.f; };

    struct Clip
    {
        std::shared_ptr<juce::AudioBuffer<float>> original, quantized;
        double sampleRate = 44100.0;
        double startSample = 0.0;           // inicio del clip en la linea de tiempo
        std::vector<Onset> onsets;
        double meanDevBeforeMs = 0.0, meanDevAfterMs = 0.0;
        int    version = 0;
    };

    // 1) deteccion de golpes (spectral flux + refinado en el dominio del tiempo)
    std::vector<int> detectOnsets (const juce::AudioBuffer<float>& audio, double sr, float sensitivity, float minGapMs);

    // 2) calcula destinos en la rejilla y renderiza el audio cuantizado SIN estirar (corta y mueve golpes con crossfade)
    void quantize (Clip& clip, const Params& p);

    // distancia media (ms) de los golpes a la rejilla
    double meanDeviationMs (const std::vector<int>& positions, double sr, const Params& p, double clipStart);
}

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <vector>

// ORYMY STROKE: tipografia propia dibujada con trazos vectoriales (no usa ninguna fuente instalada).
// Grid de cada letra: x 0..4, y 0..6 (0 = arriba).
namespace stroke
{
    using Poly = std::vector<float>;
    struct Glyph { std::vector<Poly> lines; };

    inline const Glyph* find (juce::juce_wchar c)
    {
        static const std::map<juce::juce_wchar, Glyph> g = []
        {
            std::map<juce::juce_wchar, Glyph> m;
            m['A'] = {{ {0,6,2,0,4,6}, {0.8f,4,3.2f,4} }};
            m['B'] = {{ {0,6,0,0,3,0,4,1,4,2,3,3,0,3}, {3,3,4,4,4,5,3,6,0,6} }};
            m['C'] = {{ {4,1,3,0,1,0,0,1,0,5,1,6,3,6,4,5} }};
            m['D'] = {{ {0,0,0,6,2.5f,6,4,4.5f,4,1.5f,2.5f,0,0,0} }};
            m['E'] = {{ {4,0,0,0,0,6,4,6}, {0,3,3,3} }};
            m['F'] = {{ {4,0,0,0,0,6}, {0,3,3,3} }};
            m['G'] = {{ {4,1,3,0,1,0,0,1,0,5,1,6,3,6,4,5,4,3,2,3} }};
            m['H'] = {{ {0,0,0,6}, {4,0,4,6}, {0,3,4,3} }};
            m['I'] = {{ {0.5f,0,3.5f,0}, {2,0,2,6}, {0.5f,6,3.5f,6} }};
            m['J'] = {{ {4,0,4,5,3,6,1,6,0,5} }};
            m['K'] = {{ {0,0,0,6}, {4,0,0,3.5f}, {1.4f,2.6f,4,6} }};
            m['L'] = {{ {0,0,0,6,4,6} }};
            m['M'] = {{ {0,6,0,0,2,3.5f,4,0,4,6} }};
            m['N'] = {{ {0,6,0,0,4,6,4,0} }};
            m['O'] = {{ {1,0,3,0,4,1,4,5,3,6,1,6,0,5,0,1,1,0} }};
            m['P'] = {{ {0,6,0,0,3,0,4,1,4,2,3,3,0,3} }};
            m['Q'] = {{ {1,0,3,0,4,1,4,5,3,6,1,6,0,5,0,1,1,0}, {2.5f,4,4,6.3f} }};
            m['R'] = {{ {0,6,0,0,3,0,4,1,4,2,3,3,0,3}, {2,3,4,6} }};
            m['S'] = {{ {4,1,3,0,1,0,0,1,0,2,1,3,3,3,4,4,4,5,3,6,1,6,0,5} }};
            m['T'] = {{ {0,0,4,0}, {2,0,2,6} }};
            m['U'] = {{ {0,0,0,5,1,6,3,6,4,5,4,0} }};
            m['V'] = {{ {0,0,2,6,4,0} }};
            m['W'] = {{ {0,0,1,6,2,2.5f,3,6,4,0} }};
            m['X'] = {{ {0,0,4,6}, {4,0,0,6} }};
            m['Y'] = {{ {0,0,2,3,4,0}, {2,3,2,6} }};
            m['Z'] = {{ {0,0,4,0,0,6,4,6} }};
            m['0'] = {{ {1,0,3,0,4,1,4,5,3,6,1,6,0,5,0,1,1,0}, {1,4.5f,3,1.5f} }};
            m['1'] = {{ {1,1.2f,2,0,2,6}, {0.8f,6,3.2f,6} }};
            m['2'] = {{ {0,1,1,0,3,0,4,1,4,2,0,6,4,6} }};
            m['3'] = {{ {0,0,4,0,2,2.5f,3,2.5f,4,3.5f,4,5,3,6,1,6,0,5} }};
            m['4'] = {{ {3,6,3,0,0,4,4,4} }};
            m['5'] = {{ {4,0,0,0,0,3,3,3,4,4,4,5,3,6,0,6} }};
            m['6'] = {{ {4,0,1,0,0,1,0,5,1,6,3,6,4,5,4,4,3,3,0,3} }};
            m['7'] = {{ {0,0,4,0,1.5f,6} }};
            m['8'] = {{ {1,0,3,0,4,1,4,2,3,3,1,3,0,2,0,1,1,0}, {1,3,3,3,4,4,4,5,3,6,1,6,0,5,0,4,1,3} }};
            m['9'] = {{ {0,6,3,6,4,5,4,1,3,0,1,0,0,1,0,2,1,3,4,3} }};
            m['.'] = {{ {2,5.5f,2,6} }};
            m[','] = {{ {2,5,2,6,1.4f,6.8f} }};
            m[':'] = {{ {2,1.8f,2,2.3f}, {2,4.8f,2,5.3f} }};
            m['-'] = {{ {0.5f,3,3.5f,3} }};
            m['+'] = {{ {0.5f,3,3.5f,3}, {2,1.2f,2,4.8f} }};
            m['/'] = {{ {0,6,4,0} }};
            m['|'] = {{ {2,-0.5f,2,6.5f} }};
            m['%'] = {{ {0,6,4,0}, {0.2f,0.2f,1.3f,0.2f,1.3f,1.5f,0.2f,1.5f,0.2f,0.2f}, {2.7f,4.5f,3.8f,4.5f,3.8f,5.8f,2.7f,5.8f,2.7f,4.5f} }};
            m[(juce::juce_wchar) 0x00B7] = {{ {2,2.8f,2,3.2f} }};   // punto medio
            m[(juce::juce_wchar) 0x2192] = {{ {0,3,4,3}, {2.6f,1.4f,4,3,2.6f,4.6f} }}; // flecha
            m['>'] = {{ {0.5f,1,3.5f,3,0.5f,5} }};
            m['<'] = {{ {3.5f,1,0.5f,3,3.5f,5} }};
            return m;
        }();
        auto it = g.find (c);
        return it == g.end() ? nullptr : &it->second;
    }

    // letra base + marca (acentos)
    inline juce::juce_wchar baseOf (juce::juce_wchar c, int& mark)
    {
        mark = 0;
        switch (c)
        {
            case 0xC1: mark = 1; return 'A';  case 0xC9: mark = 1; return 'E';
            case 0xCD: mark = 1; return 'I';  case 0xD3: mark = 1; return 'O';
            case 0xDA: mark = 1; return 'U';  case 0xD1: mark = 2; return 'N';
            case 0xDC: mark = 3; return 'U';
            default: return c;
        }
    }

    inline float advance (float s) { return 5.6f * s; }

    inline float textWidth (const juce::String& text, float height, float tracking = 0.0f)
    {
        const float s = height / 6.0f;
        float w = 0.0f;
        for (auto c : text.toUpperCase())
            w += (c == ' ' ? 3.0f * s : advance (s)) + tracking;
        return juce::jmax (0.0f, w - 1.6f * s - tracking);
    }

    inline void draw (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                      float height, juce::Justification just, float tracking = 0.0f, float thickness = 0.0f)
    {
        const auto up = text.toUpperCase();
        const float s = height / 6.0f;
        const float w = textWidth (up, height, tracking);
        float x = area.getX();
        if (just.testFlags (juce::Justification::horizontallyCentred))      x = area.getCentreX() - w * 0.5f;
        else if (just.testFlags (juce::Justification::right))              x = area.getRight() - w;
        float y = area.getY();
        if (just.testFlags (juce::Justification::verticallyCentred))        y = area.getCentreY() - height * 0.5f;
        else if (just.testFlags (juce::Justification::bottom))              y = area.getBottom() - height;

        const float th = thickness > 0.0f ? thickness : juce::jmax (1.0f, s * 0.62f);
        const juce::PathStrokeType st (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        for (auto ch : up)
        {
            if (ch == ' ') { x += 3.0f * s + tracking; continue; }
            int mark = 0;
            auto base = baseOf (ch, mark);
            if (auto* gl = find (base))
            {
                for (auto& pl : gl->lines)
                {
                    juce::Path p;
                    for (size_t i = 0; i + 1 < pl.size(); i += 2)
                    {
                        const float px = x + pl[i] * s, py = y + pl[i + 1] * s;
                        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
                    }
                    if (pl.size() == 4 && pl[0] == pl[2] && pl[1] == pl[3]) p.lineTo (x + pl[0] * s + 0.01f, y + pl[1] * s);
                    g.strokePath (p.createPathWithRoundedCorners (s * 0.5f), st);
                }
                juce::Path m;
                if (mark == 1)      { m.startNewSubPath (x + 2.0f * s, y - 1.0f * s); m.lineTo (x + 3.0f * s, y - 2.0f * s); }
                else if (mark == 2) { m.startNewSubPath (x + 0.8f * s, y - 1.0f * s); m.lineTo (x + 1.8f * s, y - 1.8f * s);
                                      m.lineTo (x + 2.8f * s, y - 1.0f * s); m.lineTo (x + 3.8f * s, y - 1.8f * s); }
                else if (mark == 3) { m.startNewSubPath (x + 1.0f * s, y - 1.4f * s); m.lineTo (x + 1.0f * s, y - 1.2f * s);
                                      m.startNewSubPath (x + 3.0f * s, y - 1.4f * s); m.lineTo (x + 3.0f * s, y - 1.2f * s); }
                if (mark != 0) g.strokePath (m, st);
            }
            x += advance (s) + tracking;
        }
    }

    inline void draw (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                      float height, juce::Justification just, float tracking = 0.0f, float thickness = 0.0f)
    {
        draw (g, text, area.toFloat(), height, just, tracking, thickness);
    }
}

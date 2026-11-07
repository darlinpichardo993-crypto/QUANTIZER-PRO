#pragma once
#include "PluginProcessor.h"
#include "StrokeFont.h"
#include <BinaryData.h>

namespace ui
{
    using namespace juce;
    inline String U (const char* s) { return String::fromUTF8 (s); }

    inline void T (Graphics& g, const String& t, Rectangle<float> r, float h, Justification j,
                   float trk = 0.f, float th = 0.f)
    {
        const float w = stroke::textWidth (t, h, trk);
        if (w > r.getWidth() && w > 0.f) { const float k = r.getWidth() / w; h *= k; trk *= k; th *= k; }
        stroke::draw (g, t, r, h, j, trk, th);
    }

    inline Colour white() { return Colour (0xfff4f4f4); }

    // ---------------------------------------------------------------- iconos vectoriales
    inline void drawIcon (Graphics& g, int icon, Rectangle<float> r, Colour c)
    {
        g.setColour (c);
        const auto cen = r.getCentre(); const float s = jmin (r.getWidth(), r.getHeight());
        const PathStrokeType st (jmax (1.5f, s * 0.09f), PathStrokeType::curved, PathStrokeType::rounded);
        Path p;
        switch (icon)
        {
            case 1: // lupa
                p.addEllipse (cen.x - s * 0.34f, cen.y - s * 0.36f, s * 0.54f, s * 0.54f);
                p.startNewSubPath (cen.x + s * 0.1f, cen.y + s * 0.1f); p.lineTo (cen.x + s * 0.38f, cen.y + s * 0.38f);
                g.strokePath (p, st); break;
            case 2: // onda
            {
                static const float h[] = { 0.25f, 0.55f, 0.9f, 0.5f, 1.0f, 0.5f, 0.9f, 0.55f, 0.25f };
                for (int i = 0; i < 9; ++i)
                {
                    const float x = cen.x + (i - 4) * s * 0.11f, hh = h[i] * s * 0.5f;
                    p.startNewSubPath (x, cen.y - hh); p.lineTo (x, cen.y + hh);
                }
                g.strokePath (p, PathStrokeType (jmax (1.5f, s * 0.05f), PathStrokeType::curved, PathStrokeType::rounded)); break;
            }
            case 3: // carpeta
                p.startNewSubPath (cen.x - s * 0.36f, cen.y - s * 0.22f); p.lineTo (cen.x - s * 0.36f, cen.y + s * 0.3f);
                p.lineTo (cen.x + s * 0.36f, cen.y + s * 0.3f); p.lineTo (cen.x + s * 0.36f, cen.y - s * 0.14f);
                p.lineTo (cen.x - s * 0.02f, cen.y - s * 0.14f); p.lineTo (cen.x - s * 0.1f, cen.y - s * 0.28f);
                p.lineTo (cen.x - s * 0.36f, cen.y - s * 0.28f);
                g.strokePath (p, st); break;
            case 4: // exportar (flecha abajo + base)
                p.startNewSubPath (cen.x, cen.y - s * 0.34f); p.lineTo (cen.x, cen.y + s * 0.16f);
                p.startNewSubPath (cen.x - s * 0.2f, cen.y - s * 0.04f); p.lineTo (cen.x, cen.y + s * 0.18f); p.lineTo (cen.x + s * 0.2f, cen.y - s * 0.04f);
                p.startNewSubPath (cen.x - s * 0.32f, cen.y + s * 0.34f); p.lineTo (cen.x + s * 0.32f, cen.y + s * 0.34f);
                g.strokePath (p, st); break;
            case 5: for (int i = -1; i <= 1; ++i) g.fillEllipse (cen.x + i * s * 0.22f - s * 0.06f, cen.y - s * 0.06f, s * 0.12f, s * 0.12f); break;
            case 6: g.setColour (Colour (0xffe53935)); g.fillEllipse (cen.x - s * 0.2f, cen.y - s * 0.2f, s * 0.4f, s * 0.4f); break;
            case 7: g.fillRect (cen.x - s * 0.18f, cen.y - s * 0.18f, s * 0.36f, s * 0.36f); break;
            default: break;
        }
    }

    // ---------------------------------------------------------------- look and feel propio
    struct LAF : LookAndFeel_V4
    {
        void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider&) override
        {
            auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.f);
            const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f; const auto c = b.getCentre();
            const float ang = start + pos * (end - start);
            Path track; track.addCentredArc (c.x, c.y, r, r, 0.f, start, end, true);
            g.setColour (Colour (0xff2c2c2c)); g.strokePath (track, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
            Path val; val.addCentredArc (c.x, c.y, r, r, 0.f, start, ang, true);
            g.setColour (white()); g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
            const float rb = r * 0.76f;
            g.setGradientFill (ColourGradient (Colour (0xff484848), c.x - rb * 0.4f, c.y - rb * 0.55f, Colour (0xff070707), c.x + rb * 0.6f, c.y + rb * 0.8f, true));
            g.fillEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f);
            g.setColour (Colour (0xff5a5a5a)); g.drawEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f, 1.2f);
            const Point<float> d (std::sin (ang), -std::cos (ang));
            g.setColour (white());
            g.drawLine (c.x + d.x * rb * 0.35f, c.y + d.y * rb * 0.35f, c.x + d.x * rb * 0.92f, c.y + d.y * rb * 0.92f, 3.f);
        }

        void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box) override
        {
            auto b = Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
            g.setColour (box.findColour (ComboBox::backgroundColourId)); g.fillRoundedRectangle (b, 7.f);
            g.setColour (box.findColour (ComboBox::outlineColourId));    g.drawRoundedRectangle (b, 7.f, 1.2f);
            Path a; const float cx = (float) w - 24.f, cy = (float) h * 0.5f;
            a.startNewSubPath (cx - 7.f, cy - 3.f); a.lineTo (cx, cy + 4.f); a.lineTo (cx + 7.f, cy - 3.f);
            g.setColour (box.findColour (ComboBox::arrowColourId));
            g.strokePath (a, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        void positionComboBoxText (ComboBox& box, Label& l) override
        {
            if ((bool) box.getProperties()["dark"]) l.setBounds (14, (int) (box.getHeight() * 0.40f), box.getWidth() - 50, (int) (box.getHeight() * 0.52f));
            else l.setBounds (16, 1, box.getWidth() - 56, box.getHeight() - 2);
        }

        void drawLabel (Graphics& g, Label& l) override
        {
            Colour c = l.findColour (Label::textColourId);
            if (auto* cb = dynamic_cast<ComboBox*> (l.getParentComponent())) c = cb->findColour (ComboBox::textColourId);
            g.setColour (c);
            T (g, l.getText(), l.getLocalBounds().toFloat(), 15.f, Justification::centredLeft, 1.f);
        }

        void drawPopupMenuBackground (Graphics& g, int w, int h) override
        {
            g.fillAll (Colour (0xff0b0b0b)); g.setColour (Colour (0xff555555)); g.drawRect (0, 0, w, h, 1);
        }
        void getIdealPopupMenuItemSize (const String& text, bool sep, int, int& iw, int& ih) override
        {
            if (sep) { iw = 20; ih = 8; return; }
            iw = (int) stroke::textWidth (text, 14.f, 1.f) + 44; ih = 36;
        }
        void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, bool sep, bool active, bool hi, bool ticked,
                                bool, const String& text, const String&, const Drawable*, const Colour*) override
        {
            if (sep) { g.setColour (Colour (0xff333333)); g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY())); return; }
            if (hi && active) { g.setColour (white()); g.fillRect (area.reduced (2, 1)); }
            g.setColour (hi && active ? Colours::black : (active ? white() : Colour (0xff666666)));
            T (g, text, area.reduced (16, 0).toFloat(), 14.f, Justification::centredLeft, 1.f);
            if (ticked) g.fillEllipse ((float) area.getRight() - 20.f, (float) area.getCentreY() - 3.f, 6.f, 6.f);
        }

        void drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool) override
        {
            auto r = b.getLocalBounds().toFloat();
            g.setColour (white()); T (g, b.getButtonText(), r.withTrimmedLeft (20).withTrimmedRight (90), 17.f, Justification::centredLeft, 1.f);
            auto sw = Rectangle<float> (62.f, 34.f).withCentre ({ r.getRight() - 52.f, r.getCentreY() });
            const bool on = b.getToggleState();
            g.setColour (on ? white() : Colour (over ? 0xff3a3a3a : 0xff2a2a2a)); g.fillRoundedRectangle (sw, 17.f);
            g.setColour (Colour (0xff6a6a6a)); g.drawRoundedRectangle (sw, 17.f, 1.2f);
            const float kx = on ? sw.getRight() - 17.f : sw.getX() + 17.f;
            g.setColour (on ? Colour (0xff111111) : white()); g.fillEllipse (kx - 12.f, sw.getCentreY() - 12.f, 24.f, 24.f);
        }
    };

    // ---------------------------------------------------------------- boton propio
    struct PillButton : Button
    {
        enum Style { White, Dark, Icon };
        Style style; int icon; bool iconAbove = false; float textH = 20.f;
        PillButton (const String& t, Style s, int ic = 0) : Button (t), style (s), icon (ic) {}
        void paintButton (Graphics& g, bool over, bool down) override
        {
            auto b = getLocalBounds().toFloat().reduced (1.f);
            const bool on = getToggleState();
            Colour fg;
            if (style == White)
            {
                g.setGradientFill (ColourGradient (Colour (down ? 0xffc9c9c9 : 0xfffbfbfb), 0.f, b.getY(), Colour (down ? 0xffa8a8a8 : 0xffd2d2d2), 0.f, b.getBottom(), false));
                g.fillRoundedRectangle (b, 9.f); fg = Colour (0xff0a0a0a);
            }
            else
            {
                g.setColour (on ? white() : Colour (down ? 0xff2a2a2a : (over ? 0xff1d1d1d : 0xff0f0f0f))); g.fillRoundedRectangle (b, 9.f);
                g.setColour (Colour (0xff484848)); g.drawRoundedRectangle (b, 9.f, 1.3f);
                fg = on ? Colour (0xff0a0a0a) : white();
            }
            const String t = getButtonText();
            if (t.isEmpty()) { drawIcon (g, icon, b.withSizeKeepingCentre (b.getHeight() * 0.62f, b.getHeight() * 0.62f), fg); return; }
            g.setColour (fg);
            if (icon != 0 && iconAbove)
            {
                drawIcon (g, icon, Rectangle<float> (b.getX(), b.getY() + b.getHeight() * 0.1f, b.getWidth(), b.getHeight() * 0.42f), fg);
                g.setColour (fg); T (g, t, b.withTrimmedTop (b.getHeight() * 0.58f).reduced (8, 0), textH, Justification::centredTop, 1.5f);
            }
            else if (icon != 0)
            {
                const float ih = b.getHeight() * 0.5f;
                const float tw = jmin (stroke::textWidth (t, textH, 1.5f), b.getWidth() - ih * 2.f);
                const float total = ih + 14.f + tw; const float x0 = b.getCentreX() - total * 0.5f;
                drawIcon (g, icon, Rectangle<float> (x0, b.getCentreY() - ih * 0.5f, ih, ih), fg);
                g.setColour (fg); T (g, t, Rectangle<float> (x0 + ih + 14.f, b.getY(), tw + 2.f, b.getHeight()), textH, Justification::centredLeft, 1.5f);
            }
            else T (g, t, b.reduced (8, 0), textH, Justification::centred, 1.5f);
        }
    };

    // ---------------------------------------------------------------- perilla con nombre y valor
    struct Knob : Component
    {
        Slider s; std::unique_ptr<SliderParameterAttachment> att; String name; bool below;
        Knob (RangedAudioParameter& p, const String& n, const String& suffix, int dec, bool labelBelow)
            : name (n), below (labelBelow)
        {
            s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
            addAndMakeVisible (s);
            att = std::make_unique<SliderParameterAttachment> (p, s, nullptr);
            s.setTextValueSuffix (suffix); s.setNumDecimalPlacesToDisplay (dec);
            s.setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
            s.onValueChange = [this] { repaint(); };
        }
        void resized() override { s.setBounds (getWidth() / 2 - 48, below ? 0 : 22, 96, 96); }
        void paint (Graphics& g) override
        {
            g.setColour (white());
            const auto w = (float) getWidth();
            if (below) { T (g, name, { 0.f, 108.f, w, 16.f }, 14.f, Justification::centred, 1.f); T (g, s.getTextFromValue (s.getValue()), { 0.f, 136.f, w, 16.f }, 15.f, Justification::centred, 1.f); }
            else       { T (g, name, { 0.f, 2.f, w, 16.f }, 14.f, Justification::centred, 1.f);  T (g, s.getTextFromValue (s.getValue()), { 0.f, 140.f, w, 16.f }, 15.f, Justification::centred, 1.f); }
        }
    };

    // numero arrastrable (tempo, offset)
    struct DragNumber : Component
    {
        RangedAudioParameter& p; String suffix; int dec; float h; Justification just; double sens; String title;
        DragNumber (RangedAudioParameter& pp, const String& suf, int d, float height, Justification j, double sensitivity, const String& t)
            : p (pp), suffix (suf), dec (d), h (height), just (j), sens (sensitivity), title (t) { setMouseCursor (MouseCursor::UpDownResizeCursor); }
        double get() const { return (double) p.convertFrom0to1 (p.getValue()); }
        void set (double v) { p.setValueNotifyingHost (p.convertTo0to1 ((float) v)); repaint(); }
        void paint (Graphics& g) override { g.setColour (white()); T (g, String (get(), dec) + suffix, getLocalBounds().toFloat(), h, just, 1.f); }
        void mouseDown (const MouseEvent&) override { p.beginChangeGesture(); lastY = 0; }
        void mouseUp (const MouseEvent&) override { p.endChangeGesture(); }
        void mouseDrag (const MouseEvent& e) override
        {
            const int dy = e.getDistanceFromDragStartY(); const double m = e.mods.isShiftDown() ? 0.1 : 1.0;
            set (get() - (double) (dy - lastY) * sens * m); lastY = dy;
        }
        void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override { set (get() + w.deltaY * sens * 10.0); }
        void mouseDoubleClick (const MouseEvent&) override
        {
            auto* w = new AlertWindow (title, "Escribe el valor:", MessageBoxIconType::NoIcon);
            w->addTextEditor ("v", String (get(), dec)); w->addButton ("OK", 1); w->addButton ("Cancelar", 0);
            w->enterModalState (true, ModalCallbackFunction::create ([this, w] (int r)
            { if (r == 1) { p.beginChangeGesture(); set (w->getTextEditorContents ("v").getDoubleValue()); p.endChangeGesture(); } delete w; }), false);
        }
        int lastY = 0;
    };

    struct ComboUser : ComboBox
    {
        bool userOpen = false;
        void showPopup() override { userOpen = true; ComboBox::showPopup(); }
    };
}

// ===================================================================== editor
class QuantizerEditor : public juce::AudioProcessorEditor,
                        private juce::ChangeListener,
                        private juce::Timer,
                        public juce::FileDragAndDropTarget
{
public:
    explicit QuantizerEditor (QuantizerProcessor&);
    ~QuantizerEditor() override;

    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

    struct Content;
    struct Strip;
    struct Readouts;
    struct Meter;
    struct BigButton;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaintLive(); }
    void timerCallback() override;
    void repaintLive();
    void applyPreset (int idx);
    void setParam (const char* id, float v);
    void doQuantize();
    void doAnalyze();
    void showMenu();
    bool startDragExport (juce::Component* source);

    QuantizerProcessor& proc;
    ui::LAF laf;
    std::unique_ptr<Content> content;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::File> dragExportFiles;
    int lastStamp = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuantizerEditor)
};

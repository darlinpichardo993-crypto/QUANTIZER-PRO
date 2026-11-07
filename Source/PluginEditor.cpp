#include "PluginEditor.h"

using namespace juce;
using ui::T; using ui::U; using ui::white;
using APVTS = AudioProcessorValueTreeState;

// Botón de exportación: clic = exportación manual; arrastrar = enviar WAV al DAW.
struct ExportDragButton : ui::PillButton
{
    using ui::PillButton::PillButton;
    std::function<void()> onDragExport;
    bool dragStarted = false;

    void mouseDown (const MouseEvent& e) override
    {
        dragStarted = false;
        ui::PillButton::mouseDown (e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        ui::PillButton::mouseDrag (e);
        if (! dragStarted && (std::abs (e.getDistanceFromDragStartX()) + std::abs (e.getDistanceFromDragStartY()) > 8))
        {
            dragStarted = true;
            if (onDragExport) onDragExport();
        }
    }
};

static constexpr float DW = 1672.f, DH = 1100.f;

// ===================================================================== Strip (rejilla + onda)
struct QuantizerEditor::Strip : Component
{
    QuantizerProcessor& proc;
    double viewStart = 0.0, viewSec = 8.0;
    float anim = 1.f; double animStart = 0.0;

    explicit Strip (QuantizerProcessor& p) : proc (p) {}

    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        const double sr = proc.sampleRateNow();
        if (e.mods.isCtrlDown() || e.mods.isCommandDown()) viewSec = jlimit (0.5, 60.0, viewSec * (w.deltaY > 0 ? 0.85 : 1.18));
        else viewStart -= (double) w.deltaY * viewSec * sr * 0.5;
        repaint();
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.f);
        g.setColour (Colour (0xff090909)); g.fillRoundedRectangle (b, 12.f);
        g.setColour (Colour (0xff2b2b2b)); g.drawRoundedRectangle (b, 12.f, 1.4f);
        g.setColour (white());
        T (g, U ("REJILLA DE CUANTIZACI\xc3\x93N"), { 30.f, 14.f, 300.f, 16.f }, 15.f, Justification::centredLeft, 2.f);

        const double sr = proc.sampleRateNow();
        auto clip = proc.getClip();
        String status;
        if (proc.busy.load()) status = "PROCESANDO...";
        else if (proc.capturing.load()) status = "CAPTURANDO " + String ((double) proc.capturedSamples.load() / sr, 1) + " S";
        else if (clip && clip->original) status = String ((int) clip->onsets.size()) + " GOLPES" + (clip->quantized ? U (" \xc2\xb7 CUANTIZADO") : String());
        g.setColour (Colour (0xff9a9a9a));
        T (g, status, { 350.f, 14.f, 500.f, 16.f }, 13.f, Justification::centredLeft, 1.5f);
        g.setColour (Colour (0xff8a8a8a)); T (g, "OFFSET", { (float) getWidth() - 330.f, 14.f, 90.f, 16.f }, 12.f, Justification::centredRight, 1.5f);

        auto inner = Rectangle<float> (24.f, 44.f, (float) getWidth() - 48.f, (float) getHeight() - 58.f);
        g.setColour (Colour (0xff050505)); g.fillRoundedRectangle (inner, 8.f);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (inner.toNearestInt());

        if (! clip || ! clip->original)
        {
            g.setColour (Colour (0xff7a7a7a));
            T (g, proc.capturing.load() ? U ("REPRODUCE LA CANCI\xc3\x93N EN TU DAW: EL PLUGIN GRABA LO QUE ENTRA")
                                        : U ("SUELTA AQU\xc3\x8d TU AUDIO (WAV \xc2\xb7 MP3 \xc2\xb7 AIFF) O PULSA CAPTURAR EN VIVO"),
               inner.reduced (20.f, 0.f), 17.f, Justification::centred, 2.f);
            return;
        }

        const double viewLen = viewSec * sr;
        const int total = clip->original->getNumSamples();
        if (proc.hostPlaying.load())
        {
            const double pos = proc.hostPosSamples.load() - clip->startSample;
            if (pos < viewStart || pos > viewStart + viewLen * 0.92) viewStart = pos - viewLen * 0.08;
        }
        viewStart = jlimit (0.0, jmax (0.0, (double) total - viewLen), viewStart);
        const float W = inner.getWidth(), L = inner.getX();
        auto toX = [&] (double s) { return L + (float) ((s - viewStart) / viewLen) * W; };

        // lineas de la rejilla
        const auto p = proc.readParams (clip.get());
        const double beat = 60.0 / p.bpm * sr, step = beat * qp::gridBeats (p.grid);
        const double origin = p.offsetMs * sr / 1000.0;
        const bool trip = (p.grid == 2 || p.grid == 4);
        const double swingShift = trip ? 0.0 : (double) p.swing * step / 3.0;
        const float stepPx = (float) (step / viewLen) * W;
        long long k = (long long) std::floor ((clip->startSample + viewStart - origin) / step) - 1;
        for (int guard = 0; guard < 4000; ++guard, ++k)
        {
            const double t = origin + (double) k * step + ((k & 1) ? swingShift : 0.0);
            const float x = toX (t - clip->startSample);
            if (x > inner.getRight()) break;
            if (x < L) continue;
            const double rel = (double) k * step / beat;
            const bool isBeat = std::abs (rel - std::round (rel)) < 1e-3;
            const bool isBar = isBeat && (((long long) std::llround (rel)) % 4 == 0);
            if (! isBeat && stepPx < 6.f) continue;
            g.setColour (Colour (isBar ? 0x70ffffff : (isBeat ? 0x3cffffff : 0x1affffff)));
            g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
        }

        // ondas
        const float cy = inner.getCentreY(), half = inner.getHeight() * 0.42f;
        auto wave = [&] (const AudioBuffer<float>& buf, Colour col)
        {
            const float* d = buf.getReadPointer (0); const int N = buf.getNumSamples();
            g.setColour (col);
            const int cols = (int) W;
            for (int c = 0; c < cols; ++c)
            {
                const int s0 = (int) (viewStart + (double) c * viewLen / cols), s1 = jmin (N, (int) (viewStart + (double) (c + 1) * viewLen / cols));
                if (s0 >= N) break;
                const int stride = jmax (1, (s1 - s0) / 24);
                float mn = 0.f, mx = 0.f;
                for (int i = s0; i < s1; i += stride) { mn = jmin (mn, d[i]); mx = jmax (mx, d[i]); }
                g.fillRect (L + (float) c, cy - mx * half, 1.f, jmax (1.f, (mx - mn) * half));
            }
        };
        const float e = anim < 1.f ? 1.f - std::pow (1.f - anim, 3.f) : 1.f;
        wave (*clip->original, Colour (clip->quantized ? 0xff4a4a4a : 0xffd9d9d9));
        if (clip->quantized) wave (*clip->quantized, Colour::fromFloatRGBA (0.93f, 0.93f, 0.93f, 0.25f + 0.75f * e));

        // marcas de golpes: antes (gris) -> despues (blanco, sobre la rejilla)
        for (auto& o : clip->onsets)
        {
            const float x0 = toX (o.src), x1 = toX (o.dst);
            if (x0 < L - 40.f && x1 < L - 40.f) continue;
            if (x0 > inner.getRight() + 40.f && x1 > inner.getRight() + 40.f) continue;
            if (! clip->quantized)
            {
                g.setColour (Colour (0xaaffffff)); g.drawVerticalLine ((int) x0, inner.getY() + 4.f, inner.getBottom() - 4.f);
                continue;
            }
            const float xm = x0 + (x1 - x0) * e;
            if (e < 1.f) { g.setColour (Colour (0x66ff5a4a)); g.drawVerticalLine ((int) x0, inner.getY() + 6.f, inner.getBottom() - 6.f); }
            g.setColour (Colour (0x30ffffff)); g.fillRect (xm - 2.f, inner.getY(), 4.f, inner.getHeight());
            g.setColour (white()); g.drawVerticalLine ((int) xm, inner.getY(), inner.getBottom());
            Path d; d.addTriangle (xm - 5.f, inner.getY(), xm + 5.f, inner.getY(), xm, inner.getY() + 9.f);
            g.fillPath (d);
        }
        if (anim < 1.f)      // barrido de analisis
        {
            const float sx = L + W * anim;
            g.setGradientFill (ColourGradient (Colour (0x00ffffff), sx - 90.f, 0.f, Colour (0x55ffffff), sx, 0.f, false));
            g.fillRect (sx - 90.f, inner.getY(), 90.f, inner.getHeight());
            g.setColour (white()); g.drawVerticalLine ((int) sx, inner.getY(), inner.getBottom());
        }
        if (proc.hostPlaying.load())
        {
            const float px = toX (proc.hostPosSamples.load() - clip->startSample);
            g.setColour (Colour (0xffffcc00)); g.drawVerticalLine ((int) px, inner.getY(), inner.getBottom());
        }
    }
};

// ===================================================================== lecturas
struct QuantizerEditor::Readouts : Component
{
    QuantizerProcessor& proc;
    explicit Readouts (QuantizerProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (Graphics& g) override
    {
        auto clip = proc.getClip();
        const Colour dim (0xff8c8c8c);
        g.setColour (dim);
        T (g, "NOTAS DETECTADAS", { 0.f, 4.f, 230.f, 12.f }, 10.f, Justification::centredLeft, 1.5f);
        T (g, "TEMPO (BPM)",      { 0.f, 62.f, 230.f, 12.f }, 10.f, Justification::centredLeft, 1.5f);
        T (g, U ("DESV\xc3\x8dO MEDIO"), { 0.f, 120.f, 230.f, 12.f }, 10.f, Justification::centredLeft, 1.5f);
        g.setColour (white());
        String n = "--", dev = "--";
        if (clip && clip->original) n = String ((int) clip->onsets.size());
        if (clip && ! clip->onsets.empty())
        {
            dev = String (clip->meanDevBeforeMs, 1) + " MS";
            if (clip->quantized) dev += U (" \xe2\x86\x92 ") + String (clip->meanDevAfterMs, 1) + " MS";
        }
        T (g, n,   { 0.f, 24.f, 230.f, 22.f }, 19.f, Justification::centredLeft, 1.f);
        T (g, dev, { 0.f, 140.f, 230.f, 22.f }, 19.f, Justification::centredLeft, 1.f);
    }
};

// ===================================================================== vumetro
struct QuantizerEditor::Meter : Component
{
    QuantizerProcessor& proc;
    explicit Meter (QuantizerProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (Graphics& g) override
    {
        g.setColour (Colour (0xff0a0a0a)); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 8.f);
        g.setColour (Colour (0xff2b2b2b)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 8.f, 1.2f);
        const float lv[2] = { proc.peakL.load(), proc.peakR.load() };
        for (int c = 0; c < 2; ++c)
        {
            const float db = Decibels::gainToDecibels (lv[c], -60.f);
            const float fr = jlimit (0.f, 1.f, (db + 36.f) / 36.f);
            for (int i = 0; i < 28; ++i)
            {
                const float y = 14.f + (27 - i) * 5.4f;
                const bool lit = ((float) i + 0.5f) / 28.f <= fr;
                g.setColour (lit ? (i >= 26 ? Colour (0xffffcc00) : white()) : Colour (0xff262626));
                g.fillRect (18.f + c * 22.f, y, 16.f, 3.8f);
            }
        }
        static const int marks[] = { 0, -6, -12, -18, -24, -30, -36 };
        g.setColour (Colour (0xff9a9a9a));
        for (int m : marks)
        {
            const float y = 14.f + (1.f - (float) (m + 36) / 36.f) * 27.f * 5.4f + 2.f;
            g.drawHorizontalLine ((int) y, 62.f, 70.f);
            T (g, String (m), { 74.f, y - 6.f, 48.f, 12.f }, 9.f, Justification::centredLeft, 0.5f);
        }
    }
};

// ===================================================================== boton central
struct QuantizerEditor::BigButton : Component
{
    QuantizerProcessor& proc; std::function<void()> onClick; float phase = 0.f; bool hover = false;
    explicit BigButton (QuantizerProcessor& p) : proc (p) { setMouseCursor (MouseCursor::PointingHandCursor); }
    void mouseEnter (const MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const MouseEvent&) override { hover = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (e.mouseWasClicked() && contains (e.getPosition()) && onClick) onClick(); }
    bool hitTest (int x, int y) override { return Point<float> ((float) x, (float) y).getDistanceFrom ({ 120.f, 120.f }) < 112.f; }
    void paint (Graphics& g) override
    {
        const Point<float> c (120.f, 120.f);
        g.setGradientFill (ColourGradient (Colour (hover ? 0x40ffffff : 0x28ffffff), c.x, c.y, Colour (0x00ffffff), c.x, c.y - 120.f, true));
        g.fillEllipse (0.f, 0.f, 240.f, 240.f);
        const float R = 100.f;
        g.setGradientFill (ColourGradient (Colour (0xff3c3c3c), c.x - 40.f, c.y - 55.f, Colour (0xff050505), c.x + 50.f, c.y + 80.f, true));
        g.fillEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f);
        g.setColour (Colour (0xff6a6a6a)); g.drawEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f, 1.5f);
        Path ring;
        if (proc.busy.load()) ring.addCentredArc (c.x, c.y, 108.f, 108.f, 0.f, phase, phase + 2.1f, true);
        else ring.addCentredArc (c.x, c.y, 108.f, 108.f, 0.f, MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        g.setColour (white()); g.strokePath (ring, PathStrokeType (7.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.drawLine (c.x, c.y - 70.f, c.x, c.y - 38.f, 4.f);
    }
};

// ===================================================================== contenido (espacio de diseno 1672 x 1100)
struct QuantizerEditor::Content : Component
{
    QuantizerEditor& ed; QuantizerProcessor& proc;
    Image centerImg, logoImg, signImg, iconImg;

    ui::ComboUser presetBox, momentBox, gridBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> aPreset, aMoment, aGrid;
    ui::PillButton folderBtn { "", ui::PillButton::Icon, 3 }, dotsBtn { "", ui::PillButton::Icon, 5 };
    ExportDragButton saveBtn { "", ui::PillButton::Icon, 4 };
    ui::PillButton analyzeBtn { "ANALIZAR", ui::PillButton::White, 1 }, recBtn { "CAPTURAR EN VIVO", ui::PillButton::Dark, 6 };
    ui::PillButton quantizeBtn { "CUANTIZAR", ui::PillButton::White, 2 }, hostBtn { "HOST", ui::PillButton::Dark, 0 };
    ToggleButton monoBtn { "MONO" }, bypassBtn { "BYPASS" };
    std::unique_ptr<APVTS::ButtonAttachment> aMono, aBypass, aHost;
    std::unique_ptr<ui::Knob> kStrength, kSwing, kHuman, kSens, kWin, kFade, kPre, kMix, kGain;
    std::unique_ptr<ui::DragNumber> tempo, offset;
    Strip strip; Readouts readouts; Meter meter; BigButton big;

    static void setupCombo (ui::ComboUser& b, AudioProcessor& ap, const char* id, bool dark)
    {
        b.getProperties().set ("dark", dark);
        b.setColour (ComboBox::backgroundColourId, dark ? Colour (0xff101010) : Colour (0xfff4f4f4));
        b.setColour (ComboBox::outlineColourId, dark ? Colour (0xff484848) : Colour (0xffcfcfcf));
        b.setColour (ComboBox::textColourId, dark ? white() : Colour (0xff0a0a0a));
        b.setColour (ComboBox::arrowColourId, dark ? white() : Colour (0xff0a0a0a));
        (void) id; (void) ap;
    }

    Content (QuantizerEditor& e, QuantizerProcessor& p)
        : ed (e), proc (p), strip (p), readouts (p), meter (p), big (p)
    {
        centerImg = ImageCache::getFromMemory (BinaryData::center_png, BinaryData::center_pngSize);
        logoImg   = ImageCache::getFromMemory (BinaryData::logo_png,   BinaryData::logo_pngSize);
        signImg   = ImageCache::getFromMemory (BinaryData::sign_png,   BinaryData::sign_pngSize);
        iconImg   = ImageCache::getFromMemory (BinaryData::icon_png,   BinaryData::icon_pngSize);

        auto& ap = proc.apvts;
        auto fill = [&] (ComboBox& cb, const char* id, bool dark)
        {
            setupCombo (static_cast<ui::ComboUser&> (cb), proc, id, dark);
            if (auto* pc = dynamic_cast<AudioParameterChoice*> (ap.getParameter (id))) cb.addItemList (pc->choices, 1);
            addAndMakeVisible (cb);
        };
        fill (presetBox, "preset", false); fill (momentBox, "moment", true); fill (gridBox, "grid", true);
        aPreset = std::make_unique<APVTS::ComboBoxAttachment> (ap, "preset", presetBox);
        aMoment = std::make_unique<APVTS::ComboBoxAttachment> (ap, "moment", momentBox);
        aGrid   = std::make_unique<APVTS::ComboBoxAttachment> (ap, "grid", gridBox);
        presetBox.onChange = [this] { if (presetBox.userOpen) { presetBox.userOpen = false; ed.applyPreset (presetBox.getSelectedItemIndex()); } };

        for (Component* c : std::initializer_list<Component*> { &folderBtn, &saveBtn, &dotsBtn, &analyzeBtn, &recBtn, &quantizeBtn, &hostBtn, &monoBtn, &bypassBtn, &strip, &readouts, &meter, &big })
            addAndMakeVisible (*c);
        analyzeBtn.textH = 21.f; quantizeBtn.textH = 25.f; quantizeBtn.iconAbove = true; recBtn.textH = 14.f; hostBtn.textH = 13.f;
        hostBtn.setClickingTogglesState (true);
        aHost   = std::make_unique<APVTS::ButtonAttachment> (ap, "hostsync", hostBtn);
        aMono   = std::make_unique<APVTS::ButtonAttachment> (ap, "mono", monoBtn);
        aBypass = std::make_unique<APVTS::ButtonAttachment> (ap, "bypass", bypassBtn);

        auto knob = [&] (std::unique_ptr<ui::Knob>& k, const char* id, const char* name, const char* suf, int dec, bool below)
        { k = std::make_unique<ui::Knob> (*ap.getParameter (id), name, suf, dec, below); addAndMakeVisible (*k); };
        knob (kStrength, "strength", "FUERZA", "%", 0, false);
        knob (kSwing, "swing", "SWING", "%", 0, false);
        knob (kHuman, "humanize", "HUMANIZAR", "%", 0, false);
        knob (kSens, "sens", "SENSIBILIDAD", "%", 0, false);
        knob (kWin, "window", "VENTANA", " MS", 0, true);
        knob (kFade, "fade", "FADE", " MS", 1, true);
        knob (kPre, "pre", "PRE-ATAQUE", " MS", 1, true);
        knob (kMix, "mix", "MEZCLA", "%", 0, true);
        knob (kGain, "gain", "GANANCIA", " DB", 1, true);

        tempo  = std::make_unique<ui::DragNumber> (*ap.getParameter ("bpm"), "", 2, 19.f, Justification::centredLeft, 0.05, "Tempo (BPM)");
        offset = std::make_unique<ui::DragNumber> (*ap.getParameter ("offset"), " MS", 1, 15.f, Justification::centredLeft, 0.2, "Offset (ms)");
        addAndMakeVisible (*tempo); addAndMakeVisible (*offset);

        big.onClick = [this] { ed.doQuantize(); };
        quantizeBtn.onClick = [this] { ed.doQuantize(); };
        analyzeBtn.onClick = [this] { ed.doAnalyze(); };
        recBtn.onClick = [this] { if (proc.capturing.load()) proc.stopCaptureAndAnalyze(); else proc.startCapture(); };
        dotsBtn.onClick = [this] { ed.showMenu(); };
        folderBtn.onClick = [this]
        {
            ed.chooser = std::make_unique<FileChooser> ("Selecciona el audio a cuantizar", File(), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
            ed.chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                     [this] (const FileChooser& fc) { auto f = fc.getResult(); if (f.existsAsFile()) proc.loadAudioFile (f); });
        };
        saveBtn.onClick = [this]
        {
            ed.chooser = std::make_unique<FileChooser> ("Exportar audio cuantizado", File::getSpecialLocation (File::userDesktopDirectory).getChildFile ("Quantizer Pro - cuantizado.wav"), "*.wav");
            ed.chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                                     [this] (const FileChooser& fc) { auto f = fc.getResult(); if (f != File()) proc.exportQuantizedWav (f.withFileExtension ("wav")); });
        };
        saveBtn.setTooltip ("Arrastra este icono a la Playlist de tu DAW para enviar el WAV cuantizado. Clic para exportar manualmente.");
        saveBtn.onDragExport = [this] { ed.startDragExport (&saveBtn); };
        setSize ((int) DW, (int) DH);
    }

    void resized() override
    {
        presetBox.setBounds (46, 231, 343, 46); folderBtn.setBounds (420, 230, 52, 52); saveBtn.setBounds (485, 230, 52, 52); dotsBtn.setBounds (548, 230, 52, 52);
        analyzeBtn.setBounds (57, 416, 224, 73); recBtn.setBounds (57, 505, 224, 44);
        readouts.setBounds (330, 395, 230, 170); tempo->setBounds (330, 479, 120, 26); hostBtn.setBounds (460, 474, 76, 34);
        const int cx[4] = { 100, 240, 380, 520 };
        ui::Knob* kl[4] = { kStrength.get(), kSwing.get(), kHuman.get(), kSens.get() };
        for (int i = 0; i < 4; ++i) kl[i]->setBounds (cx[i] - 65, 628, 130, 170);
        momentBox.setBounds (1116, 234, 248, 62); gridBox.setBounds (1116, 318, 248, 58); quantizeBtn.setBounds (1383, 234, 242, 142);
        const int rx[4] = { 1178, 1309, 1440, 1571 };
        ui::Knob* kr[4] = { kWin.get(), kFade.get(), kPre.get(), kMix.get() };
        for (int i = 0; i < 4; ++i) kr[i]->setBounds (rx[i] - 65, 455, 130, 160);
        kGain->setBounds (1172 - 65, 672, 130, 160);
        meter.setBounds (1262, 650, 126, 168);
        monoBtn.setBounds (1429, 680, 188, 56); bypassBtn.setBounds (1429, 742, 188, 56);
        big.setBounds (716, 550, 240, 240);
        strip.setBounds (20, 845, 1632, 160);
        offset->setBounds (1632 - 240 + 20 - 20, 855, 150, 26);
        offset->setTopLeftPosition (20 + strip.getWidth() - 230, 851 + 3);
    }

    static void panel (Graphics& g, Rectangle<float> r, const String& title = {}, float tx = 0.f, float ty = 0.f)
    {
        g.setColour (Colour (0xff0a0a0a)); g.fillRoundedRectangle (r, 14.f);
        g.setColour (Colour (0xff2b2b2b)); g.drawRoundedRectangle (r, 14.f, 1.4f);
        if (title.isNotEmpty()) { g.setColour (white()); T (g, title, { tx, ty, 360.f, 22.f }, 19.f, Justification::centredLeft, 2.f); }
    }

    void paint (Graphics& g) override
    {
        g.setGradientFill (ColourGradient (Colour (0xff111111), DW * 0.5f, 120.f, Colour (0xff030303), 0.f, DH, true)); g.fillAll();
        g.setColour (Colour (0xff3a3a3a)); g.drawRoundedRectangle (10.f, 10.f, DW - 20.f, DH - 20.f, 18.f, 2.f);
        const Point<float> screws[4] = { { 26.f, 26.f }, { DW - 26.f, 26.f }, { 26.f, DH - 26.f }, { DW - 26.f, DH - 26.f } };
        for (auto p : screws)
        { g.setColour (Colour (0xff2a2a2a)); g.fillEllipse (p.x - 8.f, p.y - 8.f, 16.f, 16.f); g.setColour (Colour (0xff6a6a6a)); g.drawEllipse (p.x - 8.f, p.y - 8.f, 16.f, 16.f, 1.f); }

        // cabecera
        panel (g, { 20.f, 14.f, 1632.f, 140.f });
        g.drawImage (logoImg, Rectangle<float> (80.f, 26.f, 175.f, 175.f * (float) logoImg.getHeight() / (float) jmax (1, logoImg.getWidth())));
        g.setGradientFill (ColourGradient (Colours::white, 0.f, 40.f, Colour (0xff9a9a9a), 0.f, 100.f, false));
        stroke::draw (g, "QUANTIZER PRO", Rectangle<float> (560.f, 40.f, 552.f, 60.f), 58.f, Justification::centred, 7.f, 6.f);
        g.setColour (Colour (0xffd8d8d8)); stroke::draw (g, "BY ORYMY", Rectangle<float> (560.f, 108.f, 552.f, 22.f), 20.f, Justification::centred, 12.f, 2.6f);
        static const float bh[] = { 14, 30, 52, 30, 70, 30, 52, 30, 14 };
        g.setColour (white());
        for (int side = 0; side < 2; ++side)
            for (int i = 0; i < 9; ++i)
            {
                const float x = (side == 0 ? 460.f : 1150.f) + i * 8.5f;
                g.drawLine (x, 70.f - bh[i] * 0.5f, x, 70.f + bh[i] * 0.5f, 2.2f);
            }
        g.setColour (Colour (0xffb0b0b0));
        T (g, U ("PRECISI\xc3\x93N \xc2\xb7 CREACI\xc3\x93N \xc2\xb7 SONIDO"), { 1250.f, 58.f, 380.f, 14.f }, 12.f, Justification::centredRight, 3.f);
        T (g, "YMY RECORDS", { 1250.f, 90.f, 380.f, 14.f }, 11.f, Justification::centredRight, 3.f);

        // preset
        panel (g, { 22.f, 172.f, 590.f, 126.f }, "PRESET", 48.f, 189.f);
        // analizar
        panel (g, { 32.f, 310.f, 540.f, 272.f });
        g.setColour (white()); ui::drawIcon (g, 2, { 56.f, 324.f, 64.f, 56.f }, white());
        T (g, "ANALIZAR SONIDOS", { 134.f, 326.f, 420.f, 22.f }, 20.f, Justification::centredLeft, 2.f);
        g.setColour (Colour (0xffa8a8a8)); T (g, "DETECTA LOS GOLPES Y PREPARA LA REJILLA", { 134.f, 358.f, 420.f, 14.f }, 11.f, Justification::centredLeft, 1.5f);
        g.setColour (Colour (0xff2b2b2b)); g.drawVerticalLine (306, 400.f, 565.f);
        // perillas izquierda
        panel (g, { 20.f, 608.f, 572.f, 214.f });
        // centro
        g.drawImageAt (centerImg, 541, 124);
        g.setColour (white());
        g.drawLine (630.f, 801.f, 700.f, 801.f, 3.f); g.drawLine (972.f, 801.f, 1042.f, 801.f, 3.f);
        stroke::draw (g, "CUANTIZAR", Rectangle<float> (700.f, 782.f, 272.f, 36.f), 32.f, Justification::centred, 4.f, 4.5f);
        // derecha
        panel (g, { 1091.f, 172.f, 561.f, 218.f }, U ("CUANTIZACI\xc3\x93N"), 1118.f, 193.f);
        g.setColour (Colour (0xff8c8c8c));
        T (g, "MOMENTO", { 1130.f, 243.f, 150.f, 12.f }, 10.f, Justification::centredLeft, 1.5f);
        T (g, "REJILLA", { 1130.f, 325.f, 150.f, 12.f }, 10.f, Justification::centredLeft, 1.5f);
        panel (g, { 1091.f, 400.f, 561.f, 222.f }, U ("PRECISI\xc3\x93N"), 1118.f, 416.f);
        panel (g, { 1091.f, 640.f, 561.f, 190.f }, "SALIDA", 1118.f, 651.f);
        g.setColour (Colour (0xff0e0e0e)); g.fillRoundedRectangle (1429.f, 675.f, 188.f, 128.f, 10.f);
        g.setColour (Colour (0xff3a3a3a)); g.drawRoundedRectangle (1429.f, 675.f, 188.f, 128.f, 10.f, 1.2f); g.drawHorizontalLine (739, 1445.f, 1601.f);

        // pie
        panel (g, { 20.f, 1020.f, 1632.f, 64.f });
        g.drawImage (iconImg, Rectangle<float> (46.f, 1032.f, 40.f, 40.f));
        g.setColour (Colour (0xffbdbdbd)); T (g, "QUANTIZER PRO   V1.0.0", { 100.f, 1043.f, 400.f, 16.f }, 14.f, Justification::centredLeft, 1.5f);
        g.setColour (Colour (0xff8c8c8c)); T (g, "YMY RECORDS  |  BY ORYMY", { 596.f, 1043.f, 480.f, 16.f }, 14.f, Justification::centred, 3.f);
        g.drawImage (signImg, Rectangle<float> (1290.f, 1030.f, 130.f, 130.f * (float) signImg.getHeight() / (float) jmax (1, signImg.getWidth())));
        Random r (77); g.setColour (Colour (0xffd8d8d8));
        for (float x = 1507.f; x < 1612.f; x += 3.f) g.fillRect (x, 1033.f, r.nextFloat() > 0.5f ? 2.f : 1.f, 28.f);
        g.setColour (Colour (0xff8c8c8c)); T (g, "YMY RECORDS", { 1500.f, 1064.f, 120.f, 10.f }, 8.f, Justification::centred, 2.f);
    }
};

// ===================================================================== editor
QuantizerEditor::QuantizerEditor (QuantizerProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    content = std::make_unique<Content> (*this, proc);
    addAndMakeVisible (*content);
    setResizable (true, true);
    setResizeLimits (836, 550, 2508, 1650);
    getConstrainer()->setFixedAspectRatio ((double) DW / (double) DH);
    setSize (1100, 724);
    lastStamp = proc.quantizeStamp.load();
    proc.addChangeListener (this);
    startTimerHz (30);
}

QuantizerEditor::~QuantizerEditor()
{
    stopTimer();
    proc.removeChangeListener (this);
    content = nullptr;
    for (auto& f : dragExportFiles) f.deleteFile();
    dragExportFiles.clear();
    setLookAndFeel (nullptr);
}

void QuantizerEditor::resized()
{
    if (content == nullptr) return;
    content->setBounds (0, 0, (int) DW, (int) DH);
    content->setTransform (AffineTransform::scale ((float) getWidth() / DW));
}

void QuantizerEditor::repaintLive()
{
    content->strip.repaint(); content->readouts.repaint();
}

void QuantizerEditor::timerCallback()
{
    auto& c = *content;
    const int st = proc.quantizeStamp.load();
    if (st != lastStamp) { lastStamp = st; c.strip.anim = 0.f; c.strip.animStart = Time::getMillisecondCounterHiRes(); }
    else if (c.strip.anim < 1.f) c.strip.anim = (float) jmin (1.0, (Time::getMillisecondCounterHiRes() - c.strip.animStart) / 1100.0);
    c.big.phase += 0.22f;
    const bool cap = proc.capturing.load();
    c.recBtn.setButtonText (cap ? "DETENER Y ANALIZAR" : "CAPTURAR EN VIVO");
    c.recBtn.icon = cap ? 7 : 6;
    c.strip.repaint(); c.meter.repaint(); c.readouts.repaint(); c.tempo->repaint(); c.offset->repaint(); c.recBtn.repaint();
    if (proc.busy.load() || c.big.isMouseOver()) c.big.repaint();
}

void QuantizerEditor::setParam (const char* id, float v)
{
    if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void QuantizerEditor::applyPreset (int idx)
{
    int n = 0; auto* pr = qp::presets (n);
    if (idx < 0 || idx >= n) return;
    const auto& p = pr[idx];
    setParam ("strength", p.strength * 100.f); setParam ("swing", p.swing * 100.f); setParam ("humanize", p.humanize * 100.f);
    setParam ("sens", p.sensitivity * 100.f); setParam ("window", p.windowMs); setParam ("fade", p.fadeMs);
    setParam ("pre", p.preMs); setParam ("grid", (float) p.grid);
}

void QuantizerEditor::doQuantize()
{
    if (proc.capturing.load()) proc.stopCaptureAndAnalyze();
    else proc.quantizeNow();
}

void QuantizerEditor::doAnalyze()
{
    if (proc.capturing.load()) proc.stopCaptureAndAnalyze();
    else proc.analyze();
}

bool QuantizerEditor::startDragExport (Component* source)
{
    const auto clip = proc.getClip();
    if (clip == nullptr || clip->original == nullptr || clip->quantized == nullptr || proc.busy.load())
        return false;

    auto dir = File::getSpecialLocation (File::tempDirectory)
                   .getChildFile ("QuantizerPro")
                   .getChildFile ("DragExports");
    if (! dir.exists() && dir.createDirectory().failed())
        return false;

    const auto name = "QuantizerPro_Quantized_" + String (Time::getMillisecondCounter()) + ".wav";
    auto file = dir.getChildFile (name);
    if (! proc.exportQuantizedWav (file))
    {
        file.deleteFile();
        return false;
    }

    dragExportFiles.add (file); // conservarlo mientras el DAW termina de leerlo
    StringArray files;
    files.add (file.getFullPathName());
    return DragAndDropContainer::performExternalDragDropOfFiles (files, false, source);
}

void QuantizerEditor::showMenu()
{
    PopupMenu m;
    m.addItem (1, "Restablecer valores del preset");
    m.addItem (2, "Quitar audio");
    m.addSeparator();
    m.addItem (3, "Quantizer Pro v1.0.0  -  Made by ORYMY", false);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&content->dotsBtn), [this] (int r)
    {
        if (r == 1) applyPreset ((int) proc.apvts.getRawParameterValue ("preset")->load());
        else if (r == 2) proc.clearClip();
    });
}

bool QuantizerEditor::isInterestedInFileDrag (const StringArray& f)
{
    for (auto& s : f) if (File (s).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg")) return true;
    return false;
}

void QuantizerEditor::filesDropped (const StringArray& f, int, int)
{
    for (auto& s : f) if (File (s).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg")) { proc.loadAudioFile (File (s)); break; }
}

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static StringArray toArray (const char* const* names, int n)
{
    StringArray a; for (int i = 0; i < n; ++i) a.add (names[i]); return a;
}

AudioProcessorValueTreeState::ParameterLayout QuantizerProcessor::createLayout()
{
    using P = AudioProcessorValueTreeState;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    int np = 0, nm = 0, ng = 0;
    auto* pr = qp::presets (np);
    StringArray pn; for (int i = 0; i < np; ++i) pn.add (pr[i].name);

    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "preset", 1 }, "Preset", pn, 1));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "moment", 1 }, "Momento", toArray (qp::momentNames (nm), nm), 0));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "grid", 1 }, "Rejilla", toArray (qp::gridNames (ng), ng), 3));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "Tempo", NormalisableRange<float> (40.f, 240.f, 0.01f), 120.f));
    v.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "hostsync", 1 }, "Tempo del host", false));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "offset", 1 }, "Offset", NormalisableRange<float> (-500.f, 500.f, 0.1f), 0.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "strength", 1 }, "Fuerza", NormalisableRange<float> (0.f, 100.f, 1.f), 88.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (0.f, 100.f, 1.f), 0.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanizar", NormalisableRange<float> (0.f, 100.f, 1.f), 15.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sens", 1 }, "Sensibilidad", NormalisableRange<float> (0.f, 100.f, 1.f), 55.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "window", 1 }, "Ventana", NormalisableRange<float> (10.f, 250.f, 1.f), 120.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "fade", 1 }, "Fade", NormalisableRange<float> (1.f, 30.f, 0.5f), 6.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "pre", 1 }, "Pre-ataque", NormalisableRange<float> (2.f, 30.f, 0.5f), 10.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mezcla", NormalisableRange<float> (0.f, 100.f, 1.f), 100.f));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gain", 1 }, "Ganancia", NormalisableRange<float> (-24.f, 12.f, 0.1f), 0.f));
    v.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "mono", 1 }, "Mono", false));
    v.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "bypass", 1 }, "Bypass", false));
    return { v.begin(), v.end() };
}

QuantizerProcessor::QuantizerProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "QUANTIZER_PRO", createLayout())
{
    formats.registerBasicFormats();
}

QuantizerProcessor::~QuantizerProcessor() { pool.removeAllJobs (true, 4000); }

bool QuantizerProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == AudioChannelSet::mono() || o == AudioChannelSet::stereo()) && o == l.getMainInputChannelSet();
}

void QuantizerProcessor::prepareToPlay (double sr, int) { currentSr = sr; lastGain = 1.f; }

double QuantizerProcessor::effectiveBpm() const
{
    const bool hs = apvts.getRawParameterValue ("hostsync")->load() > 0.5f;
    const double hb = hostBpm.load();
    return (hs && hb > 20.0) ? hb : (double) apvts.getRawParameterValue ("bpm")->load();
}

qp::Params QuantizerProcessor::readParams (const qp::Clip* c) const
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    qp::Params p;
    int np = 0; auto* pr = qp::presets (np);
    const int pi = jlimit (0, np - 1, (int) g ("preset"));
    p.bpm = effectiveBpm();
    p.grid = (int) g ("grid");
    p.moment = (int) g ("moment");
    p.strength = g ("strength") / 100.f;
    p.swing = g ("swing") / 100.f;
    p.humanize = g ("humanize") / 100.f;
    p.sensitivity = g ("sens") / 100.f;
    p.windowMs = g ("window");
    p.fadeMs = g ("fade");
    p.preMs = g ("pre");
    p.minGapMs = pr[pi].minGapMs;
    p.offsetMs = g ("offset");
    p.clipStartSamples = c ? c->startSample : 0.0;
    return p;
}

void QuantizerProcessor::setClip (std::shared_ptr<qp::Clip> c)
{
    { const SpinLock::ScopedLockType l (clipLock); clip = std::move (c); }
    sendChangeMessage();
}

void QuantizerProcessor::clearClip() { { const ScopedLock sl (pathLock); audioPath = {}; } setClip (nullptr); }

// ------------------------------------------------------------------ carga de audio
void QuantizerProcessor::loadAudioFile (const File& f)
{
    if (busy.exchange (true)) return;
    const double sr = currentSr;
    pool.addJob ([this, f, sr]
    {
        std::unique_ptr<AudioFormatReader> r (formats.createReaderFor (f));
        if (r != nullptr && r->lengthInSamples > 0 && r->lengthInSamples < (int64) (r->sampleRate * 60.0 * 12.0))
        {
            const int nch = jmin (2, (int) r->numChannels);
            const int n = (int) r->lengthInSamples;
            AudioBuffer<float> raw (nch, n);
            r->read (&raw, 0, n, 0, true, nch > 1);
            auto ob = std::make_shared<AudioBuffer<float>> ();
            if (std::abs (r->sampleRate - sr) > 1.0)
            {
                const double ratio = r->sampleRate / sr;
                const int outN = (int) std::ceil ((double) n / ratio);
                ob->setSize (nch, outN);
                for (int c = 0; c < nch; ++c)
                {
                    LagrangeInterpolator li;
                    li.process (ratio, raw.getReadPointer (c), ob->getWritePointer (c), outN);
                }
            }
            else *ob = std::move (raw);

            auto nc = std::make_shared<qp::Clip>();
            nc->original = ob; nc->sampleRate = sr; nc->startSample = 0.0;
            { const ScopedLock sl (pathLock); audioPath = f.getFullPathName(); }
            setClip (nc);
        }
        busy = false;
        sendChangeMessage();
    });
}

// ------------------------------------------------------------------ captura en vivo
void QuantizerProcessor::startCapture()
{
    if (capturing.load()) return;
    const int len = (int) (currentSr * 180.0);
    capBuf.setSize (2, len);
    capBuf.clear();
    capLen = len; capStart = -1; capturedSamples = 0;
    setClip (nullptr);
    capturing = true;
    sendChangeMessage();
}

void QuantizerProcessor::stopCaptureAndAnalyze()
{
    if (! capturing.exchange (false)) return;
    const int got = jmin (capturedSamples.load(), capLen.load());
    if (got < 4096 || capStart.load() < 0) { sendChangeMessage(); return; }
    auto ob = std::make_shared<AudioBuffer<float>> (2, got);
    for (int c = 0; c < 2; ++c) ob->copyFrom (c, 0, capBuf, c, 0, got);
    auto nc = std::make_shared<qp::Clip>();
    nc->original = ob; nc->sampleRate = currentSr; nc->startSample = (double) capStart.load();
    setClip (nc);
    analyze();
}

// ------------------------------------------------------------------ analisis / cuantizacion
void QuantizerProcessor::analyze()    { auto c = getClip(); if (c && c->original) runAnalysis (c, false); }
void QuantizerProcessor::quantizeNow() { auto c = getClip(); if (c && c->original) runAnalysis (c, true); }

void QuantizerProcessor::runAnalysis (std::shared_ptr<const qp::Clip> c, bool thenQuantize)
{
    if (busy.exchange (true)) return;
    pool.addJob ([this, c, thenQuantize]
    {
        auto nc = std::make_shared<qp::Clip> (*c);
        const auto p = readParams (nc.get());
        if (nc->onsets.empty() || ! thenQuantize)
        {
            auto pos = qp::detectOnsets (*nc->original, nc->sampleRate, p.sensitivity, p.minGapMs);
            nc->onsets.clear();
            for (int s : pos) { qp::Onset o; o.src = s; o.dst = s; nc->onsets.push_back (o); }
            nc->quantized = nullptr;
            nc->meanDevBeforeMs = qp::meanDeviationMs (pos, nc->sampleRate, p, nc->startSample);
            nc->meanDevAfterMs = nc->meanDevBeforeMs;
        }
        if (thenQuantize)
        {
            qp::quantize (*nc, p);
            ++quantizeStamp;
        }
        setClip (nc);
        busy = false;
    });
}

bool QuantizerProcessor::exportQuantizedWav (const File& f)
{
    auto c = getClip();
    if (! c || ! c->original) return false;
    auto& b = c->quantized ? *c->quantized : *c->original;
    f.deleteFile();
    std::unique_ptr<FileOutputStream> os (f.createOutputStream());
    if (os == nullptr) return false;
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (os.get(), c->sampleRate, (unsigned) b.getNumChannels(), 24, {}, 0));
    if (w == nullptr) return false;
    os.release();
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    return true;
}

// ------------------------------------------------------------------ audio
void QuantizerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int ns = buffer.getNumSamples(), nch = buffer.getNumChannels();

    long long pos = 0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto info = ph->getPosition())
        {
            if (auto t = info->getTimeInSamples()) pos = (long long) *t;
            playing = info->getIsPlaying();
            if (auto b = info->getBpm()) hostBpm = *b;
        }
    hostPlaying = playing; hostPosSamples = (double) pos;

    const bool bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    // captura de lo que entra mientras el host reproduce
    if (capturing.load() && playing)
    {
        long long cs = capStart.load();
        if (cs < 0) { capStart = pos; cs = pos; }
        const long long idx = pos - cs;
        const int len = capLen.load();
        if (idx >= 0 && idx < len)
        {
            const int cnt = (int) jmin ((long long) ns, (long long) len - idx);
            for (int c = 0; c < 2; ++c)
                capBuf.copyFrom (c, (int) idx, buffer, jmin (c, nch - 1), 0, cnt);
            capturedSamples = jmax (capturedSamples.load(), (int) idx + cnt);
        }
    }

    std::shared_ptr<const qp::Clip> c;
    { const SpinLock::ScopedTryLockType l (clipLock); if (l.isLocked()) c = clip; }

    if (! bypass && c && c->original && ! capturing.load())
    {
        const float mix = apvts.getRawParameterValue ("mix")->load() / 100.f;
        const auto& o = *c->original;
        const auto* q = c->quantized ? c->quantized.get() : &o;
        buffer.clear();
        if (playing)
        {
            for (int ch = 0; ch < nch; ++ch)
            {
                float* out = buffer.getWritePointer (ch);
                const float* so = o.getReadPointer (jmin (ch, o.getNumChannels() - 1));
                const float* sq = q->getReadPointer (jmin (ch, q->getNumChannels() - 1));
                for (int i = 0; i < ns; ++i)
                {
                    const long long idx = pos + i - (long long) c->startSample;
                    if (idx < 0) continue;
                    const float a = idx < o.getNumSamples() ? so[idx] : 0.f;
                    const float b = idx < q->getNumSamples() ? sq[idx] : 0.f;
                    out[i] = a * (1.f - mix) + b * mix;
                }
            }
        }
    }

    if (! bypass)
    {
        const float gTarget = Decibels::decibelsToGain (apvts.getRawParameterValue ("gain")->load());
        buffer.applyGainRamp (0, ns, lastGain, gTarget);
        lastGain = gTarget;
        if (apvts.getRawParameterValue ("mono")->load() > 0.5f && nch > 1)
        {
            auto* l = buffer.getWritePointer (0); auto* r = buffer.getWritePointer (1);
            for (int i = 0; i < ns; ++i) { const float m = 0.5f * (l[i] + r[i]); l[i] = r[i] = m; }
        }
    }

    peakL = jmax (peakL.load() * 0.9f, buffer.getMagnitude (0, 0, ns));
    peakR = jmax (peakR.load() * 0.9f, nch > 1 ? buffer.getMagnitude (1, 0, ns) : buffer.getMagnitude (0, 0, ns));
}

void QuantizerProcessor::getStateInformation (MemoryBlock& dest)
{
    auto st = apvts.copyState();
    { const ScopedLock sl (pathLock); st.setProperty ("audioPath", audioPath, nullptr); }
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

void QuantizerProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (ValueTree::fromXml (*xml));
            const File f (apvts.state.getProperty ("audioPath").toString());
            if (f.existsAsFile()) loadAudioFile (f);
        }
}

AudioProcessorEditor* QuantizerProcessor::createEditor() { return new QuantizerEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new QuantizerProcessor(); }

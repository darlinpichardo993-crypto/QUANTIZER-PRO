#include "QuantEngine.h"
#include <cmath>
#include <limits>
#include <algorithm>

namespace qp
{
namespace
{
    struct GridInfo
    {
        double step = 1.0, origin = 0.0, swingShift = 0.0;
        double pos (long long k) const { return origin + (double) k * step + ((k & 1) ? swingShift : 0.0); }
        long long nearest (double t, double& outPos) const
        {
            const long long k0 = (long long) std::floor ((t - origin) / step);
            long long best = k0; double bd = 1e300;
            for (long long k = k0 - 1; k <= k0 + 2; ++k)
            {
                const double d = std::abs (t - pos (k));
                if (d < bd) { bd = d; best = k; }
            }
            outPos = pos (best);
            return best;
        }
    };

    GridInfo makeGrid (const Params& p, double sr)
    {
        GridInfo g;
        g.step = 60.0 / juce::jmax (20.0, p.bpm) * sr * gridBeats (p.grid);
        g.origin = p.offsetMs * sr / 1000.0;
        const bool triplet = (p.grid == 2 || p.grid == 4);
        g.swingShift = triplet ? 0.0 : (double) p.swing * g.step / 3.0;
        return g;
    }
}

std::vector<int> detectOnsets (const juce::AudioBuffer<float>& a, double sr, float sens, float minGapMs)
{
    const int n = a.getNumSamples();
    constexpr int order = 10, N = 1 << order, hop = 256;
    if (n < N * 2) return {};

    std::vector<float> y ((size_t) n);
    const int nch = a.getNumChannels();
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int c = 0; c < nch; ++c) s += a.getReadPointer (c)[i];
        y[(size_t) i] = s / (float) juce::jmax (1, nch);
    }
    for (int i = n - 1; i > 0; --i) y[(size_t) i] -= 0.92f * y[(size_t) i - 1];   // pre-enfasis: realza ataques

    juce::dsp::FFT fft (order);
    std::vector<float> win ((size_t) N), buf ((size_t) N * 2), prev ((size_t) N / 2 + 1, 0.f);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (N - 1));

    const int frames = (n - N) / hop + 1;
    std::vector<float> flux ((size_t) frames, 0.f);
    for (int f = 0; f < frames; ++f)
    {
        std::fill (buf.begin(), buf.end(), 0.f);
        for (int i = 0; i < N; ++i) buf[(size_t) i] = y[(size_t) (f * hop + i)] * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (buf.data());
        float sum = 0.f;
        for (int k = 2; k < N / 2; ++k)
        {
            const float m = std::log1p (buf[(size_t) k] * 4.0f);
            const float d = m - prev[(size_t) k];
            if (d > 0.f) sum += d;
            prev[(size_t) k] = m;
        }
        flux[(size_t) f] = sum;
    }

    const float gmax = *std::max_element (flux.begin(), flux.end());
    if (gmax <= 1.0e-6f) return {};

    // umbral adaptativo
    const int W = juce::jmax (4, (int) (0.12 * sr / hop));
    const float mult = 1.15f + (1.0f - sens) * 1.35f;
    const float floorv = (1.0f - sens) * 0.05f * gmax + 0.01f * gmax;
    std::vector<int> peaks;
    const int gapFrames = juce::jmax (1, (int) (minGapMs * 0.001 * sr / hop));
    for (int f = 2; f < frames - 2; ++f)
    {
        double mean = 0.0; int cnt = 0;
        for (int j = juce::jmax (0, f - W); j <= juce::jmin (frames - 1, f + W); ++j) { mean += flux[(size_t) j]; ++cnt; }
        mean /= (double) juce::jmax (1, cnt);
        const float thr = (float) mean * mult + floorv;
        const float v = flux[(size_t) f];
        if (v > thr && v >= flux[(size_t) f - 1] && v >= flux[(size_t) f + 1] && v >= flux[(size_t) f - 2] && v >= flux[(size_t) f + 2])
        {
            if (! peaks.empty() && f - peaks.back() < gapFrames)
            {
                if (v > flux[(size_t) peaks.back()]) peaks.back() = f;
            }
            else peaks.push_back (f);
        }
    }

    // refinado en el tiempo: busca la subida de energia mas fuerte alrededor del pico de flux
    constexpr int B = 32;
    float peakAmp = 0.f;
    for (int i = 0; i < n; ++i) peakAmp = juce::jmax (peakAmp, std::abs (y[(size_t) i]));
    std::vector<int> out;
    for (int f : peaks)
    {
        const int s0 = juce::jmax (0, f * hop - 128), s1 = juce::jmin (n - B * 3, f * hop + N + 128);
        if (s1 - s0 < B * 4) continue;
        const int nb = (s1 - s0) / B;
        std::vector<float> e ((size_t) nb, 0.f);
        for (int b = 0; b < nb; ++b)
        {
            float m = 0.f;
            for (int i = 0; i < B; ++i) m = juce::jmax (m, std::abs (y[(size_t) (s0 + b * B + i)]));
            e[(size_t) b] = m;
        }
        int bestB = 0; float bestD = -1.f, localMax = 0.f;
        for (int b = 0; b + 2 < nb; ++b)
        {
            const float d = e[(size_t) b + 2] - e[(size_t) b];
            localMax = juce::jmax (localMax, e[(size_t) b + 2]);
            if (d > bestD) { bestD = d; bestB = b; }
        }
        if (localMax < 0.02f * peakAmp) continue;
        const int pos = s0 + bestB * B;
        if (! out.empty() && pos - out.back() < (int) (minGapMs * 0.5 * 0.001 * sr)) continue;
        out.push_back (pos);
    }
    return out;
}

double meanDeviationMs (const std::vector<int>& pos, double sr, const Params& p, double clipStart)
{
    if (pos.empty()) return 0.0;
    const auto g = makeGrid (p, sr);
    double acc = 0.0;
    for (int s : pos)
    {
        const double t = clipStart + (double) s;
        double gp; g.nearest (t, gp);
        acc += std::abs (t - gp);
    }
    return acc / (double) pos.size() / sr * 1000.0;
}

void quantize (Clip& clip, const Params& p)
{
    if (clip.original == nullptr) return;
    const auto& src = *clip.original;
    const double sr = clip.sampleRate;
    const int n = src.getNumSamples(), nch = src.getNumChannels();
    const auto g = makeGrid (p, sr);

    const float eff = juce::jlimit (0.f, 1.f, p.strength * momentFactor (p.moment));
    const int window = (int) (p.windowMs * 0.001 * sr);
    const int F = juce::jmax (8, (int) (juce::jmin (p.fadeMs, p.preMs) * 0.001 * sr));
    const int pre = juce::jmax (F, (int) (p.preMs * 0.001 * sr));
    const int N = (int) clip.onsets.size();

    std::vector<int> before;
    before.reserve ((size_t) N);
    long long prevK = std::numeric_limits<long long>::min();
    for (int i = 0; i < N; ++i)
    {
        auto& o = clip.onsets[(size_t) i];
        before.push_back (o.src);
        const double t = clip.startSample + (double) o.src;
        double gp; const long long k = g.nearest (t, gp);
        double shift = gp - t;
        if (std::abs (shift) > (double) window) shift = 0.0;     // golpe fuera de ventana: se respeta
        shift *= (double) eff;
        juce::Random r (9001 + i * 7);
        shift += (r.nextDouble() * 2.0 - 1.0) * (double) p.humanize * 0.0025 * sr;   // micro-variacion humana (+-2.5 ms max)
        int dst = o.src + (int) std::lround (shift);
        if (i > 0)
        {
            const auto& pv = clip.onsets[(size_t) i - 1];
            if (k == prevK) dst = pv.dst + (o.src - pv.src);               // mismo casillero: se conserva el flam original
            const int minSep = juce::jmin (o.src - pv.src, (int) (0.008 * sr));
            dst = juce::jmax (dst, pv.dst + minSep);
        }
        o.dst = dst;
        prevK = k;
    }

    // medicion antes/despues
    clip.meanDevBeforeMs = meanDeviationMs (before, sr, p, clip.startSample);
    std::vector<int> after;
    for (auto& o : clip.onsets) after.push_back (o.dst);
    clip.meanDevAfterMs = meanDeviationMs (after, sr, p, clip.startSample);

    // render: cortar en golpes, mover y unir con crossfade lineal (sin time-stretch)
    const int outLen = n + window + F * 4 + 64;
    auto q = std::make_shared<juce::AudioBuffer<float>> (nch, outLen);
    q->clear();

    if (N == 0)
    {
        for (int c = 0; c < nch; ++c) q->copyFrom (c, 0, src, c, 0, n);
    }
    else
    {
        for (int i = 0; i < N; ++i)
        {
            const auto& o = clip.onsets[(size_t) i];
            const bool first = (i == 0), last = (i == N - 1);
            const int a = first ? 0 : juce::jmax (0, o.src - pre);
            const int b = last ? n : juce::jmax (a + 1, clip.onsets[(size_t) i + 1].src - pre);
            const int shiftI = o.dst - o.src;
            const int dstStart = a + shiftI;
            int len = juce::jmin (n, b + (last ? 0 : F)) - a;
            if (! last)
            {
                const auto& nx = clip.onsets[(size_t) i + 1];
                const int nxStart = juce::jmax (0, nx.src - pre) + (nx.dst - nx.src);
                len = juce::jmin (len, nxStart + F - dstStart);
            }
            len = juce::jmax (len, 1);
            const int fin  = first ? 0 : F;
            const int fout = last ? juce::jmin (len, 256) : juce::jmin (F, len);
            for (int c = 0; c < nch; ++c)
            {
                const float* s = src.getReadPointer (c);
                float* d = q->getWritePointer (c);
                for (int j = 0; j < len; ++j)
                {
                    const int di = dstStart + j;
                    if (di < 0 || di >= outLen || a + j >= n) continue;
                    float w = 1.f;
                    if (j < fin) w = (float) (j + 1) / (float) (F + 1);
                    const int rem = len - j;
                    if (fout > 0 && rem <= fout) w *= (float) rem / (float) (fout + 1);
                    d[di] += s[a + j] * w;
                }
            }
        }
    }
    clip.quantized = q;
    ++clip.version;
}
}

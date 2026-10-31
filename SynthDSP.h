#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// DSP-Bausteine: Oszillatoren (Analog, FM/Digital), Granular-Engine, Filter, Hüllkurven, LFO, Delay.
namespace synth
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float semis (float s) { return std::exp2 (s * (1.0f / 12.0f)); }
inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

struct Rng
{
    uint32_t s = 0x1234567u;
    float uni() { s = s * 1664525u + 1013904223u; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    float bi()  { return uni() * 2.0f - 1.0f; }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt;             return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// ------------------------------------------------------------------ Filter (TPT-SVF, 12 dB)
struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;

    void setup (float fc, float res, float sr)
    {
        fc = clampf (fc, 20.0f, sr * 0.45f);
        const float g = std::tan (kPi * fc / sr);
        const float q = 0.6f + res * res * 17.4f;
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0; }

    // mode: 0 LP, 1 HP, 2 BP, 3 Notch
    inline float process (float v0, int mode)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        switch (mode)
        {
            case 0:  return v2;
            case 1:  return v0 - k * v1 - v2;
            case 2:  return k * v1;
            default: return v0 - k * v1;
        }
    }
};

// ------------------------------------------------------------------ Hüllkurve
struct AdsrCoef { float ca = 0, cd = 0, cr = 0, s = 0; };

inline AdsrCoef makeAdsr (float A, float D, float S, float R, float sr)
{
    AdsrCoef c;
    c.ca = 1.0f - std::exp (-1.79f / (std::max (A, 0.0005f) * sr));
    c.cd = 1.0f - std::exp (-4.6f  / (std::max (D, 0.005f)  * sr));
    c.cr = 1.0f - std::exp (-4.6f  / (std::max (R, 0.005f)  * sr));
    c.s = S;
    return c;
}

struct Adsr
{
    enum Stage { Idle, Attack, Decay, Sustain, Release } st = Idle;
    float v = 0;

    void gate (bool on) { st = on ? Attack : (st == Idle ? Idle : Release); }

    inline float tick (const AdsrCoef& c)
    {
        switch (st)
        {
            case Attack:  v += (1.2f - v) * c.ca; if (v >= 1.0f) { v = 1.0f; st = Decay; } break;
            case Decay:   v += (c.s - v) * c.cd;  if (std::abs (v - c.s) < 1.0e-4f) { v = c.s; st = Sustain; } break;
            case Sustain: v = c.s; break;
            case Release: v -= v * c.cr;          if (v < 1.0e-4f) { v = 0; st = Idle; } break;
            default:      v = 0; break;
        }
        return v;
    }
};

// ------------------------------------------------------------------ LFO
struct Lfo
{
    float ph = 0, sh = 0;
    Rng rng;

    float tick (float rate, int shape, float sr)
    {
        ph += rate / sr;
        if (ph >= 1.0f) { ph -= 1.0f; sh = rng.bi(); }
        switch (shape)
        {
            case 0:  return std::sin (kTwoPi * ph);
            case 1:  return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
            case 2:  return 2.0f * ph - 1.0f;
            case 3:  return ph < 0.5f ? 1.0f : -1.0f;
            default: return sh;
        }
    }
};

// ------------------------------------------------------------------ Analog-Oszillator
// Zwei verstimmbare PolyBLEP-Oszillatoren plus Suboszillator und langsame Drift.
inline float oscSample (int wave, float ph, float dt, float pw)
{
    switch (wave)
    {
        case 0:  return 2.0f * ph - 1.0f - polyBlep (ph, dt);
        case 1:
        {
            float s = ph < pw ? 1.0f : -1.0f;
            s += polyBlep (ph, dt);
            float q = ph - pw; if (q < 0.0f) q += 1.0f;
            s -= polyBlep (q, dt);
            return s;
        }
        case 2:  return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
        default: return std::sin (kTwoPi * ph);
    }
}

struct AnalogOsc
{
    float ph1 = 0, ph2 = 0, phSub = 0, driftLp = 0;
    Rng rng;

    float process (float freq, int wave, float pw, float detCents, float sub, float drift, float sr)
    {
        driftLp += 0.00005f * (rng.bi() - driftLp);
        const float f1 = freq * semis (driftLp * drift * 30.0f);
        const float f2 = f1 * semis (detCents * 0.01f);
        const float dt1 = std::min (f1 / sr, 0.45f), dt2 = std::min (f2 / sr, 0.45f), dtS = dt1 * 0.5f;

        ph1 += dt1; if (ph1 >= 1.0f) ph1 -= 1.0f;
        ph2 += dt2; if (ph2 >= 1.0f) ph2 -= 1.0f;
        phSub += dtS; if (phSub >= 1.0f) phSub -= 1.0f;

        float o = (oscSample (wave, ph1, dt1, pw) + oscSample (wave, ph2, dt2, pw)) * 0.5f;
        if (sub > 0.0f)
        {
            float s = phSub < 0.5f ? 1.0f : -1.0f;
            s += polyBlep (phSub, dtS);
            float q = phSub + 0.5f; if (q >= 1.0f) q -= 1.0f;
            s -= polyBlep (q, dtS);
            o += sub * s * 0.5f;
        }
        return o;
    }
};

// ------------------------------------------------------------------ Digital-Oszillator (2-Operator-FM + Crusher)
struct DigitalOsc
{
    float phC = 0, phM = 0, fb1 = 0, fb2 = 0, hold = 0;
    int holdCount = 0;

    float process (float freq, float ratio, float index, float fb, int bits, int rateDiv, float sr)
    {
        const float m = std::sin (kTwoPi * phM + fb * 1.5f * (fb1 + fb2));
        fb2 = fb1; fb1 = m;
        float out = std::sin (kTwoPi * phC + index * m);

        phC += freq / sr;           phC -= std::floor (phC);
        phM += freq * ratio / sr;   phM -= std::floor (phM);

        if (rateDiv > 1)
        {
            if (holdCount <= 0) { hold = out; holdCount = rateDiv; }
            --holdCount;
            out = hold;
        }
        if (bits < 16)
        {
            const float levels = (float) (1 << (bits - 1));
            out = std::round (out * levels) / levels;
        }
        return out;
    }
};

// ------------------------------------------------------------------ Granular
struct GrainSource
{
    const float* data = nullptr;
    int size = 0;
    float rootFreq = 100.0f;
    float srcRate = 44100.0f;
    bool live = false;
};

struct GranularParams
{
    float pos = 0.3f, spray = 0.2f, sizeMs = 80.0f, dens = 30.0f, pitch = 0.0f, pspray = 0.0f,
          spread = 0.6f, rev = 0.0f;
    int shape = 0;
    bool track = true;
};

struct Windows
{
    float t[3][1025];
    Windows()
    {
        for (int i = 0; i <= 1024; ++i)
        {
            const float x = (float) i / 1024.0f;
            t[0][i] = 0.5f - 0.5f * std::cos (kTwoPi * x);
            t[1][i] = x < 0.5f ? 2.0f * x : 2.0f - 2.0f * x;
            t[2][i] = x < 0.25f ? x * 4.0f : (x > 0.75f ? (1.0f - x) * 4.0f : 1.0f);
        }
    }
};
inline const Windows& windows() { static const Windows w; return w; }

struct GranularVoice
{
    static constexpr int kMax = 32;
    static constexpr int kRing = 65536;

    struct Grain
    {
        bool on = false;
        double pos = 0, rate = 1;
        int dir = 1, age = 0, len = 1;
        float gl = 1, gr = 1;
    };

    Grain grains[kMax];
    std::vector<float> ring;
    int wr = 0;
    float spawn = 1.0f;
    Rng rng;

    void prepare (uint32_t seed)
    {
        ring.assign ((size_t) kRing, 0.0f);
        rng.s = 0x9E3779B9u ^ (seed * 2654435761u);
        reset();
    }

    void reset()
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        wr = 0; spawn = 1.0f;
        for (auto& g : grains) g.on = false;
    }

    inline void pushLive (float x) { ring[(size_t) wr] = x; wr = (wr + 1) & (kRing - 1); }

    inline float read (const GrainSource& s, double p) const
    {
        if (s.live)
        {
            const int i = (int) std::floor (p);
            const float f = (float) (p - (double) i);
            const int a = i & (kRing - 1), b = (a + 1) & (kRing - 1);
            return ring[(size_t) a] + (ring[(size_t) b] - ring[(size_t) a]) * f;
        }
        const int n = s.size;
        if (n < 2) return 0.0f;
        double q = std::fmod (p, (double) n);
        if (q < 0.0) q += (double) n;
        const int i = (int) q;
        const float f = (float) (q - (double) i);
        const int j = (i + 1 >= n) ? 0 : i + 1;
        return s.data[i] + (s.data[j] - s.data[i]) * f;
    }

    void startGrain (const GrainSource& src, const GranularParams& p, float noteFreq, float sr)
    {
        Grain* g = nullptr;
        for (auto& x : grains) if (! x.on) { g = &x; break; }
        if (g == nullptr) return;

        const int len = std::max (16, (int) (p.sizeMs * 0.001f * sr));
        const float pitchRatio = semis (p.pitch + rng.bi() * p.pspray);
        const float posN = clampf (p.pos + rng.bi() * p.spray * 0.5f, 0.0f, 1.0f);
        const int dir = rng.uni() < p.rev ? -1 : 1;

        double rate;
        if (src.live) rate = pitchRatio;                                    // Live-Material klingt schon in der Tonhöhe der Note
        else          rate = (double) ((p.track ? noteFreq / src.rootFreq : 1.0f) * pitchRatio * src.srcRate / sr);
        rate = std::min (rate, 8.0);

        double start;
        if (! src.live)
        {
            const double span = std::min ((double) len * rate, (double) src.size - 2.0);
            const double startMax = std::max (0.0, (double) src.size - span);
            start = (double) posN * startMax;
            if (dir < 0) start += span;
        }
        else
        {
            // Position = Abstand zum Schreibkopf. Vorwärts lesende Grains dürfen ihn nicht überholen.
            const double minOff = (dir > 0 ? (double) len * std::max (0.0, rate - 1.0) : 0.0) + 64.0;
            const double maxOff = std::max (minOff, (double) kRing - (double) len * (rate + 1.0) - 64.0);
            const double off = minOff + (double) posN * (std::min (maxOff, (double) kRing * 0.75) - minOff);
            start = (double) wr - off;
        }

        const float pan = rng.bi() * p.spread;
        const float ang = (pan + 1.0f) * 0.78539816f;
        g->on = true; g->pos = start; g->rate = rate; g->dir = dir; g->age = 0; g->len = len;
        g->gl = std::cos (ang) * 1.41421356f;
        g->gr = std::sin (ang) * 1.41421356f;
    }

    void process (const GrainSource& src, const GranularParams& p, float noteFreq, float sr, float& outL, float& outR)
    {
        spawn += (p.dens / sr) * (0.8f + 0.4f * rng.uni());
        if (spawn >= 1.0f)
        {
            spawn = std::min (spawn - 1.0f, 1.0f);
            startGrain (src, p, noteFreq, sr);
        }

        const float* win = windows().t[std::min (std::max (p.shape, 0), 2)];
        float l = 0.0f, r = 0.0f;
        for (auto& g : grains)
        {
            if (! g.on) continue;
            const float widx = (float) g.age / (float) g.len * 1024.0f;
            const int wi = (int) widx;
            const float w = win[wi] + (win[wi + 1] - win[wi]) * (widx - (float) wi);
            const float v = read (src, g.pos) * w;
            l += v * g.gl; r += v * g.gr;
            g.pos += (double) g.dir * g.rate;
            if (++g.age >= g.len) g.on = false;
        }
        const float norm = 1.0f / std::sqrt (std::max (1.0f, p.dens * p.sizeMs * 0.001f));
        outL = l * norm; outR = r * norm;
    }
};

// ------------------------------------------------------------------ Eingebaute Granular-Quellen
// Alle bei 44.1 kHz mit 100 Hz Grundton (441 Samples/Periode), je 2 s lang. Die Position scannt durch das Timbre.
inline void buildSawScan (std::vector<float>& out)
{
    const int period = 441, frames = 200;
    out.assign ((size_t) (period * frames), 0.0f);
    std::vector<float> sinTab ((size_t) period);
    for (int i = 0; i < period; ++i) sinTab[(size_t) i] = std::sin (kTwoPi * (float) i / (float) period);
    for (int f = 0; f < frames; ++f)
    {
        const float t = (float) f / (float) (frames - 1);
        const int nh = 2 + (int) (t * t * 70.0f);
        for (int n = 0; n < period; ++n)
        {
            float acc = 0.0f;
            for (int k = 1; k <= nh; ++k) acc += sinTab[(size_t) ((k * n) % period)] / (float) k;
            out[(size_t) (f * period + n)] = acc;
        }
    }
}

inline void buildVowelScan (std::vector<float>& out)
{
    const int period = 441, frames = 200;
    out.assign ((size_t) (period * frames), 0.0f);
    std::vector<float> sinTab ((size_t) period);
    for (int i = 0; i < period; ++i) sinTab[(size_t) i] = std::sin (kTwoPi * (float) i / (float) period);
    static const float F1[5] = { 800, 400, 300, 450, 325 };
    static const float F2[5] = { 1150, 2000, 2300, 800, 700 };
    for (int f = 0; f < frames; ++f)
    {
        const float t = (float) f / (float) (frames - 1) * 4.0f;
        const int v0 = std::min ((int) t, 3);
        const float fr = t - (float) v0;
        const float f1 = F1[v0] + (F1[v0 + 1] - F1[v0]) * fr;
        const float f2 = F2[v0] + (F2[v0 + 1] - F2[v0]) * fr;
        float amp[80];
        for (int k = 1; k < 80; ++k)
        {
            const float hz = 100.0f * (float) k;
            const float e1 = (hz - f1) / 130.0f, e2 = (hz - f2) / 200.0f;
            amp[k] = (std::exp (-e1 * e1) + 0.6f * std::exp (-e2 * e2) + 0.03f) / (float) k;
        }
        for (int n = 0; n < period; ++n)
        {
            float acc = 0.0f;
            for (int k = 1; k < 80; ++k) acc += amp[k] * sinTab[(size_t) ((k * n) % period)];
            out[(size_t) (f * period + n)] = acc;
        }
    }
}

inline void buildBell (std::vector<float>& out)
{
    const int n = 88200;
    out.assign ((size_t) n, 0.0f);
    static const float ratio[6] = { 1.0f, 2.76f, 5.4f, 8.93f, 13.34f, 18.64f };
    static const float amp[6]   = { 1.0f, 0.6f, 0.45f, 0.3f, 0.2f, 0.12f };
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) i / 44100.0f;
        float acc = 0.0f;
        for (int p = 0; p < 6; ++p)
            acc += amp[p] * std::exp (-t / (1.4f / (float) (p + 1))) * std::sin (kTwoPi * 200.0f * ratio[p] * t);
        out[(size_t) i] = acc;
    }
}

inline void buildNoise (std::vector<float>& out)
{
    const int n = 88200;
    out.assign ((size_t) n, 0.0f);
    Rng rng; rng.s = 777u;
    float y = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float a = 0.03f + 0.5f * (float) i / (float) n;     // Helligkeit wächst entlang der Position
        y += a * (rng.bi() - y);
        out[(size_t) i] = y;
    }
}

inline void normalise (std::vector<float>& v, float peak = 0.9f)
{
    float m = 0.0f;
    for (float x : v) m = std::max (m, std::abs (x));
    if (m > 0.0f) for (float& x : v) x *= peak / m;
}

// ------------------------------------------------------------------ Stereo-Delay (Ping-Pong)
struct StereoDelay
{
    std::vector<float> bl, br;
    int size = 0, w = 0;
    float cur = 0;

    void prepare (float sr)
    {
        size = (int) (sr * 1.2f) + 8;
        bl.assign ((size_t) size, 0.0f); br.assign ((size_t) size, 0.0f);
        w = 0; cur = 0.3f * sr;
    }

    inline void process (float inL, float inR, float timeMs, float fb, float mix, float sr, float& oL, float& oR)
    {
        const float target = clampf (timeMs * 0.001f * sr, 2.0f, (float) (size - 4));
        cur += (target - cur) * 0.0008f;
        float rp = (float) w - cur; if (rp < 0.0f) rp += (float) size;
        const int i = (int) rp; const float f = rp - (float) i;
        const int j = (i + 1 >= size) ? 0 : i + 1;
        const float wl = bl[(size_t) i] + (bl[(size_t) j] - bl[(size_t) i]) * f;
        const float wr = br[(size_t) i] + (br[(size_t) j] - br[(size_t) i]) * f;
        bl[(size_t) w] = inL + fb * wr;
        br[(size_t) w] = inR + fb * wl;
        if (++w >= size) w = 0;
        oL = inL + mix * wl;
        oR = inR + mix * wr;
    }
};
}

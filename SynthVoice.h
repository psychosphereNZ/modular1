#pragma once
#include "SynthParams.h"
#include "SynthDSP.h"

namespace synth
{
// Alles, was eine Stimme pro Block von außen braucht
struct Ctx
{
    const float* pv = nullptr;                    // Parameterwerte (Index = ParamId)
    float sr = 44100.0f;
    int mSrc[kSlots] {}, mDst[kSlots] {};         // Modulationsmatrix
    float mAmt[kSlots] {};
    float lfo1 = 0, lfo2 = 0, wheel = 0, bend = 0;
    AdsrCoef ampC, modC;
    GrainSource gsrc;                             // statische Quelle (Scan/Bell/Noise/Sample)
    int gsel = 0;                                 // gewählte Granular-Quelle (0..6)
};

struct Voice
{
    bool active = false, held = false, sustained = false;
    int note = 60;
    float vel = 0, rnd = 0;
    uint32_t age = 0;

    AnalogOsc ao;
    DigitalOsc dop;
    GranularVoice gv;
    Svf fL1, fR1, fL2, fR2;
    Adsr amp, mod;
    Rng rng;

    // Steuerwerte (alle 16 Samples aktualisiert)
    int ctrl = 0;
    float fA = 440, fD = 440, fNote = 440;
    float aPW = 0.5f, aDet = 8, dIdx = 1, dRat = 2;
    GranularParams gp;
    float lvA = 0, lvD = 0, lvG = 0, pan = 0, ampMod = 1, drive = 0;
    float panL = 1, panR = 1;
    int ftype = 0, slope = 0;
    float lastMod = 0, lastAmp = 0;

    void prepare (uint32_t seed)
    {
        gv.prepare (seed);
        ao.rng.s = 0xABCDEFu + seed * 977u;
        rng.s = 0x55AA55u + seed * 131u;
    }

    void start (int n, float v, uint32_t a)
    {
        if (! active)
        {
            fL1.reset(); fR1.reset(); fL2.reset(); fR2.reset();
            gv.reset();
            ao.ph1 = ao.ph2 = ao.phSub = 0; dop = DigitalOsc();
        }
        active = true; held = true; sustained = false;
        note = n; vel = v; age = a; rnd = rng.bi();
        amp.gate (true); mod.gate (true);
        ctrl = 0;
    }

    void release()
    {
        held = false; sustained = false;
        amp.gate (false); mod.gate (false);
    }

    void updateControl (const Ctx& c)
    {
        const float* P = c.pv;
        const float sv[kNumSources] = { 0.0f, c.lfo1, c.lfo2, lastMod, lastAmp, vel,
                                        (float) note / 127.0f, c.wheel, rnd };
        float acc[kNumDests] = {};
        for (int s = 0; s < kSlots; ++s)
            if (c.mSrc[s] > 0 && c.mSrc[s] < kNumSources && c.mDst[s] > 0 && c.mDst[s] < kNumDests)
                acc[c.mDst[s]] += c.mAmt[s] * sv[c.mSrc[s]];

        const float semi = (float) (note - 69) + acc[1] * 12.0f + c.bend;
        fNote = 440.0f * semis (semi);
        fA = fNote * semis (P[p_a_tune] + P[p_a_fine] * 0.01f);
        fD = fNote * semis (P[p_d_tune] + P[p_d_fine] * 0.01f);

        aPW  = clampf (P[p_a_pw] + acc[2] * 0.45f, 0.05f, 0.95f);
        aDet = std::max (0.0f, P[p_a_detune] + acc[3] * 50.0f);
        dIdx = std::max (0.0f, P[p_d_index] + acc[4] * 5.0f);
        dRat = clampf (P[p_d_ratio] + acc[5] * 2.0f, 0.25f, 16.0f);

        gp.pos    = clampf (P[p_g_pos] + acc[6] * 0.5f, 0.0f, 1.0f);
        gp.dens   = clampf (P[p_g_dens] * std::exp2 (acc[7] * 2.0f), 1.0f, 200.0f);
        gp.sizeMs = clampf (P[p_g_size] * std::exp2 (acc[8] * 2.0f), 3.0f, 800.0f);
        gp.pitch  = P[p_g_pitch] + acc[9] * 12.0f;
        gp.spray  = P[p_g_spray];
        gp.pspray = P[p_g_pspray];
        gp.spread = P[p_g_spread];
        gp.rev    = P[p_g_rev];
        gp.shape  = (int) P[p_g_shape];
        gp.track  = P[p_g_track] > 0.5f;

        const float octs = P[p_f_env] * 5.0f * lastMod + P[p_f_key] * (float) (note - 60) / 12.0f + acc[10] * 4.0f;
        const float cutoff = P[p_f_cutoff] * std::exp2 (octs);
        const float reso = clampf (P[p_f_reso] + acc[11] * 0.5f, 0.0f, 1.0f);
        fL1.setup (cutoff, reso, c.sr);
        for (Svf* f : { &fR1, &fL2, &fR2 }) { f->a1 = fL1.a1; f->a2 = fL1.a2; f->a3 = fL1.a3; f->k = fL1.k; }
        ftype = (int) P[p_f_type];
        slope = (int) P[p_f_slope];
        drive = P[p_f_drive];

        lvA = clampf (P[p_m_a] + acc[12], 0.0f, 1.5f);
        lvD = clampf (P[p_m_d] + acc[13], 0.0f, 1.5f);
        lvG = clampf (P[p_m_g] + acc[14], 0.0f, 1.5f);
        pan = clampf (acc[15], -1.0f, 1.0f);
        const float ang = (pan + 1.0f) * 0.78539816f;
        panL = std::cos (ang) * 1.41421356f; panR = std::sin (ang) * 1.41421356f;
        ampMod = clampf (1.0f + acc[16], 0.0f, 2.0f);
    }

    void render (const Ctx& c, float& outL, float& outR)
    {
        outL = outR = 0.0f;
        if (! active) return;

        const float aEnv = amp.tick (c.ampC);
        const float mEnv = mod.tick (c.modC);
        lastAmp = aEnv; lastMod = mEnv;

        if (amp.st == Adsr::Idle || (amp.st == Adsr::Sustain && aEnv < 1.0e-4f)) { active = false; return; }

        if (ctrl-- <= 0)
        {
            updateControl (c);
            ctrl = 15;
        }

        const float* P = c.pv;
        const bool liveA = c.gsel == 5, liveD = c.gsel == 6;

        float a = 0.0f, d = 0.0f;
        if (lvA > 0.0f || liveA)
            a = ao.process (fA, (int) P[p_a_wave], aPW, aDet, P[p_a_sub], P[p_a_drift], c.sr);
        if (lvD > 0.0f || liveD)
            d = dop.process (fD, dRat, dIdx, P[p_d_fb], (int) P[p_d_bits], (int) P[p_d_rate], c.sr);

        float gL = 0.0f, gR = 0.0f;
        if (lvG > 0.0f)
        {
            GrainSource src = c.gsrc;
            if (liveA || liveD)
            {
                gv.pushLive (liveA ? a : d);
                src.live = true;
            }
            gv.process (src, gp, fNote, c.sr, gL, gR);
        }
        else if (liveA || liveD)
        {
            gv.pushLive (liveA ? a : d);
        }

        float xl = lvA * a + lvD * d + lvG * gL;
        float xr = lvA * a + lvD * d + lvG * gR;

        if (drive > 0.0f)
        {
            const float g = 1.0f + drive * 4.0f;
            xl = std::tanh (xl * g); xr = std::tanh (xr * g);
        }
        xl = fL1.process (xl, ftype); xr = fR1.process (xr, ftype);
        if (slope == 1) { xl = fL2.process (xl, ftype); xr = fR2.process (xr, ftype); }

        const float g = aEnv * (0.3f + 0.7f * vel) * ampMod * 0.35f;
        outL = xl * g * panL;
        outR = xr * g * panR;
    }
};
}

#pragma once
// Zentrale Parameterliste: eine Zeile pro Parameter (Enum, ID-String und Plugin-Parameter entstehen daraus).
// X(id, Anzeigename, Typ, min, max, default, skew, Auswahl-Texte)
// Typen: PF = float, PI = int, PB = bool, PC = Auswahl (Texte mit '|' getrennt)
namespace synth
{
enum PType { PF, PI, PB, PC };

#define SYNTH_PARAMS(X) \
    /* Analog */ \
    X(a_wave,   "Analog Wave",        PC, 0, 3, 0, 1, "Saw|Square|Triangle|Sine") \
    X(a_pw,     "Analog PW",          PF, 0.05, 0.95, 0.5, 1, "") \
    X(a_tune,   "Analog Tune",        PF, -24, 24, 0, 1, "") \
    X(a_fine,   "Analog Fine",        PF, -100, 100, 0, 1, "") \
    X(a_detune, "Analog Detune",      PF, 0, 50, 8, 1, "") \
    X(a_sub,    "Analog Sub",         PF, 0, 1, 0, 1, "") \
    X(a_drift,  "Analog Drift",       PF, 0, 1, 0.2, 1, "") \
    /* Digital (FM) */ \
    X(d_ratio,  "Digital Ratio",      PF, 0.5, 8, 2, 0.6, "") \
    X(d_index,  "Digital Index",      PF, 0, 10, 1.5, 0.6, "") \
    X(d_fb,     "Digital Feedback",   PF, 0, 1, 0, 1, "") \
    X(d_tune,   "Digital Tune",       PF, -24, 24, 0, 1, "") \
    X(d_fine,   "Digital Fine",       PF, -100, 100, 0, 1, "") \
    X(d_bits,   "Digital Bits",       PI, 1, 16, 16, 1, "") \
    X(d_rate,   "Digital Rate Div",   PI, 1, 32, 1, 1, "") \
    /* Granular */ \
    X(g_src,    "Granular Source",    PC, 0, 6, 0, 1, "Saw Scan|Vowel Scan|Bell|Noise|Sample|Live Analog|Live Digital") \
    X(g_pos,    "Granular Position",  PF, 0, 1, 0.3, 1, "") \
    X(g_spray,  "Granular Spray",     PF, 0, 1, 0.2, 1, "") \
    X(g_size,   "Granular Size",      PF, 5, 500, 80, 0.4, "") \
    X(g_dens,   "Granular Density",   PF, 1, 120, 30, 0.4, "") \
    X(g_pitch,  "Granular Pitch",     PF, -24, 24, 0, 1, "") \
    X(g_pspray, "Granular Pitch Spray", PF, 0, 12, 0, 1, "") \
    X(g_spread, "Granular Spread",    PF, 0, 1, 0.6, 1, "") \
    X(g_rev,    "Granular Reverse",   PF, 0, 1, 0, 1, "") \
    X(g_shape,  "Granular Window",    PC, 0, 2, 0, 1, "Hann|Triangle|Trapezoid") \
    X(g_track,  "Granular Key Track", PB, 0, 1, 1, 1, "") \
    X(g_root,   "Sample Root Note",   PI, 24, 96, 60, 1, "") \
    /* Mixer */ \
    X(m_a,      "Mix Analog",         PF, 0, 1, 0.8, 1, "") \
    X(m_d,      "Mix Digital",        PF, 0, 1, 0, 1, "") \
    X(m_g,      "Mix Granular",       PF, 0, 1, 0, 1, "") \
    /* Filter */ \
    X(f_type,   "Filter Type",        PC, 0, 3, 0, 1, "Lowpass|Highpass|Bandpass|Notch") \
    X(f_slope,  "Filter Slope",       PC, 0, 1, 0, 1, "12 dB|24 dB") \
    X(f_cutoff, "Filter Cutoff",      PF, 20, 18000, 2000, 0.3, "") \
    X(f_reso,   "Filter Resonance",   PF, 0, 1, 0.2, 1, "") \
    X(f_env,    "Filter Env Amount",  PF, -1, 1, 0.3, 1, "") \
    X(f_key,    "Filter Key Track",   PF, 0, 1, 0.3, 1, "") \
    X(f_drive,  "Filter Drive",       PF, 0, 1, 0, 1, "") \
    /* Hüllkurven */ \
    X(aa,       "Amp Attack",         PF, 0.001, 5, 0.005, 0.4, "") \
    X(ad,       "Amp Decay",          PF, 0.01, 5, 0.3, 0.4, "") \
    X(as,       "Amp Sustain",        PF, 0, 1, 0.8, 1, "") \
    X(ar,       "Amp Release",        PF, 0.01, 8, 0.25, 0.4, "") \
    X(ma,       "Mod Attack",         PF, 0.001, 5, 0.005, 0.4, "") \
    X(md,       "Mod Decay",          PF, 0.01, 5, 0.4, 0.4, "") \
    X(ms,       "Mod Sustain",        PF, 0, 1, 0.3, 1, "") \
    X(mr,       "Mod Release",        PF, 0.01, 8, 0.3, 0.4, "") \
    /* LFOs */ \
    X(l1_shape, "LFO1 Shape",         PC, 0, 4, 0, 1, "Sine|Triangle|Saw|Square|S&H") \
    X(l1_rate,  "LFO1 Rate",          PF, 0.05, 30, 2, 0.4, "") \
    X(l2_shape, "LFO2 Shape",         PC, 0, 4, 1, 1, "Sine|Triangle|Saw|Square|S&H") \
    X(l2_rate,  "LFO2 Rate",          PF, 0.05, 30, 0.5, 0.4, "") \
    /* Effekte */ \
    X(dl_time,  "Delay Time",         PF, 10, 1000, 320, 0.5, "") \
    X(dl_fb,    "Delay Feedback",     PF, 0, 0.95, 0.35, 1, "") \
    X(dl_mix,   "Delay Mix",          PF, 0, 1, 0, 1, "") \
    X(rv_size,  "Reverb Size",        PF, 0, 1, 0.5, 1, "") \
    X(rv_mix,   "Reverb Mix",         PF, 0, 1, 0, 1, "") \
    /* Master */ \
    X(master,   "Master",             PF, 0, 1, 0.7, 1, "")

enum ParamId
{
#define X(id, name, type, lo, hi, def, skew, choices) p_##id,
    SYNTH_PARAMS(X)
#undef X
    kNumParams
};

struct ParamDef
{
    const char* id; const char* name; PType type;
    float lo, hi, def, skew; const char* choices;
};

inline const ParamDef* paramDefs()
{
    static const ParamDef d[] = {
#define X(id, name, type, lo, hi, def, skew, choices) { #id, name, type, (float) lo, (float) hi, (float) def, (float) skew, choices },
        SYNTH_PARAMS(X)
#undef X
    };
    return d;
}

// Modulationsmatrix (Patch-Slots)
constexpr int kSlots = 6;
constexpr int kNumSources = 9;
constexpr int kNumDests = 17;
inline const char* sourceNames() { return "None|LFO 1|LFO 2|Mod Env|Amp Env|Velocity|Key|Mod Wheel|Random"; }
inline const char* destNames()
{
    return "None|Pitch All|Analog PW|Analog Detune|Digital Index|Digital Ratio|Granular Pos|Granular Density|"
           "Granular Size|Granular Pitch|Filter Cutoff|Filter Reso|Level Analog|Level Digital|Level Granular|Pan|Amp Level";
}
}

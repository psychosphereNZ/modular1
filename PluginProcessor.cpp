#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

using namespace synth;
using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
    struct Preset { const char* name; std::vector<std::pair<const char*, float>> v; };

    // Werte in Klartext; Auswahlparameter als Index. Alles, was nicht genannt ist, steht auf dem Standardwert.
    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> p = {
            { "Init (Analog Saw)", {} },
            { "Analog Acid Bass", {
                { "a_wave", 0 }, { "a_sub", 0.5f }, { "a_detune", 4 }, { "m_a", 0.4f },
                { "f_cutoff", 250 }, { "f_reso", 0.4f }, { "f_env", 0.75f }, { "f_slope", 1 }, { "f_drive", 0.1f },
                { "aa", 0.003f }, { "ad", 0.25f }, { "as", 0.7f }, { "ar", 0.12f },
                { "ma", 0.003f }, { "md", 0.22f }, { "ms", 0.0f }, { "mr", 0.2f } } },
            { "FM Bell", {
                { "m_a", 0 }, { "m_d", 0.8f }, { "d_ratio", 3.5f }, { "d_index", 2.2f },
                { "f_cutoff", 12000 }, { "f_env", 0 }, { "f_key", 0.5f },
                { "aa", 0.001f }, { "ad", 1.5f }, { "as", 0 }, { "ar", 1.8f },
                { "md", 0.8f }, { "ms", 0 }, { "mr", 1.2f },
                { "m0_src", 3 }, { "m0_dst", 4 }, { "m0_amt", 0.5f },
                { "rv_mix", 0.35f }, { "rv_size", 0.7f } } },
            { "Granular Cloud", {
                { "m_a", 0 }, { "m_g", 0.9f }, { "g_src", 1 }, { "g_pos", 0.2f }, { "g_spray", 0.5f },
                { "g_size", 140 }, { "g_dens", 45 }, { "g_pspray", 3 }, { "g_spread", 1 },
                { "aa", 0.4f }, { "as", 1 }, { "ar", 1.2f }, { "f_cutoff", 6000 }, { "f_env", 0 },
                { "l1_rate", 0.15f }, { "m0_src", 1 }, { "m0_dst", 6 }, { "m0_amt", 0.6f },
                { "rv_mix", 0.45f }, { "rv_size", 0.8f } } },
            { "Hybrid Pad", {
                { "a_wave", 2 }, { "a_detune", 12 }, { "m_a", 0.5f }, { "m_d", 0.3f }, { "m_g", 0.5f },
                { "d_ratio", 1.0f }, { "d_index", 0.8f }, { "g_src", 0 }, { "g_pos", 0.6f }, { "g_size", 120 },
                { "aa", 0.6f }, { "as", 1 }, { "ar", 1.5f }, { "f_cutoff", 2500 }, { "f_reso", 0.25f }, { "f_env", 0.1f },
                { "l2_rate", 0.3f }, { "m0_src", 2 }, { "m0_dst", 10 }, { "m0_amt", 0.3f },
                { "dl_mix", 0.25f }, { "rv_mix", 0.3f } } },
            { "Live Granulator", {
                { "a_wave", 0 }, { "m_a", 0 }, { "m_g", 1 }, { "g_src", 5 }, { "g_pos", 0.15f },
                { "g_size", 70 }, { "g_dens", 60 }, { "g_pspray", 7 }, { "g_spray", 0.3f },
                { "aa", 0.01f }, { "as", 1 }, { "ar", 0.6f }, { "f_cutoff", 7000 }, { "f_env", 0 },
                { "m0_src", 7 }, { "m0_dst", 7 }, { "m0_amt", 0.5f }, { "rv_mix", 0.3f } } }
        };
        return p;
    }
}

SynthProcessor::SynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    const auto* defs = paramDefs();
    for (int i = 0; i < kNumParams; ++i) ptr[i] = apvts.getRawParameterValue (defs[i].id);
    for (int s = 0; s < kSlots; ++s)
    {
        const auto n = juce::String (s);
        mSrcP[s] = apvts.getRawParameterValue ("m" + n + "_src");
        mDstP[s] = apvts.getRawParameterValue ("m" + n + "_dst");
        mAmtP[s] = apvts.getRawParameterValue ("m" + n + "_amt");
    }

    buildSawScan (bank[0]);   normalise (bank[0]);
    buildVowelScan (bank[1]); normalise (bank[1]);
    buildBell (bank[2]);      normalise (bank[2]);
    buildNoise (bank[3]);     normalise (bank[3]);

    for (int i = 0; i < kVoices; ++i) voices[(size_t) i].prepare ((uint32_t) (i + 1));
}

SynthProcessor::~SynthProcessor() = default;

APVTS::ParameterLayout SynthProcessor::createLayout()
{
    APVTS::ParameterLayout l;
    const auto* defs = paramDefs();

    for (int i = 0; i < kNumParams; ++i)
    {
        const auto& d = defs[i];
        const juce::ParameterID pid { d.id, 1 };
        switch (d.type)
        {
            case PF:
                l.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name,
                        juce::NormalisableRange<float> (d.lo, d.hi, 0.0f, d.skew), d.def));
                break;
            case PI:
                l.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.lo, (int) d.hi, (int) d.def));
                break;
            case PB:
                l.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
                break;
            case PC:
                l.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name,
                        juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.def));
                break;
        }
    }

    for (int s = 0; s < kSlots; ++s)
    {
        const auto n = juce::String (s);
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "m" + n + "_src", 1 }, "Patch " + juce::String (s + 1) + " Source",
                                                             juce::StringArray::fromTokens (sourceNames(), "|", ""), s == 0 ? 7 : 0));
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "m" + n + "_dst", 1 }, "Patch " + juce::String (s + 1) + " Destination",
                                                             juce::StringArray::fromTokens (destNames(), "|", ""), s == 0 ? 10 : 0));
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "m" + n + "_amt", 1 }, "Patch " + juce::String (s + 1) + " Amount",
                                                            juce::NormalisableRange<float> (-1.0f, 1.0f), s == 0 ? 0.5f : 0.0f));
    }
    return l;
}

juce::AudioProcessorEditor* SynthProcessor::createEditor() { return new SynthEditor (*this); }

// ---------------------------------------------------------------- Programme (Presets)

int SynthProcessor::getNumPrograms() { return (int) presets().size(); }
const juce::String SynthProcessor::getProgramName (int i)
{
    return (i >= 0 && i < getNumPrograms()) ? juce::String (presets()[(size_t) i].name) : juce::String();
}

void SynthProcessor::setPlain (const juce::String& id, float plain)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

void SynthProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& kv : presets()[(size_t) index].v)
        setPlain (kv.first, kv.second);
}

// ---------------------------------------------------------------- Sample laden / State

bool SynthProcessor::loadSample (const juce::File& file)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 64) return false;

    const int n = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 60.0));
    juce::AudioBuffer<float> tmp ((int) reader->numChannels, n);
    reader->read (&tmp, 0, n, 0, true, true);

    SampleData::Ptr sd (new SampleData());
    sd->rate = reader->sampleRate;
    sd->data.assign ((size_t) n, 0.0f);
    for (int c = 0; c < tmp.getNumChannels(); ++c)
        for (int i = 0; i < n; ++i)
            sd->data[(size_t) i] += tmp.getSample (c, i) / (float) tmp.getNumChannels();
    normalise (sd->data);

    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        pendingSample = sd;
    }
    samplePath = file.getFullPathName();
    sampleName = file.getFileName();
    return true;
}

void SynthProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("samplePath", samplePath, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void SynthProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        const juce::String path = tree.getProperty ("samplePath").toString();
        apvts.replaceState (tree);
        if (path.isNotEmpty() && juce::File (path).existsAsFile())
            loadSample (juce::File (path));
    }
}

// ---------------------------------------------------------------- Audio

void SynthProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    delay.prepare ((float) sr);
    reverb.setSampleRate (sr);
    reverb.reset();
    for (auto& v : voices) { v.active = false; v.amp = Adsr(); v.mod = Adsr(); }
    sustainDown = false;
}

void SynthProcessor::noteOn (int note, float vel)
{
    Voice* v = nullptr;
    for (auto& x : voices) if (x.active && x.note == note) { v = &x; break; }
    if (v == nullptr) for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr)
    {
        uint32_t best = 0xFFFFFFFFu;
        for (auto& x : voices)
        {
            const uint32_t score = x.age + (x.held ? 0x40000000u : 0u);   // zuerst ausklingende, dann älteste
            if (score < best) { best = score; v = &x; }
        }
    }
    v->start (note, vel, ++ageCounter);
}

void SynthProcessor::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.held && v.note == note)
        {
            if (sustainDown) { v.held = false; v.sustained = true; }
            else             v.release();
        }
}

void SynthProcessor::allNotesOff()
{
    for (auto& v : voices) if (v.active) v.release();
}

void SynthProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())               noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())         noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())      bendSemis = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
    else if (m.isController())
    {
        if (m.getControllerNumber() == 1) wheel = (float) m.getControllerValue() / 127.0f;
        else if (m.getControllerNumber() == 64)
        {
            const bool down = m.getControllerValue() >= 64;
            if (sustainDown && ! down)
                for (auto& v : voices) if (v.active && v.sustained) v.release();
            sustainDown = down;
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
}

void SynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    for (int i = 0; i < kNumParams; ++i) pv[i] = ptr[i]->load();

    {
        const juce::SpinLock::ScopedTryLockType lock (sampleLock);
        if (lock.isLocked() && pendingSample != activeSample) activeSample = pendingSample;
    }

    Ctx c;
    c.pv = pv;
    c.sr = (float) sr;
    for (int s = 0; s < kSlots; ++s)
    {
        c.mSrc[s] = (int) std::lround (mSrcP[s]->load());
        c.mDst[s] = (int) std::lround (mDstP[s]->load());
        c.mAmt[s] = mAmtP[s]->load();
    }
    c.ampC = makeAdsr (pv[p_aa], pv[p_ad], pv[p_as], pv[p_ar], c.sr);
    c.modC = makeAdsr (pv[p_ma], pv[p_md], pv[p_ms], pv[p_mr], c.sr);

    c.gsel = (int) pv[p_g_src];
    if (c.gsel <= 3)
    {
        const auto& b = bank[c.gsel];
        c.gsrc = { b.data(), (int) b.size(), bankRoot[c.gsel], 44100.0f, false };
    }
    else if (c.gsel == 4)
    {
        if (activeSample != nullptr)
            c.gsrc = { activeSample->data.data(), (int) activeSample->data.size(),
                       440.0f * semis (pv[p_g_root] - 69.0f), (float) activeSample->rate, false };
        else
            c.gsrc = { bank[0].data(), (int) bank[0].size(), bankRoot[0], 44100.0f, false };
    }

    const float l1rate = pv[p_l1_rate], l2rate = pv[p_l2_rate];
    const int l1shape = (int) pv[p_l1_shape], l2shape = (int) pv[p_l2_shape];
    const float dlTime = pv[p_dl_time], dlFb = pv[p_dl_fb], dlMix = pv[p_dl_mix];
    const float rvMix = pv[p_rv_mix];
    const float master = pv[p_master];
    const float fsr = (float) sr;

    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    auto midiIt = midi.begin();
    const auto midiEnd = midi.end();
    int sp = scopePos.load();

    for (int i = 0; i < n; ++i)
    {
        while (midiIt != midiEnd && (*midiIt).samplePosition <= i)
        {
            handleMidi ((*midiIt).getMessage());
            ++midiIt;
        }

        c.lfo1 = lfo1.tick (l1rate, l1shape, fsr);
        c.lfo2 = lfo2.tick (l2rate, l2shape, fsr);
        c.wheel = wheel;
        c.bend = bendSemis;

        float l = 0.0f, r = 0.0f;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            float vl, vr;
            v.render (c, vl, vr);
            l += vl; r += vr;
        }

        if (dlMix > 0.001f)
        {
            float dl, dr;
            delay.process (l, r, dlTime, dlFb, dlMix, fsr, dl, dr);
            l = dl; r = dr;
        }

        outL[i] = l;
        if (outR) outR[i] = r;
    }

    if (outR != nullptr && rvMix > 0.001f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.65f * pv[p_rv_size];
        rp.damping = 0.5f;
        rp.wetLevel = rvMix * 0.3f;
        rp.dryLevel = 1.0f - 0.5f * rvMix;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (outL, outR, n);
    }

    for (int i = 0; i < n; ++i)
    {
        outL[i] = std::tanh (outL[i] * master * 2.0f);
        if (outR) outR[i] = std::tanh (outR[i] * master * 2.0f);
        scope[(size_t) (sp & 1023)] = outR ? 0.5f * (outL[i] + outR[i]) : outL[i];
        ++sp;
    }
    scopePos.store (sp);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SynthProcessor(); }

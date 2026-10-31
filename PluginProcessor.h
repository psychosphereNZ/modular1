#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include "SynthParams.h"
#include "SynthDSP.h"
#include "SynthVoice.h"

// Modularer Synthesizer: Analog-, Digital(FM)- und Granular-Modul -> Mixer -> Filter -> Effekte,
// verdrahtet über eine Modulationsmatrix mit sechs Patch-Slots.
class SynthProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kVoices = 8;

    SynthProcessor();
    ~SynthProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TrinityModular"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    bool loadSample (const juce::File&);
    juce::String getSampleName() const { return sampleName; }

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;
    std::array<float, 1024> scope {};
    std::atomic<int> scopePos { 0 };

private:
    using P = std::atomic<float>*;

    struct SampleData : public juce::ReferenceCountedObject
    {
        std::vector<float> data;
        double rate = 44100.0;
        using Ptr = juce::ReferenceCountedObjectPtr<SampleData>;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setPlain (const juce::String& id, float plain);

    void handleMidi (const juce::MidiMessage&);
    void noteOn (int note, float vel);
    void noteOff (int note);
    void allNotesOff();

    double sr = 44100.0;
    std::array<synth::Voice, kVoices> voices;
    uint32_t ageCounter = 0;
    bool sustainDown = false;
    float wheel = 0.0f, bendSemis = 0.0f;

    synth::Lfo lfo1, lfo2;
    synth::StereoDelay delay;
    juce::Reverb reverb;

    P ptr[synth::kNumParams] {};
    float pv[synth::kNumParams] {};
    P mSrcP[synth::kSlots] {}, mDstP[synth::kSlots] {}, mAmtP[synth::kSlots] {};

    std::vector<float> bank[4];
    float bankRoot[4] { 100.0f, 100.0f, 200.0f, 100.0f };

    SampleData::Ptr pendingSample, activeSample;
    juce::SpinLock sampleLock;
    juce::String sampleName, samplePath;

    int currentProgram = 0;
};

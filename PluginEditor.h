#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SynthLookAndFeel.h"
#include <memory>
#include <vector>

class SynthEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SynthEditor (SynthProcessor&);
    ~SynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob  { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };
    struct Combo { juce::ComboBox box; std::unique_ptr<ComboAtt> att; };
    struct Panel { juce::String title; juce::Colour col; juce::Rectangle<int> r; };

    Knob* addKnob (std::vector<std::unique_ptr<Knob>>& list, const juce::String& id, const juce::String& text, juce::Colour c);
    void setupCombo (Combo& c, const juce::String& id, const juce::StringArray& items);

    SynthLookAndFeel laf;   // muss vor allen Komponenten deklariert sein
    SynthProcessor& proc;

    std::vector<std::unique_ptr<Knob>> analogK, digitalK, granK, mixK, filterK, ampK, modK, lfoK1, lfoK2, delayK, reverbK, masterK;
    Combo waveBox, gsrcBox, gshapeBox, ftypeBox, fslopeBox, l1Box, l2Box;
    Combo mSrc[synth::kSlots], mDst[synth::kSlots];
    juce::Slider mAmt[synth::kSlots];
    std::unique_ptr<SliderAtt> mAmtAtt[synth::kSlots];

    juce::TextButton trackBtn { "KEY TRACK" }, loadBtn { "LOAD SAMPLE" };
    std::unique_ptr<ButtonAtt> trackAtt;
    juce::Label sampleLabel;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ComboBox presetBox;
    juce::MidiKeyboardComponent keyboard;

    std::vector<Panel> panels;
    juce::Rectangle<int> scopeRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};

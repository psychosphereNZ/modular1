#include "PluginEditor.h"

namespace
{
    const juce::Colour kAnalog  (0xffee7f2d);
    const juce::Colour kDigital (0xff4fc3f7);
    const juce::Colour kGran    (0xff9ccc65);
    const juce::Colour kGrey    (0xffd8d8dc);
    const juce::Colour kYellow  (0xfff0c93a);
    const juce::Colour kPurple  (0xffba68c8);
    const juce::Colour kTeal    (0xff80cbc4);
    const juce::Colour kText    (0xffd8d8dc);
    const juce::Colour kDim     (0xff9a9ca3);

    template <typename List>
    void layoutKnobs (juce::Rectangle<int> area, List& list, int cols)
    {
        if (list.empty()) return;
        const int rows = ((int) list.size() + cols - 1) / cols;
        const int w = area.getWidth() / cols, h = area.getHeight() / rows;
        for (size_t i = 0; i < list.size(); ++i)
        {
            juce::Rectangle<int> cell (area.getX() + (int) (i % (size_t) cols) * w, area.getY() + (int) (i / (size_t) cols) * h, w, h);
            list[i]->label.setBounds (cell.removeFromTop (13));
            list[i]->slider.setBounds (cell.withTrimmedBottom (3));
        }
    }
}

SynthEditor::Knob* SynthEditor::addKnob (std::vector<std::unique_ptr<Knob>>& list, const juce::String& id,
                                         const juce::String& text, juce::Colour c)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->slider);
    if (! (id == "g_root" || id == "d_bits" || id == "d_rate"))
    {
        k->slider.textFromValueFunction = [] (double v)
        {
            const double a = std::abs (v);
            return juce::String (v, a >= 100.0 ? 0 : (a >= 10.0 ? 1 : ((a > 0.0 && a < 0.1) ? 3 : 2)));
        };
        k->slider.updateText();
    }
    list.push_back (std::move (k));
    return list.back().get();
}

void SynthEditor::setupCombo (Combo& c, const juce::String& id, const juce::StringArray& items)
{
    c.box.addItemList (items, 1);
    addAndMakeVisible (c.box);
    c.att = std::make_unique<ComboAtt> (proc.apvts, id, c.box);
}

SynthEditor::SynthEditor (SynthProcessor& p)
    : AudioProcessorEditor (&p), proc (p), keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&laf);

    // Analog
    setupCombo (waveBox, "a_wave", { "Saw", "Square", "Triangle", "Sine" });
    addKnob (analogK, "a_pw", "PULSE W", kAnalog);
    addKnob (analogK, "a_tune", "TUNE", kAnalog);
    addKnob (analogK, "a_fine", "FINE", kAnalog);
    addKnob (analogK, "a_detune", "DETUNE", kAnalog);
    addKnob (analogK, "a_sub", "SUB", kAnalog);
    addKnob (analogK, "a_drift", "DRIFT", kAnalog);

    // Digital
    addKnob (digitalK, "d_ratio", "RATIO", kDigital);
    addKnob (digitalK, "d_index", "INDEX", kDigital);
    addKnob (digitalK, "d_fb", "FEEDBACK", kDigital);
    addKnob (digitalK, "d_tune", "TUNE", kDigital);
    addKnob (digitalK, "d_fine", "FINE", kDigital);
    addKnob (digitalK, "d_bits", "BITS", kDigital);
    addKnob (digitalK, "d_rate", "RATE DIV", kDigital);

    // Granular
    setupCombo (gsrcBox, "g_src", { "Saw Scan", "Vowel Scan", "Bell", "Noise", "Sample", "Live Analog", "Live Digital" });
    setupCombo (gshapeBox, "g_shape", { "Hann", "Triangle", "Trapezoid" });
    trackBtn.setClickingTogglesState (true);
    trackBtn.setColour (juce::TextButton::buttonOnColourId, kGran);
    addAndMakeVisible (trackBtn);
    trackAtt = std::make_unique<ButtonAtt> (proc.apvts, "g_track", trackBtn);
    addAndMakeVisible (loadBtn);
    sampleLabel.setColour (juce::Label::textColourId, kDim);
    sampleLabel.setFont (juce::FontOptions (11.0f));
    sampleLabel.setText (proc.getSampleName().isEmpty() ? "no sample" : proc.getSampleName(), juce::dontSendNotification);
    addAndMakeVisible (sampleLabel);
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Sample wählen (WAV, AIFF, FLAC)", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile() && proc.loadSample (f))
                                  {
                                      sampleLabel.setText (f.getFileName(), juce::dontSendNotification);
                                      gsrcBox.box.setSelectedId (5);   // Quelle "Sample"
                                  }
                              });
    };
    addKnob (granK, "g_pos", "POSITION", kGran);
    addKnob (granK, "g_spray", "SPRAY", kGran);
    addKnob (granK, "g_size", "SIZE ms", kGran);
    addKnob (granK, "g_dens", "DENSITY", kGran);
    addKnob (granK, "g_pitch", "PITCH", kGran);
    addKnob (granK, "g_pspray", "P-SPRAY", kGran);
    addKnob (granK, "g_spread", "SPREAD", kGran);
    addKnob (granK, "g_rev", "REVERSE", kGran);
    addKnob (granK, "g_root", "ROOT", kGran);

    // Mixer / Filter / Hüllkurven
    addKnob (mixK, "m_a", "ANALOG", kAnalog);
    addKnob (mixK, "m_d", "DIGITAL", kDigital);
    addKnob (mixK, "m_g", "GRANULAR", kGran);

    setupCombo (ftypeBox, "f_type", { "LP", "HP", "BP", "Notch" });
    setupCombo (fslopeBox, "f_slope", { "12 dB", "24 dB" });
    addKnob (filterK, "f_cutoff", "CUTOFF", kGrey);
    addKnob (filterK, "f_reso", "RESO", kGrey);
    addKnob (filterK, "f_env", "ENV AMT", kGrey);
    addKnob (filterK, "f_key", "KEY TRK", kGrey);
    addKnob (filterK, "f_drive", "DRIVE", kGrey);

    addKnob (ampK, "aa", "A", kYellow);  addKnob (ampK, "ad", "D", kYellow);
    addKnob (ampK, "as", "S", kYellow);  addKnob (ampK, "ar", "R", kYellow);
    addKnob (modK, "ma", "A", kPurple);  addKnob (modK, "md", "D", kPurple);
    addKnob (modK, "ms", "S", kPurple);  addKnob (modK, "mr", "R", kPurple);

    setupCombo (l1Box, "l1_shape", { "Sine", "Tri", "Saw", "Square", "S&H" });
    setupCombo (l2Box, "l2_shape", { "Sine", "Tri", "Saw", "Square", "S&H" });
    addKnob (lfoK1, "l1_rate", "RATE", kPurple);
    addKnob (lfoK2, "l2_rate", "RATE", kPurple);

    addKnob (delayK, "dl_time", "TIME ms", kTeal);
    addKnob (delayK, "dl_fb", "FEEDBACK", kTeal);
    addKnob (delayK, "dl_mix", "MIX", kTeal);
    addKnob (reverbK, "rv_size", "SIZE", kTeal);
    addKnob (reverbK, "rv_mix", "MIX", kTeal);
    addKnob (masterK, "master", "MASTER", juce::Colour (0xffd9342b));

    // Patch-Matrix
    for (int s = 0; s < synth::kSlots; ++s)
    {
        const auto n = juce::String (s);
        setupCombo (mSrc[s], "m" + n + "_src", juce::StringArray::fromTokens (synth::sourceNames(), "|", ""));
        setupCombo (mDst[s], "m" + n + "_dst", juce::StringArray::fromTokens (synth::destNames(), "|", ""));
        mAmt[s].setSliderStyle (juce::Slider::LinearHorizontal);
        mAmt[s].setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 18);
        addAndMakeVisible (mAmt[s]);
        mAmtAtt[s] = std::make_unique<SliderAtt> (proc.apvts, "m" + n + "_amt", mAmt[s]);
        mAmt[s].textFromValueFunction = [] (double v) { return juce::String (v, 2); };
        mAmt[s].updateText();
    }

    // Preset-Auswahl
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    keyboard.setAvailableRange (36, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (31.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kAnalog.withAlpha (0.8f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, kAnalog.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe9e9ec));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1b1b1e));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff4a4b52));
    addAndMakeVisible (keyboard);

    setSize (1180, 752);   // erst nach dem Anlegen aller Komponenten
    startTimerHz (30);
}

SynthEditor::~SynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SynthEditor::timerCallback()
{
    repaint (scopeRect);
}

void SynthEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2c31), 0.0f, 0.0f,
                                             juce::Colour (0xff16171a), 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colour (0xfff2f2f4));
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("TRINITY MODULAR", 14, 8, 300, 28, juce::Justification::left);
    g.setColour (kDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("ANALOG  /  DIGITAL  /  GRANULAR", 250, 14, 300, 18, juce::Justification::left);
    g.drawText ("PRESET", getWidth() - 14 - 260 - 52, 14, 50, 18, juce::Justification::right);

    const juce::Colour stripe[3] = { kAnalog, kDigital, kGran };
    const int sw = (getWidth() - 28) / 3;
    for (int i = 0; i < 3; ++i) { g.setColour (stripe[i]); g.fillRect (14 + i * sw, 40, sw, 3); }

    for (const auto& p : panels)
    {
        g.setColour (juce::Colour (0x22ffffff));
        g.fillRoundedRectangle (p.r.toFloat(), 5.0f);
        g.setColour (p.col.withAlpha (0.85f));
        g.fillRect (p.r.getX() + 6, p.r.getY() + 2, p.r.getWidth() - 12, 2);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (p.title, p.r.getX() + 8, p.r.getY() + 5, 300, 16, juce::Justification::left);
    }

    // Oszilloskop
    if (! scopeRect.isEmpty())
    {
        auto r = scopeRect.toFloat();
        g.setColour (juce::Colour (0xff0e0f11));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (kDim);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("OUTPUT", scopeRect.getX() + 8, scopeRect.getY() + 3, 80, 14, juce::Justification::left);

        std::array<float, 1024> snap;
        const int pos = proc.scopePos.load();
        for (int i = 0; i < 1024; ++i) snap[(size_t) i] = proc.scope[(size_t) ((pos + i) & 1023)];
        int start = 0;
        for (int i = 0; i < 480; ++i) if (snap[(size_t) i] < 0.0f && snap[(size_t) i + 1] >= 0.0f) { start = i; break; }

        juce::Path path;
        const auto inner = r.reduced (6.0f, 18.0f);
        for (int i = 0; i < 512; ++i)
        {
            const float x = inner.getX() + inner.getWidth() * (float) i / 511.0f;
            const float y = inner.getCentreY() - snap[(size_t) (start + i)] * inner.getHeight() * 0.5f;
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (kGran);
        g.strokePath (path, juce::PathStrokeType (1.3f));
    }
}

void SynthEditor::resized()
{
    panels.clear();
    auto r = getLocalBounds().reduced (14);
    auto title = r.removeFromTop (38);
    presetBox.setBounds (juce::Rectangle<int> (title.getRight() - 260, 15, 260, 24));

    auto inner = [] (juce::Rectangle<int> p, int header) { p.removeFromTop (header); return p.reduced (4, 2); };

    // --- Zeile A: die drei Klangquellen
    auto rowA = r.removeFromTop (240);
    auto aR = rowA.removeFromLeft (290); rowA.removeFromLeft (8);
    auto dR = rowA.removeFromLeft (340); rowA.removeFromLeft (8);
    auto gR = rowA;
    panels.push_back ({ "ANALOG", kAnalog, aR });
    panels.push_back ({ "DIGITAL  (2-OP FM)", kDigital, dR });
    panels.push_back ({ "GRANULAR", kGran, gR });

    {
        auto h = aR.withHeight (26).withTrimmedLeft (110).reduced (4, 3);
        waveBox.box.setBounds (h);
        layoutKnobs (inner (aR, 30), analogK, 3);
        layoutKnobs (inner (dR, 30), digitalK, 4);

        auto h2 = gR.withHeight (26).withTrimmedLeft (120).reduced (4, 3);
        gsrcBox.box.setBounds (h2);
        auto strip = gR.withTrimmedTop (28).withHeight (28).reduced (6, 2);
        loadBtn.setBounds (strip.removeFromLeft (100));
        strip.removeFromLeft (6);
        gshapeBox.box.setBounds (strip.removeFromRight (100));
        strip.removeFromRight (6);
        trackBtn.setBounds (strip.removeFromRight (84));
        strip.removeFromRight (6);
        sampleLabel.setBounds (strip);
        layoutKnobs (inner (gR, 58), granK, 5);
    }

    r.removeFromTop (8);

    // --- Zeile B: Mixer, Filter, Hüllkurven
    auto rowB = r.removeFromTop (112);
    auto mixR = rowB.removeFromLeft (216); rowB.removeFromLeft (8);
    auto filR = rowB.removeFromLeft (346); rowB.removeFromLeft (8);
    auto ampR = rowB.removeFromLeft (282); rowB.removeFromLeft (8);
    auto modR = rowB.removeFromLeft (282);
    panels.push_back ({ "MIXER", kGrey, mixR });
    panels.push_back ({ "FILTER", kGrey, filR });
    panels.push_back ({ "AMP ENV", kYellow, ampR });
    panels.push_back ({ "MOD ENV", kPurple, modR });
    layoutKnobs (inner (mixR, 26), mixK, 3);
    layoutKnobs (inner (filR, 26), filterK, 5);
    layoutKnobs (inner (ampR, 26), ampK, 4);
    layoutKnobs (inner (modR, 26), modK, 4);
    fslopeBox.box.setBounds (filR.withHeight (24).removeFromRight (74).reduced (2, 3));
    ftypeBox.box.setBounds (filR.withHeight (24).withTrimmedRight (76).removeFromRight (74).reduced (2, 3));

    r.removeFromTop (8);

    // --- Zeile C: LFOs, Effekte, Master, Oszilloskop
    auto rowC = r.removeFromTop (112);
    auto l1R = rowC.removeFromLeft (124); rowC.removeFromLeft (8);
    auto l2R = rowC.removeFromLeft (124); rowC.removeFromLeft (8);
    auto dlR = rowC.removeFromLeft (210); rowC.removeFromLeft (8);
    auto rvR = rowC.removeFromLeft (144); rowC.removeFromLeft (8);
    auto maR = rowC.removeFromLeft (80);  rowC.removeFromLeft (8);
    scopeRect = rowC;
    panels.push_back ({ "LFO 1", kPurple, l1R });
    panels.push_back ({ "LFO 2", kPurple, l2R });
    panels.push_back ({ "DELAY", kTeal, dlR });
    panels.push_back ({ "REVERB", kTeal, rvR });
    panels.push_back ({ "OUT", juce::Colour (0xffd9342b), maR });
    l1Box.box.setBounds (l1R.withHeight (24).withTrimmedLeft (50).reduced (2, 3));
    l2Box.box.setBounds (l2R.withHeight (24).withTrimmedLeft (50).reduced (2, 3));
    layoutKnobs (inner (l1R, 26), lfoK1, 1);
    layoutKnobs (inner (l2R, 26), lfoK2, 1);
    layoutKnobs (inner (dlR, 26), delayK, 3);
    layoutKnobs (inner (rvR, 26), reverbK, 2);
    layoutKnobs (inner (maR, 26), masterK, 1);

    r.removeFromTop (8);

    // --- Zeile D: Patch-Matrix (6 Slots)
    auto rowD = r.removeFromTop (118);
    panels.push_back ({ "PATCH MATRIX  (SOURCE  >  DESTINATION)", kAnalog, rowD });
    auto body = rowD.withTrimmedTop (24).reduced (8, 2);
    const int colW = (body.getWidth() - 12) / 2;
    for (int s = 0; s < synth::kSlots; ++s)
    {
        auto cell = juce::Rectangle<int> (body.getX() + (s / 3) * (colW + 12), body.getY() + (s % 3) * 30, colW, 28);
        mSrc[s].box.setBounds (cell.removeFromLeft (118).reduced (0, 2));
        cell.removeFromLeft (8);
        mDst[s].box.setBounds (cell.removeFromLeft (160).reduced (0, 2));
        cell.removeFromLeft (8);
        mAmt[s].setBounds (cell);
    }

    r.removeFromTop (8);
    keyboard.setBounds (r.removeFromTop (66));
}

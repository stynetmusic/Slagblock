#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::Colour bgDeep    { 0xff0a0a0e };
    const juce::Colour bgPanel   { 0xff121218 };
    const juce::Colour bgHeader  { 0xff101016 };
    const juce::Colour stroke    { 0xff252530 };
    const juce::Colour textMain  { 0xffececf2 };
    const juce::Colour textDim   { 0xff8888a0 };
    const juce::Colour cyan      { 0xff22d3ee };
    const juce::Colour cyanDim   { 0xff0e7490 };
    const juce::Colour trackBg   { 0xff1e1e28 };
    const juce::Colour trackFill { 0xff22d3ee };

    constexpr int kW       = 1080;
    constexpr int kHMain   = 820;
    constexpr int kFxExtra = 230;
    constexpr int kImExtra = 250;
    constexpr int kPad     = 16;
    constexpr int kGap     = 12;
    constexpr int kBtnH    = 36;
    constexpr int kLabelW  = 78;
}

// ============================================================================
// LookAndFeel
// ============================================================================

SlagLookAndFeel::SlagLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, juce::Colours::white);
    setColour (juce::Slider::trackColourId, cyan);
    setColour (juce::Slider::backgroundColourId, trackBg);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1a1a24));
    setColour (juce::TextButton::textColourOffId, textMain);
    setColour (juce::Label::textColourId, textDim);
}

void SlagLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float /*min*/, float /*max*/,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool hover = slider.isMouseOverOrDragging();
    const float trackH = 5.0f;
    const float cy = (float) y + (float) height * 0.5f;
    const auto track = juce::Rectangle<float> ((float) x, cy - trackH * 0.5f, (float) width, trackH);

    g.setColour (trackBg);
    g.fillRoundedRectangle (track, 2.5f);

    auto fill = track.withWidth (juce::jmax (0.0f, sliderPos - (float) x));
    g.setColour (hover ? cyan.brighter (0.15f) : cyan.withAlpha (0.85f));
    g.fillRoundedRectangle (fill, 2.5f);

    // Thumb with cyan ring
    const float thr = hover ? 8.0f : 7.0f;
    g.setColour (cyan.withAlpha (hover ? 0.55f : 0.35f));
    g.drawEllipse (sliderPos - thr - 2.0f, cy - thr - 2.0f, (thr + 2.0f) * 2.0f, (thr + 2.0f) * 2.0f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (sliderPos - thr, cy - thr, thr * 2.0f, thr * 2.0f);

    juce::ignoreUnused (style);
}

void SlagLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float pos, float startAng, float endAng, juce::Slider& slider)
{
    const bool hover = slider.isMouseOverOrDragging();
    const float radius = juce::jmin (width, height) * 0.38f;
    const float cx = (float) x + (float) width * 0.5f;
    const float cy = (float) y + (float) height * 0.5f;
    const float ang = startAng + pos * (endAng - startAng);

    // Outer cyan ring
    g.setColour (cyan.withAlpha (hover ? 0.55f : 0.28f));
    g.drawEllipse (cx - radius - 3.0f, cy - radius - 3.0f, (radius + 3.0f) * 2.0f, (radius + 3.0f) * 2.0f, 1.6f);

    // Body
    g.setColour (juce::Colour (0xff1a1a22));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (stroke);
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Value arc
    juce::Path arc;
    arc.addCentredArc (cx, cy, radius - 2.0f, radius - 2.0f, 0.0f, startAng, ang, true);
    g.setColour (cyan.withAlpha (0.9f));
    g.strokePath (arc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    juce::Path p;
    const float pr = radius * 0.62f;
    p.startNewSubPath (cx, cy);
    p.lineTo (cx + std::cos (ang - juce::MathConstants<float>::halfPi) * pr,
              cy + std::sin (ang - juce::MathConstants<float>::halfPi) * pr);
    g.setColour (juce::Colours::white);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

void SlagLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    auto bg = backgroundColour;

    if (button.getToggleState())
        bg = cyan.withAlpha (0.22f);
    else if (down)
        bg = bg.brighter (0.12f);
    else if (highlighted)
        bg = bg.brighter (0.08f);

    g.setColour (bg);
    g.fillRoundedRectangle (r, 8.0f);

    g.setColour (button.getToggleState() || highlighted ? cyan.withAlpha (0.55f) : stroke);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
}

juce::Font SlagLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (juce::FontOptions (13.0f, juce::Font::bold));
}

juce::Font SlagLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (12.0f));
}

// ============================================================================
// Editor
// ============================================================================

SlagblockAudioProcessorEditor::SlagblockAudioProcessorEditor (SlagblockAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&slagLaf);

    titleLabel.setText ("SLAGBLOCK", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, textMain);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    tagLabel.setText ("NEURO FM", juce::dontSendNotification);
    tagLabel.setFont (juce::FontOptions (11.0f));
    tagLabel.setColour (juce::Label::textColourId, cyan);
    addAndMakeVisible (tagLabel);

    statusLabel.setText ("~/Library/Audio/Presets/Slagblock", juce::dontSendNotification);
    statusLabel.setFont (juce::FontOptions (11.0f));
    statusLabel.setColour (juce::Label::textColourId, textDim);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    setupHeader (oscHeader, "OSCILLATOR / FM");
    setupHeader (subHeader, "SUB");
    setupHeader (envHeader, "ENVELOPE");
    setupHeader (filterHeader, "FILTER");
    setupHeader (lfoHeader, "LFO");
    setupHeader (fxHeader, "FX");
    setupHeader (imHeader, "IMITATION");
    setupHeader (knobsHeader, "MAIN");

    setupButton (randomButton, "Random");
    setupButton (fxButton, "FX", true);
    setupButton (randomFxButton, "Random FX");
    setupButton (imitationButton, "Imitation", true);
    setupButton (randomImButton, "Random IM");
    setupButton (savePresetButton, "Save");
    setupButton (loadPresetButton, "Load");
    setupButton (undoButton, "Undo");
    setupButton (redoButton, "Redo");

    randomButton.onClick = [this] { audioProcessor.randomizeNeuroPatch(); };
    fxButton.onClick = [this] { setFxPanelVisible (fxButton.getToggleState()); };
    randomFxButton.onClick = [this]
    {
        if (! fxPanelVisible)
        {
            fxButton.setToggleState (true, juce::dontSendNotification);
            setFxPanelVisible (true);
        }
        audioProcessor.randomizeFXOnly();
    };
    imitationButton.onClick = [this] { setImitationPanelVisible (imitationButton.getToggleState()); };
    randomImButton.onClick = [this]
    {
        if (! imPanelVisible)
        {
            imitationButton.setToggleState (true, juce::dontSendNotification);
            setImitationPanelVisible (true);
        }
        audioProcessor.randomizeImitationOnly();
    };
    savePresetButton.onClick = [this] { savePresetClicked(); };
    loadPresetButton.onClick = [this] { loadPresetClicked(); };
    undoButton.onClick = [this] { if (audioProcessor.undo()) syncPanelsFromProcessor(); };
    redoButton.onClick = [this] { if (audioProcessor.redo()) syncPanelsFromProcessor(); };

    audioProcessor.setStateChangeCallback ([this]
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SlagblockAudioProcessorEditor> (this)]
        {
            if (safe != nullptr)
                safe->syncPanelsFromProcessor();
        });
    });

    // Left column controls
    setupHSlider (ratioSlider, ratioLabel, "Ratio");
    setupHSlider (indexSlider, indexLabel, "Index");
    setupHSlider (feedbackSlider, feedbackLabel, "Feedback");
    setupHSlider (driveSlider, driveLabel, "Drive");
    setupHSlider (subLevelSlider, subLevelLabel, "Level");
    setupHSlider (subOctaveSlider, subOctaveLabel, "Octave");
    setupHSlider (attackSlider, attackLabel, "Attack");
    setupHSlider (decaySlider, decayLabel, "Decay");
    setupHSlider (sustainSlider, sustainLabel, "Sustain");
    setupHSlider (releaseSlider, releaseLabel, "Release");

    // Right column
    setupHSlider (cutoffSlider, cutoffLabel, "Cutoff");
    setupHSlider (resonanceSlider, resonanceLabel, "Resonance");
    setupHSlider (noiseSlider, noiseLabel, "Noise");
    setupHSlider (lfoRateSlider, lfoRateLabel, "Rate Hz");
    setupToggle (lfoSyncToggle, "Sync");
    lfoSyncLabel.setText ("Tempo", juce::dontSendNotification);
    lfoSyncLabel.setFont (juce::FontOptions (12.0f));
    lfoSyncLabel.setColour (juce::Label::textColourId, textDim);
    lfoSyncLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lfoSyncLabel);

    lfoDivLabel.setText ("Grid", juce::dontSendNotification);
    lfoDivLabel.setFont (juce::FontOptions (12.0f));
    lfoDivLabel.setColour (juce::Label::textColourId, textDim);
    lfoDivLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lfoDivLabel);

    lfoDivisionBox.addItemList (SlagblockAudioProcessor::getLfoDivisionNames(), 1);
    lfoDivisionBox.setJustificationType (juce::Justification::centredLeft);
    lfoDivisionBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a24));
    lfoDivisionBox.setColour (juce::ComboBox::outlineColourId, stroke);
    lfoDivisionBox.setColour (juce::ComboBox::textColourId, textMain);
    lfoDivisionBox.setColour (juce::ComboBox::arrowColourId, cyan);
    addAndMakeVisible (lfoDivisionBox);

    // Clean ASCII labels only (no unicode arrows — avoid encoding corruption)
    setupHSlider (lfoCutoffSlider, lfoCutoffLabel, "Cutoff");
    setupHSlider (lfoIndexSlider, lfoIndexLabel, "Index");
    setupHSlider (lfoDriveSlider, lfoDriveLabel, "Drive");
    setupToggle (lfoToAmpToggle, "Amp");
    setupHSlider (lfoAmpDepthSlider, lfoAmpDepthLabel, "Amp Depth");

    lfoSyncToggle.onClick = [this] { updateLfoRateEnabled(); };

    // Bottom knobs
    setupKnob (knobCutoff, knobCutoffLab, "Cutoff");
    setupKnob (knobReso, knobResoLab, "Reso");
    setupKnob (knobDrive, knobDriveLab, "Drive");
    setupKnob (knobIndex, knobIndexLab, "Index");
    setupKnob (knobFeedback, knobFeedbackLab, "Fdbk");
    setupKnob (knobSub, knobSubLab, "Sub");

    // FX
    setupHSlider (fxDistSlider, fxDistLabel, "Distortion");
    setupHSlider (fxBitsSlider, fxBitsLabel, "Crush Bits");
    setupHSlider (fxRateSlider, fxRateLabel, "Crush Rate");
    setupHSlider (fxDlyTimeSlider, fxDlyTimeLabel, "Delay Time");
    setupHSlider (fxDlyFbSlider, fxDlyFbLabel, "Delay FB");
    setupHSlider (fxDlyMixSlider, fxDlyMixLabel, "Delay Mix");
    setupHSlider (fxChoMixSlider, fxChoMixLabel, "Chorus Mix");
    setupHSlider (fxChoDepthSlider, fxChoDepthLabel, "Chorus Dep");

    // Imitation
    setupToggle (imVolOn, "On"); setupToggle (imDriveOn, "On"); setupToggle (imTimeOn, "On");
    setupToggle (imFiltOn, "On"); setupToggle (imCrushOn, "On"); setupToggle (imNoiseOn, "On");
    setupToggle (imWidthOn, "On");
    setupHSlider (imVolSlider, imVolLabel, "Volume");
    setupHSlider (imDriveSlider, imDriveLabel, "Drive");
    setupHSlider (imTimeSlider, imTimeLabel, "Time");
    setupHSlider (imFiltSlider, imFiltLabel, "Filter");
    setupHSlider (imCrushSlider, imCrushLabel, "Crush");
    setupHSlider (imNoiseSlider, imNoiseLabel, "Noise");
    setupHSlider (imWidthSlider, imWidthLabel, "Width");

    setFxControlsVisible (false);
    setImControlsVisible (false);

    auto& apvts = audioProcessor.getAPVTS();

    ratioAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramRatio, ratioSlider);
    indexAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramIndex, indexSlider);
    driveAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramDrive, driveSlider);
    feedbackAtt  = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFeedback, feedbackSlider);
    cutoffAtt    = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramCutoff, cutoffSlider);
    resonanceAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramResonance, resonanceSlider);
    noiseAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramNoise, noiseSlider);
    attackAtt    = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramAttack, attackSlider);
    decayAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramDecay, decaySlider);
    sustainAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramSustain, sustainSlider);
    releaseAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramRelease, releaseSlider);
    subLevelAtt  = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramSubLevel, subLevelSlider);
    subOctaveAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramSubOctave, subOctaveSlider);
    lfoRateAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramLfoRate, lfoRateSlider);
    lfoSyncAtt     = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramLfoSync, lfoSyncToggle);
    lfoDivisionAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, SlagblockAudioProcessor::paramLfoDivision, lfoDivisionBox);
    lfoCutoffAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramLfoCutoff, lfoCutoffSlider);
    lfoIndexAtt    = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramLfoIndex, lfoIndexSlider);
    lfoDriveAtt    = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramLfoDrive, lfoDriveSlider);
    lfoToAmpAtt    = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramLfoToAmp, lfoToAmpToggle);
    lfoAmpDepthAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramLfoAmpDepth, lfoAmpDepthSlider);

    updateLfoRateEnabled();

    // Bottom knobs share same parameters (mirror controls)
    knobCutoffAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramCutoff, knobCutoff);
    knobResoAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramResonance, knobReso);
    knoDriveAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramDrive, knobDrive);
    knobIndexAtt  = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramIndex, knobIndex);
    knobFdbkAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFeedback, knobFeedback);
    knobSubAtt    = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramSubLevel, knobSub);

    fxDistAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxDist, fxDistSlider);
    fxBitsAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxCrushBits, fxBitsSlider);
    fxRateAtt     = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxCrushRate, fxRateSlider);
    fxDlyTimeAtt  = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxDelayTime, fxDlyTimeSlider);
    fxDlyFbAtt    = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxDelayFb, fxDlyFbSlider);
    fxDlyMixAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxDelayMix, fxDlyMixSlider);
    fxChoMixAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxChorusMix, fxChoMixSlider);
    fxChoDepthAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramFxChorusDepth, fxChoDepthSlider);

    imVolAtt   = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImVol, imVolSlider);
    imDriveAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImDrive, imDriveSlider);
    imTimeAtt  = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImTime, imTimeSlider);
    imFiltAtt  = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImFilt, imFiltSlider);
    imCrushAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImCrush, imCrushSlider);
    imNoiseAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImNoise, imNoiseSlider);
    imWidthAtt = std::make_unique<SA> (apvts, SlagblockAudioProcessor::paramImWidth, imWidthSlider);

    imVolOnAtt   = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImVolOn, imVolOn);
    imDriveOnAtt = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImDriveOn, imDriveOn);
    imTimeOnAtt  = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImTimeOn, imTimeOn);
    imFiltOnAtt  = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImFiltOn, imFiltOn);
    imCrushOnAtt = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImCrushOn, imCrushOn);
    imNoiseOnAtt = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImNoiseOn, imNoiseOn);
    imWidthOnAtt = std::make_unique<BA> (apvts, SlagblockAudioProcessor::paramImWidthOn, imWidthOn);

    if (auto* fxOn = apvts.getRawParameterValue (SlagblockAudioProcessor::paramFxOn))
        if (fxOn->load() > 0.5f)
        {
            fxButton.setToggleState (true, juce::dontSendNotification);
            fxPanelVisible = true;
            setFxControlsVisible (true);
        }

    if (auto* imOn = apvts.getRawParameterValue (SlagblockAudioProcessor::paramImOn))
        if (imOn->load() > 0.5f)
        {
            imitationButton.setToggleState (true, juce::dontSendNotification);
            imPanelVisible = true;
            setImControlsVisible (true);
        }

    startTimerHz (30); // smooth scope animation + undo snapshots
    updateWindowSize();
    setResizable (true, true);
    setResizeLimits (960, 720, 1600, 1400);
}

SlagblockAudioProcessorEditor::~SlagblockAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.setStateChangeCallback ({});
    setLookAndFeel (nullptr);
}

// ============================================================================
// Setup
// ============================================================================

void SlagblockAudioProcessorEditor::timerCallback()
{
    phaseAnim += 0.045f;
    if (phaseAnim > juce::MathConstants<float>::twoPi)
        phaseAnim -= juce::MathConstants<float>::twoPi;

    // Debounced undo (every ~10 frames)
    static int tick = 0;
    if ((++tick % 10) == 0)
        audioProcessor.pushUndoSnapshot ("Parameter edit");

    undoButton.setEnabled (audioProcessor.canUndo());
    redoButton.setEnabled (audioProcessor.canRedo());
    updateLfoRateEnabled();

    const auto name = audioProcessor.getLastPresetName();
    if (name.isNotEmpty())
        statusLabel.setText ("Preset: " + name, juce::dontSendNotification);

    repaint (scopeArea.expanded (4));
    repaint (adsrArea.expanded (4));
}

void SlagblockAudioProcessorEditor::setupHeader (juce::Label& lab, const juce::String& text)
{
    lab.setText (text, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    lab.setColour (juce::Label::textColourId, textMain);
    lab.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupHSlider (juce::Slider& s, juce::Label& lab, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
    s.setScrollWheelEnabled (true);
    s.setColour (juce::Slider::textBoxTextColourId, textMain);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0e0e14));
    s.setColour (juce::Slider::textBoxOutlineColourId, stroke);
    addAndMakeVisible (s);

    lab.setText (name, juce::dontSendNotification);
    lab.setColour (juce::Label::textColourId, textDim);
    lab.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& lab, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 16);
    s.setScrollWheelEnabled (true);
    s.setColour (juce::Slider::textBoxTextColourId, textDim);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    lab.setText (name, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    lab.setColour (juce::Label::textColourId, textDim);
    lab.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupToggle (juce::ToggleButton& b, const juce::String& text)
{
    b.setButtonText (text);
    b.setColour (juce::ToggleButton::textColourId, textDim);
    b.setColour (juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible (b);
}

void SlagblockAudioProcessorEditor::updateLfoRateEnabled()
{
    // Free Hz rate only when Sync is off
    const bool syncOn = lfoSyncToggle.getToggleState();
    lfoRateSlider.setEnabled (! syncOn);
    lfoRateLabel.setEnabled (! syncOn);
    lfoDivisionBox.setEnabled (syncOn);
    lfoDivLabel.setEnabled (syncOn);
}

void SlagblockAudioProcessorEditor::setupButton (juce::TextButton& b, const juce::String& text, bool toggle)
{
    b.setButtonText (text);
    b.setClickingTogglesState (toggle);
    b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1a1a24));
    b.setColour (juce::TextButton::buttonOnColourId, cyan.withAlpha (0.25f));
    b.setColour (juce::TextButton::textColourOffId, textMain);
    b.setColour (juce::TextButton::textColourOnId, cyan);
    addAndMakeVisible (b);
}

float SlagblockAudioProcessorEditor::readParam (const char* id, float fallback) const
{
    if (auto* p = audioProcessor.getAPVTS().getRawParameterValue (id))
        return p->load();
    return fallback;
}

// ============================================================================
// Layout helpers
// ============================================================================

void SlagblockAudioProcessorEditor::layoutHList (juce::Rectangle<int> area,
                                                 juce::Slider* const* sliders,
                                                 juce::Label* const* labels,
                                                 int count)
{
    if (count <= 0)
        return;

    const int rowH = juce::jmax (26, area.getHeight() / count);
    for (int i = 0; i < count; ++i)
    {
        auto row = area.removeFromTop (rowH).reduced (0, 2);
        labels[i]->setBounds (row.removeFromLeft (kLabelW));
        row.removeFromLeft (6);
        sliders[i]->setBounds (row);
    }
}

void SlagblockAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int> area,
                                                   juce::Slider* const* knobs,
                                                   juce::Label* const* labels,
                                                   int count)
{
    if (count <= 0)
        return;

    const int colW = area.getWidth() / count;
    for (int i = 0; i < count; ++i)
    {
        auto col = area.withX (area.getX() + i * colW).withWidth (colW).reduced (6, 0);
        labels[i]->setBounds (col.removeFromTop (18));
        knobs[i]->setBounds (col.reduced (4, 0));
    }
}

void SlagblockAudioProcessorEditor::layoutFxGrid (juce::Rectangle<int> area)
{
    juce::Slider* s[] = {
        &fxDistSlider, &fxBitsSlider, &fxRateSlider, &fxDlyTimeSlider,
        &fxDlyFbSlider, &fxDlyMixSlider, &fxChoMixSlider, &fxChoDepthSlider
    };
    juce::Label* l[] = {
        &fxDistLabel, &fxBitsLabel, &fxRateLabel, &fxDlyTimeLabel,
        &fxDlyFbLabel, &fxDlyMixLabel, &fxChoMixLabel, &fxChoDepthLabel
    };

    constexpr int n = 8, cols = 2;
    const int rows = n / cols;
    const int colW = area.getWidth() / cols;
    const int rowH = area.getHeight() / rows;

    for (int i = 0; i < n; ++i)
    {
        const int r = i % rows;
        const int c = i / rows;
        auto cell = juce::Rectangle<int> (area.getX() + c * colW, area.getY() + r * rowH,
                                          colW, rowH).reduced (8, 3);
        l[i]->setBounds (cell.removeFromLeft (96));
        cell.removeFromLeft (6);
        s[i]->setBounds (cell);
    }
}

void SlagblockAudioProcessorEditor::layoutImitation (juce::Rectangle<int> area)
{
    juce::ToggleButton* toggles[] = {
        &imVolOn, &imDriveOn, &imTimeOn, &imFiltOn, &imCrushOn, &imNoiseOn, &imWidthOn
    };
    juce::Slider* sliders[] = {
        &imVolSlider, &imDriveSlider, &imTimeSlider, &imFiltSlider,
        &imCrushSlider, &imNoiseSlider, &imWidthSlider
    };
    juce::Label* labels[] = {
        &imVolLabel, &imDriveLabel, &imTimeLabel, &imFiltLabel,
        &imCrushLabel, &imNoiseLabel, &imWidthLabel
    };

    constexpr int n = 7, cols = 2;
    const int rows = (n + cols - 1) / cols;
    const int colW = area.getWidth() / cols;
    const int rowH = area.getHeight() / rows;

    for (int i = 0; i < n; ++i)
    {
        const int r = i / cols;
        const int c = i % cols;
        auto cell = juce::Rectangle<int> (area.getX() + c * colW, area.getY() + r * rowH,
                                          colW, rowH).reduced (8, 3);
        toggles[i]->setBounds (cell.removeFromLeft (40));
        labels[i]->setBounds (cell.removeFromLeft (64));
        cell.removeFromLeft (4);
        sliders[i]->setBounds (cell);
    }
}

void SlagblockAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r)
{
    if (r.isEmpty())
        return;
    g.setColour (bgPanel);
    g.fillRoundedRectangle (r.toFloat(), 12.0f);
    g.setColour (stroke);
    g.drawRoundedRectangle (r.toFloat(), 12.0f, 1.0f);
}

void SlagblockAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0c0c12));
    g.fillRoundedRectangle (r, 10.0f);

    // Soft cyan glow
    g.setColour (cyan.withAlpha (0.06f));
    g.fillRoundedRectangle (r.reduced (2.0f), 10.0f);
    g.setColour (cyan.withAlpha (0.35f));
    g.drawRoundedRectangle (r, 10.0f, 1.2f);

    const float ratio = readParam (SlagblockAudioProcessor::paramRatio, 2.01f);
    const float index = readParam (SlagblockAudioProcessor::paramIndex, 11.0f);
    const float fdbk  = readParam (SlagblockAudioProcessor::paramFeedback, 0.55f);
    const float sub   = readParam (SlagblockAudioProcessor::paramSubLevel, 0.55f);

    juce::Path wave;
    const int N = juce::jmax (64, (int) r.getWidth());
    const float midY = r.getCentreY();
    const float amp = r.getHeight() * 0.32f;
    float prevMod = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / (float) (N - 1);
        const float ph = t * juce::MathConstants<float>::twoPi * 2.0f + phaseAnim;
        const float mod = std::sin (ph * ratio + prevMod * fdbk * 0.9f);
        prevMod = mod;
        const float fm = std::sin (ph + index * 0.12f * mod);
        const float subW = std::sin (ph * 0.5f) * sub * 0.45f;
        const float y = midY - (fm * 0.72f + subW) * amp;

        const float x = r.getX() + t * r.getWidth();
        if (i == 0)
            wave.startNewSubPath (x, y);
        else
            wave.lineTo (x, y);
    }

    g.setColour (cyan.withAlpha (0.25f));
    g.strokePath (wave, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (cyan);
    g.strokePath (wave, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SlagblockAudioProcessorEditor::drawAdsrShape (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0c0c12));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (stroke);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    float a = readParam (SlagblockAudioProcessor::paramAttack, 0.001f);
    float d = readParam (SlagblockAudioProcessor::paramDecay, 0.05f);
    float s = readParam (SlagblockAudioProcessor::paramSustain, 0.58f);
    float rel = readParam (SlagblockAudioProcessor::paramRelease, 0.08f);

    // Normalize segment widths for display
    a = std::pow (juce::jlimit (0.0005f, 1.5f, a) / 1.5f, 0.45f);
    d = std::pow (juce::jlimit (0.005f, 2.0f, d) / 2.0f, 0.45f);
    rel = std::pow (juce::jlimit (0.005f, 3.0f, rel) / 3.0f, 0.45f);
    const float susW = 0.28f;
    float sum = a + d + susW + rel;
    a /= sum; d /= sum; const float sw = susW / sum; rel /= sum;

    const float x0 = r.getX() + 8.0f;
    const float w = r.getWidth() - 16.0f;
    const float y0 = r.getBottom() - 8.0f;
    const float y1 = r.getY() + 8.0f;
    const float yS = y0 - s * (y0 - y1);

    juce::Path env;
    env.startNewSubPath (x0, y0);
    env.lineTo (x0 + a * w, y1);
    env.lineTo (x0 + (a + d) * w, yS);
    env.lineTo (x0 + (a + d + sw) * w, yS);
    env.lineTo (x0 + w, y0);

    g.setColour (cyan.withAlpha (0.12f));
    juce::Path fill = env;
    fill.lineTo (x0, y0);
    fill.closeSubPath();
    g.fillPath (fill);

    g.setColour (cyan.withAlpha (0.9f));
    g.strokePath (env, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ============================================================================
// Panel visibility (unchanged logic)
// ============================================================================

void SlagblockAudioProcessorEditor::setFxControlsVisible (bool v)
{
    juce::Component* c[] = {
        &fxHeader, &fxDistSlider, &fxBitsSlider, &fxRateSlider,
        &fxDlyTimeSlider, &fxDlyFbSlider, &fxDlyMixSlider, &fxChoMixSlider, &fxChoDepthSlider,
        &fxDistLabel, &fxBitsLabel, &fxRateLabel, &fxDlyTimeLabel, &fxDlyFbLabel,
        &fxDlyMixLabel, &fxChoMixLabel, &fxChoDepthLabel
    };
    for (auto* p : c)
        p->setVisible (v);
}

void SlagblockAudioProcessorEditor::setImControlsVisible (bool v)
{
    juce::Component* c[] = {
        &imHeader, &imVolOn, &imDriveOn, &imTimeOn, &imFiltOn, &imCrushOn, &imNoiseOn, &imWidthOn,
        &imVolSlider, &imDriveSlider, &imTimeSlider, &imFiltSlider, &imCrushSlider, &imNoiseSlider, &imWidthSlider,
        &imVolLabel, &imDriveLabel, &imTimeLabel, &imFiltLabel, &imCrushLabel, &imNoiseLabel, &imWidthLabel
    };
    for (auto* p : c)
        p->setVisible (v);
}

void SlagblockAudioProcessorEditor::updateWindowSize()
{
    int h = kHMain;
    if (fxPanelVisible) h += kFxExtra;
    if (imPanelVisible) h += kImExtra;
    setSize (kW, h);
}

void SlagblockAudioProcessorEditor::setFxPanelVisible (bool show)
{
    fxPanelVisible = show;
    setFxControlsVisible (show);
    audioProcessor.setFxProcessingEnabled (show);
    updateWindowSize();
    resized();
}

void SlagblockAudioProcessorEditor::setImitationPanelVisible (bool show)
{
    imPanelVisible = show;
    setImControlsVisible (show);
    audioProcessor.setImitationEnabled (show);
    updateWindowSize();
    resized();
}

void SlagblockAudioProcessorEditor::syncPanelsFromProcessor()
{
    auto& apvts = audioProcessor.getAPVTS();
    const bool fxOn = apvts.getRawParameterValue (SlagblockAudioProcessor::paramFxOn)
                   && apvts.getRawParameterValue (SlagblockAudioProcessor::paramFxOn)->load() > 0.5f;
    const bool imOn = apvts.getRawParameterValue (SlagblockAudioProcessor::paramImOn)
                   && apvts.getRawParameterValue (SlagblockAudioProcessor::paramImOn)->load() > 0.5f;

    if (fxOn != fxPanelVisible)
    {
        fxButton.setToggleState (fxOn, juce::dontSendNotification);
        fxPanelVisible = fxOn;
        setFxControlsVisible (fxOn);
    }
    if (imOn != imPanelVisible)
    {
        imitationButton.setToggleState (imOn, juce::dontSendNotification);
        imPanelVisible = imOn;
        setImControlsVisible (imOn);
    }
    updateWindowSize();
    resized();
}

void SlagblockAudioProcessorEditor::savePresetClicked()
{
    const auto dir = SlagblockAudioProcessor::getPresetsDirectory();
    const auto def = audioProcessor.getLastPresetName().isNotEmpty()
                         ? audioProcessor.getLastPresetName() + ".slagpreset"
                         : "NeuroPatch.slagpreset";

    fileChooser = std::make_unique<juce::FileChooser> ("Save Preset", dir.getChildFile (def),
                                                       "*.slagpreset;*.xml");
    constexpr auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != juce::File{} && audioProcessor.savePresetToFile (f))
            statusLabel.setText ("Saved: " + f.getFileName(), juce::dontSendNotification);
    });
}

void SlagblockAudioProcessorEditor::loadPresetClicked()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Load Preset",
                                                       SlagblockAudioProcessor::getPresetsDirectory(),
                                                       "*.slagpreset;*.xml");
    constexpr auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != juce::File{} && audioProcessor.loadPresetFromFile (f))
        {
            statusLabel.setText ("Loaded: " + f.getFileName(), juce::dontSendNotification);
            syncPanelsFromProcessor();
        }
    });
}

// ============================================================================
// Paint / Resize
// ============================================================================

void SlagblockAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgDeep);

    // Header
    g.setColour (bgHeader);
    g.fillRect (headerBar);
    g.setColour (cyan.withAlpha (0.4f));
    g.fillRect (headerBar.getX() + kPad, headerBar.getBottom() - 2, 100, 2);

    drawPanel (g, leftCol);
    drawPanel (g, centerCol);
    drawPanel (g, rightCol);
    drawPanel (g, knobsSec);

    if (fxPanelVisible)
        drawPanel (g, fxSec);
    if (imPanelVisible)
        drawPanel (g, imSec);

    // Scope + ADSR
    if (! scopeArea.isEmpty())
        drawWaveform (g, scopeArea.toFloat());
    if (! adsrArea.isEmpty())
        drawAdsrShape (g, adsrArea.toFloat());
}

void SlagblockAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ---- Header (single line of buttons) ----
    headerBar = bounds.removeFromTop (56);
    auto h = headerBar.reduced (kPad, 10);

    titleLabel.setBounds (h.removeFromLeft (150));
    tagLabel.setBounds (h.removeFromLeft (90).withTrimmedTop (4));

    // Buttons on the right side of header
    auto rb = h.removeFromRight (720);
    statusLabel.setBounds (h.withTrimmedTop (8));

    auto placeBtn = [&] (juce::TextButton& b, int w)
    {
        b.setBounds (rb.removeFromLeft (w).withHeight (kBtnH));
        rb.removeFromLeft (6);
    };

    placeBtn (randomButton, 86);
    placeBtn (fxButton, 56);
    placeBtn (randomFxButton, 96);
    placeBtn (imitationButton, 96);
    placeBtn (randomImButton, 96);
    placeBtn (savePresetButton, 70);
    placeBtn (loadPresetButton, 70);
    placeBtn (undoButton, 64);
    placeBtn (redoButton, 64);

    bounds.reduce (kPad, kPad);

    // ---- Bottom expandable sections ----
    if (imPanelVisible)
    {
        imSec = bounds.removeFromBottom (kImExtra - 24);
        bounds.removeFromBottom (kGap);
    }
    else imSec = {};

    if (fxPanelVisible)
    {
        fxSec = bounds.removeFromBottom (kFxExtra - 24);
        bounds.removeFromBottom (kGap);
    }
    else fxSec = {};

    // Knobs strip
    knobsSec = bounds.removeFromBottom (130);
    bounds.removeFromBottom (kGap);

    // ---- Three columns (equal side widths for mirror layout) ----
    const int colGap = kGap;
    const int usable = bounds.getWidth() - colGap * 2;
    const int sideW = usable * 29 / 100;
    const int centerW = usable - sideW * 2;

    leftCol = bounds.removeFromLeft (sideW);
    bounds.removeFromLeft (colGap);
    centerCol = bounds.removeFromLeft (centerW);
    bounds.removeFromLeft (colGap);
    rightCol = bounds; // same width as leftCol

    auto padBody = [] (juce::Rectangle<int> r)
    {
        return r.reduced (14, 12);
    };

    // Shared vertical proportions for left / right columns
    const int colInnerH = padBody (leftCol).getHeight();
    const int blockTopH = colInnerH * 36 / 100; // OSC / FILTER
    const int blockMidH = colInnerH * 22 / 100; // SUB / (LFO header+sync area starts)
    const int sectionGap = 8;
    const int headerH = 22;

    // LEFT: OSC → SUB → ENV
    {
        auto a = padBody (leftCol);

        auto osc = a.removeFromTop (blockTopH);
        oscHeader.setBounds (osc.removeFromTop (headerH));
        juce::Slider* s1[] = { &ratioSlider, &indexSlider, &feedbackSlider, &driveSlider };
        juce::Label*  l1[] = { &ratioLabel, &indexLabel, &feedbackLabel, &driveLabel };
        layoutHList (osc, s1, l1, 4);

        a.removeFromTop (sectionGap);
        auto sub = a.removeFromTop (blockMidH);
        subHeader.setBounds (sub.removeFromTop (headerH));
        juce::Slider* s2[] = { &subLevelSlider, &subOctaveSlider };
        juce::Label*  l2[] = { &subLevelLabel, &subOctaveLabel };
        layoutHList (sub, s2, l2, 2);

        a.removeFromTop (sectionGap);
        auto env = a;
        envHeader.setBounds (env.removeFromTop (headerH));
        juce::Slider* s3[] = { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider };
        juce::Label*  l3[] = { &attackLabel, &decayLabel, &sustainLabel, &releaseLabel };
        layoutHList (env, s3, l3, 4);
    }

    // CENTER: scope + adsr viz
    {
        auto a = padBody (centerCol);
        scopeArea = a.removeFromTop (a.getHeight() * 68 / 100).reduced (4);
        a.removeFromTop (10);
        adsrArea = a.reduced (4);
    }

    // RIGHT: FILTER (mirror of OSC height) → LFO (clean rows)
    {
        auto a = padBody (rightCol);

        // FILTER — same height band as OSCILLATOR / FM
        auto filt = a.removeFromTop (blockTopH);
        filterHeader.setBounds (filt.removeFromTop (headerH));
        juce::Slider* s1[] = { &cutoffSlider, &resonanceSlider, &noiseSlider };
        juce::Label*  l1[] = { &cutoffLabel, &resonanceLabel, &noiseLabel };
        layoutHList (filt, s1, l1, 3);

        a.removeFromTop (sectionGap);
        auto lfo = a;
        lfoHeader.setBounds (lfo.removeFromTop (headerH));

        // Equal rows for remaining LFO content
        const int rows = 7; // sync, rate, cut, idx, drive, amp toggle+depth line1, depth line optional merge
        const int rowH = juce::jmax (24, lfo.getHeight() / rows);

        // Row: Tempo Sync + Grid
        {
            auto row = lfo.removeFromTop (rowH).reduced (0, 2);
            lfoSyncLabel.setBounds (row.removeFromLeft (kLabelW));
            row.removeFromLeft (6);
            lfoSyncToggle.setBounds (row.removeFromLeft (64));
            row.removeFromLeft (10);
            lfoDivLabel.setBounds (row.removeFromLeft (kLabelW));
            row.removeFromLeft (6);
            lfoDivisionBox.setBounds (row);
        }

        // Row: Rate Hz
        {
            auto row = lfo.removeFromTop (rowH).reduced (0, 2);
            lfoRateLabel.setBounds (row.removeFromLeft (kLabelW));
            row.removeFromLeft (6);
            lfoRateSlider.setBounds (row);
        }

        // Modulation block: Cutoff / Index / Drive (same style as left sliders)
        juce::Slider* destS[] = { &lfoCutoffSlider, &lfoIndexSlider, &lfoDriveSlider };
        juce::Label*  destL[] = { &lfoCutoffLabel,  &lfoIndexLabel,  &lfoDriveLabel };
        layoutHList (lfo.removeFromTop (rowH * 3), destS, destL, 3);

        // Amp enable + Amp Depth on one clean row
        {
            auto row = lfo.removeFromTop (rowH).reduced (0, 2);
            lfoToAmpToggle.setBounds (row.removeFromLeft (56));
            row.removeFromLeft (8);
            lfoAmpDepthLabel.setBounds (row.removeFromLeft (kLabelW));
            row.removeFromLeft (6);
            lfoAmpDepthSlider.setBounds (row);
        }

        updateLfoRateEnabled();
    }

    // MAIN knobs
    {
        auto a = padBody (knobsSec);
        knobsHeader.setBounds (a.removeFromTop (20));
        a.removeFromTop (4);
        juce::Slider* k[] = {
            &knobCutoff, &knobReso, &knobDrive, &knobIndex, &knobFeedback, &knobSub
        };
        juce::Label* l[] = {
            &knobCutoffLab, &knobResoLab, &knobDriveLab, &knobIndexLab, &knobFeedbackLab, &knobSubLab
        };
        layoutKnobRow (a, k, l, 6);
    }

    if (fxPanelVisible)
    {
        auto a = padBody (fxSec);
        fxHeader.setBounds (a.removeFromTop (22));
        a.removeFromTop (4);
        layoutFxGrid (a);
    }

    if (imPanelVisible)
    {
        auto a = padBody (imSec);
        imHeader.setBounds (a.removeFromTop (22));
        a.removeFromTop (4);
        layoutImitation (a);
    }
}

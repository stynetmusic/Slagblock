#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace Col
{
    using C = juce::Colour;
    const C frame     { SlagLookAndFeel::kFrame };
    const C frameHi   { SlagLookAndFeel::kFrameHi };
    const C header    { SlagLookAndFeel::kHeader };
    const C panel     { SlagLookAndFeel::kPanel };
    const C panelHi   { SlagLookAndFeel::kPanelHi };
    const C panelLo   { SlagLookAndFeel::kPanelLo };
    const C inset     { SlagLookAndFeel::kInset };
    const C border    { SlagLookAndFeel::kBorder };
    const C borderHi  { SlagLookAndFeel::kBorderHi };
    const C text      { SlagLookAndFeel::kText };
    const C textDim   { SlagLookAndFeel::kTextDim };
    const C textMute  { SlagLookAndFeel::kTextMute };
    const C gold      { SlagLookAndFeel::kGold };
    const C goldHi    { SlagLookAndFeel::kGoldHi };
    const C bronze    { SlagLookAndFeel::kBronze };
    const C bronzeHi  { SlagLookAndFeel::kBronzeHi };
    const C tealKnob  { SlagLookAndFeel::kTealKnob };
    const C tealKnobHi{ SlagLookAndFeel::kTealKnobHi };
    const C faderTrack{ SlagLookAndFeel::kFaderTrack };
    const C badge     { SlagLookAndFeel::kBadge };
}

namespace Layout
{
    constexpr int W = 1280;
    constexpr int H = 780;
    constexpr int pad = 12;
    constexpr int gap = 10;
    constexpr int topH = 96;
    constexpr int botH = 52;
}

// ============================================================================
// LookAndFeel — matches refer.png PHASEDYN
// ============================================================================

SlagLookAndFeel::SlagLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, Col::goldHi);
    setColour (juce::Slider::trackColourId, Col::gold);
    setColour (juce::Slider::backgroundColourId, Col::faderTrack);
    setColour (juce::TextButton::buttonColourId, Col::badge);
    setColour (juce::TextButton::textColourOffId, Col::text);
    setColour (juce::Label::textColourId, Col::textDim);
    setColour (juce::ComboBox::backgroundColourId, Col::inset);
    setColour (juce::ComboBox::outlineColourId, Col::border);
    setColour (juce::ComboBox::textColourId, Col::text);
    setColour (juce::ComboBox::arrowColourId, Col::gold);
    setColour (juce::PopupMenu::backgroundColourId, Col::frameHi);
    setColour (juce::PopupMenu::textColourId, Col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Col::gold.withAlpha (0.22f));
    setColour (juce::PopupMenu::highlightedTextColourId, Col::goldHi);
}

void SlagLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float startAng, float endAng, juce::Slider& s)
{
    const bool hover = s.isMouseOverOrDragging();
    const bool en = s.isEnabled();
    const bool teal = s.getComponentID() == kTealKnobId;

    const float size = (float) juce::jmin (w, h);
    // Big knobs when area is large (filter cutoff style)
    const float radius = size * (size > 90.0f ? 0.40f : 0.36f);
    const float cx = (float) x + (float) w * 0.5f;
    const float cy = (float) y + (float) h * 0.40f;
    const float ang = startAng + pos * (endAng - startAng);

    // Soft drop shadow under knob
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (cx - radius + 1.5f, cy - radius + 3.0f, radius * 2.0f, radius * 2.0f);

    // Outer metal body
    if (teal)
    {
        juce::ColourGradient body (Col::tealKnobHi.brighter (hover ? 0.12f : 0.0f),
                                   cx - radius * 0.5f, cy - radius,
                                   Col::tealKnob.darker (0.35f), cx + radius * 0.3f, cy + radius, true);
        g.setGradientFill (body);
    }
    else
    {
        juce::ColourGradient body (Col::bronzeHi.brighter (hover ? 0.15f : 0.05f),
                                   cx - radius * 0.4f, cy - radius,
                                   Col::bronze.darker (0.25f), cx + radius * 0.2f, cy + radius, true);
        g.setGradientFill (body);
    }
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Inner ring / bevel
    const float ir = radius * 0.78f;
    {
        juce::ColourGradient inner (juce::Colours::black.withAlpha (0.25f), cx, cy - ir,
                                    juce::Colours::white.withAlpha (0.08f), cx, cy + ir, false);
        g.setGradientFill (inner);
        g.fillEllipse (cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);
    }

    // Center cap
    const float cr = radius * 0.55f;
    if (teal)
    {
        juce::ColourGradient cap (Col::tealKnobHi, cx, cy - cr, Col::tealKnob.darker (0.2f), cx, cy + cr, false);
        g.setGradientFill (cap);
    }
    else
    {
        juce::ColourGradient cap (Col::bronzeHi.brighter (0.1f), cx, cy - cr,
                                  Col::bronze.darker (0.15f), cx, cy + cr, false);
        g.setGradientFill (cap);
    }
    g.fillEllipse (cx - cr, cy - cr, cr * 2.0f, cr * 2.0f);

    // Rim highlight
    g.setColour ((teal ? Col::tealKnobHi : Col::gold).withAlpha (en ? (hover ? 0.55f : 0.28f) : 0.12f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.4f);

    // Gold indicator line (like PHASEDYN)
    const float pr = radius * 0.62f;
    const float px = cx + std::cos (ang - juce::MathConstants<float>::halfPi) * pr;
    const float py = cy + std::sin (ang - juce::MathConstants<float>::halfPi) * pr;
    g.setColour (en ? Col::goldHi : Col::gold.withAlpha (0.3f));
    g.drawLine (cx, cy, px, py, 2.4f);

    // Center pin
    g.setColour (Col::gold.withAlpha (en ? 0.9f : 0.3f));
    g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);

}

void SlagLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float /*min*/, float /*max*/,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    const bool hover = s.isMouseOverOrDragging();
    const bool en = s.isEnabled();
    const bool vert = style == juce::Slider::LinearVertical
                   || style == juce::Slider::LinearBarVertical;

    if (vert)
    {
        // PHASEDYN-style vertical fader: dark track + metal cap
        const float trackW = 7.0f;
        const float cx = (float) x + (float) width * 0.5f;
        auto track = juce::Rectangle<float> (cx - trackW * 0.5f, (float) y + 6.0f,
                                             trackW, (float) height - 12.0f);
        g.setColour (Col::faderTrack);
        g.fillRoundedRectangle (track, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (track, 3.0f, 1.0f);

        // Gold fill from bottom
        const float fillH = juce::jmax (0.0f, track.getBottom() - sliderPos);
        if (fillH > 1.0f)
        {
            auto fill = juce::Rectangle<float> (track.getX(), sliderPos, trackW, fillH);
            juce::ColourGradient gr (Col::goldHi, fill.getX(), fill.getBottom(),
                                     Col::gold.darker (0.2f), fill.getX(), fill.getY(), false);
            g.setGradientFill (gr);
            g.setOpacity (en ? 0.85f : 0.3f);
            g.fillRoundedRectangle (fill, 3.0f);
            g.setOpacity (1.0f);
        }

        // Rectangular metal thumb (PHASEDYN style)
        const float th = 14.0f;
        const float tw = 18.0f;
        auto thumb = juce::Rectangle<float> (cx - tw * 0.5f, sliderPos - th * 0.5f, tw, th);
        {
            juce::ColourGradient tg (Col::bronzeHi.brighter (hover ? 0.2f : 0.05f),
                                     thumb.getX(), thumb.getY(),
                                     Col::bronze.darker (0.2f),
                                     thumb.getX(), thumb.getBottom(), false);
            g.setGradientFill (tg);
            g.setOpacity (en ? 1.0f : 0.4f);
            g.fillRoundedRectangle (thumb, 2.5f);
            g.setOpacity (1.0f);
        }
        g.setColour (Col::gold.withAlpha (en ? 0.5f : 0.2f));
        g.drawRoundedRectangle (thumb, 2.5f, 1.0f);
        // grip lines
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        for (int i = -1; i <= 1; ++i)
            g.fillRect (thumb.getX() + 4.0f, thumb.getCentreY() + (float) i * 3.0f - 0.5f,
                        thumb.getWidth() - 8.0f, 1.0f);
        return;
    }

    // Horizontal — subtle gold track
    const float trackH = 5.0f;
    const float cy = (float) y + (float) height * 0.5f;
    auto track = juce::Rectangle<float> ((float) x + 2.0f, cy - trackH * 0.5f, (float) width - 4.0f, trackH);
    g.setColour (Col::faderTrack);
    g.fillRoundedRectangle (track, 2.5f);

    const float fillW = juce::jmax (0.0f, sliderPos - track.getX());
    if (fillW > 1.0f)
    {
        g.setColour (Col::gold.withAlpha (en ? 0.85f : 0.3f));
        g.fillRoundedRectangle (track.withWidth (juce::jmin (fillW, track.getWidth())), 2.5f);
    }

    const float thr = hover ? 7.5f : 6.5f;
    {
        juce::ColourGradient tg (Col::bronzeHi, sliderPos, cy - thr,
                                 Col::bronze, sliderPos, cy + thr, false);
        g.setGradientFill (tg);
        g.setOpacity (en ? 1.0f : 0.35f);
        g.fillEllipse (sliderPos - thr, cy - thr, thr * 2.0f, thr * 2.0f);
        g.setOpacity (1.0f);
    }
    g.setColour (Col::gold.withAlpha (en ? 0.6f : 0.2f));
    g.drawEllipse (sliderPos - thr, cy - thr, thr * 2.0f, thr * 2.0f, 1.1f);
}

void SlagLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();
    auto bg = backgroundColour;

    const bool isAuto = button.getComponentID() == kAutoBtnId;

    if (isAuto)
    {
        auto fill = on ? Col::gold : Col::badge.brighter (0.08f);
        if (down) fill = fill.brighter (0.08f);
        else if (highlighted) fill = fill.brighter (0.10f);

        {
            juce::ColourGradient gr (fill.brighter (on ? 0.18f : 0.06f), r.getX(), r.getY(),
                                     fill.darker (on ? 0.08f : 0.18f), r.getX(), r.getBottom(), false);
            g.setGradientFill (gr);
            g.fillRoundedRectangle (r, 8.0f);
        }
        g.setColour (on ? Col::goldHi : Col::gold.withAlpha (highlighted ? 0.75f : 0.45f));
        g.drawRoundedRectangle (r, 8.0f, on ? 2.0f : 1.4f);
        return;
    }

    if (on) bg = Col::gold.withAlpha (0.22f);
    else if (down) bg = bg.brighter (0.1f);
    else if (highlighted) bg = bg.brighter (0.06f);

    {
        juce::ColourGradient gr (bg.brighter (0.08f), r.getX(), r.getY(),
                                 bg.darker (0.1f), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, 6.0f);
    }
    g.setColour (on || highlighted ? Col::gold.withAlpha (0.65f) : Col::border.withAlpha (0.8f));
    g.drawRoundedRectangle (r, 6.0f, 1.1f);
}

void SlagLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool highlighted, bool /*down*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    // PHASEDYN Sync-style rounded rect button
    auto r = bounds.reduced (1.0f);
    const bool on = button.getToggleState();

    g.setColour (on ? Col::gold.withAlpha (0.28f) : Col::inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on || highlighted ? Col::gold.withAlpha (0.7f) : Col::borderHi.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    g.setColour (on ? Col::goldHi : Col::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), r, juce::Justification::centred, true);
}

void SlagLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    const float ax = (float) width - 14.0f;
    const float cy = (float) height * 0.5f;
    juce::Path arrow;
    arrow.addTriangle (ax - 4.0f, cy - 2.0f, ax + 4.0f, cy - 2.0f, ax, cy + 3.5f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void SlagLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.setColour (Col::frameHi);
    g.fillRect (0, 0, width, height);
    g.setColour (Col::border);
    g.drawRect (0, 0, width, height, 1);
}

void SlagLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool, const juce::String& text,
                                         const juce::String&, const juce::Drawable*,
                                         const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (Col::border);
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    if (isHighlighted && isActive)
    {
        g.setColour (Col::gold.withAlpha (0.18f));
        g.fillRoundedRectangle (area.toFloat().reduced (2.0f, 1.0f), 4.0f);
    }
    auto ta = area.reduced (10, 0);
    if (isTicked)
    {
        g.setColour (Col::gold);
        g.fillEllipse ((float) ta.getX(), (float) ta.getCentreY() - 3.0f, 6.0f, 6.0f);
        ta.removeFromLeft (14);
    }
    g.setColour (isHighlighted ? Col::goldHi : Col::text);
    g.setFont (juce::FontOptions (12.5f));
    g.drawText (text, ta, juce::Justification::centredLeft, true);
}

juce::Font SlagLookAndFeel::getTextButtonFont (juce::TextButton&, int h)
{
    return juce::Font (juce::FontOptions (h >= 34 ? 12.5f : 11.5f, juce::Font::bold));
}
juce::Font SlagLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (11.0f));
}
juce::Font SlagLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (11.5f));
}

// ============================================================================
// Editor
// ============================================================================

SlagblockAudioProcessorEditor::SlagblockAudioProcessorEditor (SlagblockAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    logoLabel.setText ("SLAGBLOCK", juce::dontSendNotification);
    logoLabel.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    logoLabel.setColour (juce::Label::textColourId, Col::goldHi);
    logoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (logoLabel);

    tagLabel.setText ("NEURO FM  ·  VST3", juce::dontSendNotification);
    tagLabel.setFont (juce::FontOptions (10.0f));
    tagLabel.setColour (juce::Label::textColourId, Col::textMute);
    addAndMakeVisible (tagLabel);

    presetLabel.setText ("Init", juce::dontSendNotification);
    presetLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, Col::text);
    presetLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetLabel);

    setupSectionTitle (oscTitle,    "FM OSCILLATOR");
    setupSectionTitle (subTitle,    "SUB OSCILLATOR");
    setupSectionTitle (filterTitle, "FILTER");
    setupSectionTitle (envTitle,    "AMP ENV");
    setupSectionTitle (lfoTitle,    "LFO");
    setupSectionTitle (fxTitle,     "FX");
    setupSectionTitle (shaperTitle, "IMITATION / SHAPER");
    setupSectionTitle (vizTitle,    "VISUALIZATION");

    setupBadge (oscBadge,    "OSC");
    setupBadge (subBadge,    "SUB");
    setupBadge (filterBadge, "FLT");
    setupBadge (envBadge,    "ADSR");
    setupBadge (lfoBadge,    "LFO");
    setupBadge (fxBadge,     "FX");
    setupBadge (shaperBadge, "SHP");
    setupBadge (vizBadge,    "VIZ");

    setupButton (btnSave, "Save");
    setupButton (btnLoad, "Load");
    setupButton (btnUndo, "Undo");
    setupButton (btnRedo, "Redo");
    setupButton (btnRandom, "Random");
    setupButton (btnRandomFx, "Random FX");
    setupButton (btnFx, "FX", true);
    setupButton (btnShaper, "Imitation", true);
    setupButton (btnRandomIm, "Rand IM");
    setupButton (btnAuto, "AUTO", true);
    btnAuto.setComponentID (SlagLookAndFeel::kAutoBtnId);
    btnAuto.setColour (juce::TextButton::textColourOffId, Col::goldHi);
    btnAuto.setColour (juce::TextButton::textColourOnId, Col::frame);

    btnRandom.onClick   = [this] { proc.randomizeNeuroPatch(); };
    btnRandomFx.onClick = [this] {
        if (! fxOn) { btnFx.setToggleState (true, juce::dontSendNotification); setFxOn (true); }
        proc.randomizeFXOnly();
    };
    btnRandomIm.onClick = [this] {
        if (! imOn) { btnShaper.setToggleState (true, juce::dontSendNotification); setImOn (true); }
        proc.randomizeImitationOnly();
    };
    btnFx.onClick     = [this] { setFxOn (btnFx.getToggleState()); };
    btnShaper.onClick = [this] { setImOn (btnShaper.getToggleState()); };
    btnSave.onClick   = [this] { savePresetClicked(); };
    btnLoad.onClick   = [this] { loadPresetClicked(); };
    btnUndo.onClick   = [this] { if (proc.undo()) syncFromProcessor(); };
    btnRedo.onClick   = [this] { if (proc.redo()) syncFromProcessor(); };

    proc.setStateChangeCallback ([this] {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SlagblockAudioProcessorEditor> (this)] {
            if (safe) safe->syncFromProcessor();
        });
    });

    // Bronze knobs for osc/sub/lfo
    setupKnob (ratioKnob,    ratioLab,    "Ratio");
    setupKnob (indexKnob,    indexLab,    "Index");
    setupKnob (driveKnob,    driveLab,    "Drive");
    setupKnob (feedbackKnob, feedbackLab, "Feedback");
    setupKnob (subLevelKnob,  subLevelLab,  "Level");
    setupKnob (subOctaveKnob, subOctaveLab, "Octave");

    // Teal knobs for filter (like PHASEDYN Cutoff/Res)
    setupKnob (cutoffKnob, cutoffLab, "Cutoff", true);
    setupKnob (resoKnob,   resoLab,   "Resonance", true);
    setupKnob (noiseKnob,  noiseLab,  "Noise", true);

    setupVFader (attackFader,  attackLab,  "Attack");
    setupVFader (decayFader,   decayLab,   "Decay");
    setupVFader (sustainFader, sustainLab, "Sustain");
    setupVFader (releaseFader, releaseLab, "Release");

    setupKnob (lfoRateKnob, lfoRateLab, "Rate");
    setupKnob (lfoCutKnob,  lfoCutLab,  "Cut Amt");
    setupKnob (lfoIdxKnob,  lfoIdxLab,  "Idx Amt");
    setupKnob (lfoDrvKnob,  lfoDrvLab,  "Drv Amt");
    setupKnob (lfoAmpKnob,  lfoAmpLab,  "Amp Amt");
    setupToggle (lfoSyncBtn, "Sync");
    setupToggle (lfoAmpBtn,  "Amp");
    lfoSyncLab.setVisible (false);
    lfoGridLab.setText ("Grid", juce::dontSendNotification);
    lfoGridLab.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    lfoGridLab.setColour (juce::Label::textColourId, Col::textMute);
    addAndMakeVisible (lfoGridLab);
    lfoGridBox.addItemList (SlagblockAudioProcessor::getLfoDivisionNames(), 1);
    lfoGridBox.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lfoGridBox);
    lfoSyncBtn.onClick = [this] { updateLfoRateEnabled(); };

    // Teal FX knobs like reference Delay/Reverb/Chorus row
    setupKnob (fxDist,   fxDistLab,   "Dist", true);
    setupKnob (fxBits,   fxBitsLab,   "Bits", true);
    setupKnob (fxRate,   fxRateLab,   "Rate", true);
    setupKnob (fxDly,    fxDlyLab,    "Delay", true);
    setupKnob (fxDlyFb,  fxDlyFbLab,  "FB", true);
    setupKnob (fxDlyMix, fxDlyMixLab, "Dly Mix", true);
    setupKnob (fxCho,    fxChoLab,    "Chorus", true);
    setupKnob (fxChoDep, fxChoDepLab, "Depth", true);

    setupToggle (imVolOn, " "); setupToggle (imDriveOn, " "); setupToggle (imTimeOn, " ");
    setupToggle (imFiltOn, " "); setupToggle (imCrushOn, " "); setupToggle (imNoiseOn, " ");
    setupToggle (imWidthOn, " ");
    setupHSlider (imVol,   imVolLab,   "Vol");
    setupHSlider (imDrive, imDriveLab, "Drive");
    setupHSlider (imTime,  imTimeLab,  "Time");
    setupHSlider (imFilt,  imFiltLab,  "Filt");
    setupHSlider (imCrush, imCrushLab, "Crush");
    setupHSlider (imNoise, imNoiseLab, "Noise");
    setupHSlider (imWidth, imWidthLab, "Width");

    setupKnob (autoSpeedKnob, autoSpeedLab, "Speed");
    setupKnob (autoDepthKnob, autoDepthLab, "Depth");
    setupKnob (autoChaosKnob, autoChaosLab, "Chaos");

    auto& ap = proc.getAPVTS();
    using P = SlagblockAudioProcessor;

    aRatio    = std::make_unique<SA> (ap, P::paramRatio, ratioKnob);
    aIndex    = std::make_unique<SA> (ap, P::paramIndex, indexKnob);
    aDrive    = std::make_unique<SA> (ap, P::paramDrive, driveKnob);
    aFeedback = std::make_unique<SA> (ap, P::paramFeedback, feedbackKnob);
    aCut      = std::make_unique<SA> (ap, P::paramCutoff, cutoffKnob);
    aReso     = std::make_unique<SA> (ap, P::paramResonance, resoKnob);
    aNoise    = std::make_unique<SA> (ap, P::paramNoise, noiseKnob);
    aAtt      = std::make_unique<SA> (ap, P::paramAttack, attackFader);
    aDec      = std::make_unique<SA> (ap, P::paramDecay, decayFader);
    aSus      = std::make_unique<SA> (ap, P::paramSustain, sustainFader);
    aRel      = std::make_unique<SA> (ap, P::paramRelease, releaseFader);
    aSubLvl   = std::make_unique<SA> (ap, P::paramSubLevel, subLevelKnob);
    aSubOct   = std::make_unique<SA> (ap, P::paramSubOctave, subOctaveKnob);
    aLfoRate  = std::make_unique<SA> (ap, P::paramLfoRate, lfoRateKnob);
    aLfoSync  = std::make_unique<BA> (ap, P::paramLfoSync, lfoSyncBtn);
    aLfoGrid  = std::make_unique<CA> (ap, P::paramLfoDivision, lfoGridBox);
    aLfoCut   = std::make_unique<SA> (ap, P::paramLfoCutoff, lfoCutKnob);
    aLfoIdx   = std::make_unique<SA> (ap, P::paramLfoIndex, lfoIdxKnob);
    aLfoDrv   = std::make_unique<SA> (ap, P::paramLfoDrive, lfoDrvKnob);
    aLfoToAmp = std::make_unique<BA> (ap, P::paramLfoToAmp, lfoAmpBtn);
    aLfoAmp   = std::make_unique<SA> (ap, P::paramLfoAmpDepth, lfoAmpKnob);

    aFxDist   = std::make_unique<SA> (ap, P::paramFxDist, fxDist);
    aFxBits   = std::make_unique<SA> (ap, P::paramFxCrushBits, fxBits);
    aFxRate   = std::make_unique<SA> (ap, P::paramFxCrushRate, fxRate);
    aFxDly    = std::make_unique<SA> (ap, P::paramFxDelayTime, fxDly);
    aFxDlyFb  = std::make_unique<SA> (ap, P::paramFxDelayFb, fxDlyFb);
    aFxDlyMix = std::make_unique<SA> (ap, P::paramFxDelayMix, fxDlyMix);
    aFxCho    = std::make_unique<SA> (ap, P::paramFxChorusMix, fxCho);
    aFxChoDep = std::make_unique<SA> (ap, P::paramFxChorusDepth, fxChoDep);

    aImVol     = std::make_unique<SA> (ap, P::paramImVol, imVol);
    aImDrive   = std::make_unique<SA> (ap, P::paramImDrive, imDrive);
    aImTime    = std::make_unique<SA> (ap, P::paramImTime, imTime);
    aImFilt    = std::make_unique<SA> (ap, P::paramImFilt, imFilt);
    aImCrush   = std::make_unique<SA> (ap, P::paramImCrush, imCrush);
    aImNoise   = std::make_unique<SA> (ap, P::paramImNoise, imNoise);
    aImWidth   = std::make_unique<SA> (ap, P::paramImWidth, imWidth);
    aImVolOn   = std::make_unique<BA> (ap, P::paramImVolOn, imVolOn);
    aImDriveOn = std::make_unique<BA> (ap, P::paramImDriveOn, imDriveOn);
    aImTimeOn  = std::make_unique<BA> (ap, P::paramImTimeOn, imTimeOn);
    aImFiltOn  = std::make_unique<BA> (ap, P::paramImFiltOn, imFiltOn);
    aImCrushOn = std::make_unique<BA> (ap, P::paramImCrushOn, imCrushOn);
    aImNoiseOn = std::make_unique<BA> (ap, P::paramImNoiseOn, imNoiseOn);
    aImWidthOn = std::make_unique<BA> (ap, P::paramImWidthOn, imWidthOn);

    aAutoOn    = std::make_unique<BA> (ap, P::paramAutoOn,    btnAuto);
    aAutoSpeed = std::make_unique<SA> (ap, P::paramAutoSpeed, autoSpeedKnob);
    aAutoDepth = std::make_unique<SA> (ap, P::paramAutoDepth, autoDepthKnob);
    aAutoChaos = std::make_unique<SA> (ap, P::paramAutoChaos, autoChaosKnob);

    updateLfoRateEnabled();

    if (auto* v = ap.getRawParameterValue (P::paramFxOn); v && v->load() > 0.5f)
    {
        btnFx.setToggleState (true, juce::dontSendNotification);
        fxOn = true; proc.setFxProcessingEnabled (true);
    }
    if (auto* v = ap.getRawParameterValue (P::paramImOn); v && v->load() > 0.5f)
    {
        btnShaper.setToggleState (true, juce::dontSendNotification);
        imOn = true; proc.setImitationEnabled (true);
    }

    startTimerHz (40);
    setSize (Layout::W, Layout::H);
    setResizable (true, true);
    setResizeLimits (1100, 700, 1700, 1000);
}

SlagblockAudioProcessorEditor::~SlagblockAudioProcessorEditor()
{
    stopTimer();
    proc.setStateChangeCallback ({});
    setLookAndFeel (nullptr);
}

// ============================================================================
// Setup
// ============================================================================

void SlagblockAudioProcessorEditor::timerCallback()
{
    phase += 0.048f;
    spectrumPhase += 0.025f;
    if (phase > juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;

    static int tick = 0;
    if ((++tick % 12) == 0)
        proc.pushUndoSnapshot ("Parameter edit");

    btnUndo.setEnabled (proc.canUndo());
    btnRedo.setEnabled (proc.canRedo());
    updateLfoRateEnabled();

    const auto name = proc.getLastPresetName();
    if (name.isNotEmpty())
        presetLabel.setText (name, juce::dontSendNotification);

    if (! vizPanel.isEmpty()) repaint (vizPanel.expanded (2));
    if (! envPanel.isEmpty()) repaint (envPanel.expanded (2));
    if (! filterPanel.isEmpty()) repaint (filterPanel.expanded (2));

    const bool autoOn = proc.isAutoOn();
    const bool autoLive = proc.isAutoAudible();
    if (autoOn || autoLive)
        proc.tickAutoVisual (0.025f);

    applyLiveVisuals();

    if (autoLive || lastAutoAudible)
    {
        if (! autoCluster.isEmpty())
            repaint (autoCluster.expanded (12));
        repaint (oscPanel);
        repaint (filterPanel);
        repaint (fxPanel);
        repaint (lfoPanel);
        repaint (subPanel);
        repaint (shaperPanel);
    }
    lastAutoAudible = autoLive;
}

void SlagblockAudioProcessorEditor::setupSectionTitle (juce::Label& lab, const juce::String& text)
{
    lab.setText (text, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    lab.setColour (juce::Label::textColourId, Col::text);
    lab.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupBadge (juce::Label& lab, const juce::String& text)
{
    lab.setText (text, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    lab.setColour (juce::Label::textColourId, Col::textDim);
    lab.setColour (juce::Label::backgroundColourId, Col::badge);
    lab.setJustificationType (juce::Justification::centred);
    lab.setBorderSize (juce::BorderSize<int> (2, 6, 2, 6));
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& lab,
                                               const juce::String& name, bool teal)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    s.setScrollWheelEnabled (true);
    if (teal) s.setComponentID (SlagLookAndFeel::kTealKnobId);
    addAndMakeVisible (s);

    lab.setText (name, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (10.5f));
    lab.setColour (juce::Label::textColourId, Col::textDim);
    lab.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupVFader (juce::Slider& s, juce::Label& lab, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::LinearVertical);
    s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    s.setScrollWheelEnabled (true);
    addAndMakeVisible (s);

    lab.setText (name, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (10.0f));
    lab.setColour (juce::Label::textColourId, Col::textDim);
    lab.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupHSlider (juce::Slider& s, juce::Label& lab, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 15);
    s.setScrollWheelEnabled (true);
    s.setColour (juce::Slider::textBoxTextColourId, Col::text);
    s.setColour (juce::Slider::textBoxBackgroundColourId, Col::inset);
    s.setColour (juce::Slider::textBoxOutlineColourId, Col::border);
    addAndMakeVisible (s);

    lab.setText (name, juce::dontSendNotification);
    lab.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    lab.setColour (juce::Label::textColourId, Col::textMute);
    lab.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lab);
}

void SlagblockAudioProcessorEditor::setupToggle (juce::ToggleButton& b, const juce::String& text)
{
    b.setButtonText (text);
    b.setColour (juce::ToggleButton::textColourId, Col::textDim);
    addAndMakeVisible (b);
}

void SlagblockAudioProcessorEditor::setupButton (juce::TextButton& b, const juce::String& text, bool toggle)
{
    b.setButtonText (text);
    b.setClickingTogglesState (toggle);
    b.setColour (juce::TextButton::buttonColourId, Col::badge);
    b.setColour (juce::TextButton::buttonOnColourId, Col::gold.withAlpha (0.25f));
    b.setColour (juce::TextButton::textColourOffId, Col::text);
    b.setColour (juce::TextButton::textColourOnId, Col::goldHi);
    addAndMakeVisible (b);
}

void SlagblockAudioProcessorEditor::updateLfoRateEnabled()
{
    const bool autoLive = proc.isAutoAudible();
    const bool sync = lfoSyncBtn.getToggleState() && ! autoLive;
    lfoRateKnob.setEnabled (! sync);
    lfoRateLab.setEnabled (! sync);
    lfoGridBox.setEnabled (sync);
    lfoGridLab.setEnabled (sync);
}

float SlagblockAudioProcessorEditor::readParam (const char* id, float fallback) const
{
    return proc.getEffectiveParam (id, fallback);
}

bool SlagblockAudioProcessorEditor::isSliderDragged (const juce::Slider& s) const noexcept
{
    return s.getThumbBeingDragged() >= 0 || s.isMouseButtonDown();
}

void SlagblockAudioProcessorEditor::applyLiveVisuals()
{
    using P = SlagblockAudioProcessor;
    const bool live = proc.isAutoAudible();

    struct Item { juce::Slider* slider; const char* id; };
    const Item items[] = {
        { &ratioKnob,    P::paramRatio },
        { &indexKnob,    P::paramIndex },
        { &driveKnob,    P::paramDrive },
        { &feedbackKnob, P::paramFeedback },
        { &subLevelKnob, P::paramSubLevel },
        { &cutoffKnob,   P::paramCutoff },
        { &resoKnob,     P::paramResonance },
        { &noiseKnob,    P::paramNoise },
        { &lfoRateKnob,  P::paramLfoRate },
        { &lfoCutKnob,   P::paramLfoCutoff },
        { &lfoIdxKnob,   P::paramLfoIndex },
        { &lfoDrvKnob,   P::paramLfoDrive },
        { &lfoAmpKnob,   P::paramLfoAmpDepth },
        { &fxDist,       P::paramFxDist },
        { &fxBits,       P::paramFxCrushBits },
        { &fxRate,       P::paramFxCrushRate },
        { &fxDly,        P::paramFxDelayTime },
        { &fxDlyFb,      P::paramFxDelayFb },
        { &fxDlyMix,     P::paramFxDelayMix },
        { &fxCho,        P::paramFxChorusMix },
        { &fxChoDep,     P::paramFxChorusDepth },
        { &imVol,        P::paramImVol },
        { &imDrive,      P::paramImDrive },
        { &imTime,       P::paramImTime },
        { &imFilt,       P::paramImFilt },
        { &imCrush,      P::paramImCrush },
        { &imNoise,      P::paramImNoise },
        { &imWidth,      P::paramImWidth }
    };

    if (! live)
    {
        if (lastAutoAudible)
            restoreStoredVisuals();
        return;
    }

    for (const auto& it : items)
    {
        if (it.slider == nullptr || isSliderDragged (*it.slider))
            continue;
        const float v = proc.getEffectiveParam (it.id, (float) it.slider->getValue());
        if (std::abs ((float) it.slider->getValue() - v) > 1.0e-5f)
            it.slider->setValue (v, juce::dontSendNotification);
    }
}

void SlagblockAudioProcessorEditor::restoreStoredVisuals()
{
    using P = SlagblockAudioProcessor;
    auto& ap = proc.getAPVTS();

    struct Item { juce::Slider* slider; const char* id; };
    const Item items[] = {
        { &ratioKnob,    P::paramRatio },
        { &indexKnob,    P::paramIndex },
        { &driveKnob,    P::paramDrive },
        { &feedbackKnob, P::paramFeedback },
        { &subLevelKnob, P::paramSubLevel },
        { &cutoffKnob,   P::paramCutoff },
        { &resoKnob,     P::paramResonance },
        { &noiseKnob,    P::paramNoise },
        { &lfoRateKnob,  P::paramLfoRate },
        { &lfoCutKnob,   P::paramLfoCutoff },
        { &lfoIdxKnob,   P::paramLfoIndex },
        { &lfoDrvKnob,   P::paramLfoDrive },
        { &lfoAmpKnob,   P::paramLfoAmpDepth },
        { &fxDist,       P::paramFxDist },
        { &fxBits,       P::paramFxCrushBits },
        { &fxRate,       P::paramFxCrushRate },
        { &fxDly,        P::paramFxDelayTime },
        { &fxDlyFb,      P::paramFxDelayFb },
        { &fxDlyMix,     P::paramFxDelayMix },
        { &fxCho,        P::paramFxChorusMix },
        { &fxChoDep,     P::paramFxChorusDepth },
        { &imVol,        P::paramImVol },
        { &imDrive,      P::paramImDrive },
        { &imTime,       P::paramImTime },
        { &imFilt,       P::paramImFilt },
        { &imCrush,      P::paramImCrush },
        { &imNoise,      P::paramImNoise },
        { &imWidth,      P::paramImWidth }
    };

    for (const auto& it : items)
    {
        if (it.slider == nullptr || isSliderDragged (*it.slider))
            continue;
        if (auto* p = ap.getRawParameterValue (it.id))
            it.slider->setValue (p->load(), juce::dontSendNotification);
    }
}

// ============================================================================
// Layout
// ============================================================================

void SlagblockAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int> area,
                                                   juce::Slider* const* knobs,
                                                   juce::Label* const* labels,
                                                   int count)
{
    if (count <= 0) return;
    const int colW = area.getWidth() / count;
    for (int i = 0; i < count; ++i)
    {
        auto col = area.withX (area.getX() + i * colW).withWidth (colW).reduced (3, 2);
        // knob on top, label below (PHASEDYN style)
        auto knobArea = col;
        labels[i]->setBounds (knobArea.removeFromBottom (16));
        knobs[i]->setBounds (knobArea.reduced (2, 0));
    }
}

void SlagblockAudioProcessorEditor::layoutVFaders (juce::Rectangle<int> area,
                                                   juce::Slider* const* faders,
                                                   juce::Label* const* labels,
                                                   int count)
{
    if (count <= 0) return;
    const int colW = area.getWidth() / count;
    for (int i = 0; i < count; ++i)
    {
        auto col = area.withX (area.getX() + i * colW).withWidth (colW).reduced (4, 2);
        labels[i]->setBounds (col.removeFromBottom (16));
        faders[i]->setBounds (col.reduced (2, 2));
    }
}

void SlagblockAudioProcessorEditor::layoutShaper (juce::Rectangle<int> area)
{
    juce::ToggleButton* t[] = { &imVolOn, &imDriveOn, &imTimeOn, &imFiltOn, &imCrushOn, &imNoiseOn, &imWidthOn };
    juce::Slider* s[] = { &imVol, &imDrive, &imTime, &imFilt, &imCrush, &imNoise, &imWidth };
    juce::Label* l[]  = { &imVolLab, &imDriveLab, &imTimeLab, &imFiltLab, &imCrushLab, &imNoiseLab, &imWidthLab };

    // 7 equal columns of toggle+label+slider stacked vertically? Or horizontal rows 2-col
    constexpr int n = 7;
    const int colGap = 10;
    auto left = area.removeFromLeft ((area.getWidth() - colGap) / 2);
    area.removeFromLeft (colGap);
    auto right = area;

    auto place = [&] (juce::Rectangle<int> col, int start, int count)
    {
        const int rowH = col.getHeight() / count;
        for (int i = 0; i < count; ++i)
        {
            const int idx = start + i;
            auto row = col.removeFromTop (rowH).reduced (0, juce::jmax (1, (rowH - 24) / 2));
            t[idx]->setBounds (row.removeFromLeft (36));
            l[idx]->setBounds (row.removeFromLeft (46));
            row.removeFromLeft (4);
            s[idx]->setBounds (row);
        }
    };
    place (left, 0, 4);
    place (right, 4, 3);
    juce::ignoreUnused (n);
}

// ============================================================================
// State
// ============================================================================

void SlagblockAudioProcessorEditor::setFxOn (bool on)
{
    fxOn = on;
    proc.setFxProcessingEnabled (on);
    repaint (fxPanel);
}

void SlagblockAudioProcessorEditor::setImOn (bool on)
{
    imOn = on;
    proc.setImitationEnabled (on);
    repaint (shaperPanel);
}

void SlagblockAudioProcessorEditor::syncFromProcessor()
{
    auto& ap = proc.getAPVTS();
    using P = SlagblockAudioProcessor;
    const bool f = ap.getRawParameterValue (P::paramFxOn)
                && ap.getRawParameterValue (P::paramFxOn)->load() > 0.5f;
    const bool m = ap.getRawParameterValue (P::paramImOn)
                && ap.getRawParameterValue (P::paramImOn)->load() > 0.5f;
    if (f != fxOn) { btnFx.setToggleState (f, juce::dontSendNotification); fxOn = f; }
    if (m != imOn) { btnShaper.setToggleState (m, juce::dontSendNotification); imOn = m; }
    repaint();
}

void SlagblockAudioProcessorEditor::savePresetClicked()
{
    const auto dir = SlagblockAudioProcessor::getPresetsDirectory();
    const auto def = proc.getLastPresetName().isNotEmpty()
                         ? proc.getLastPresetName() + ".slagpreset" : "NeuroPatch.slagpreset";
    fileChooser = std::make_unique<juce::FileChooser> ("Save Preset", dir.getChildFile (def),
                                                       "*.slagpreset;*.xml");
    constexpr auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != juce::File{} && proc.savePresetToFile (f))
            presetLabel.setText (f.getFileNameWithoutExtension(), juce::dontSendNotification);
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
        if (f != juce::File{} && proc.loadPresetFromFile (f))
        {
            presetLabel.setText (f.getFileNameWithoutExtension(), juce::dontSendNotification);
            syncFromProcessor();
        }
    });
}

// ============================================================================
// Drawing — PHASEDYN teal panels on brown frame
// ============================================================================

void SlagblockAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, bool active)
{
    // Soft shadow
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 12.0f);

    // Teal body with subtle vertical gradient (PHASEDYN)
    {
        juce::ColourGradient body (Col::panelHi.withAlpha (active ? 1.0f : 0.75f), r.getX(), r.getY(),
                                   Col::panelLo.withAlpha (active ? 1.0f : 0.75f), r.getX(), r.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (r, 12.0f);
    }

    // Top edge highlight
    {
        juce::Path clip;
        clip.addRoundedRectangle (r, 12.0f);
        g.saveState();
        g.reduceClipRegion (clip);
        g.setColour (juce::Colours::white.withAlpha (active ? 0.08f : 0.04f));
        g.fillRect (r.withHeight (18.0f));
        g.restoreState();
    }

    // Dark border
    g.setColour (Col::border.withAlpha (active ? 0.9f : 0.5f));
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.3f);

    // Inner soft rim
    g.setColour (Col::borderHi.withAlpha (active ? 0.25f : 0.12f));
    g.drawRoundedRectangle (r.reduced (2.0f), 10.5f, 1.0f);
}

void SlagblockAudioProcessorEditor::drawInset (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour (Col::inset);
    g.fillRoundedRectangle (r, radius);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setColour (Col::borderHi.withAlpha (0.2f));
    g.drawRoundedRectangle (r.reduced (1.5f), radius - 1.0f, 0.8f);
}

void SlagblockAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawInset (g, r, 6.0f);

    using P = SlagblockAudioProcessor;
    const float ratio = readParam (P::paramRatio, 2.01f);
    const float index = readParam (P::paramIndex, 11.0f);
    const float fdbk  = readParam (P::paramFeedback, 0.55f);
    const float sub   = readParam (P::paramSubLevel, 0.55f);
    const float drive = readParam (P::paramDrive, 0.5f);

    juce::Path wave;
    const int N = juce::jmax (80, (int) r.getWidth());
    const float midY = r.getCentreY();
    const float amp = r.getHeight() * 0.34f;
    float prev = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / (float) (N - 1);
        const float ph = t * juce::MathConstants<float>::twoPi * 2.5f + phase;
        const float mod = std::sin (ph * ratio + prev * fdbk * 0.95f);
        prev = mod;
        float fm = std::sin (ph + index * 0.12f * mod);
        fm = std::tanh (fm * (1.0f + drive * 1.6f));
        const float subW = std::sin (ph * 0.5f) * sub * 0.4f;
        const float y = midY - (fm * 0.7f + subW) * amp;
        const float x = r.getX() + 6.0f + t * (r.getWidth() - 12.0f);
        if (i == 0) wave.startNewSubPath (x, y);
        else wave.lineTo (x, y);
    }

    g.setColour (Col::gold.withAlpha (0.25f));
    g.strokePath (wave, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Col::goldHi);
    g.strokePath (wave, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SlagblockAudioProcessorEditor::drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawInset (g, r, 6.0f);

    using P = SlagblockAudioProcessor;
    const float cut  = readParam (P::paramCutoff, 0.5f);
    const float reso = readParam (P::paramResonance, 0.3f);
    const float noise= readParam (P::paramNoise, 0.1f);
    const float idx  = juce::jlimit (0.0f, 1.0f, readParam (P::paramIndex, 8.0f) / 24.0f);

    // Filled spectrum mountain (like PHASEDYN viz)
    juce::Path fill, line;
    constexpr int bars = 64;
    const float x0 = r.getX() + 6.0f;
    const float w  = r.getWidth() - 12.0f;
    const float y0 = r.getBottom() - 6.0f;
    const float maxH = r.getHeight() - 14.0f;

    fill.startNewSubPath (x0, y0);
    for (int i = 0; i < bars; ++i)
    {
        const float t = (float) i / (float) (bars - 1);
        float env = 1.0f / (1.0f + std::pow (juce::jmax (0.0f, t - cut) * 7.0f, 2.0f));
        const float peak = std::exp (-std::pow ((t - cut) * 12.0f, 2.0f)) * reso * 0.9f;
        const float partial = 0.3f + 0.5f * std::abs (std::sin (t * 14.0f + spectrumPhase)) * idx
                            + 0.15f * noise * std::abs (std::sin (t * 30.0f + phase));
        float h = juce::jlimit (0.02f, 1.0f, env * partial + peak) * maxH;
        const float x = x0 + t * w;
        const float y = y0 - h;
        fill.lineTo (x, y);
        if (i == 0) line.startNewSubPath (x, y);
        else line.lineTo (x, y);
    }
    fill.lineTo (x0 + w, y0);
    fill.closeSubPath();

    {
        juce::ColourGradient gr (Col::gold.withAlpha (0.55f), x0, y0 - maxH,
                                 Col::gold.withAlpha (0.08f), x0, y0, false);
        g.setGradientFill (gr);
        g.fillPath (fill);
    }
    g.setColour (Col::goldHi.withAlpha (0.9f));
    g.strokePath (line, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SlagblockAudioProcessorEditor::drawEnvShape (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawInset (g, r, 6.0f);

    using P = SlagblockAudioProcessor;
    float a = readParam (P::paramAttack, 0.001f);
    float d = readParam (P::paramDecay, 0.05f);
    float s = readParam (P::paramSustain, 0.58f);
    float rel = readParam (P::paramRelease, 0.08f);

    a   = std::pow (juce::jlimit (0.0005f, 1.5f, a) / 1.5f, 0.45f);
    d   = std::pow (juce::jlimit (0.005f, 2.0f, d) / 2.0f, 0.45f);
    rel = std::pow (juce::jlimit (0.005f, 3.0f, rel) / 3.0f, 0.45f);
    const float sw = 0.28f;
    float sum = a + d + sw + rel;
    a /= sum; d /= sum; const float susW = sw / sum; rel /= sum;

    const float x0 = r.getX() + 8.0f;
    const float w  = r.getWidth() - 16.0f;
    const float y0 = r.getBottom() - 8.0f;
    const float y1 = r.getY() + 8.0f;
    const float yS = y0 - s * (y0 - y1);

    juce::Path env;
    env.startNewSubPath (x0, y0);
    env.lineTo (x0 + a * w, y1);
    env.lineTo (x0 + (a + d) * w, yS);
    env.lineTo (x0 + (a + d + susW) * w, yS);
    env.lineTo (x0 + w, y0);

    juce::Path fill = env;
    fill.lineTo (x0, y0);
    fill.closeSubPath();
    g.setColour (Col::gold.withAlpha (0.22f));
    g.fillPath (fill);
    g.setColour (Col::goldHi);
    g.strokePath (env, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SlagblockAudioProcessorEditor::drawFilterCurve (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawInset (g, r, 6.0f);

    using P = SlagblockAudioProcessor;
    const float cut  = readParam (P::paramCutoff, 0.5f);
    const float reso = readParam (P::paramResonance, 0.3f);

    juce::Path curve, fill;
    const int N = juce::jmax (40, (int) r.getWidth());
    const float x0 = r.getX() + 6.0f;
    const float w  = r.getWidth() - 12.0f;
    const float y0 = r.getBottom() - 6.0f;
    const float h  = r.getHeight() - 12.0f;

    fill.startNewSubPath (x0, y0);
    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / (float) (N - 1);
        float resp = 1.0f / (1.0f + std::pow (juce::jmax (0.0f, t - cut) * 10.0f, 2.2f));
        const float peak = std::exp (-std::pow ((t - cut) * 16.0f, 2.0f)) * reso * 0.95f;
        resp = juce::jlimit (0.0f, 1.0f, resp + peak);
        const float y = y0 - resp * h * 0.9f;
        const float x = x0 + t * w;
        fill.lineTo (x, y);
        if (i == 0) curve.startNewSubPath (x, y);
        else curve.lineTo (x, y);
    }
    fill.lineTo (x0 + w, y0);
    fill.closeSubPath();

    g.setColour (Col::gold.withAlpha (0.2f));
    g.fillPath (fill);
    g.setColour (Col::goldHi);
    g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SlagblockAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> r)
{
    juce::ColourGradient gr (Col::header.brighter (0.05f), 0.0f, (float) r.getY(),
                             Col::frame, 0.0f, (float) r.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRect (r);

    // Preset name pill — follow the label so it doesn't collide with Auto
    auto pill = presetLabel.getBounds().toFloat().reduced (8.0f, 4.0f);
    if (pill.getWidth() > 40.0f)
    {
        g.setColour (Col::inset);
        g.fillRoundedRectangle (pill, 8.0f);
        g.setColour (Col::borderHi.withAlpha (0.4f));
        g.drawRoundedRectangle (pill, 8.0f, 1.0f);
    }

    g.setColour (Col::bronze.withAlpha (0.5f));
    g.fillRect (r.getX(), r.getBottom() - 2, r.getWidth(), 2);
}

void SlagblockAudioProcessorEditor::drawFooter (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (Col::frame);
    g.fillRect (r);
    g.setColour (Col::border.withAlpha (0.6f));
    g.fillRect (r.getX(), r.getY(), r.getWidth(), 1);
}

// ============================================================================
// Paint / Resize
// ============================================================================

void SlagblockAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark brown wooden frame background
    g.fillAll (Col::frame);
    {
        juce::ColourGradient gr (Col::frameHi, 0.0f, 0.0f, Col::frame, 0.0f, (float) getHeight(), false);
        g.setGradientFill (gr);
        g.fillAll();
    }

    drawHeader (g, topBar);
    drawFooter (g, bottomBar);

    const bool autoLive = proc.isAutoAudible();

    drawPanel (g, oscPanel.toFloat());
    drawPanel (g, subPanel.toFloat());
    drawPanel (g, filterPanel.toFloat());
    drawPanel (g, envPanel.toFloat());
    drawPanel (g, lfoPanel.toFloat());
    drawPanel (g, vizPanel.toFloat());
    drawPanel (g, fxPanel.toFloat(), fxOn || autoLive);
    drawPanel (g, shaperPanel.toFloat(), imOn || autoLive);

    // Badge backgrounds (small dark chips)
    auto paintBadge = [&] (juce::Label& b)
    {
        auto br = b.getBounds().toFloat().expanded (2.0f, 1.0f);
        g.setColour (Col::badge);
        g.fillRoundedRectangle (br, 5.0f);
        g.setColour (Col::borderHi.withAlpha (0.35f));
        g.drawRoundedRectangle (br, 5.0f, 0.8f);
    };
    paintBadge (oscBadge); paintBadge (subBadge); paintBadge (filterBadge);
    paintBadge (envBadge); paintBadge (lfoBadge); paintBadge (fxBadge);
    paintBadge (shaperBadge); paintBadge (vizBadge);

    if (! spectrumArea.isEmpty()) drawSpectrum (g, spectrumArea);
    if (! waveArea.isEmpty())     drawWaveform (g, waveArea);
    if (! envShapeArea.isEmpty()) drawEnvShape (g, envShapeArea);
    if (! filterCurveArea.isEmpty()) drawFilterCurve (g, filterCurveArea);

    if (! fxOn && ! autoLive && ! fxPanel.isEmpty())
    {
        g.setColour (juce::Colours::black.withAlpha (0.32f));
        g.fillRoundedRectangle (fxPanel.toFloat(), 12.0f);
    }
    if (! imOn && ! autoLive && ! shaperPanel.isEmpty())
    {
        g.setColour (juce::Colours::black.withAlpha (0.32f));
        g.fillRoundedRectangle (shaperPanel.toFloat(), 12.0f);
    }

    if (! autoCluster.isEmpty())
    {
        auto r = autoCluster.toFloat().expanded (6.0f, 4.0f);
        const float mix = proc.getAutoMix();
        g.setColour (Col::inset.withAlpha (0.65f + mix * 0.20f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (Col::gold.withAlpha (0.22f + mix * 0.55f));
        g.drawRoundedRectangle (r, 10.0f, mix > 0.05f ? 1.6f : 1.0f);
    }
}

void SlagblockAudioProcessorEditor::drawLiveKnobMarker (juce::Graphics& g, const juce::Slider& s, float live01) const
{
    if (! s.isVisible() || s.getWidth() < 8 || s.getHeight() < 8)
        return;

    const auto b = s.getBounds().toFloat();
    const float w = b.getWidth();
    const float h = b.getHeight();
    const float size = juce::jmin (w, h);
    const float radius = size * (size > 90.0f ? 0.40f : 0.36f);
    const float cx = b.getX() + w * 0.5f;
    const float cy = b.getY() + h * 0.40f;

    const auto rp = s.getRotaryParameters();
    const float pos = juce::jlimit (0.0f, 1.0f, live01);
    const float ang = rp.startAngleRadians + pos * (rp.endAngleRadians - rp.startAngleRadians);
    const float pr = radius * 0.90f;
    const float px = cx + std::cos (ang - juce::MathConstants<float>::halfPi) * pr;
    const float py = cy + std::sin (ang - juce::MathConstants<float>::halfPi) * pr;

    g.setColour (Col::frame.withAlpha (0.55f));
    g.fillEllipse (px - 3.4f, py - 3.4f, 6.8f, 6.8f);
    g.setColour (Col::goldHi.withAlpha (0.95f));
    g.fillEllipse (px - 2.2f, py - 2.2f, 4.4f, 4.4f);
}

void SlagblockAudioProcessorEditor::drawLiveSliderMarker (juce::Graphics& g, const juce::Slider& s, float live01) const
{
    if (! s.isVisible() || s.getWidth() < 8 || s.getHeight() < 8)
        return;

    const auto b = s.getBounds().toFloat();
    const float pos = juce::jlimit (0.0f, 1.0f, live01);
    const bool vert = s.isVertical();

    if (vert)
    {
        const float x = b.getCentreX();
        const float y = b.getBottom() - 8.0f - pos * juce::jmax (1.0f, b.getHeight() - 16.0f);
        g.setColour (Col::goldHi.withAlpha (0.9f));
        g.fillRoundedRectangle (x - 9.0f, y - 1.5f, 18.0f, 3.0f, 1.0f);
    }
    else
    {
        const float y = b.getCentreY();
        const float x = b.getX() + 4.0f + pos * juce::jmax (1.0f, b.getWidth() - 8.0f);
        g.setColour (Col::goldHi.withAlpha (0.9f));
        g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }
}

void SlagblockAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    if (! proc.isAutoAudible())
        return;

    using P = SlagblockAudioProcessor;
    struct Knob { const juce::Slider* s; const char* id; };
    const Knob knobs[] = {
        { &ratioKnob,    P::paramRatio },
        { &indexKnob,    P::paramIndex },
        { &driveKnob,    P::paramDrive },
        { &feedbackKnob, P::paramFeedback },
        { &subLevelKnob, P::paramSubLevel },
        { &cutoffKnob,   P::paramCutoff },
        { &resoKnob,     P::paramResonance },
        { &noiseKnob,    P::paramNoise },
        { &lfoRateKnob,  P::paramLfoRate },
        { &lfoCutKnob,   P::paramLfoCutoff },
        { &lfoIdxKnob,   P::paramLfoIndex },
        { &lfoDrvKnob,   P::paramLfoDrive },
        { &lfoAmpKnob,   P::paramLfoAmpDepth },
        { &fxDist,       P::paramFxDist },
        { &fxBits,       P::paramFxCrushBits },
        { &fxRate,       P::paramFxCrushRate },
        { &fxDly,        P::paramFxDelayTime },
        { &fxDlyFb,      P::paramFxDelayFb },
        { &fxDlyMix,     P::paramFxDelayMix },
        { &fxCho,        P::paramFxChorusMix },
        { &fxChoDep,     P::paramFxChorusDepth }
    };

    for (const auto& k : knobs)
        drawLiveKnobMarker (g, *k.s, proc.getStoredParam01 (k.id));

    struct Slide { const juce::Slider* s; const char* id; };
    const Slide slides[] = {
        { &imVol,   P::paramImVol },
        { &imDrive, P::paramImDrive },
        { &imTime,  P::paramImTime },
        { &imFilt,  P::paramImFilt },
        { &imCrush, P::paramImCrush },
        { &imNoise, P::paramImNoise },
        { &imWidth, P::paramImWidth }
    };

    for (const auto& sl : slides)
        drawLiveSliderMarker (g, *sl.s, proc.getStoredParam01 (sl.id));
}

void SlagblockAudioProcessorEditor::layoutAutoCluster (juce::Rectangle<int> area)
{
    autoCluster = area;
    auto r = area;

    btnAuto.setBounds (r.removeFromLeft (72).withSizeKeepingCentre (72, 36));
    r.removeFromLeft (8);

    juce::Slider* knobs[] = { &autoSpeedKnob, &autoDepthKnob, &autoChaosKnob };
    juce::Label*  labs[]  = { &autoSpeedLab,  &autoDepthLab,  &autoChaosLab };
    const int colW = r.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto col = r.removeFromLeft (colW).reduced (2, 0);
        labs[i]->setBounds (col.removeFromBottom (14));
        knobs[i]->setBounds (col);
    }
}

void SlagblockAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ---- HEADER (PHASEDYN-style) ----
    topBar = bounds.removeFromTop (Layout::topH);
    {
        auto h = topBar.reduced (Layout::pad, 10);
        // Left: logo + tag
        auto left = h.removeFromLeft (220);
        logoLabel.setBounds (left.removeFromTop (28));
        tagLabel.setBounds (left.removeFromTop (18));

        // Right: Save Load Undo Redo
        auto right = h.removeFromRight (300);
        auto place = [&] (juce::TextButton& b, int w)
        {
            b.setBounds (right.removeFromRight (w).withHeight (30)
                             .withY (topBar.getY() + (Layout::topH - 30) / 2 + 4));
            right.removeFromRight (7);
        };
        place (btnRedo, 58);
        place (btnUndo, 58);
        place (btnLoad, 58);
        place (btnSave, 58);

        // Auto cluster: AUTO + Speed / Depth / Chaos
        auto autoArea = h.removeFromRight (280).reduced (4, 2);
        h.removeFromRight (8);
        layoutAutoCluster (autoArea);

        // Center: preset in pill (drawn in paint)
        presetLabel.setBounds (h.withTrimmedTop (12).withTrimmedBottom (8));
    }

    // ---- FOOTER ----
    bottomBar = bounds.removeFromBottom (Layout::botH);
    {
        constexpr int n = 4;
        constexpr int bw = 140;
        constexpr int bgap = 12;
        const int total = n * bw + (n - 1) * bgap;
        int x = bottomBar.getCentreX() - total / 2;
        const int y = bottomBar.getY() + (Layout::botH - 34) / 2;
        juce::TextButton* bs[] = { &btnRandom, &btnRandomFx, &btnFx, &btnShaper };
        for (int i = 0; i < n; ++i) { bs[i]->setBounds (x, y, bw, 34); x += bw + bgap; }
        btnRandomIm.setBounds (bottomBar.getRight() - Layout::pad - 90, y, 90, 34);
    }

    bounds.reduce (Layout::pad, 8);

    // ---- Bottom strips: SHAPER then FX (like PHASEDYN bottom FX row) ----
    shaperPanel = bounds.removeFromBottom (118);
    bounds.removeFromBottom (Layout::gap);
    fxPanel = bounds.removeFromBottom (118);
    bounds.removeFromBottom (Layout::gap);

    // ---- Main 3 columns matching refer.png proportions ----
    // Left ~OSC stack 26% | Center FILTER+ENV 36% | Right VIZ+LFO 38%
    const int g = Layout::gap;
    const int usable = bounds.getWidth() - g * 2;
    const int leftW   = usable * 26 / 100;
    const int centerW = usable * 36 / 100;

    auto leftCol   = bounds.removeFromLeft (leftW);
    bounds.removeFromLeft (g);
    auto centerCol = bounds.removeFromLeft (centerW);
    bounds.removeFromLeft (g);
    auto rightCol  = bounds;

    // Left: OSC top ~58% / SUB bottom
    {
        const int oscH = (leftCol.getHeight() - g) * 58 / 100;
        oscPanel = leftCol.removeFromTop (oscH);
        leftCol.removeFromTop (g);
        subPanel = leftCol;
    }

    // Center: FILTER top ~48% / ADSR bottom (like refer FILTER + AMP ENV)
    {
        const int filtH = (centerCol.getHeight() - g) * 48 / 100;
        filterPanel = centerCol.removeFromTop (filtH);
        centerCol.removeFromTop (g);
        envPanel = centerCol;
    }

    // Right: VIZ top ~58% / LFO bottom
    {
        const int vizH = (rightCol.getHeight() - g) * 58 / 100;
        vizPanel = rightCol.removeFromTop (vizH);
        rightCol.removeFromTop (g);
        lfoPanel = rightCol;
    }

    auto head = [] (juce::Rectangle<int>& a, juce::Label& title, juce::Label& badge)
    {
        auto row = a.removeFromTop (24);
        title.setBounds (row.removeFromLeft (juce::jmin (160, row.getWidth() - 50)));
        badge.setBounds (row.removeFromRight (44).reduced (0, 2));
        a.removeFromTop (4);
    };
    auto body = [] (juce::Rectangle<int> r) { return r.reduced (12, 10); };

    // FM OSC — 4 bronze knobs in a row (or 2x2 if tall)
    {
        auto a = body (oscPanel);
        head (a, oscTitle, oscBadge);
        juce::Slider* k[] = { &ratioKnob, &indexKnob, &driveKnob, &feedbackKnob };
        juce::Label*  l[] = { &ratioLab,  &indexLab,  &driveLab,  &feedbackLab };
        if (a.getHeight() > 150)
        {
            // 2x2
            const int cellW = a.getWidth() / 2;
            const int cellH = a.getHeight() / 2;
            for (int i = 0; i < 4; ++i)
            {
                const int row = i / 2, col = i % 2;
                auto cell = juce::Rectangle<int> (a.getX() + col * cellW, a.getY() + row * cellH,
                                                  cellW, cellH).reduced (4);
                l[i]->setBounds (cell.removeFromBottom (16));
                k[i]->setBounds (cell);
            }
        }
        else
        {
            layoutKnobRow (a, k, l, 4);
        }
    }

    // SUB
    {
        auto a = body (subPanel);
        head (a, subTitle, subBadge);
        juce::Slider* k[] = { &subLevelKnob, &subOctaveKnob };
        juce::Label*  l[] = { &subLevelLab,  &subOctaveLab };
        layoutKnobRow (a, k, l, 2);
    }

    // FILTER — large cutoff + reso + noise + curve (PHASEDYN style)
    {
        auto a = body (filterPanel);
        head (a, filterTitle, filterBadge);

        // Bottom: filter curve display
        filterCurveArea = a.removeFromBottom (juce::jmax (56, a.getHeight() * 28 / 100)).toFloat();
        a.removeFromBottom (6);

        // Large cutoff left, reso + noise right
        auto cutArea = a.removeFromLeft (a.getWidth() * 48 / 100);
        a.removeFromLeft (6);
        cutoffLab.setBounds (cutArea.removeFromBottom (16));
        cutoffKnob.setBounds (cutArea);

        // Right: reso + noise stacked or side by side
        juce::Slider* k[] = { &resoKnob, &noiseKnob };
        juce::Label*  l[] = { &resoLab,  &noiseLab };
        layoutKnobRow (a, k, l, 2);
    }

    // ADSR — faders + envelope graph on right (PHASEDYN AMP ENV)
    {
        auto a = body (envPanel);
        head (a, envTitle, envBadge);

        envShapeArea = a.removeFromRight (juce::jmax (70, a.getWidth() * 28 / 100)).toFloat();
        a.removeFromRight (8);

        juce::Slider* f[] = { &attackFader, &decayFader, &sustainFader, &releaseFader };
        juce::Label*  l[] = { &attackLab,   &decayLab,   &sustainLab,   &releaseLab };
        layoutVFaders (a, f, l, 4);
    }

    // VISUALIZATION — spectrum top, wave bottom (exact PHASEDYN)
    {
        auto a = body (vizPanel);
        head (a, vizTitle, vizBadge);
        const int mid = a.getHeight() / 2;
        spectrumArea = a.removeFromTop (mid - 3).toFloat();
        a.removeFromTop (6);
        waveArea = a.toFloat();
    }

    // LFO — Rate + Amount destinations + Sync (PHASEDYN LFO style)
    {
        auto a = body (lfoPanel);
        head (a, lfoTitle, lfoBadge);

        auto top = a.removeFromTop (28);
        lfoSyncBtn.setBounds (top.removeFromRight (56).reduced (0, 2));
        top.removeFromRight (6);
        lfoGridBox.setBounds (top.removeFromRight (90).reduced (0, 2));
        top.removeFromRight (4);
        lfoGridLab.setBounds (top.removeFromRight (36));

        a.removeFromTop (4);
        juce::Slider* k[] = { &lfoRateKnob, &lfoCutKnob, &lfoIdxKnob, &lfoDrvKnob, &lfoAmpKnob };
        juce::Label*  l[] = { &lfoRateLab,  &lfoCutLab,  &lfoIdxLab,  &lfoDrvLab,  &lfoAmpLab };
        layoutKnobRow (a, k, l, 5);

        // Amp toggle near amp amount
        lfoAmpBtn.setBounds (lfoAmpLab.getX(), lfoAmpLab.getBottom() + 2, 48, 20);

        updateLfoRateEnabled();
    }

    // FX — row of teal knobs (PHASEDYN FX strip)
    {
        auto a = body (fxPanel);
        head (a, fxTitle, fxBadge);
        juce::Slider* k[] = { &fxDist, &fxBits, &fxRate, &fxDly, &fxDlyFb, &fxDlyMix, &fxCho, &fxChoDep };
        juce::Label*  l[] = { &fxDistLab, &fxBitsLab, &fxRateLab, &fxDlyLab,
                              &fxDlyFbLab, &fxDlyMixLab, &fxChoLab, &fxChoDepLab };
        layoutKnobRow (a, k, l, 8);
    }

    // SHAPER
    {
        auto a = body (shaperPanel);
        head (a, shaperTitle, shaperBadge);
        layoutShaper (a);
    }
}

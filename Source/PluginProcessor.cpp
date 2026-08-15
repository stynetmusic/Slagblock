#include "PluginProcessor.h"
#include "PluginEditor.h"

SlagblockAudioProcessor::SlagblockAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#else
    :
#endif
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    ratioParam     = apvts.getRawParameterValue (paramRatio);
    indexParam     = apvts.getRawParameterValue (paramIndex);
    driveParam     = apvts.getRawParameterValue (paramDrive);
    feedbackParam  = apvts.getRawParameterValue (paramFeedback);
    cutoffParam    = apvts.getRawParameterValue (paramCutoff);
    resonanceParam = apvts.getRawParameterValue (paramResonance);
    noiseParam     = apvts.getRawParameterValue (paramNoise);
    attackParam    = apvts.getRawParameterValue (paramAttack);
    decayParam     = apvts.getRawParameterValue (paramDecay);
    sustainParam   = apvts.getRawParameterValue (paramSustain);
    releaseParam   = apvts.getRawParameterValue (paramRelease);

    subLevelParam  = apvts.getRawParameterValue (paramSubLevel);
    subOctaveParam = apvts.getRawParameterValue (paramSubOctave);
    lfoRateParam     = apvts.getRawParameterValue (paramLfoRate);
    lfoSyncParam     = apvts.getRawParameterValue (paramLfoSync);
    lfoDivisionParam = apvts.getRawParameterValue (paramLfoDivision);
    lfoCutoffParam   = apvts.getRawParameterValue (paramLfoCutoff);
    lfoIndexParam    = apvts.getRawParameterValue (paramLfoIndex);
    lfoDriveParam    = apvts.getRawParameterValue (paramLfoDrive);
    lfoToAmpParam    = apvts.getRawParameterValue (paramLfoToAmp);
    lfoAmpDepthParam = apvts.getRawParameterValue (paramLfoAmpDepth);

    fxOnParam          = apvts.getRawParameterValue (paramFxOn);
    fxDistParam        = apvts.getRawParameterValue (paramFxDist);
    fxCrushBitsParam   = apvts.getRawParameterValue (paramFxCrushBits);
    fxCrushRateParam   = apvts.getRawParameterValue (paramFxCrushRate);
    fxDelayTimeParam   = apvts.getRawParameterValue (paramFxDelayTime);
    fxDelayFbParam     = apvts.getRawParameterValue (paramFxDelayFb);
    fxDelayMixParam    = apvts.getRawParameterValue (paramFxDelayMix);
    fxChorusMixParam   = apvts.getRawParameterValue (paramFxChorusMix);
    fxChorusDepthParam = apvts.getRawParameterValue (paramFxChorusDepth);

    imOnParam      = apvts.getRawParameterValue (paramImOn);
    imVolOnParam   = apvts.getRawParameterValue (paramImVolOn);
    imVolParam     = apvts.getRawParameterValue (paramImVol);
    imDriveOnParam = apvts.getRawParameterValue (paramImDriveOn);
    imDriveParam   = apvts.getRawParameterValue (paramImDrive);
    imTimeOnParam  = apvts.getRawParameterValue (paramImTimeOn);
    imTimeParam    = apvts.getRawParameterValue (paramImTime);
    imFiltOnParam  = apvts.getRawParameterValue (paramImFiltOn);
    imFiltParam    = apvts.getRawParameterValue (paramImFilt);
    imCrushOnParam = apvts.getRawParameterValue (paramImCrushOn);
    imCrushParam   = apvts.getRawParameterValue (paramImCrush);
    imNoiseOnParam = apvts.getRawParameterValue (paramImNoiseOn);
    imNoiseParam   = apvts.getRawParameterValue (paramImNoise);
    imWidthOnParam = apvts.getRawParameterValue (paramImWidthOn);
    imWidthParam   = apvts.getRawParameterValue (paramImWidth);

    autoOnParam    = apvts.getRawParameterValue (paramAutoOn);
    autoSpeedParam = apvts.getRawParameterValue (paramAutoSpeed);
    autoDepthParam = apvts.getRawParameterValue (paramAutoDepth);
    autoChaosParam = apvts.getRawParameterValue (paramAutoChaos);

    chaos.bind (apvts);

    // Seed undo history with default state
    lastPushedState = captureState();
    undoStack.push_back (lastPushedState.createCopy());
}

SlagblockAudioProcessor::~SlagblockAudioProcessor() = default;

void SlagblockAudioProcessor::setParamFloat (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

void SlagblockAudioProcessor::setParamBool (const char* id, bool value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (value ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

void SlagblockAudioProcessor::setFxProcessingEnabled (bool shouldEnable)
{
    setParamBool (paramFxOn, shouldEnable);
}

void SlagblockAudioProcessor::setImitationEnabled (bool shouldEnable)
{
    setParamBool (paramImOn, shouldEnable);
}

// ============================================================================
// Presets + Undo / Redo
// ============================================================================

juce::File SlagblockAudioProcessor::getPresetsDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                   .getChildFile ("Library")
                   .getChildFile ("Audio")
                   .getChildFile ("Presets")
                   .getChildFile ("Slagblock");

    if (! dir.isDirectory())
        dir.createDirectory();

    return dir;
}

juce::ValueTree SlagblockAudioProcessor::captureState()
{
    return apvts.copyState().createCopy();
}

void SlagblockAudioProcessor::applyState (const juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    isRestoringState = true;
    apvts.replaceState (state.createCopy());
    isRestoringState = false;
    notifyStateChanged();
}

void SlagblockAudioProcessor::notifyStateChanged()
{
    if (stateChangeCallback)
        stateChangeCallback();
}

void SlagblockAudioProcessor::pushUndoSnapshot (const juce::String& /*reason*/)
{
    if (isRestoringState)
        return;

    auto current = captureState();

    // Skip if identical to last push
    if (lastPushedState.isValid() && current.isEquivalentTo (lastPushedState))
        return;

    undoStack.push_back (current);
    if (static_cast<int> (undoStack.size()) > maxUndoSteps)
        undoStack.erase (undoStack.begin());

    lastPushedState = current.createCopy();
    redoStack.clear();
}

bool SlagblockAudioProcessor::canUndo() const noexcept
{
    return undoStack.size() > 1; // need at least previous + current
}

bool SlagblockAudioProcessor::canRedo() const noexcept
{
    return ! redoStack.empty();
}

bool SlagblockAudioProcessor::undo()
{
    if (undoStack.size() < 2)
        return false;

    // Current state -> redo
    auto current = captureState();
    redoStack.push_back (current);

    // Pop current snapshot, restore previous
    undoStack.pop_back();
    auto previous = undoStack.back().createCopy();
    lastPushedState = previous.createCopy();
    applyState (previous);
    return true;
}

bool SlagblockAudioProcessor::redo()
{
    if (redoStack.empty())
        return false;

    auto next = redoStack.back().createCopy();
    redoStack.pop_back();

    undoStack.push_back (next);
    lastPushedState = next.createCopy();
    applyState (next);
    return true;
}

bool SlagblockAudioProcessor::savePresetToFile (const juce::File& file)
{
    auto state = captureState();
    auto xml = state.createXml();
    if (xml == nullptr)
        return false;

    xml->setAttribute ("plugin", "Slagblock");
    xml->setAttribute ("presetVersion", 1);
    xml->setAttribute ("presetName", file.getFileNameWithoutExtension());

    auto target = file;
    if (! target.hasFileExtension (".slagpreset") && ! target.hasFileExtension (".xml"))
        target = target.withFileExtension (".slagpreset");

    if (! target.getParentDirectory().isDirectory())
        target.getParentDirectory().createDirectory();

    const bool ok = xml->writeTo (target);
    if (ok)
        lastPresetName = target.getFileNameWithoutExtension();

    return ok;
}

bool SlagblockAudioProcessor::loadPresetFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return false;

    // Accept our APVTS root type or wrapped presets
    juce::ValueTree tree;
    if (xml->hasTagName (apvts.state.getType()))
    {
        tree = juce::ValueTree::fromXml (*xml);
    }
    else if (auto* child = xml->getChildByName (apvts.state.getType()))
    {
        tree = juce::ValueTree::fromXml (*child);
    }
    else
    {
        // Try parsing as generic ValueTree XML
        tree = juce::ValueTree::fromXml (*xml);
        if (tree.getType() != apvts.state.getType())
            return false;
    }

    if (! tree.isValid())
        return false;

    pushUndoSnapshot ("Before load preset");
    applyState (tree);
    lastPresetName = file.getFileNameWithoutExtension();

    // Keep loaded state as latest undo head
    lastPushedState = captureState();
    undoStack.push_back (lastPushedState.createCopy());
    if (static_cast<int> (undoStack.size()) > maxUndoSteps)
        undoStack.erase (undoStack.begin());

    return true;
}

void SlagblockAudioProcessor::randomizeNeuroPatch()
{
    pushUndoSnapshot ("Before random synth");
    auto& rng = juce::Random::getSystemRandom();

    float ratio = 2.01f;
    const float roll = rng.nextFloat();

    if (roll < 0.72f)
    {
        static constexpr float bases[] = { 1.5f, 2.0f, 2.5f, 3.0f, 3.5f };
        const float base = bases[static_cast<size_t> (rng.nextInt (5))];
        const float micro = (rng.nextFloat() < 0.85f)
                                ? (0.01f + rng.nextFloat() * 0.09f)
                                : (-0.03f + rng.nextFloat() * 0.04f);
        ratio = base + micro;
    }
    else if (roll < 0.90f)
        ratio = 1.5f + rng.nextFloat() * 2.0f;
    else
        ratio = 1.2f + rng.nextFloat() * 3.5f;

    ratio = juce::jlimit (0.25f, 8.0f, ratio);
    const float t = rng.nextFloat();

    setParamFloat (paramRatio,     ratio);
    setParamFloat (paramIndex,     8.0f  + rng.nextFloat() * 10.0f);
    setParamFloat (paramDrive,     4.0f  + rng.nextFloat() * 8.0f);
    setParamFloat (paramFeedback,  0.3f  + rng.nextFloat() * 0.6f);
    setParamFloat (paramCutoff,    450.0f + (t * t) * 2800.0f);
    setParamFloat (paramResonance, 0.35f + rng.nextFloat() * 0.45f);
    setParamFloat (paramNoise,     0.04f + rng.nextFloat() * 0.16f);
    setParamFloat (paramAttack,    0.0005f + rng.nextFloat() * 0.04f);
    setParamFloat (paramDecay,     0.02f   + rng.nextFloat() * 0.25f);
    setParamFloat (paramSustain,   0.35f   + rng.nextFloat() * 0.50f);
    setParamFloat (paramRelease,   0.03f   + rng.nextFloat() * 0.35f);

    setParamFloat (paramSubLevel,  0.35f + rng.nextFloat() * 0.55f);
    setParamFloat (paramSubOctave, rng.nextFloat() > 0.55f ? 2.0f : 1.0f);
    setParamFloat (paramLfoRate,     0.08f + rng.nextFloat() * 2.5f);
    setParamBool  (paramLfoSync,     rng.nextFloat() > 0.4f);
    // Prefer musical divisions around 1/4 … 1/16
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramLfoDivision)))
    {
        const int n = choice->choices.size();
        const int idx = juce::jlimit (0, juce::jmax (0, n - 1), 6 + rng.nextInt (juce::jmax (1, n - 6)));
        choice->beginChangeGesture();
        *choice = idx;
        choice->endChangeGesture();
    }
    setParamFloat (paramLfoCutoff,   rng.nextFloat() * 0.55f);
    setParamFloat (paramLfoIndex,    rng.nextFloat() * 0.50f);
    setParamFloat (paramLfoDrive,    rng.nextFloat() * 0.35f);
    setParamBool  (paramLfoToAmp,    rng.nextFloat() > 0.55f);
    setParamFloat (paramLfoAmpDepth, rng.nextFloat() * 0.45f);
}

juce::StringArray SlagblockAudioProcessor::getLfoDivisionNames()
{
    return {
        "4 Bars", "2 Bars", "1 Bar",
        "1/2", "1/2 D", "1/2 T",
        "1/4", "1/4 D", "1/4 T",
        "1/8", "1/8 D", "1/8 T",
        "1/16", "1/16 D", "1/16 T",
        "1/32"
    };
}

float SlagblockAudioProcessor::getLfoDivisionBeats (int divisionIndex)
{
    // Length of one LFO cycle in quarter notes
    switch (juce::jlimit (0, 15, divisionIndex))
    {
        case 0:  return 16.0f;   // 4 bars
        case 1:  return 8.0f;    // 2 bars
        case 2:  return 4.0f;    // 1 bar
        case 3:  return 2.0f;    // 1/2
        case 4:  return 3.0f;    // 1/2 dotted
        case 5:  return 4.0f / 3.0f; // 1/2 triplet (3 in time of 2)
        case 6:  return 1.0f;    // 1/4
        case 7:  return 1.5f;    // 1/4 dotted
        case 8:  return 2.0f / 3.0f; // 1/4 T
        case 9:  return 0.5f;    // 1/8
        case 10: return 0.75f;   // 1/8 D
        case 11: return 1.0f / 3.0f; // 1/8 T
        case 12: return 0.25f;   // 1/16
        case 13: return 0.375f;  // 1/16 D
        case 14: return 1.0f / 6.0f; // 1/16 T
        case 15: return 0.125f;  // 1/32
        default: return 1.0f;
    }
}

float SlagblockAudioProcessor::lfoHzFromTempo (double bpm, int divisionIndex)
{
    const double b = bpm > 1.0 ? bpm : 120.0;
    const double beats = static_cast<double> (getLfoDivisionBeats (divisionIndex));
    // hz = 1 / (seconds per cycle) = bpm / (60 * beats)
    return static_cast<float> (b / (60.0 * juce::jmax (1.0e-6, beats)));
}

void SlagblockAudioProcessor::randomizeFXOnly()
{
    pushUndoSnapshot ("Before random FX");
    auto& rng = juce::Random::getSystemRandom();
    setParamBool (paramFxOn, true);

    setParamFloat (paramFxDist,        0.35f + rng.nextFloat() * 0.55f);
    setParamFloat (paramFxCrushBits,   6.0f  + rng.nextFloat() * 6.0f);
    setParamFloat (paramFxCrushRate,   0.15f + rng.nextFloat() * 0.55f);
    setParamFloat (paramFxDelayTime,   60.0f + rng.nextFloat() * 200.0f);
    setParamFloat (paramFxDelayFb,     0.15f + rng.nextFloat() * 0.45f);
    setParamFloat (paramFxDelayMix,    0.10f + rng.nextFloat() * 0.30f);
    setParamFloat (paramFxChorusMix,   0.10f + rng.nextFloat() * 0.40f);
    setParamFloat (paramFxChorusDepth, 0.25f + rng.nextFloat() * 0.60f);
}

void SlagblockAudioProcessor::randomizeImitationOnly()
{
    pushUndoSnapshot ("Before random imitation");
    auto& rng = juce::Random::getSystemRandom();
    setParamBool (paramImOn, true);

    auto maybeOn = [&rng]() { return rng.nextFloat() > 0.35f; }; // ~65% chance on

    setParamBool  (paramImVolOn,   maybeOn());
    setParamFloat (paramImVol,     0.25f + rng.nextFloat() * 0.65f);

    setParamBool  (paramImDriveOn, maybeOn());
    setParamFloat (paramImDrive,   0.30f + rng.nextFloat() * 0.60f);

    setParamBool  (paramImTimeOn,  maybeOn());
    setParamFloat (paramImTime,    0.20f + rng.nextFloat() * 0.55f);

    setParamBool  (paramImFiltOn,  maybeOn());
    setParamFloat (paramImFilt,    0.15f + rng.nextFloat() * 0.70f);

    setParamBool  (paramImCrushOn, maybeOn());
    setParamFloat (paramImCrush,   0.20f + rng.nextFloat() * 0.60f);

    setParamBool  (paramImNoiseOn, rng.nextFloat() > 0.5f);
    setParamFloat (paramImNoise,   0.05f + rng.nextFloat() * 0.40f);

    setParamBool  (paramImWidthOn, maybeOn());
    setParamFloat (paramImWidth,   0.25f + rng.nextFloat() * 0.70f);
}

juce::AudioProcessorValueTreeState::ParameterLayout SlagblockAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const char* id, const char* name,
                               juce::NormalisableRange<float> range, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    auto addBool = [&params] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id, 1 }, name, def));
    };

    // Synth
    addFloat (paramRatio, "Ratio", juce::NormalisableRange<float> (0.25f, 8.0f, 0.01f, 0.5f), 2.01f);
    addFloat (paramIndex, "Index", juce::NormalisableRange<float> (0.0f, 20.0f, 0.01f), 11.0f);
    addFloat (paramDrive, "Drive", juce::NormalisableRange<float> (0.5f, 24.0f, 0.01f, 0.4f), 7.5f);
    addFloat (paramFeedback, "Feedback", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f);
    addFloat (paramCutoff, "Filter Cutoff", juce::NormalisableRange<float> (80.0f, 16000.0f, 0.1f, 0.3f), 2400.0f);
    addFloat (paramResonance, "Filter Resonance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f);
    addFloat (paramNoise, "Noise Amount", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.12f);
    addFloat (paramAttack, "Attack", juce::NormalisableRange<float> (0.0005f, 1.5f, 0.0001f, 0.35f), 0.001f);
    addFloat (paramDecay, "Decay", juce::NormalisableRange<float> (0.005f, 2.0f, 0.0001f, 0.35f), 0.05f);
    addFloat (paramSustain, "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.58f);
    addFloat (paramRelease, "Release", juce::NormalisableRange<float> (0.005f, 3.0f, 0.0001f, 0.35f), 0.08f);

    // Sub oscillator (independent of FM carrier)
    addFloat (paramSubLevel, "Sub Level", juce::NormalisableRange<float> (0.0f, 1.2f, 0.001f), 0.55f);
    addFloat (paramSubOctave, "Sub Octave", juce::NormalisableRange<float> (1.0f, 2.0f, 1.0f), 1.0f); // 1=-1, 2=-2

    // LFO → destinations + tempo grid
    addFloat (paramLfoRate, "LFO Rate", juce::NormalisableRange<float> (0.05f, 20.0f, 0.001f, 0.35f), 0.45f);
    addBool  (paramLfoSync, "LFO Sync", true);
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { paramLfoDivision, 1 },
        "LFO Division",
        getLfoDivisionNames(),
        6)); // default 1/4
    addFloat (paramLfoCutoff, "LFO → Cutoff", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f);
    addFloat (paramLfoIndex, "LFO → Index", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f);
    addFloat (paramLfoDrive, "LFO → Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f);
    addBool  (paramLfoToAmp, "LFO → Amp", false);
    addFloat (paramLfoAmpDepth, "LFO Amp Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f);

    // FX
    addBool  (paramFxOn, "FX On", false);
    addFloat (paramFxDist, "FX Distortion", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f);
    addFloat (paramFxCrushBits, "FX Crush Bits", juce::NormalisableRange<float> (4.0f, 16.0f, 0.1f), 9.0f);
    addFloat (paramFxCrushRate, "FX Crush Rate", juce::NormalisableRange<float> (0.05f, 1.0f, 0.001f), 0.38f);
    addFloat (paramFxDelayTime, "FX Delay Time", juce::NormalisableRange<float> (10.0f, 600.0f, 0.1f, 0.4f), 118.0f);
    addFloat (paramFxDelayFb, "FX Delay Feedback", juce::NormalisableRange<float> (0.0f, 0.9f, 0.001f), 0.30f);
    addFloat (paramFxDelayMix, "FX Delay Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.20f);
    addFloat (paramFxChorusMix, "FX Chorus Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.28f);
    addFloat (paramFxChorusDepth, "FX Chorus Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f);

    // Imitation shapers
    addBool  (paramImOn, "Imitation On", false);
    addBool  (paramImVolOn, "Im Vol On", true);
    addFloat (paramImVol, "Im Volume", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f);
    addBool  (paramImDriveOn, "Im Drive On", false);
    addFloat (paramImDrive, "Im Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.50f);
    addBool  (paramImTimeOn, "Im Time On", false);
    addFloat (paramImTime, "Im Time", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.40f);
    addBool  (paramImFiltOn, "Im Filter On", false);
    addFloat (paramImFilt, "Im Filter", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f);
    addBool  (paramImCrushOn, "Im Crush On", false);
    addFloat (paramImCrush, "Im Crush", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.40f);
    addBool  (paramImNoiseOn, "Im Noise On", false);
    addFloat (paramImNoise, "Im Noise", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.20f);
    addBool  (paramImWidthOn, "Im Width On", false);
    addFloat (paramImWidth, "Im Width", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f);

    // Auto / chaos motion — high defaults so flipping Auto instantly animates the patch
    addBool  (paramAutoOn,    "Auto",        false);
    addFloat (paramAutoSpeed, "Auto Rate",   juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.48f);
    addFloat (paramAutoDepth, "Auto Depth",  juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.78f);
    addFloat (paramAutoChaos, "Auto Chaos",  juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.62f);

    return { params.begin(), params.end() };
}

void SlagblockAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    fx.prepare (sampleRate, samplesPerBlock);
    imitation.prepare (sampleRate, samplesPerBlock);
    chaos.prepare (sampleRate);

    lfoScratch.assign (static_cast<size_t> (juce::jmax (1, samplesPerBlock)), 0.0f);
    // ~20 ms smooth for amp enable (click-free toggle)
    ampEnableCoef = 1.0f - std::exp (-1.0f / (0.020f * static_cast<float> (sampleRate)));
    ampEnableSmoothed = 0.0f;
}

void SlagblockAudioProcessor::releaseResources()
{
    engine.reset();
    fx.reset();
    imitation.reset();
}

bool SlagblockAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

float SlagblockAudioProcessor::getEffectiveParam (const char* id, float fallback) const
{
    if (id == nullptr)
        return fallback;

    if (chaos.isAudible() && chaos.hasTargetId (id))
        return chaos.getLiveById (id, fallback);

    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();

    return fallback;
}

bool SlagblockAudioProcessor::isAutoOn() const noexcept
{
    return autoOnParam != nullptr && autoOnParam->load() > 0.5f;
}

float SlagblockAudioProcessor::getStoredParam01 (const char* id, float fallback) const
{
    if (id == nullptr)
        return fallback;

    if (auto* p = apvts.getParameter (id))
        return p->getValue();

    return fallback;
}

void SlagblockAudioProcessor::tickAutoVisual (float dtSeconds)
{
    const auto now = juce::Time::getMillisecondCounter();
    if (now - lastAudioProcessMs.load (std::memory_order_relaxed) < 80)
        return;

    const bool autoWanted = isAutoOn();
    chaos.setControls (autoWanted,
                       autoSpeedParam != nullptr ? autoSpeedParam->load() : 0.48f,
                       autoDepthParam != nullptr ? autoDepthParam->load() : 0.78f,
                       autoChaosParam != nullptr ? autoChaosParam->load() : 0.62f);
    chaos.advanceSeconds (dtSeconds);
    chaos.snapshotLive();
}

void SlagblockAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    using CM = slagblock::ChaosMotion;

    const bool autoWanted = autoOnParam != nullptr && autoOnParam->load() > 0.5f;
    chaos.setControls (autoWanted,
                       autoSpeedParam != nullptr ? autoSpeedParam->load() : 0.48f,
                       autoDepthParam != nullptr ? autoDepthParam->load() : 0.78f,
                       autoChaosParam != nullptr ? autoChaosParam->load() : 0.62f);
    chaos.advance (buffer.getNumSamples());
    lastAudioProcessMs.store (juce::Time::getMillisecondCounter(), std::memory_order_relaxed);
    const bool autoLive = chaos.isAudible();

    auto base = [] (std::atomic<float>* p, float d) { return p != nullptr ? p->load() : d; };
    auto live = [this] (CM::Target t, std::atomic<float>* p, float d)
    {
        return chaos.modulate (t, p != nullptr ? p->load() : d);
    };

    engine.setRatio           (live (CM::Ratio,     ratioParam,     2.01f));
    engine.setIndex           (live (CM::Index,     indexParam,     11.0f));
    engine.setDrive           (live (CM::Drive,     driveParam,     7.5f));
    engine.setFeedback        (live (CM::Feedback,  feedbackParam,  0.55f));
    engine.setFilterCutoff    (live (CM::Cutoff,    cutoffParam,    2400.0f));
    engine.setFilterResonance (live (CM::Resonance, resonanceParam, 0.45f));
    engine.setNoiseAmount     (live (CM::Noise,     noiseParam,     0.12f));
    engine.setAttack          (base (attackParam,  0.001f));
    engine.setDecay           (base (decayParam,   0.05f));
    engine.setSustain         (base (sustainParam, 0.58f));
    engine.setRelease         (base (releaseParam, 0.08f));

    engine.setSubLevel  (live (CM::SubLevel, subLevelParam, 0.55f));
    engine.setSubOctave (subOctaveParam != nullptr ? static_cast<int> (subOctaveParam->load() + 0.5f) : 1);
    // LFO rate: free Hz or host-tempo musical grid.
    // Auto drives the free-rate path so the LFO itself wanders.
    {
        const bool syncOn = ! autoLive
                         && lfoSyncParam != nullptr && lfoSyncParam->load() > 0.5f;
        float rateHz = live (CM::LfoRate, lfoRateParam, 0.45f);

        if (syncOn)
        {
            double bpm = 120.0;
            if (auto* playHead = getPlayHead())
            {
                if (auto pos = playHead->getPosition())
                {
                    if (auto b = pos->getBpm())
                        bpm = *b;
                }
            }

            int divIdx = 6; // 1/4
            if (lfoDivisionParam != nullptr)
            {
                // Choice params expose normalized 0…1 via raw value in some JUCE versions;
                // convert properly through the parameter object.
                if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramLfoDivision)))
                    divIdx = p->getIndex();
                else
                    divIdx = juce::jlimit (0, 15, static_cast<int> (lfoDivisionParam->load() + 0.5f));
            }

            rateHz = lfoHzFromTempo (bpm, divIdx);
        }

        engine.setLfoRate (rateHz);
    }

    engine.setLfoToCutoff (live (CM::LfoCutoff, lfoCutoffParam, 0.0f));
    engine.setLfoToIndex  (live (CM::LfoIndex,  lfoIndexParam,  0.0f));
    engine.setLfoToDrive  (live (CM::LfoDrive,  lfoDriveParam,  0.0f));

    const bool fxManual = fxOnParam != nullptr && fxOnParam->load() > 0.5f;
    const bool fxOn = fxManual || autoLive;
    fx.setEnabled (fxOn);

    if (fxOn)
    {
        fx.setDistortion   (live (CM::FxDist,        fxDistParam,        0.55f));
        fx.setCrushBits    (live (CM::FxCrushBits,   fxCrushBitsParam,   9.0f));
        fx.setCrushRate    (live (CM::FxCrushRate,   fxCrushRateParam,   0.38f));
        fx.setDelayTimeMs  (live (CM::FxDelayTime,   fxDelayTimeParam,   118.0f));
        fx.setDelayFeedback(live (CM::FxDelayFb,     fxDelayFbParam,     0.30f));
        fx.setDelayMix     (live (CM::FxDelayMix,    fxDelayMixParam,    0.20f));
        fx.setChorusMix    (live (CM::FxChorusMix,   fxChorusMixParam,   0.28f));
        fx.setChorusDepth  (live (CM::FxChorusDepth, fxChorusDepthParam, 0.45f));
    }

    const bool imManual = imOnParam != nullptr && imOnParam->load() > 0.5f;
    const bool imOn = imManual || autoLive;
    imitation.setRackEnabled (imOn);

    if (imOn)
    {
        auto on = [] (std::atomic<float>* p) { return p != nullptr && p->load() > 0.5f; };
        const bool force = autoLive;
        imitation.setVolume (force || on (imVolOnParam),   live (CM::ImVol,   imVolParam,   0.55f));
        imitation.setDrive  (force || on (imDriveOnParam), live (CM::ImDrive, imDriveParam, 0.50f));
        imitation.setTime   (force || on (imTimeOnParam),  live (CM::ImTime,  imTimeParam,  0.40f));
        imitation.setFilter (force || on (imFiltOnParam),  live (CM::ImFilt,  imFiltParam,  0.35f));
        imitation.setCrush  (force || on (imCrushOnParam), live (CM::ImCrush, imCrushParam, 0.40f));
        imitation.setNoise  (force || on (imNoiseOnParam), live (CM::ImNoise, imNoiseParam, 0.20f));
        imitation.setWidth  (force || on (imWidthOnParam), live (CM::ImWidth, imWidthParam, 0.55f));
    }

    const int numSamples = buffer.getNumSamples();
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    if (static_cast<int> (lfoScratch.size()) < numSamples)
        lfoScratch.resize (static_cast<size_t> (numSamples), 0.0f);

    float* lfoOut = lfoScratch.data();
    int samplePos = 0;

    for (const auto metadata : midi)
    {
        const int eventSample = metadata.samplePosition;
        const auto msg = metadata.getMessage();

        const int chunk = eventSample - samplePos;
        if (chunk > 0)
        {
            engine.process (left + samplePos, right + samplePos, chunk, lfoOut + samplePos);
            samplePos = eventSample;
        }

        if (msg.isNoteOn())
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            engine.allNotesOff();
    }

    if (samplePos < numSamples)
        engine.process (left + samplePos, right + samplePos, numSamples - samplePos, lfoOut + samplePos);

    if (fxOn)
        fx.process (left, right, numSamples);

    if (imOn)
        imitation.process (left, right, numSamples);

    // ---- LFO → Amp on final output gain (after osc / filter / drive / FX / imitation) ----
    const float ampDepthLive = live (CM::LfoAmp, lfoAmpDepthParam, 0.35f);
    const bool ampOn = (lfoToAmpParam != nullptr && lfoToAmpParam->load() > 0.5f)
                    || (autoLive && ampDepthLive > 0.02f);
    const float depth = juce::jlimit (0.0f, 1.0f, ampDepthLive);
    const float enableTarget = ampOn ? 1.0f : 0.0f;
    const float coef = ampEnableCoef > 0.0f ? ampEnableCoef : 0.05f;

    for (int i = 0; i < numSamples; ++i)
    {
        ampEnableSmoothed += (enableTarget - ampEnableSmoothed) * coef;
        const float depthEff = depth * ampEnableSmoothed;

        // bipolar LFO −1…+1 → 0…1
        const float lfo01 = 0.5f * (lfoOut[i] + 1.0f);
        // finalGain = baseGain * (1 - depth + depth * lfoNorm)
        const float gain = 1.0f - depthEff + depthEff * lfo01;

        left[i]  *= gain;
        if (right != left)
            right[i] *= gain;
    }
}

juce::AudioProcessorEditor* SlagblockAudioProcessor::createEditor()
{
    return new SlagblockAudioProcessorEditor (*this);
}

void SlagblockAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
    {
        xml->setAttribute ("plugin", "Slagblock");
        xml->setAttribute ("presetVersion", 1);
        copyXmlToBinary (*xml, destData);
    }
}

void SlagblockAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            isRestoringState = true;
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            isRestoringState = false;
            lastPushedState = captureState();
            undoStack.clear();
            redoStack.clear();
            undoStack.push_back (lastPushedState.createCopy());
            notifyStateChanged();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlagblockAudioProcessor();
}

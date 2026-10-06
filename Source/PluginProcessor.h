#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "engine/FMEngine.h"
#include "engine/NeuroFX.h"
#include "engine/ShaperImitation.h"
#include "engine/ChaosMotion.h"
#include <vector>
#include <functional>

class SlagblockAudioProcessor : public juce::AudioProcessor
{
public:
    SlagblockAudioProcessor();
    ~SlagblockAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Slagblock"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.8; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void randomizeNeuroPatch();
    void randomizeFXOnly();
    void randomizeImitationOnly();
    void setFxProcessingEnabled (bool shouldEnable);
    void setImitationEnabled (bool shouldEnable);

    /** True while Auto is fading in/out or fully on (sound is being driven). */
    bool isAutoAudible() const noexcept { return chaos.isAudible(); }
    /** True if the Auto button/parameter is engaged (may still be fading). */
    bool isAutoOn() const noexcept;
    /** 0–1 Auto mix, for header glow. */
    float getAutoMix() const noexcept { return chaos.getMix(); }
    /** Live (modulated if Auto is on) value for GUI viz / knob markers. */
    float getEffectiveParam (const char* id, float fallback) const;
    /** Stored (unmodulated) 0–1 value — the knob's parked centre. */
    float getStoredParam01 (const char* id, float fallback = 0.0f) const;
    /**
     * Keep chaotic walkers moving when the host is not calling processBlock
     * (editor open, transport stopped). No-op if audio just ran.
     */
    void tickAutoVisual (float dtSeconds);

    // --- Presets ---
    static juce::File getPresetsDirectory();
    bool savePresetToFile (const juce::File& file);
    bool loadPresetFromFile (const juce::File& file);
    juce::String getLastPresetName() const { return lastPresetName; }

    // --- Undo / Redo (full-state snapshots of all parameters) ---
    void pushUndoSnapshot (const juce::String& reason = {});
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    bool undo();
    bool redo();

    /** Called by editor after state restore so UI panels can resync. */
    void setStateChangeCallback (std::function<void()> cb) { stateChangeCallback = std::move (cb); }

    // Synth
    static constexpr const char* paramRatio      = "ratio";
    static constexpr const char* paramIndex      = "index";
    static constexpr const char* paramDrive      = "drive";
    static constexpr const char* paramFeedback   = "feedback";
    static constexpr const char* paramCutoff     = "cutoff";
    static constexpr const char* paramResonance  = "resonance";
    static constexpr const char* paramNoise      = "noise";
    static constexpr const char* paramAttack     = "attack";
    static constexpr const char* paramDecay      = "decay";
    static constexpr const char* paramSustain    = "sustain";
    static constexpr const char* paramRelease    = "release";

    static constexpr const char* paramSubLevel  = "subLevel";
    static constexpr const char* paramSubOctave = "subOctave";

    static constexpr const char* paramOsc3Ratio = "osc3Ratio";
    static constexpr const char* paramOsc3Index = "osc3Index";
    static constexpr const char* paramOsc3Level = "osc3Level";
    static constexpr const char* paramOsc3Wave  = "osc3Wave";
    static constexpr const char* paramOsc3Role  = "osc3Role";

    static juce::StringArray getOsc3WaveNames();
    static juce::StringArray getOsc3RoleNames();

    static constexpr const char* paramLfoRate     = "lfoRate";
    static constexpr const char* paramLfoSync     = "lfoSync";      // tempo sync on/off
    static constexpr const char* paramLfoDivision = "lfoDivision";  // note grid when synced
    static constexpr const char* paramLfoCutoff   = "lfoCutoff";
    static constexpr const char* paramLfoIndex    = "lfoIndex";
    static constexpr const char* paramLfoDrive    = "lfoDrive";
    static constexpr const char* paramLfoToAmp    = "lfoToAmp";     // bool enable
    static constexpr const char* paramLfoAmpDepth = "lfoAmp";       // depth 0…1 (legacy id)

    /** Musical grid labels for LFO tempo sync (matches choice parameter order). */
    static juce::StringArray getLfoDivisionNames();
    /** Quarter-note length of one LFO cycle for a division index. */
    static float getLfoDivisionBeats (int divisionIndex);
    /** Hz from BPM + division index. */
    static float lfoHzFromTempo (double bpm, int divisionIndex);

    static constexpr const char* paramFxOn          = "fxOn";
    static constexpr const char* paramFxDist        = "fxDist";
    static constexpr const char* paramFxCrushBits   = "fxCrushBits";
    static constexpr const char* paramFxCrushRate   = "fxCrushRate";
    static constexpr const char* paramFxDelayTime   = "fxDelayTime";
    static constexpr const char* paramFxDelayFb     = "fxDelayFb";
    static constexpr const char* paramFxDelayMix    = "fxDelayMix";
    static constexpr const char* paramFxChorusMix   = "fxChorusMix";
    static constexpr const char* paramFxChorusDepth = "fxChorusDepth";

    static constexpr const char* paramImOn        = "imOn";
    static constexpr const char* paramImVolOn     = "imVolOn";
    static constexpr const char* paramImVol       = "imVol";
    static constexpr const char* paramImDriveOn   = "imDriveOn";
    static constexpr const char* paramImDrive     = "imDrive";
    static constexpr const char* paramImTimeOn    = "imTimeOn";
    static constexpr const char* paramImTime      = "imTime";
    static constexpr const char* paramImFiltOn    = "imFiltOn";
    static constexpr const char* paramImFilt      = "imFilt";
    static constexpr const char* paramImCrushOn   = "imCrushOn";
    static constexpr const char* paramImCrush     = "imCrush";
    static constexpr const char* paramImNoiseOn   = "imNoiseOn";
    static constexpr const char* paramImNoise     = "imNoise";
    static constexpr const char* paramImWidthOn   = "imWidthOn";
    static constexpr const char* paramImWidth     = "imWidth";

    static constexpr const char* paramAutoOn    = "autoOn";
    static constexpr const char* paramAutoSpeed = "autoSpeed";
    static constexpr const char* paramAutoDepth = "autoDepth";
    static constexpr const char* paramAutoChaos = "autoChaos";

private:
    void setParamFloat (const char* id, float value);
    void setParamBool  (const char* id, bool value);
    juce::ValueTree captureState();
    void applyState (const juce::ValueTree& state);
    void notifyStateChanged();

    juce::AudioProcessorValueTreeState apvts;
    slagblock::FMEngine engine;
    slagblock::NeuroFX fx;
    slagblock::ShaperImitation imitation;
    slagblock::ChaosMotion chaos;

    // Full-state undo/redo
    static constexpr int maxUndoSteps = 64;
    std::vector<juce::ValueTree> undoStack;
    std::vector<juce::ValueTree> redoStack;
    juce::ValueTree lastPushedState;
    bool isRestoringState = false;
    juce::String lastPresetName;
    std::function<void()> stateChangeCallback;

    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* indexParam     = nullptr;
    std::atomic<float>* driveParam     = nullptr;
    std::atomic<float>* feedbackParam  = nullptr;
    std::atomic<float>* cutoffParam    = nullptr;
    std::atomic<float>* resonanceParam = nullptr;
    std::atomic<float>* noiseParam     = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* decayParam     = nullptr;
    std::atomic<float>* sustainParam   = nullptr;
    std::atomic<float>* releaseParam   = nullptr;

    std::atomic<float>* subLevelParam  = nullptr;
    std::atomic<float>* subOctaveParam = nullptr;
    std::atomic<float>* osc3RatioParam = nullptr;
    std::atomic<float>* osc3IndexParam = nullptr;
    std::atomic<float>* osc3LevelParam = nullptr;
    std::atomic<float>* osc3WaveParam  = nullptr;
    std::atomic<float>* osc3RoleParam  = nullptr;
    std::atomic<float>* lfoRateParam     = nullptr;
    std::atomic<float>* lfoSyncParam     = nullptr;
    std::atomic<float>* lfoDivisionParam = nullptr;
    std::atomic<float>* lfoCutoffParam   = nullptr;
    std::atomic<float>* lfoIndexParam    = nullptr;
    std::atomic<float>* lfoDriveParam    = nullptr;
    std::atomic<float>* lfoToAmpParam    = nullptr;
    std::atomic<float>* lfoAmpDepthParam = nullptr;

    // Post-FX LFO→amp (shared phase with engine via lfoScratch)
    std::vector<float> lfoScratch;
    float ampEnableSmoothed = 0.0f;
    float ampEnableCoef     = 0.0f;

    std::atomic<float>* fxOnParam          = nullptr;
    std::atomic<float>* fxDistParam        = nullptr;
    std::atomic<float>* fxCrushBitsParam   = nullptr;
    std::atomic<float>* fxCrushRateParam   = nullptr;
    std::atomic<float>* fxDelayTimeParam   = nullptr;
    std::atomic<float>* fxDelayFbParam     = nullptr;
    std::atomic<float>* fxDelayMixParam    = nullptr;
    std::atomic<float>* fxChorusMixParam   = nullptr;
    std::atomic<float>* fxChorusDepthParam = nullptr;

    std::atomic<float>* imOnParam      = nullptr;
    std::atomic<float>* imVolOnParam   = nullptr;
    std::atomic<float>* imVolParam     = nullptr;
    std::atomic<float>* imDriveOnParam = nullptr;
    std::atomic<float>* imDriveParam   = nullptr;
    std::atomic<float>* imTimeOnParam  = nullptr;
    std::atomic<float>* imTimeParam    = nullptr;
    std::atomic<float>* imFiltOnParam  = nullptr;
    std::atomic<float>* imFiltParam    = nullptr;
    std::atomic<float>* imCrushOnParam = nullptr;
    std::atomic<float>* imCrushParam   = nullptr;
    std::atomic<float>* imNoiseOnParam = nullptr;
    std::atomic<float>* imNoiseParam   = nullptr;
    std::atomic<float>* imWidthOnParam = nullptr;
    std::atomic<float>* imWidthParam   = nullptr;

    std::atomic<float>* autoOnParam    = nullptr;
    std::atomic<float>* autoSpeedParam = nullptr;
    std::atomic<float>* autoDepthParam = nullptr;
    std::atomic<float>* autoChaosParam = nullptr;

    std::atomic<juce::int64> lastAudioProcessMs { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlagblockAudioProcessor)
};

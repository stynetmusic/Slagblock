#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace slagblock
{

/** 2-op FM + Osc 3 (mod / carrier / mix) + independent Sub + LFO-modulated neuro bass engine. */
class FMEngine
{
public:
    static constexpr int maxVoices = 8;

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        invSampleRate = 1.0f / static_cast<float> (sampleRate);
        nyquist = static_cast<float> (sampleRate) * 0.49f;
        updateEnvelopeRates();
        updateFilterCoeffs (cutoffHz);
        reset();
    }

    void reset() noexcept
    {
        for (auto& v : voices)
            v = {};

        filtZ1 = filtZ2 = filtZ3 = filtZ4 = 0.0f;
        noiseState = 0x12345678u;
        lfoPhase = 0.0f;
    }

    void setRatio (float r) noexcept       { ratio = std::clamp (r, 0.25f, 16.0f); }
    void setIndex (float i) noexcept       { baseIndex = std::clamp (i, 0.0f, 24.0f); }
    void setDrive (float d) noexcept       { baseDrive = std::clamp (d, 0.5f, 32.0f); }
    void setFeedback (float f) noexcept    { feedback = std::clamp (f, 0.0f, 1.0f); }
    void setNoiseAmount (float n) noexcept { noiseAmount = std::clamp (n, 0.0f, 1.0f); }

    void setSubLevel (float level) noexcept { subLevel = std::clamp (level, 0.0f, 1.5f); }

    void setOsc3Ratio (float r) noexcept { osc3Ratio = std::clamp (r, 0.25f, 16.0f); }
    void setOsc3Index (float i) noexcept { osc3Index = std::clamp (i, 0.0f, 24.0f); }
    void setOsc3Level (float l) noexcept { osc3Level = std::clamp (l, 0.0f, 1.5f); }
    void setOsc3Wave  (int w) noexcept   { osc3Wave  = (w < 0) ? 0 : (w > 3 ? 3 : w); }
    /** 0 = extra modulator, 1 = extra carrier, 2 = additive mix */
    void setOsc3Role  (int r) noexcept   { osc3Role  = (r < 0) ? 0 : (r > 2 ? 2 : r); }

    /** 1 = one octave down, 2 = two octaves down */
    void setSubOctave (int octDown) noexcept
    {
        subOctaveDown = (octDown >= 2) ? 2 : 1;
        subFreqMul = (subOctaveDown == 2) ? 0.25f : 0.5f;
    }

    void setLfoRate (float hz) noexcept { lfoRateHz = std::clamp (hz, 0.01f, 30.0f); }
    void setLfoToCutoff (float a) noexcept { lfoToCutoff = std::clamp (a, 0.0f, 1.0f); }
    void setLfoToIndex  (float a) noexcept { lfoToIndex  = std::clamp (a, 0.0f, 1.0f); }
    void setLfoToDrive  (float a) noexcept { lfoToDrive  = std::clamp (a, 0.0f, 1.0f); }
    // Note: LFO→Amp is applied post-FX in the processor (output gain), not here.

    void setAttack (float seconds) noexcept
    {
        attackSec = std::clamp (seconds, 0.0005f, 3.0f);
        updateEnvelopeRates();
    }

    void setDecay (float seconds) noexcept
    {
        decaySec = std::clamp (seconds, 0.005f, 3.0f);
        updateEnvelopeRates();
    }

    void setSustain (float level) noexcept
    {
        sustainLevel = std::clamp (level, 0.0f, 1.0f);
        updateEnvelopeRates();
    }

    void setRelease (float seconds) noexcept
    {
        releaseSec = std::clamp (seconds, 0.005f, 5.0f);
        updateEnvelopeRates();
    }

    void setFilterCutoff (float hz) noexcept
    {
        baseCutoffHz = std::clamp (hz, 40.0f, nyquist);
        cutoffHz = baseCutoffHz;
        updateFilterCoeffs (cutoffHz);
    }

    void setFilterResonance (float r) noexcept
    {
        resonance = std::clamp (r, 0.0f, 1.0f);
        updateFilterCoeffs (cutoffHz);
    }

    void noteOn (int midiNote, float velocity) noexcept
    {
        if (midiNote < 0 || midiNote > 127)
            return;

        const float vel = std::clamp (velocity, 0.0f, 1.0f);
        const float freq = midiNoteToHz (midiNote);

        Voice* target = nullptr;
        float quietestEnv = 1.0e9f;

        for (auto& v : voices)
        {
            if (! v.active)
            {
                target = &v;
                break;
            }

            if (v.env < quietestEnv)
            {
                quietestEnv = v.env;
                target = &v;
            }
        }

        if (target == nullptr)
            target = &voices[0];

        target->active      = true;
        target->note        = midiNote;
        target->freq        = freq;
        target->velocity    = 0.25f + 0.75f * vel;
        target->env         = 0.0f;
        target->stage       = EnvStage::Attack;
        target->releaseRate = 0.0f;
        target->phaseC      = 0.0f;
        target->phaseM      = vel * 0.41f;
        target->phaseSub    = 0.0f;
        target->phase3      = vel * 0.17f;
        target->modFb       = 0.0f;
    }

    void noteOff (int midiNote) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == midiNote && v.stage != EnvStage::Release)
            {
                v.stage = EnvStage::Release;
                const float samples = std::max (1.0f, releaseSec * static_cast<float> (sampleRate));
                v.releaseRate = v.env / samples;
            }
        }
    }

    void allNotesOff() noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.stage != EnvStage::Release)
            {
                v.stage = EnvStage::Release;
                const float samples = std::max (1.0f, releaseSec * static_cast<float> (sampleRate));
                v.releaseRate = v.env / samples;
            }
        }
    }

    /**
     * Render mono → L/R. Optionally writes bipolar LFO (−1…+1) into lfoOut
     * (same length as numSamples) for post-FX amp modulation in the processor.
     */
    void process (float* left, float* right, int numSamples, float* lfoOut = nullptr) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float lfo = 0.0f;
            const float s = renderSample (lfo);
            left[i]  = s;
            right[i] = s;
            if (lfoOut != nullptr)
                lfoOut[i] = lfo;
        }
    }

    float renderSample (float& lfoOut) noexcept
    {
        // --- Global LFO (bipolar sine −1…1) ---
        lfoPhase += lfoRateHz * invSampleRate;
        if (lfoPhase >= 1.0f)
            lfoPhase -= std::floor (lfoPhase);

        const float lfo = std::sin (twoPiApprox * lfoPhase);
        lfoOut = lfo;

        // Modulated targets (amp is applied post-FX in PluginProcessor)
        const float modIndex = baseIndex + lfo * lfoToIndex * 8.0f;
        const float modDrive = std::max (0.5f, baseDrive + lfo * lfoToDrive * 8.0f);

        // Cutoff: exponential-ish sweep around base
        const float cutMul = std::pow (2.0f, lfo * lfoToCutoff * 2.5f); // ±2.5 oct at full depth
        const float modCutoff = std::clamp (baseCutoffHz * cutMul, 40.0f, nyquist);
        if (std::abs (modCutoff - cutoffHz) > 0.5f)
        {
            cutoffHz = modCutoff;
            updateFilterCoeffs (cutoffHz);
        }

        float mix = 0.0f;
        constexpr float twoPi = twoPiApprox;
        constexpr float pi    = 3.141592653589793f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            switch (v.stage)
            {
                case EnvStage::Attack:
                    v.env += attackInc;
                    if (v.env >= 1.0f)
                    {
                        v.env = 1.0f;
                        v.stage = EnvStage::Decay;
                    }
                    break;

                case EnvStage::Decay:
                    v.env -= decayInc;
                    if (v.env <= sustainLevel)
                    {
                        v.env = sustainLevel;
                        v.stage = EnvStage::Sustain;
                    }
                    break;

                case EnvStage::Sustain:
                    v.env = sustainLevel;
                    break;

                case EnvStage::Release:
                    v.env -= v.releaseRate;
                    if (v.env <= 1.0e-4f)
                    {
                        v.active = false;
                        v.env = 0.0f;
                        continue;
                    }
                    break;
            }

            const float phaseIncC   = v.freq * invSampleRate;
            const float phaseIncM   = v.freq * ratio * invSampleRate;
            const float phaseIncSub = v.freq * subFreqMul * invSampleRate;
            const float phaseInc3   = v.freq * osc3Ratio * invSampleRate;

            const float fbDepth = feedback * pi * 0.95f;
            const float o3      = evalWave (v.phase3, osc3Wave);
            const float lvl3    = osc3Level;

            // Osc 3 is gated by Level so Level=0 leaves the original 2-op sound untouched.
            const bool osc3On = lvl3 > 1.0e-4f;
            const float extraFm = (osc3On && osc3Role == 0) ? (osc3Index * lvl3 * o3) : 0.0f;
            const float series3 = extraFm * 0.35f;

            const float mod = std::sin (twoPi * v.phaseM + fbDepth * v.modFb + series3);
            v.modFb = mod;

            float osc3Audio = 0.0f;
            if (osc3On)
            {
                switch (osc3Role)
                {
                    case 0: // extra modulator — FM complexity plus a little dry presence
                        osc3Audio = o3 * lvl3 * 0.10f;
                        break;
                    case 1: // extra carrier, FM'd by the main modulator
                    {
                        const float fmShift = (osc3Index * mod) * (1.0f / twoPi);
                        osc3Audio = evalWave (v.phase3 + fmShift, osc3Wave) * lvl3 * 0.68f;
                        break;
                    }
                    default: // additive mix, Index folds the wave
                    {
                        const float fold = 1.0f + osc3Index * 0.18f;
                        osc3Audio = std::tanh (o3 * fold) * lvl3 * 0.68f;
                        break;
                    }
                }
            }

            // Main FM carrier (no fixed sub baked in)
            const float idx = std::max (0.0f, modIndex);
            const float fm  = std::sin (twoPi * v.phaseC + idx * mod + extraFm);
            const float noise = nextNoise() * noiseAmount * 1.35f;

            // Independent Sub oscillator
            const float sub = std::sin (twoPi * v.phaseSub);

            float voiceOut = fm * 0.78f
                           + sub * (subLevel * 0.95f)
                           + mod * (0.08f * std::min (idx, 14.0f) / 14.0f)
                           + osc3Audio
                           + noise;

            if (v.stage == EnvStage::Attack || (v.stage == EnvStage::Decay && v.env > 0.85f))
                voiceOut += noise * 0.35f;

            voiceOut *= v.env * v.velocity;
            mix += voiceOut;

            v.phaseC   += phaseIncC;
            v.phaseM   += phaseIncM;
            v.phaseSub += phaseIncSub;
            v.phase3   += phaseInc3;
            v.phaseC   -= std::floor (v.phaseC);
            v.phaseM   -= std::floor (v.phaseM);
            v.phaseSub -= std::floor (v.phaseSub);
            v.phase3   -= std::floor (v.phase3);
        }

        mix *= 0.48f;
        mix = processFilter (mix);

        const float pre  = mix * modDrive;
        const float wet  = std::tanh (pre);
        const float post = wet * (1.25f / (0.32f + 0.075f * modDrive));

        return std::clamp (post, -1.0f, 1.0f);
    }

private:
    enum class EnvStage { Attack, Decay, Sustain, Release };

    struct Voice
    {
        bool     active      = false;
        int      note        = -1;
        float    freq        = 440.0f;
        float    velocity    = 1.0f;
        float    env         = 0.0f;
        float    releaseRate = 0.0f;
        EnvStage stage       = EnvStage::Release;
        float    phaseC      = 0.0f;
        float    phaseM      = 0.0f;
        float    phaseSub    = 0.0f;
        float    phase3      = 0.0f;
        float    modFb       = 0.0f;
    };

    static float evalWave (float phase, int wave) noexcept
    {
        phase -= std::floor (phase);

        switch (wave)
        {
            case 1: // triangle
                return 1.0f - 4.0f * std::abs (phase - 0.5f);
            case 2: // saw
                return 2.0f * phase - 1.0f;
            case 3: // square
                return phase < 0.5f ? 1.0f : -1.0f;
            default:
                return std::sin (twoPiApprox * phase);
        }
    }

    static float midiNoteToHz (int note) noexcept
    {
        return 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
    }

    void updateEnvelopeRates() noexcept
    {
        const float sr = static_cast<float> (sampleRate);
        attackInc = 1.0f / std::max (1.0f, attackSec * sr);
        const float drop = std::max (0.0001f, 1.0f - sustainLevel);
        decayInc = drop / std::max (1.0f, decaySec * sr);
    }

    float nextNoise() noexcept
    {
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return static_cast<float> (static_cast<int32_t> (noiseState)) * (1.0f / 2147483648.0f);
    }

    void updateFilterCoeffs (float hz) noexcept
    {
        const float res = 0.05f + resonance * 0.92f;
        const float fc = std::clamp (hz, 40.0f, nyquist);
        const float x = std::exp (-twoPiApprox * fc * invSampleRate);
        filterG = 1.0f - x;
        filterFeedback = res * (1.0f + 1.0f / (filterG + 1.0e-6f)) * 0.35f;
    }

    float processFilter (float input) noexcept
    {
        float x = input - filterFeedback * filtZ4;
        x = std::tanh (x * 1.15f);

        filtZ1 += filterG * (x      - filtZ1);
        filtZ2 += filterG * (filtZ1 - filtZ2);
        filtZ3 += filterG * (filtZ2 - filtZ3);
        filtZ4 += filterG * (filtZ3 - filtZ4);

        return filtZ4 * 0.72f + filtZ2 * 0.28f;
    }

    static constexpr float twoPiApprox = 6.283185307179586f;

    double sampleRate    = 44100.0;
    float  invSampleRate = 1.0f / 44100.0f;
    float  nyquist       = 20000.0f;

    float attackSec    = 0.001f;
    float decaySec     = 0.05f;
    float sustainLevel = 0.58f;
    float releaseSec   = 0.08f;
    float attackInc    = 0.01f;
    float decayInc     = 0.01f;

    float ratio       = 2.01f;
    float baseIndex   = 11.0f;
    float baseDrive   = 7.5f;
    float feedback    = 0.55f;
    float noiseAmount = 0.12f;
    float baseCutoffHz = 2400.0f;
    float cutoffHz    = 2400.0f;
    float resonance   = 0.45f;

    // Independent sub
    float subLevel      = 0.55f;
    int   subOctaveDown = 1;
    float subFreqMul    = 0.5f;

    // Osc 3 / Operator 3
    float osc3Ratio = 3.01f;
    float osc3Index = 6.0f;
    float osc3Level = 0.0f;
    int   osc3Wave  = 0;
    int   osc3Role  = 0;

    // LFO (amp destination handled in PluginProcessor post-FX)
    float lfoRateHz   = 0.45f;
    float lfoPhase    = 0.0f;
    float lfoToCutoff = 0.0f;
    float lfoToIndex  = 0.0f;
    float lfoToDrive  = 0.0f;

    float filterG        = 0.5f;
    float filterFeedback = 0.0f;
    float filtZ1 = 0.0f, filtZ2 = 0.0f, filtZ3 = 0.0f, filtZ4 = 0.0f;

    uint32_t noiseState = 0x12345678u;

    std::array<Voice, maxVoices> voices {};
};

} // namespace slagblock

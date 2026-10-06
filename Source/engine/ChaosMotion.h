#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace slagblock
{

/**
 * Independent chaotic motion for every major synth/FX/imitation parameter.
 *
 * Not a regular LFO: each target is a mix of a drunken walk, cosine-interpolated
 * sample-and-hold, and one-pole-smoothed noise. Speed / depth / chaos scale the
 * whole field. Modulation is a process-time overlay — stored APVTS values stay
 * as the manual centre and are restored as soon as Auto fades out.
 */
class ChaosMotion
{
public:
    enum Target : int
    {
        Ratio = 0,
        Index,
        Drive,
        Feedback,
        Cutoff,
        Resonance,
        Noise,
        SubLevel,
        LfoRate,
        LfoCutoff,
        LfoIndex,
        LfoDrive,
        LfoAmp,
        FxDist,
        FxCrushBits,
        FxCrushRate,
        FxDelayTime,
        FxDelayFb,
        FxDelayMix,
        FxChorusMix,
        FxChorusDepth,
        ImVol,
        ImDrive,
        ImTime,
        ImFilt,
        ImCrush,
        ImNoise,
        ImWidth,
        Osc3Ratio,
        Osc3Index,
        Osc3Level,
        Count
    };

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        mix = 0.0f;
        wanted = false;
        wasWanted = false;
        initWalkers();
    }

    void bind (juce::AudioProcessorValueTreeState& apvts) noexcept
    {
        auto bindOne = [this, &apvts] (Target t, const char* id)
        {
            infos[static_cast<size_t> (t)].id    = id;
            infos[static_cast<size_t> (t)].param = apvts.getParameter (id);
        };

        bindOne (Ratio,         "ratio");
        bindOne (Index,         "index");
        bindOne (Drive,         "drive");
        bindOne (Feedback,      "feedback");
        bindOne (Cutoff,        "cutoff");
        bindOne (Resonance,     "resonance");
        bindOne (Noise,         "noise");
        bindOne (SubLevel,      "subLevel");
        bindOne (LfoRate,       "lfoRate");
        bindOne (LfoCutoff,     "lfoCutoff");
        bindOne (LfoIndex,      "lfoIndex");
        bindOne (LfoDrive,      "lfoDrive");
        bindOne (LfoAmp,        "lfoAmp");
        bindOne (FxDist,        "fxDist");
        bindOne (FxCrushBits,   "fxCrushBits");
        bindOne (FxCrushRate,   "fxCrushRate");
        bindOne (FxDelayTime,   "fxDelayTime");
        bindOne (FxDelayFb,     "fxDelayFb");
        bindOne (FxDelayMix,    "fxDelayMix");
        bindOne (FxChorusMix,   "fxChorusMix");
        bindOne (FxChorusDepth, "fxChorusDepth");
        bindOne (ImVol,         "imVol");
        bindOne (ImDrive,       "imDrive");
        bindOne (ImTime,        "imTime");
        bindOne (ImFilt,        "imFilt");
        bindOne (ImCrush,       "imCrush");
        bindOne (ImNoise,       "imNoise");
        bindOne (ImWidth,       "imWidth");
        bindOne (Osc3Ratio,     "osc3Ratio");
        bindOne (Osc3Index,     "osc3Index");
        bindOne (Osc3Level,     "osc3Level");
    }

    void setControls (bool on, float speed01, float depth01, float chaos01) noexcept
    {
        wanted      = on;
        speed       = juce::jlimit (0.0f, 1.0f, speed01);
        depth       = juce::jlimit (0.0f, 1.0f, depth01);
        chaosAmount = juce::jlimit (0.0f, 1.0f, chaos01);
    }

    /** Advance walkers by an explicit time step (editor idle motion). */
    void advanceSeconds (float dt) noexcept
    {
        const int n = juce::jmax (1, (int) std::lround ((double) juce::jlimit (1.0e-4f, 0.08f, dt) * sampleRate));
        advance (n);
    }

    /** Recompute live values from current APVTS centres. Safe after a visual-only advance. */
    void snapshotLive() noexcept
    {
        for (int i = 0; i < Count; ++i)
        {
            auto* p = infos[static_cast<size_t> (i)].param;
            if (p == nullptr)
                continue;
            const float base = p->convertFrom0to1 (p->getValue());
            modulate (static_cast<Target> (i), base);
        }
    }

    /** Advance walkers by one audio block. Call once per processBlock. */
    void advance (int numSamples) noexcept
    {
        const float dt = static_cast<float> (numSamples) / static_cast<float> (sampleRate);
        const float clampedDt = juce::jlimit (1.0e-5f, 0.05f, dt);

        if (wanted && ! wasWanted)
            scrambleAll();

        wasWanted = wanted;

        // Fast fade-in so Auto is immediately audible; slightly slower fade-out
        // so the return to the manual patch is click-free.
        const float tau   = wanted ? 0.012f : 0.045f;
        const float coef  = 1.0f - std::exp (-clampedDt / tau);
        const float target = wanted ? 1.0f : 0.0f;
        mix += (target - mix) * coef;
        if (std::abs (mix - target) < 0.0005f)
            mix = target;

        mixAtomic.store (mix, std::memory_order_relaxed);

        if (mix < 1.0e-4f && ! wanted)
        {
            for (int i = 0; i < Count; ++i)
                walkers[static_cast<size_t> (i)].smoothed = walkers[static_cast<size_t> (i)].value;
            return;
        }

        for (int i = 0; i < Count; ++i)
            walkers[static_cast<size_t> (i)].tick (clampedDt, speed, chaosAmount);
    }

    bool isAudible() const noexcept { return mix > 0.008f; }
    float getMix() const noexcept   { return mixAtomic.load (std::memory_order_relaxed); }

    /** Modulated value in parameter units. Safe to call from the audio thread. */
    float modulate (Target t, float base) noexcept
    {
        const size_t i = static_cast<size_t> (t);
        auto* p = infos[i].param;
        float actual = base;

        if (p != nullptr && mix > 1.0e-5f)
        {
            const float base01  = p->convertTo0to1 (base);
            const float offset  = walkers[i].smoothed * depth * kDepthScale[i];
            const float out01   = juce::jlimit (0.0f, 1.0f, base01 + offset);
            const float mixed01 = base01 + (out01 - base01) * mix;
            actual = p->convertFrom0to1 (mixed01);
        }

        infos[i].live.store (actual, std::memory_order_relaxed);
        return actual;
    }

    float getLive (Target t) const noexcept
    {
        return infos[static_cast<size_t> (t)].live.load (std::memory_order_relaxed);
    }

    float getLiveById (const char* id, float fallback) const noexcept
    {
        if (id == nullptr)
            return fallback;

        for (int i = 0; i < Count; ++i)
            if (infos[static_cast<size_t> (i)].id != nullptr
                && std::strcmp (infos[static_cast<size_t> (i)].id, id) == 0)
                return infos[static_cast<size_t> (i)].live.load (std::memory_order_relaxed);

        return fallback;
    }

    /** Normalised 0–1 live position for GUI markers. */
    float getLive01 (Target t) const noexcept
    {
        const size_t i = static_cast<size_t> (t);
        auto* p = infos[i].param;
        if (p == nullptr)
            return 0.0f;
        return p->convertTo0to1 (infos[i].live.load (std::memory_order_relaxed));
    }

    bool hasTargetId (const char* id) const noexcept
    {
        if (id == nullptr)
            return false;
        for (int i = 0; i < Count; ++i)
            if (infos[static_cast<size_t> (i)].id != nullptr
                && std::strcmp (infos[static_cast<size_t> (i)].id, id) == 0)
                return true;
        return false;
    }

private:
    struct Walker
    {
        uint32_t rngState = 1;
        float walk        = 0.0f;
        float walkVel     = 0.0f;
        float holdFrom    = 0.0f;
        float holdTo      = 0.0f;
        float holdPhase   = 1.0f;
        float holdRate    = 0.35f;
        float noise       = 0.0f;
        float value       = 0.0f;
        float smoothed    = 0.0f;
        float speedMul    = 1.0f;
        float jitterMul   = 1.0f;

        float next01() noexcept
        {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            return static_cast<float> (rngState) * (1.0f / 4294967295.0f);
        }

        float nextBipolar() noexcept { return next01() * 2.0f - 1.0f; }

        void scramble() noexcept
        {
            walk     = nextBipolar() * 0.82f;
            walkVel  = nextBipolar() * 0.65f;
            holdFrom = nextBipolar();
            holdTo   = nextBipolar();
            holdPhase = next01();
            holdRate  = 0.18f + next01() * 1.6f;
            noise     = nextBipolar() * 0.45f;
            value     = juce::jlimit (-1.0f, 1.0f,
                                      walk * 0.52f + holdFrom * 0.33f + noise * 0.22f);
            smoothed  = value;
        }

        void tick (float dt, float speed01, float chaos01) noexcept
        {
            // Map 0–1 speed to a wide but usable motion range.
            const float spd = (0.055f + speed01 * speed01 * 7.4f) * speedMul;

            // Occasional velocity kicks — more frequent / harder as chaos rises.
            if (next01() < (0.018f + chaos01 * 0.11f) * (0.35f + spd * 0.12f))
                walkVel += nextBipolar() * (0.55f + chaos01 * 1.65f);

            walkVel *= 0.978f - chaos01 * 0.03f;
            walk    += walkVel * dt * spd;

            if (walk > 1.0f)  { walk = 1.0f;  walkVel *= -0.35f; }
            if (walk < -1.0f) { walk = -1.0f; walkVel *= -0.35f; }

            // Cosine-interpolated random hold (slow morph, not a hard step).
            holdPhase += dt * holdRate * spd;
            while (holdPhase >= 1.0f)
            {
                holdPhase -= 1.0f;
                holdFrom   = holdTo;
                holdTo     = nextBipolar();
                holdRate   = 0.12f + next01() * (0.45f + chaos01 * 2.1f);
            }

            const float ht = 0.5f - 0.5f * std::cos (holdPhase * juce::MathConstants<float>::pi);
            const float holdVal = holdFrom + (holdTo - holdFrom) * ht;

            // Fast smoothed noise / jitter.
            const float n = nextBipolar();
            const float nHz = (1.6f + chaos01 * 22.0f) * jitterMul;
            const float nCoef = 1.0f - std::exp (-dt * nHz);
            noise += (n - noise) * nCoef;

            const float jitterW = 0.12f + chaos01 * 0.32f;
            const float raw = walk * 0.48f + holdVal * 0.36f + noise * jitterW;
            value = std::tanh (raw);

            const float smoothHz = 8.0f + speed01 * 28.0f + chaos01 * 18.0f;
            const float sCoef = 1.0f - std::exp (-dt * smoothHz);
            smoothed += (value - smoothed) * sCoef;
        }
    };

    struct Info
    {
        const char* id = nullptr;
        juce::RangedAudioParameter* param = nullptr;
        std::atomic<float> live { 0.0f };
    };

    static constexpr float kDepthScale[Count] = {
        0.64f, // Ratio
        0.90f, // Index
        0.82f, // Drive
        0.74f, // Feedback
        0.94f, // Cutoff
        0.72f, // Resonance
        0.58f, // Noise
        0.50f, // SubLevel
        0.78f, // LfoRate
        0.88f, // LfoCutoff
        0.88f, // LfoIndex
        0.82f, // LfoDrive
        0.74f, // LfoAmp
        0.84f, // FxDist
        0.70f, // FxCrushBits
        0.72f, // FxCrushRate
        0.62f, // FxDelayTime
        0.58f, // FxDelayFb
        0.60f, // FxDelayMix
        0.64f, // FxChorusMix
        0.68f, // FxChorusDepth
        0.52f, // ImVol
        0.80f, // ImDrive
        0.72f, // ImTime
        0.84f, // ImFilt
        0.74f, // ImCrush
        0.62f, // ImNoise
        0.72f, // ImWidth
        0.70f, // Osc3Ratio
        0.86f, // Osc3Index
        0.68f  // Osc3Level
    };

    void initWalkers() noexcept
    {
        // Distinct seeds + characters so nothing moves in lockstep.
        static constexpr uint32_t kSeeds[Count] = {
            0xA341316Cu, 0xC8013EA4u, 0xAD90777Du, 0x7E95761Eu,
            0x690C3C5Au, 0x2E6D31BBu, 0x91E10DA5u, 0x4F57C5B1u,
            0xB5D99BEEu, 0x17F116D1u, 0x6C3E9A27u, 0xD4A5112Fu,
            0x8B13C0D9u, 0x3A27E64Bu, 0xF1C02A88u, 0x59D7B403u,
            0xC29E17A6u, 0x0E84F5D2u, 0xA7B3901Cu, 0x41F6C8E5u,
            0xE20B4D73u, 0x76A1E29Fu, 0x9C58D014u, 0x25F3A8CBu,
            0xD8916B3Eu, 0x4C07F9A2u, 0xB3E24D70u, 0x18A9C65Fu,
            0x5E8C2A17u, 0xF4B07D91u, 0x2C6E19A3u
        };

        for (int i = 0; i < Count; ++i)
        {
            auto& w = walkers[static_cast<size_t> (i)];
            w = {};
            w.rngState = kSeeds[static_cast<size_t> (i)];
            if (w.rngState == 0)
                w.rngState = 1;

            // Spread speeds ~0.35x … 1.9x and jitter characters.
            const float t = static_cast<float> (i) / static_cast<float> (Count - 1);
            w.speedMul  = 0.34f + t * 1.55f + (static_cast<float> (i % 5) * 0.07f);
            w.jitterMul = 0.45f + static_cast<float> ((i * 7) % 11) * 0.12f;
            w.scramble();
            w.value = 0.0f;
            w.smoothed = 0.0f;
        }
    }

    void scrambleAll() noexcept
    {
        for (auto& w : walkers)
            w.scramble();
    }

    std::array<Walker, Count> walkers {};
    std::array<Info, Count>   infos   {};

    double sampleRate = 44100.0;
    float  mix        = 0.0f;
    float  speed      = 0.48f;
    float  depth      = 0.78f;
    float  chaosAmount = 0.62f;
    bool   wanted     = false;
    bool   wasWanted  = false;
    std::atomic<float> mixAtomic { 0.0f };
};

} // namespace slagblock

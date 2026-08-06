#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

namespace slagblock
{

/**
 * Neuro / DnB FX rack with live-tweakable parameters:
 *   wavefolder → bitcrush → chorus/unison → slap delay
 */
class NeuroFX
{
public:
    void prepare (double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        invSampleRate = 1.0f / static_cast<float> (sampleRate);

        const int maxDelay = static_cast<int> (sampleRate * 1.5) + 8;
        delayL.assign (static_cast<size_t> (maxDelay), 0.0f);
        delayR.assign (static_cast<size_t> (maxDelay), 0.0f);
        writePos = 0;
        delaySize = maxDelay;
        reset();
    }

    void reset() noexcept
    {
        std::fill (delayL.begin(), delayL.end(), 0.0f);
        std::fill (delayR.begin(), delayR.end(), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
        crushHold = 0.0f;
        crushCounter = 0;
    }

    void setEnabled (bool on) noexcept { enabled = on; }
    bool isEnabled() const noexcept { return enabled; }

    // --- Live parameters ---
    void setDistortion (float amount) noexcept { distortion = std::clamp (amount, 0.0f, 1.0f); }
    void setCrushBits  (float bits)   noexcept { crushBits  = std::clamp (bits, 4.0f, 16.0f); }
    void setCrushRate  (float rate)   noexcept { crushRate  = std::clamp (rate, 0.05f, 1.0f); }
    void setDelayTimeMs (float ms)    noexcept { delayTimeMs = std::clamp (ms, 10.0f, 800.0f); }
    void setDelayFeedback (float fb)  noexcept { delayFeedback = std::clamp (fb, 0.0f, 0.92f); }
    void setDelayMix (float mix)      noexcept { delayMix = std::clamp (mix, 0.0f, 1.0f); }
    void setChorusMix (float mix)     noexcept { chorusMix = std::clamp (mix, 0.0f, 1.0f); }
    void setChorusDepth (float d)     noexcept { chorusDepth = std::clamp (d, 0.0f, 1.0f); }

    void process (float* left, float* right, int numSamples) noexcept
    {
        if (! enabled || left == nullptr)
            return;

        const bool stereo = (right != nullptr && right != left);

        // Map UI distortion 0–1 → fold drive + wet
        const float foldDrive = 1.0f + distortion * 5.5f;
        const float foldMix   = distortion * 0.85f;

        // Crush wet follows how aggressive bits/rate are (still fully user-driven)
        const float crushWet = std::clamp ((16.0f - crushBits) / 12.0f * 0.55f
                                           + (1.0f - crushRate) * 0.45f, 0.0f, 1.0f);

        // Chorus depth in ms modulation
        const float depthMs = 0.5f + chorusDepth * 8.0f;
        const float chorusRateHz = 0.25f + chorusDepth * 0.55f;

        for (int i = 0; i < numSamples; ++i)
        {
            float dry = left[i];
            float x = dry;

            // 1) Wavefolder / extra distortion
            if (foldMix > 0.001f)
            {
                const float folded = wavefold (dry * foldDrive);
                x = dry * (1.0f - foldMix) + folded * foldMix;
            }

            // 2) Bitcrush
            if (crushWet > 0.001f)
                x = bitcrush (x, crushWet);

            // 3) Chorus / unison
            lfoPhase += chorusRateHz * invSampleRate;
            if (lfoPhase >= 1.0f)
                lfoPhase -= std::floor (lfoPhase);

            const float lfo = std::sin (twoPi * lfoPhase);
            const float baseMs = 9.0f + chorusDepth * 6.0f;
            const float modMs  = depthMs * lfo;
            const float delayMsL = baseMs + modMs;
            const float delayMsR = baseMs - modMs + 1.4f;

            float wetL = x;
            float wetR = x;

            if (chorusMix > 0.001f)
            {
                const float chL = readDelay (delayL, delayMsL);
                const float chR = readDelay (delayR, delayMsR);
                wetL = x * (1.0f - chorusMix) + chL * chorusMix;
                wetR = x * (1.0f - chorusMix) + chR * chorusMix;
            }

            // 4) Slap delay
            const float dL = readDelay (delayL, delayTimeMs);
            const float dR = readDelay (delayR, delayTimeMs * 1.08f);

            if (delayMix > 0.001f)
            {
                wetL = wetL * (1.0f - delayMix) + dL * delayMix;
                wetR = wetR * (1.0f - delayMix) + dR * delayMix;
            }

            // Delay line write with feedback
            const float feed = 0.5f * (wetL + wetR);
            const float fbIn = std::tanh (feed + delayFeedback * 0.5f * (dL + dR));
            writeDelay (fbIn);

            wetL = std::tanh (wetL * 1.04f);
            wetR = std::tanh (wetR * 1.04f);

            left[i] = wetL;
            if (stereo)
                right[i] = wetR;
            else
                left[i] = 0.5f * (wetL + wetR);
        }
    }

private:
    static constexpr float twoPi = 6.283185307179586f;

    float wavefold (float x) const noexcept
    {
        const float s = std::sin (x);
        float y = x;
        // Limit fold iterations for safety
        for (int n = 0; n < 8 && (y > 1.0f || y < -1.0f); ++n)
        {
            if (y > 1.0f)  y = 2.0f - y;
            if (y < -1.0f) y = -2.0f - y;
        }
        return s * 0.55f + y * 0.45f;
    }

    float bitcrush (float x, float wet) noexcept
    {
        const int holdSamples = std::max (1, static_cast<int> (1.0f / std::max (0.05f, crushRate)));
        if (++crushCounter >= holdSamples)
        {
            crushCounter = 0;
            const float levels = std::pow (2.0f, crushBits);
            crushHold = std::floor (x * levels + 0.5f) / levels;
        }
        return x * (1.0f - wet) + crushHold * wet;
    }

    float readDelay (const std::vector<float>& buf, float delayMs) const noexcept
    {
        if (delaySize <= 1 || buf.empty())
            return 0.0f;

        float delaySamples = delayMs * 0.001f * static_cast<float> (sampleRate);
        delaySamples = std::clamp (delaySamples, 1.0f, static_cast<float> (delaySize - 2));

        float readPos = static_cast<float> (writePos) - delaySamples;
        while (readPos < 0.0f)
            readPos += static_cast<float> (delaySize);

        const int i0 = static_cast<int> (readPos) % delaySize;
        const int i1 = (i0 + 1) % delaySize;
        const float frac = readPos - std::floor (readPos);
        return buf[static_cast<size_t> (i0)] * (1.0f - frac)
             + buf[static_cast<size_t> (i1)] * frac;
    }

    void writeDelay (float sample) noexcept
    {
        if (delaySize <= 0 || delayL.empty())
            return;

        delayL[static_cast<size_t> (writePos)] = sample;
        delayR[static_cast<size_t> (writePos)] = sample * 0.97f;
        writePos = (writePos + 1) % delaySize;
    }

    bool enabled = false;
    double sampleRate = 44100.0;
    float invSampleRate = 1.0f / 44100.0f;

    float distortion    = 0.55f;
    float crushBits     = 9.0f;
    float crushRate     = 0.38f;
    float delayTimeMs   = 118.0f;
    float delayFeedback = 0.30f;
    float delayMix      = 0.20f;
    float chorusMix     = 0.28f;
    float chorusDepth   = 0.45f;

    std::vector<float> delayL, delayR;
    int writePos = 0;
    int delaySize = 0;

    float lfoPhase = 0.0f;
    float crushHold = 0.0f;
    int crushCounter = 0;
};

} // namespace slagblock

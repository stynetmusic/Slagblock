#pragma once

#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace slagblock
{

/**
 * Simplified Cableguys ShaperBox–style multi-effect rack:
 * Volume → Drive → Time → Filter → Crush → Noise → Width/Pan
 * Each stage has enable + one main amount control.
 */
class ShaperImitation
{
public:
    void prepare (double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        invSampleRate = 1.0f / static_cast<float> (sampleRate);
        nyquist = static_cast<float> (sampleRate) * 0.49f;

        const int maxDelay = static_cast<int> (sampleRate * 1.2) + 8;
        delayBuf.assign (static_cast<size_t> (maxDelay), 0.0f);
        delaySize = maxDelay;
        writePos = 0;
        reset();
    }

    void reset() noexcept
    {
        std::fill (delayBuf.begin(), delayBuf.end(), 0.0f);
        writePos = 0;
        volPhase = 0.0f;
        filtZ = 0.0f;
        crushHoldL = crushHoldR = 0.0f;
        crushCounter = 0;
        noiseState = 0xC0FFEEu;
    }

    void setRackEnabled (bool on) noexcept { rackOn = on; }

    void setVolume (bool on, float amount) noexcept
    {
        volOn = on;
        volAmount = std::clamp (amount, 0.0f, 1.0f);
    }

    void setDrive (bool on, float amount) noexcept
    {
        driveOn = on;
        driveAmount = std::clamp (amount, 0.0f, 1.0f);
    }

    void setTime (bool on, float amount) noexcept
    {
        timeOn = on;
        timeAmount = std::clamp (amount, 0.0f, 1.0f);
    }

    void setFilter (bool on, float amount) noexcept
    {
        filtOn = on;
        filtAmount = std::clamp (amount, 0.0f, 1.0f);
        // amount 0 = open (~12k), 1 = closed (~120 Hz)
        const float hz = 120.0f * std::pow (12000.0f / 120.0f, 1.0f - filtAmount);
        const float g = 1.0f - std::exp (-6.2831853f * std::min (hz, nyquist) * invSampleRate);
        filtG = std::clamp (g, 0.0005f, 0.95f);
    }

    void setCrush (bool on, float amount) noexcept
    {
        crushOn = on;
        crushAmount = std::clamp (amount, 0.0f, 1.0f);
    }

    void setNoise (bool on, float amount) noexcept
    {
        noiseOn = on;
        noiseAmount = std::clamp (amount, 0.0f, 1.0f);
    }

    void setWidth (bool on, float amount) noexcept
    {
        widthOn = on;
        // 0 = mono, 0.5 = normal, 1 = super wide + pan drift feel
        widthAmount = std::clamp (amount, 0.0f, 1.0f);
    }

    void process (float* left, float* right, int numSamples) noexcept
    {
        if (! rackOn || left == nullptr)
            return;

        const bool stereo = (right != nullptr && right != left);
        if (! stereo)
            right = left;

        for (int i = 0; i < numSamples; ++i)
        {
            float l = left[i];
            float r = right[i];

            // --- 1) Volume Shaper (hard-edged trem / step sequence feel) ---
            if (volOn && volAmount > 0.001f)
            {
                // Rate 1–12 Hz from amount (higher amount also deeper)
                const float rateHz = 1.0f + volAmount * 11.0f;
                volPhase += rateHz * invSampleRate;
                if (volPhase >= 1.0f)
                    volPhase -= std::floor (volPhase);

                // Pulse-ish shape: duck for first 45% of cycle
                const float gate = (volPhase < 0.45f)
                                       ? (0.05f + 0.25f * (volPhase / 0.45f)) // ramp up in open? actually duck
                                       : 1.0f;
                // Invert: low in first part for classic volume sequence punch
                const float seq = (volPhase < (0.15f + 0.25f * volAmount)) ? 0.08f : 1.0f;
                const float shaped = seq * (1.0f - 0.35f * std::sin (volPhase * 6.2831853f));
                const float gain = 1.0f - volAmount * (1.0f - shaped);
                l *= gain;
                r *= gain;
                (void) gate;
            }

            // --- 2) Drive ---
            if (driveOn && driveAmount > 0.001f)
            {
                const float pre = 1.0f + driveAmount * 12.0f;
                const float wetL = std::tanh (l * pre);
                const float wetR = std::tanh (r * pre);
                const float mix = 0.35f + driveAmount * 0.65f;
                l = l * (1.0f - mix) + wetL * mix;
                r = r * (1.0f - mix) + wetR * mix;
            }

            // --- 3) Time (slap delay) ---
            if (timeOn && timeAmount > 0.001f)
            {
                const float delayMs = 20.0f + timeAmount * 280.0f; // 20–300 ms
                const float mix = 0.12f + timeAmount * 0.35f;
                const float fb = 0.15f + timeAmount * 0.35f;
                const float delayed = readDelay (delayMs);
                l = l * (1.0f - mix) + delayed * mix;
                r = r * (1.0f - mix) + delayed * mix * 0.96f;
                writeDelay (std::tanh (0.5f * (l + r) + delayed * fb));
            }
            else
            {
                // Keep delay line moving mildly to avoid zipper when enabling
                writeDelay (0.5f * (l + r) * 0.001f);
            }

            // --- 4) Filter (LP) ---
            if (filtOn && filtAmount > 0.001f)
            {
                const float mono = 0.5f * (l + r);
                filtZ += filtG * (mono - filtZ);
                const float mix = 0.4f + filtAmount * 0.6f;
                l = l * (1.0f - mix) + filtZ * mix;
                r = r * (1.0f - mix) + filtZ * mix;
            }

            // --- 5) Crush ---
            if (crushOn && crushAmount > 0.001f)
            {
                const float bits = 12.0f - crushAmount * 8.0f; // 12 → 4
                const float rate = 1.0f - crushAmount * 0.85f; // 1 → 0.15
                const int hold = std::max (1, static_cast<int> (1.0f / std::max (0.08f, rate)));
                if (++crushCounter >= hold)
                {
                    crushCounter = 0;
                    const float levels = std::pow (2.0f, bits);
                    crushHoldL = std::floor (l * levels + 0.5f) / levels;
                    crushHoldR = std::floor (r * levels + 0.5f) / levels;
                }
                const float mix = 0.3f + crushAmount * 0.7f;
                l = l * (1.0f - mix) + crushHoldL * mix;
                r = r * (1.0f - mix) + crushHoldR * mix;
            }

            // --- 6) Noise ---
            if (noiseOn && noiseAmount > 0.001f)
            {
                const float n = nextNoise() * noiseAmount * 0.35f;
                l += n;
                r += n * 0.92f;
            }

            // --- 7) Width / Pan ---
            if (widthOn)
            {
                // Mid-side width
                const float mid  = 0.5f * (l + r);
                const float side = 0.5f * (l - r);
                const float w = 0.2f + widthAmount * 2.2f; // mono-ish → wide
                float outL = mid + side * w;
                float outR = mid - side * w;

                // Mild pan bias when amount high
                const float pan = (widthAmount - 0.5f) * 0.35f;
                outL *= 1.0f - std::max (0.0f,  pan);
                outR *= 1.0f - std::max (0.0f, -pan);

                l = outL;
                r = outR;
            }

            l = std::clamp (l, -1.2f, 1.2f);
            r = std::clamp (r, -1.2f, 1.2f);

            left[i] = l;
            if (stereo)
                right[i] = r;
        }
    }

private:
    float nextNoise() noexcept
    {
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return static_cast<float> (static_cast<int32_t> (noiseState)) * (1.0f / 2147483648.0f);
    }

    float readDelay (float delayMs) const noexcept
    {
        if (delaySize <= 1 || delayBuf.empty())
            return 0.0f;

        float ds = delayMs * 0.001f * static_cast<float> (sampleRate);
        ds = std::clamp (ds, 1.0f, static_cast<float> (delaySize - 2));
        float rp = static_cast<float> (writePos) - ds;
        while (rp < 0.0f)
            rp += static_cast<float> (delaySize);

        const int i0 = static_cast<int> (rp) % delaySize;
        const int i1 = (i0 + 1) % delaySize;
        const float frac = rp - std::floor (rp);
        return delayBuf[static_cast<size_t> (i0)] * (1.0f - frac)
             + delayBuf[static_cast<size_t> (i1)] * frac;
    }

    void writeDelay (float s) noexcept
    {
        if (delaySize <= 0 || delayBuf.empty())
            return;
        delayBuf[static_cast<size_t> (writePos)] = s;
        writePos = (writePos + 1) % delaySize;
    }

    bool rackOn = false;
    double sampleRate = 44100.0;
    float invSampleRate = 1.0f / 44100.0f;
    float nyquist = 20000.0f;

    bool  volOn = false;
    float volAmount = 0.5f;
    float volPhase = 0.0f;

    bool  driveOn = false;
    float driveAmount = 0.5f;

    bool  timeOn = false;
    float timeAmount = 0.4f;

    bool  filtOn = false;
    float filtAmount = 0.4f;
    float filtG = 0.2f;
    float filtZ = 0.0f;

    bool  crushOn = false;
    float crushAmount = 0.4f;
    float crushHoldL = 0.0f, crushHoldR = 0.0f;
    int   crushCounter = 0;

    bool  noiseOn = false;
    float noiseAmount = 0.2f;
    uint32_t noiseState = 0xC0FFEEu;

    bool  widthOn = false;
    float widthAmount = 0.5f;

    std::vector<float> delayBuf;
    int writePos = 0;
    int delaySize = 0;
};

} // namespace slagblock

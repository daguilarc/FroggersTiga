#pragma once

// synth_froggers::dsp::{DelayParams, StereoDelay, MapRowsToDelayParams} --
// packet 3 task 3.10 (extended DSP port, design D15).
// openspec/changes/froggers-sheaf-app/tasks.md section 3, item 3.10. A
// **copy** (design D3) of the cited Froggers formulas -- read directly from
// the frozen source before porting, not from memory.
//
// CORRECTED SCOPE (per this task's own brief, lead review 2026-07-26): an
// earlier draft claimed 8 of 9 Delay-page params had no frozen original
// because it searched only src/core/. A full original exists in
// sim/DelayState.hpp + sim/StereoDelay.hpp (absent from src/core/ because
// the delay is a sim/desktop feature the Daisy firmware never shipped).
// All nine params are ported here; nothing on this page is newly authored.
//
// Ported from:
//   - sim/StereoDelay.hpp (whole file) -- DelayParams, WetPair (renamed
//     DelayWetPair to avoid any collision), and StereoDelay::{setSampleRate,
//     clearBuffers, process, toReverbMono, readAt, writeSample, wrapIndex,
//     advanceWrite}, verbatim.
//   - sim/DelayState.hpp:165-198 (processInsert) -- specifically the
//     row -> DelayParams mapping at :180-186 (dtim/dsnd/dfbk/dwid/ddet/
//     dmod/dmix <- rows 0-6) and the Color/Halo folding at :187-193
//     (`params.ddet = clamp(0.5*(ddet+color))`,
//     `params.dmod = clamp(0.5*(dmod+halo))`, folded under
//     `m_useV2Layout` -- this app IS the V2 layout, so the fold always
//     applies), plus the final `delay.process(bumpIn, params)` /
//     `delay.toReverbMono(bumpIn, wet, params.dmix)` call shape at :196-198.
//
// NOT ported (deliberately, per this task's explicit instruction): the rest
// of DelayState -- knob smoothing (RuntimeParam `smoothed[]`), mod-source /
// mod-depth routing (`modSource`, `modDepth`, `blendKnob`), the fuego
// cascade (`blendRow`/`Fuegoize`/`V2FuegoStack` -- packet 3.5 already ported
// Fuegoize itself as its own standalone unit; the seam that applies it is
// packet 5's concern), crispy-row handling, and randomize/sanitize
// machinery (`randomizeKnobs`, `randomizeMod`, `clearModRoutesForIndex`,
// `sanitizeModSources`). Sheaf's parameter model and packet 5's fuego seam
// already own all of that -- this port is the AUDIO path only: the row ->
// DelayParams mapping, the Color/Halo fold, and StereoDelay itself.

#include "DspMath.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace synth_froggers::dsp {

// sim/StereoDelay.hpp:10-19.
struct DelayParams
{
    float dtim = 0.0f;
    float dsnd = 0.0f;
    float dfbk = 0.0f;
    float dwid = 0.0f;
    float ddet = 0.0f;
    float dmod = 0.0f;
    float dmix = 0.0f;
};

// sim/StereoDelay.hpp:21-25 (WetPair).
struct DelayWetPair
{
    float l = 0.0f;
    float r = 0.0f;
};

// sim/StereoDelay.hpp:27-157 (StereoDelay), verbatim.
struct StereoDelay
{
    static constexpr float kMaxDelaySeconds = 2.0f;
    static constexpr size_t kMaxDelaySamples = 96000;
    static constexpr float kMaxDetuneCents = 50.0f;

    // :33-40 (clearBuffers).
    void ClearBuffers()
    {
        std::fill(lineL.begin(), lineL.end(), 0.0f);
        std::fill(lineR.begin(), lineR.end(), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
        lastWet = {};
    }

    // Task 2.3 (Tier 1 recovery, app/FroggersAppCore.hpp): a thin name
    // alias, not a new implementation -- delegates straight to the existing
    // ClearBuffers() above so RecoverIfNonFinite<Unit>() (FroggersAppCore.hpp)
    // can call a uniform Reset() across every recoverable unit without a
    // special case for this one's differently-named method.
    void Reset() { ClearBuffers(); }

    // Task 2.3: dsp::StereoDelay::Process() has an early-return guard
    // (`p.dsnd <= 0.0001f`) that most patches take, since Send defaults to
    // 0 -- but once Send is nonzero this unit is just as exposed to
    // upstream-fault cascades as dsp::Reverb (ReadAt/WriteSample have no
    // finiteness guard of their own), so it gets the same Tier 1 coverage.
    bool StateFinite() const
    {
        if (!std::isfinite(lfoPhase) || !std::isfinite(lastWet.l) || !std::isfinite(lastWet.r))
        {
            return false;
        }
        for (const float sample : lineL)
        {
            if (!std::isfinite(sample))
            {
                return false;
            }
        }
        for (const float sample : lineR)
        {
            if (!std::isfinite(sample))
            {
                return false;
            }
        }
        return true;
    }

    // :42-56 (setSampleRate).
    void SetSampleRate(float hz)
    {
        sampleRate = hz > 0.0f ? hz : 44100.0f;
        capacity = std::min(kMaxDelaySamples, static_cast<size_t>(std::ceil(kMaxDelaySeconds * sampleRate)));
        if (capacity < 4)
        {
            capacity = 4;
        }
        lineL.assign(capacity, 0.0f);
        lineR.assign(capacity, 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
        lfoInc = 2.0f * 3.14159265f * 0.25f / sampleRate;
    }

    // :58-101 (process).
    DelayWetPair Process(float bumpIn, const DelayParams& p)
    {
        if (p.dsnd <= 0.0001f || capacity == 0)
        {
            lastWet = {};
            return lastWet;
        }

        const float baseSeconds = ExpMapCompute(0.001f, kMaxDelaySeconds, p.dtim);
        lfoPhase += lfoInc;
        if (lfoPhase > 6.2831853f)
        {
            lfoPhase -= 6.2831853f;
        }
        const float modSeconds = std::sin(lfoPhase) * p.dmod * baseSeconds * 0.08f;
        const float widthSpread = p.dwid * baseSeconds * 0.35f;
        float timeL = std::max(0.001f, baseSeconds + modSeconds);
        float timeR = std::max(0.001f, baseSeconds + modSeconds + widthSpread);

        const float detune = std::min(std::max(p.ddet, 0.0f), 1.0f);
        const float cents = detune * kMaxDetuneCents;
        const float ratioL = std::pow(2.0f, cents / 1200.0f);
        const float ratioR = std::pow(2.0f, -cents / 1200.0f);
        timeL /= ratioL;
        timeR /= ratioR;

        const float dL = ReadAt(timeL, lineL);
        const float dR = ReadAt(timeR, lineR);

        const float cross = p.dwid * 0.5f;
        const float fbL = dL * (1.0f - cross) + dR * cross;
        const float fbR = dR * (1.0f - cross) + dL * cross;
        const float fbk = std::min(std::max(p.dfbk, 0.0f), 0.98f);
        const float send = std::min(std::max(p.dsnd, 0.0f), 1.0f);
        const float inSignal = bumpIn * send;

        WriteSample(inSignal + fbL * fbk, lineL);
        WriteSample(inSignal + fbR * fbk, lineR);
        AdvanceWrite();

        lastWet.l = dL;
        lastWet.r = dR;
        return lastWet;
    }

    DelayWetPair GetLastWet() const { return lastWet; }

    // :108-113 (toReverbMono).
    float ToReverbMono(float bumpIn, DelayWetPair wet, float dmix) const
    {
        const float mix = std::min(std::max(dmix, 0.0f), 1.0f);
        const float monoWet = (wet.l + wet.r) * 0.5f;
        return (1.0f - mix) * bumpIn + mix * monoWet;
    }

private:
    // :116-124 (readAt). NOTE: before `writePos` has advanced past
    // `delaySamples` (i.e. during the first ~delaySamples calls after
    // construction/clear), `readPos` is negative, and casting a negative
    // float straight to `size_t` is a negative-float-to-unsigned
    // conversion the standard leaves unspecified/UB when the value is out
    // of range -- identical to the frozen sim/StereoDelay.hpp:120.
    //
    // FIX, NOT A REPRODUCTION (task "Fix 1b", strict-executor brief): as
    // with DigitalReorganizer::Process (Drive.hpp), there is no correct
    // frozen reference to port here instead -- the frozen sim/StereoDelay.hpp
    // hits the identical UB, so carrying it forward would have no parity
    // value. The intended behavior during warm-up (documented above as
    // "confirmed harmless on this target -- correct silence, since the
    // still-unwritten region of the buffer is zero-filled") is made
    // portable and well-defined by flooring and wrapping the index in a
    // signed integral domain *before* ever narrowing to `size_t`, instead
    // of relying on this target's incidental truncating-cast-of-negative-
    // float-saturates-to-0 behavior. For any `readPos >= 0` (the
    // already-well-defined case) `static_cast<long long>(floorPos)` equals
    // the original `static_cast<size_t>(readPos)` exactly (both truncate
    // toward zero for non-negative values, and `readPos - floor(readPos)`
    // is unchanged), so behavior is bit-for-bit identical there; only the
    // negative case, previously UB, is newly defined (floor-mod wraps into
    // the buffer, reading back the zero it was cleared to).
    float ReadAt(float seconds, const std::vector<float>& line) const
    {
        const float delaySamples = seconds * sampleRate;
        const float readPos = static_cast<float>(writePos) - delaySamples;
        const float floorPos = std::floor(readPos);
        const float frac = readPos - floorPos;

        const long long capacityI = static_cast<long long>(capacity);
        long long idx0I = static_cast<long long>(floorPos) % capacityI;
        if (idx0I < 0)
        {
            idx0I += capacityI;
        }
        const size_t idx0 = WrapIndex(static_cast<size_t>(idx0I));
        const size_t idx1 = WrapIndex(idx0 + 1);
        return line[idx0] * (1.0f - frac) + line[idx1] * frac;
    }

    // :126-129 (writeSample).
    void WriteSample(float sample, std::vector<float>& line) { line[writePos] = sample; }

    // :131-138 (wrapIndex).
    size_t WrapIndex(size_t idx) const
    {
        while (idx >= capacity)
        {
            idx -= capacity;
        }
        return idx;
    }

    // :140-147 (advanceWrite).
    void AdvanceWrite()
    {
        writePos++;
        if (writePos >= capacity)
        {
            writePos = 0;
        }
    }

    float sampleRate = 44100.0f;
    size_t capacity = 0;
    size_t writePos = 0;
    std::vector<float> lineL;
    std::vector<float> lineR;
    DelayWetPair lastWet{};
    float lfoPhase = 0.0f;
    float lfoInc = 0.0f;
};

// sim/DelayState.hpp:165-198 (processInsert), the row -> DelayParams mapping
// (:180-186) and the Color/Halo fold (:187-193) only. Callers supply the
// nine already-fuegoized/modulated 0..1 row values (rows 0-8); this
// function owns none of the smoothing/mod/fuego machinery that produces
// them (see file header "NOT ported").
inline DelayParams MapRowsToDelayParams(float row0Time,
                                        float row1Send,
                                        float row2Feedback,
                                        float row3Width,
                                        float row4Detune,
                                        float row5Mod,
                                        float row6Mix,
                                        float row7Color,
                                        float row8Halo)
{
    DelayParams params;
    params.dtim = row0Time;   // :180
    params.dsnd = row1Send;   // :181
    params.dfbk = row2Feedback;  // :182
    params.dwid = row3Width;  // :183
    params.ddet = row4Detune;  // :184
    params.dmod = row5Mod;    // :185
    params.dmix = row6Mix;    // :186

    // :187-193 -- Color/Halo bias ddet/dmod, folded under m_useV2Layout
    // (this app is exactly that layout, so the fold always applies here).
    params.ddet = std::min(std::max(0.5f * (params.ddet + row7Color), 0.0f), 1.0f);
    params.dmod = std::min(std::max(0.5f * (params.dmod + row8Halo), 0.0f), 1.0f);
    return params;
}

}  // namespace synth_froggers::dsp

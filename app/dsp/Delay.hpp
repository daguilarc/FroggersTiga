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
#include "FilterFx.hpp"
#include "Limiter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace synth_froggers::dsp {

// B6a (openspec/changes/frogg3rs-modulation-truth-and-voicing/tasks.md,
// "Group B outcomes" / operator: "why can't we just have limiters for
// reverb and delay?"): tuning for `StereoDelay::wetLimiterL`/`wetLimiterR`
// below, a per-channel pair of `dsp::OutputLimiter` instances (dsp/
// Limiter.hpp) inserted on the WET tap (`dL`/`dR`), strictly AFTER the
// feedback loop's own `WriteSample` calls -- so the loop keeps writing the
// unlimited value (B2's in-loop `PadeSaturator::Saturate` bound,
// `|inSignal| + fbk`, is completely untouched) and only what ESCAPES this
// stage toward Reverb/the master limiter gets bounded further.
//
// Chosen BY MEASUREMENT (scratch harness driving this exact struct with
// dtim swept from 0.0 to 1.0, dfbk pinned to 1.0 -> fbk clamps to 0.98,
// dsnd=1.0, dwid=0.3, sustained full-scale input, 20000 samples per point --
// not by analogy to the master limiter, following this task's own binding
// warning that analogy-picked constants have been measured wrong before):
//
//   - Starting point was the master's own tuning (threshold 0.9, attack
//     1ms, release 100ms) per this task's explicit instruction to start
//     there and measure. It is NOT sufficient: at the shortest reachable
//     delay time (dtim=0 -> ~48-sample round trip at 48kHz), the raw wet
//     tap rises from 0 to its ~1.96 per-sample bound (`inputAmplitude +
//     fbk`) within about 100 samples (~2ms) -- far faster than a 1ms
//     attack's own time constant can track -- and the 1ms-attack limiter's
//     own transient overshoot measured 1.673412, comfortably past the 1.0
//     ceiling. This is the delay's own version of B5's peak-branch finding:
//     "sustained material" is true of the STEADY STATE but the ONSET at
//     minimum delay time is a fast transient, not a slow swell, so a slow
//     attack under-reacts to it exactly like a slow attack under-reacted
//     to B5's per-sample-random height steps.
//   - kDelayWetLimiterThreshold (0.9): kept AT the master's own value (not
//     lowered) -- measurement (below) shows the attack alone, not the
//     threshold, is what needs to move; this stage still "catches first"
//     because it sits upstream of Reverb and the master in the signal
//     path (FroggersAppCore.hpp's `delay_.Process` runs before
//     `reverb_.Process` and before `SanitizeOutputSample`), so by the time
//     the master ever sees this signal it has already been reduced here.
//   - kDelayWetLimiterAttackSeconds (2 microseconds): sweeping attack alone
//     (threshold 0.9, release 100ms fixed) across dtim in {0, 0.02, 0.05,
//     0.1, 0.3, 1.0} found the worst-case (always at dtim=0, the fast-
//     round-trip extreme) crosses under the 1.0 ceiling between 2.5us
//     (1.000113) and 2.0us (0.999999); 2 microseconds sits just past that
//     crossing with a working margin, and going faster still (1.5us:
//     0.999998, 1.0us: 0.999998) buys nothing further -- the curve has
//     already flattened at the DesiredMagnitude asymptote for this bound,
//     so 2us is the measured knee, not an arbitrary small number.
//   - kDelayWetLimiterReleaseSeconds (100ms, matches the master): the
//     worst-case transient overshoot above is entirely an ATTACK-side
//     effect (a fast-rising raw signal outrunning the envelope's fall);
//     release governs recovery afterward, not this peak, and 100ms is the
//     same "gain reduction that does not pump" value this codebase has
//     already accepted for that job (dsp::OutputLimiter::kDefaultReleaseSeconds,
//     dsp::kPeakLimiterReleaseSeconds) -- reused rather than a fresh number
//     invented where the measurement gave no reason to move it.
inline constexpr float kDelayWetLimiterThreshold = 0.9f;
inline constexpr float kDelayWetLimiterCeiling = kSharedCeiling;
inline constexpr float kDelayWetLimiterAttackSeconds = 2.0e-6f;   // 2 microseconds -- see comment above.
inline constexpr float kDelayWetLimiterReleaseSeconds = kSharedReleaseSeconds;  // shared; see Limiter.hpp.

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

    // B6a: per-channel wet-output limiters (see this file's header comment,
    // above the struct, for the tuning derivation). Per-channel, not one
    // instance driven by max(|dL|,|dR|): the codebase's own established
    // idiom for a two-instance choice is per-unit/per-channel independence
    // (mirrors `outputLimiter_`/`filterChain_.peakLimiter` being separate,
    // independently-configured instances rather than a single shared one,
    // dsp/Limiter.hpp's own header comment on why one shared-tuning instance
    // was wrong for two different jobs). Concretely for THIS stage: `dwid`'s
    // cross-feed (`fbL = dL*(1-cross) + dR*cross`) already keeps L and R
    // close to each other in practice, so the two channels rarely diverge
    // enough for one limiter engaging alone to read as an image shift; a
    // linked (max-driven) limiter would instead force BOTH channels to duck
    // whenever EITHER one alone crossed threshold, which is exactly the
    // whole-mix-ducking failure mode this task exists to eliminate, just
    // narrowed from "the whole mix" to "the whole delay stage" -- a smaller
    // version of the same mistake. Per-channel keeps each channel's own
    // reduction tied to its own excess only, matching how every other
    // per-stage limiter in this codebase (`peakLimiter`, `outputLimiter_`)
    // is scoped to exactly the signal it bounds and nothing wider.
    OutputLimiter wetLimiterL;
    OutputLimiter wetLimiterR;

    // :33-40 (clearBuffers).
    void ClearBuffers()
    {
        std::fill(lineL.begin(), lineL.end(), 0.0f);
        std::fill(lineR.begin(), lineR.end(), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
        lastWet = {};
        // B6a: the wet limiters carry their own per-sample `envelope` state
        // (dsp::OutputLimiter::Process), so a buffer clear -- whether the
        // Stop-transport reset or Tier 1 fault recovery via Reset() below --
        // must reset them too, the same way it resets `lastWet` above. Not
        // load-bearing for post-Stop silence (a reduced-gain envelope only
        // ever multiplies toward zero, never away from it, and the lines
        // are already zeroed), but keeps this unit in a single deterministic
        // state after a clear, matching `lfoPhase`'s own reset rationale.
        wetLimiterL.Reset();
        wetLimiterR.Reset();
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
        // B6a: the wet limiters' own `envelope` state participates in this
        // unit's aggregate finiteness the same way every other member here
        // does -- `RecoverIfNonFinite(delay_)` (FroggersAppCore.hpp) calls
        // this StateFinite()/the Reset() above uniformly across the whole
        // unit, so a poisoned limiter envelope must be visible here rather
        // than needing its own separate top-level recovery call.
        if (!wetLimiterL.StateFinite() || !wetLimiterR.StateFinite())
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
        // B6a: the wet limiters' attack/release coefficients are sample-
        // rate-dependent (dsp::OutputLimiter::Configure), so they are
        // (re)configured here, the one place a real sample rate is known --
        // every caller of this struct (production's `delay_.SetSampleRate`
        // in FroggersAppCore::PrepareToPlay(), and every DSP-parity test
        // that constructs a StereoDelay) already calls SetSampleRate()
        // before ever calling Process(), so there is no path that reaches
        // the limiter unconfigured.
        wetLimiterL.Configure(sampleRate, kDelayWetLimiterThreshold, kDelayWetLimiterCeiling,
                               kDelayWetLimiterAttackSeconds, kDelayWetLimiterReleaseSeconds);
        wetLimiterR.Configure(sampleRate, kDelayWetLimiterThreshold, kDelayWetLimiterCeiling,
                               kDelayWetLimiterAttackSeconds, kDelayWetLimiterReleaseSeconds);
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

        // B2 (tasks.md CONSOLIDATED PUSH table; W2.1-MATH-2's "delay is the
        // only unsaturated feedback stage"): `fbL`/`fbR` are unbounded reads
        // straight off the delay line (ReadAt), so the pre-fix
        // `inSignal + fbL * fbk` fed a linear, unsaturated loop -- steady
        // state `in*send/(1-fbk)`, 50x input at fbk's 0.98 clamp. Wraps the
        // fed-back term in the SAME PadeSaturator::Saturate the comb's own
        // loop already uses (`Comb::Process` above, FilterFx.hpp -- reused,
        // not reimplemented, per §8), applied to the tapped signal before
        // the feedback-gain multiply, exactly mirroring `feedback *
        // Saturate(filter.Process(tapped))`. `Saturate` clamps to +-1
        // unconditionally, so this line can never write more than
        // `|inSignal| + fbk` regardless of how many round trips have
        // already run -- a per-sample bound, not just a steady-state one.
        WriteSample(inSignal + fbk * PadeSaturator::Saturate(fbL), lineL);
        WriteSample(inSignal + fbk * PadeSaturator::Saturate(fbR), lineR);
        AdvanceWrite();

        // B6a (tasks.md CONSOLIDATED PUSH table; "Group B outcomes" /
        // operator: "why can't we just have limiters for reverb and
        // delay?"): the loop write above already committed using the
        // UNLIMITED `dL`/`dR` taps -- B2's in-loop saturator alone still
        // governs the loop's own dynamics/persistence, exactly as before
        // this task. `wetLimiterL`/`wetLimiterR` are applied strictly AFTER
        // that write, to what actually ESCAPES this stage toward
        // `ToReverbMono`/Reverb/the master limiter, so a hot delay tail no
        // longer forces the downstream chain (Reverb, then the master) to
        // duck around it. See this file's header comment (above the struct)
        // for the tuning and its measurement.
        lastWet.l = wetLimiterL.Process(dL);
        lastWet.r = wetLimiterR.Process(dR);
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

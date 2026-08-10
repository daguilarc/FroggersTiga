#pragma once

// synth_froggers::dsp::{PolynomialDrive, SampleRateReducer,
// DigitalReorganizer, Oversampler2x, FrogBlock, DriveBlendPhase} -- packet
// 3 task 3.9 (extended DSP port, design D15).
// openspec/changes/froggers-sheaf-app/tasks.md section 3, item 3.9. A
// **copy** (design D3) of the cited Froggers formulas -- read directly from
// the frozen source before porting, not from memory.
//
// Ported (7 of the Drive page's 9 params -- Drive, Shape [a wavefolder,
// different from the Audio bank's VCO-morph Shape of task 3.1], SRR 1,
// SRR 2, XOR, Bit depth, Fuzz) from:
//   - src/core/PolynomialDrive.hpp
//       PolynomialDrive::SetGain/:34   Drive knob -> ExpMap(1,5,knob)
//       PolynomialDrive::SetCoefs/:38-66  Shape knob -> 5-coefficient
//         space-filling-curve wavefolder (uses the CURRENT gain TARGET,
//         not the smoothed value, at :40 `m_gain.m_target`)
//       PolynomialDrive::Process/:23-30   the polynomial evaluation itself
//       SampleRateReducer (whole file)    SRR 1 / SRR 2
//       DigitalReorganizer/:125-163       XOR (SetFlip) / Bit depth
//         (SetHash) and its bit-scramble Process
//       Oversampler2x/:69-123             2x oversample + anti-alias wrap
//         around the polynomial-drive + fuzz stage
//       FrogBlock/:165-203                the whole chain's order
//   - src/core/FroggersEngine.hpp:81-92    member declarations (m_srr1,
//     m_srr2, m_fuzz, m_digr, m_hash, m_frogBlock)
//   - src/core/FroggersEngine.hpp:207      RuntimeParam smoothing-rate
//     application (confirms these are ordinary smoothed knobs; smoothing
//     itself is NOT ported -- see note below)
//   - src/core/FroggersEngine.hpp:483-490  param wiring:
//       :483 SRR 1 = 1e-2 + ZeroedExp(10, 1 - GetParam(2))
//       :484 SRR 2 = 1e-2 + ZeroedExp(10, 1 - GetParam(3))
//       :485 XOR (m_digr) = GetParam(4), direct passthrough
//       :486 Bit depth (m_hash) = GetParam(5), direct passthrough
//       :487 Fuzz = GetParam(6), direct passthrough
//       :489 Drive = PolynomialDrive::SetGain(GetParam(0))
//       :490 Shape = PolynomialDrive::SetCoefs(GetParam(1))
//   - src/core/FroggersEngine.hpp:569-573  block-rate setter calls
//     (SetFreq/SetFlip/SetHash/fuzz assignment) confirming these five feed
//     FrogBlock's members directly by name.
//   - src/core/FroggersEngine.hpp:640-650  the Drive page's InitParam order
//     (GAIN, SHAPE, SRR1, SRR2, DIGR, HASH, FUZZ), confirming param indices
//     0-6 map to Drive/Shape/SRR1/SRR2/XOR/BitDepth/Fuzz in that order.
//   - src/core/FroggersEngine.hpp:872      `m_frogBlock.Process(chainIn)`,
//     confirming FrogBlock is the whole unit's entry point.
//
// TanhSaturator<false> reduces to PadeSaturator: FrogBlock's fuzz path
// (PolynomialDrive.hpp:195) reads `m_tanhSaturator.Process(out)`, where
// m_tanhSaturator's input gain is set exactly once, in FrogBlock's own
// constructor (`m_tanhSaturator.SetInputGain(1.0f)`, PolynomialDrive.hpp:184)
// and never touched again anywhere in FroggersEngine.hpp (confirmed by
// grep) -- so `TanhSaturator<false>::Process(x)` always evaluates
// `Saturate(1.0f * x)` with Normalize=false, i.e. exactly
// `dsp::PadeSaturator::Saturate(x)` (already ported in FilterFx.hpp, same
// Pade formula and clamp as TanhSaturator.hpp:25-30). Reused rather than
// re-defined.
//
// Smoothing NOT ported: the same convention as Reverb.hpp/Vco.hpp --
// FroggersEngine.hpp reads Drive/Shape/SRR1/SRR2/XOR/BitDepth/Fuzz through
// RuntimeParam (one-pole knob smoothing); that smoothing is parameter-model
// infrastructure owned by Sheaf's parameter model, not DSP. Callers here
// pass already-resolved 0..1 knob values.
//
// NOT ported (deliberately, design D15): Blend and Phase (Drive page slots
// 7 and 8). `m_driveParams->GetParam(7)`/`GetParam(8)` are never read
// anywhere in FroggersEngine.hpp -- confirmed by grep -- so there is no
// frozen formula to pin. They are newly authored below (DriveBlendPhase),
// clearly marked, with behavioral (not parity) tests.

#include "DspMath.hpp"
#include "FilterFx.hpp"  // reuse dsp::PadeSaturator (see note above)
#include "Limiter.hpp"   // reuse dsp::OutputLimiter (B7.2, see DriveBlendPhase below)
#include "RecoveryTier.hpp"  // F3.3: dsp::FiniteOnly/dsp::Magnitude, this file's own ForEachStatefulUnit calls below
                              // (already reachable transitively via FilterFx.hpp; included directly since this
                              // file names both types itself, not just through FilterFx.hpp's own use of them).

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace synth_froggers::dsp {

// PolynomialDrive.hpp:12-67.
struct PolynomialDrive
{
    float gain = 1.0f;
    float coefs[5] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    // PolynomialDrive.hpp:32-36 (SetGain).
    void SetGain(float gainKnob01) { gain = ExpMapCompute(1.0f, 5.0f, gainKnob01); }

    // PolynomialDrive.hpp:38-66 (SetCoefs). Uses the CURRENT `gain` (the
    // frozen code's m_gain.m_target, i.e. the un-smoothed target) -- call
    // SetGain before SetCoefs, same order as FroggersEngine.hpp:489-490.
    void SetCoefs(float shapeKnob01)
    {
        const float computedGain = gain;
        const float coefsKnob = ZeroedExpCompute(30.0f, shapeKnob01);

        coefs[0] = 1.0f + 10.0f * Sine01(coefsKnob * 1.0f);
        coefs[1] = 10.0f * Sine01(coefsKnob * 1.618f + 0.25f * (computedGain - 1.0f));
        coefs[2] = 10.0f * Sine01(coefsKnob * 2.718f);
        coefs[3] = 10.0f * Sine01(coefsKnob * 3.141f + 0.25f * (computedGain - 1.0f));
        coefs[4] = 10.0f * Sine01(coefsKnob * 4.669f);
    }

    // PolynomialDrive.hpp:23-30 (Process).
    float Process(float input) const
    {
        const float input2 = input * input;
        const float input3 = input2 * input;
        const float input4 = input3 * input;
        const float input5 = input3 * input2;
        return gain * (input * coefs[0] + input2 * coefs[1] + input3 * coefs[2]
                        + input4 * coefs[3] + input5 * coefs[4]);
    }
};

// PolynomialDrive.hpp:69-123 (Oversampler2x), using dsp::OnePoleLowPass
// (DspMath.hpp) as the anti-alias filter -- identical to OPLowPassFilter.
struct Oversampler2x
{
    float prevInput = 0.0f;
    bool firstSample = true;
    OnePoleLowPass antiAlias;

    // F3.3 SPEC CORRECTED 2026-08-07: Tier 2's per-unit sustained-over-
    // ceiling counter, owned here rather than in FroggersAppCore -- see
    // dsp::Vco::overCeilingSeconds's own comment (Vco.hpp) for the full
    // rationale.
    float overCeilingSeconds = 0.0f;

    Oversampler2x() { antiAlias.SetAlphaFromNatFreq(0.4f); }

    template <typename ProcessFunc>
    float Process(float input, ProcessFunc processFunc)
    {
        float output;
        if (firstSample)
        {
            const float output1 = processFunc(input);
            const float output2 = processFunc(input);
            antiAlias.Process(output1);
            output = antiAlias.Process(output2);
            firstSample = false;
        }
        else
        {
            const float interpolated = (prevInput + input) * 0.5f;
            const float output1 = processFunc(interpolated);
            const float output2 = processFunc(input);
            antiAlias.Process(output1);
            output = antiAlias.Process(output2);
        }
        prevInput = input;
        return output;
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only the
    // recursive state -- prevInput (the interpolation history) and
    // antiAlias.output (the anti-alias filter's one-pole state) -- and
    // rearms firstSample so the very next Process() call re-enters the
    // "first sample" branch rather than interpolating against a just-zeroed
    // prevInput as if it were real history. NOT touched: antiAlias.alpha,
    // set once in the constructor and never reconfigured per-block, so
    // clearing it would be a tuning change, not a state clear.
    void Reset()
    {
        prevInput = 0.0f;
        firstSample = true;
        antiAlias.output = 0.0f;
        overCeilingSeconds = 0.0f;
    }

    bool StateFinite() const { return std::isfinite(prevInput) && std::isfinite(antiAlias.output); }
    float StateMagnitude() const { return std::max(std::fabs(prevInput), std::fabs(antiAlias.output)); }
};

// SampleRateReducer.hpp (whole file), verbatim.
struct SampleRateReducer
{
    float freq = 0.0f;
    float phase = 0.0f;
    float output = 0.0f;

    // F3.3 SPEC CORRECTED 2026-08-07: Tier 2's per-unit sustained-over-
    // ceiling counter, owned here rather than in FroggersAppCore -- see
    // dsp::Vco::overCeilingSeconds's own comment (Vco.hpp) for the full
    // rationale. This struct has two independent instances (sampleRateReducer1/2
    // on dsp::FrogBlock), each with its own counter, same as any other member.
    float overCeilingSeconds = 0.0f;

    void SetFreq(float f) { freq = f; }

    float Process(float input)
    {
        if (freq >= 1.0f)
        {
            return input;
        }
        if (freq <= 0.0f)
        {
            return output;
        }
        phase += freq;
        if (phase >= 1.0f)
        {
            phase = phase - std::floor(phase);
            output = input;
        }
        return output;
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only
    // phase/output, the recursive sample-and-hold state -- NOT freq, which
    // is config reassigned every block by the caller's SetFreq(), not
    // signal state.
    void Reset()
    {
        phase = 0.0f;
        output = 0.0f;
        overCeilingSeconds = 0.0f;
    }

    bool StateFinite() const { return std::isfinite(phase) && std::isfinite(output); }
    float StateMagnitude() const { return std::max(std::fabs(phase), std::fabs(output)); }
};

// PolynomialDrive.hpp:125-163 (DigitalReorganizer). NOTE: :138's
// `std::round(inputUp)` assigned to a uint8_t is float-to-integer
// narrowing that is well-defined only while `inputUp` (== (input+1)*128)
// stays within [0,255] -- i.e. input in roughly [-1, 0.9921875]. At
// input==1.0 exactly, inputUp==256 and the cast is undefined behavior in
// the FROZEN source too (confirmed by reading PolynomialDrive.hpp:135-151
// directly).
//
// FIX, NOT A REPRODUCTION (task "Fix 1a", strict-executor brief): unlike
// the fuegoize UB (packet 3, sim/Fuegoize.hpp), which is carried forward
// because the frozen tree also contains a *correct* reference (the
// firmware's Parameter.hpp:143) to port instead, there is no such correct
// reference here -- both PolynomialDrive.hpp:138 in the frozen `src/core/`
// tree and this port hit the same undefined cast at input==1.0. This is
// newly written code this app owns, and reproducing UB has no parity
// value, so this port clamps the rounded value to the uint8_t-representable
// range [0,255] *before* the narrowing cast, then computes `inputRemainder`
// against that same clamped value (not the raw `inputUp`). Two
// consequences: (1) everywhere the original expression was well-defined
// (`inputUp` in [0,255], i.e. input in [-1, 0.9921875]) the clamp is a
// no-op, so `round(inputUp)` is already in range and behavior is bit-for-
// bit unchanged; (2) at the boundary and beyond (input >= ~0.9921875, or
// input < -1 -- reachable in practice, since PolynomialDrive's output
// upstream of this stage is not amplitude-bounded to [-1,1]) the clamp
// saturates the integer part while the remainder still carries the
// leftover delta, so with flip==0 and hashBits==0 (the pass-through
// configuration) `Process(1.0f)` still reconstructs to exactly `1.0f`
// rather than invoking UB -- see the regression test at input==1.0 in
// FroggersDspParityTests.cpp.
//
// DELIBERATE PARITY DIVERGENCE #2 (openspec/changes/archive/2026-08-09-
// frogg3rs-parametric-slew-and-stop-root-cause/tasks.md S1.3, proposal.md
// §2-4 -- same class of operator-approved divergence as
// dsp::Comb::GetFeedback's +-1.1 -> +-0.95 above (FilterFx.hpp) and the
// resonant-peak ceiling's 10x -> 4x -> 2x (FilterFx.hpp's
// kMaxResonantBumpHeight), each carrying its own in-code note):
//
// `Process(0.0f)` -- digital silence -- is NOT silent. At input==0,
// `inputUp` is exactly 128 and `inputRemainder` is exactly 0, but the XOR by
// `flip` and the mask-gated bit scramble below still run, so the result is
// nonzero for any `flip != 0` (exactly -1.0 at flip==128) and also nonzero
// at flip==0 when hashBits==8. This stage sits upstream of three recursive
// loops (dsp::Comb, dsp::StereoDelay, dsp::Reverb -- the last with no
// in-loop saturator), which amplify that seed without bound: this is F3,
// "Stop doesn't stop" (measured and traced end-to-end, proposal.md §2).
//
// PLAINLY: the frozen firmware (src/core/PolynomialDrive.hpp:125-163) has
// this exact same f(0) != 0 behaviour -- it is a property of the original
// bit-scramble math, not a porting error -- and no DC blocker or highpass
// exists anywhere in this signal chain (src/core/, sim/, and app/ all
// checked). On real hardware, an analog output stage AC-couples a DC offset
// away for free, so this was inaudible on the original instrument; this
// port has no such output stage, so without a fix nothing downstream ever
// removes it.
//
// FIX: the scramble is factored into `Mangle()` below (identical math, no
// behaviour change by itself), and `Process` returns
// `Mangle(input, flip, hashBits) - Mangle(0.0f, flip, hashBits)` -- a DC
// block anchored to this stage's own silent-input response, computed fresh
// on every call rather than cached (`flip`/`hashBits` are public fields
// that can be assigned directly, bypassing SetFlip()/SetHash(), so a
// correction cached only inside those setters would go stale the moment
// they were bypassed). At the pass-through configuration (flip==0,
// hashBits==0), `Mangle(0.0f, 0, 0) == 0.0f` exactly, so the correction
// term is exactly zero there and this fix is a no-op: `Process(1.0f) ==
// 1.0f` (Fix 1a's own regression test, above) and every parity case at
// default flip/hash are unaffected. `FroggersDspParityTests.cpp`'s
// digital_reorganizer_process_matches_bit_scramble_formula pins `Process()`
// against a hand-transcribed replica of this formula at nonzero flip/hash;
// it is re-asserted against this corrected behaviour in the same change
// (see that test's own comment), never deleted.
struct DigitalReorganizer
{
    uint8_t flip = 0;
    uint8_t hashBits = 0;

    // The bit-mangle itself -- unchanged from the original formula (Fix 1a's
    // clamp above still applies); factored out of Process() purely so the
    // divergence note above can call it twice: once at the real input, once
    // at silence. OMNI §6: reused (2 call sites) and isolates a distinct
    // transformation stage. OMNI §8: one definition, so neither call site in
    // Process() below copies this expression.
    static float Mangle(float input, uint8_t flip, uint8_t hashBits)
    {
        const float inputUp = (input + 1.0f) * 128.0f;
        const float roundedUp = std::round(inputUp);
        const float clampedUp = std::min(std::max(roundedUp, 0.0f), 255.0f);
        uint8_t inputInt = static_cast<uint8_t>(clampedUp);
        const float inputRemainder = inputUp - clampedUp;

        inputInt = static_cast<uint8_t>(inputInt ^ flip);
        const uint8_t mask = static_cast<uint8_t>((1 << hashBits) - 1);
        uint8_t lowerBits = static_cast<uint8_t>(inputInt & mask);

        lowerBits = static_cast<uint8_t>(lowerBits ^ ((lowerBits << 3) & mask));
        lowerBits = static_cast<uint8_t>(lowerBits ^ ((lowerBits >> 5) & mask));
        lowerBits = static_cast<uint8_t>(lowerBits ^ ((lowerBits << 1) & mask));

        inputInt = static_cast<uint8_t>((inputInt & ~mask) | lowerBits);

        return (static_cast<float>(inputInt) + inputRemainder) / 128.0f - 1.0f;
    }

    // DC-blocked at this stage's own silent-input response -- see the
    // divergence note above this struct. Computed fresh every sample, never
    // cached (flip/hashBits are public and may be reassigned directly,
    // bypassing SetFlip()/SetHash()).
    float Process(float input) const
    {
        return Mangle(input, flip, hashBits) - Mangle(0.0f, flip, hashBits);
    }

    void SetFlip(float flipKnob01) { flip = static_cast<uint8_t>(flipKnob01 * 255.0f); }  // :154-157, truncates
    void SetHash(float hashKnob01) { hashBits = static_cast<uint8_t>(std::round(hashKnob01 * 8.0f)); }  // :159-162, rounds
};

// PolynomialDrive.hpp:9-30 / TanhSaturator.hpp:25-30 already ported as
// dsp::PadeSaturator (FilterFx.hpp) -- reused directly, see file-header note.

// PolynomialDrive.hpp:165-203 (FrogBlock).
struct FrogBlock
{
    PolynomialDrive polynomialDrive;
    SampleRateReducer sampleRateReducer1;
    SampleRateReducer sampleRateReducer2;
    DigitalReorganizer digitalReorganizer;
    Oversampler2x oversampler;
    float fuzz = 0.0f;

    // F3.3 (openspec/changes/archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md):
    // this struct's own contribution to the "every stateful unit in the
    // audio path" enumeration -- lists ONLY the members declared above that
    // RecoverPoisonedUnitState ever watched (not polynomialDrive/
    // digitalReorganizer/fuzz -- see app/FroggersAppCore.hpp's own
    // RecoverPoisonedUnitState comment for which units that was and why).
    // Composed, not re-listed, by FroggersAppCore::ForEachStatefulUnit below.
    template <typename Visitor>
    void ForEachStatefulUnit(Visitor&& visit)
    {
        visit(sampleRateReducer1, Magnitude{});
        visit(sampleRateReducer2, Magnitude{});
        visit(oversampler, Magnitude{});
    }

    // PolynomialDrive.hpp:187-202 (FrogBlock::Process), verbatim order.
    float Process(float input)
    {
        float output = oversampler.Process(input, [this](float in) -> float {
            const float out = polynomialDrive.Process(in);
            const float sinIn = out / 4.0f;
            return Sine01(sinIn) * (1.0f - fuzz) + fuzz * PadeSaturator::Saturate(out);
        });

        output = digitalReorganizer.Process(output);
        output = sampleRateReducer1.Process(output);
        output = sampleRateReducer2.Process(output);
        return output;
    }
};

// -------------------------------------------------------------------------
// Authored, NOT ported (design D15): Blend and Phase, Drive page slots 7/8.
// No Froggers original exists (see file header) -- design rationale below.
//
// Blend crossfades the dry input against the driven (FrogBlock) signal --
// the common "parallel drive" pattern that keeps the raw input available
// underneath the processed tone. Phase applies a first-order allpass to
// the wet signal before the blend; the coefficient is mapped from the knob
// into (-0.98, 0.98) -- STRICTLY inside the unit circle, not [-1, 1] --
// pairing an allpass with a dry/wet blend is the standard way parallel-
// drive designs avoid comb-filtering / phase-cancellation artefacts when
// the two paths recombine. At the neutral default (blendKnob01 == 0),
// Process() returns `dry` exactly regardless of phaseKnob01 or wet, so this
// authored stage never disturbs the seven ported params -- callers can
// drive the ported FrogBlock alone and ignore this struct entirely.
//
// Item 3 (new, found while reading the code; revised 2026-07-28): an
// earlier revision of this struct mapped the knob to the CLOSED interval
// [-1, 1] and claimed that "keeps a first-order allpass unconditionally
// stable (energy-preserving) for any input." That claim is false at the
// endpoints. Process()'s recurrence is
// `y[n] = -a*x[n] + x[n-1] + a*y[n-1]` -- a first-order allpass whose pole
// sits at `z = a`. Stability requires `|a| < 1` STRICTLY; `|a| = 1` places
// the pole exactly ON the unit circle, where the homogeneous response
// (`y[n] = a*y[n-1]` once the input stops exciting it) neither grows nor
// decays -- it rings forever at constant amplitude. Both endpoints were
// reachable (phaseKnob01 clamps to [0,1] inclusive, matching every other
// knob in this app), and phaseKnob01 DEFAULTS to 0, i.e. a == -1 ships by
// default: every fresh instance of this app started with its pole sitting
// on the unit circle, state that never decays once excited. Fixed by
// scaling the coefficient strictly inside the unit circle (0.98, the same
// margin dsp::Reverb's own Decay/Hold ceiling uses, Reverb.hpp:136,207) --
// this leaves the audible sweep essentially unchanged (0.98 vs. 1.0 shifts
// the allpass's frequency-dependent group delay by a negligible amount at
// every phaseKnob01 value) while guaranteeing every pole strictly inside
// the unit circle, so the state provably decays instead of only "usually"
// decaying. This is NOT the kind of clamp task 2.6 forbids -- that task's
// ban is on guarding an UNREACHABLE zero divisor; here `|a| == 1` is both
// reachable and the shipped default, so the fix changes real, exercised
// behavior rather than adding dead code.
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// B7.2 (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md
// §K.1/§K.4): the allpass coefficient `a` below was read fresh from the
// Phase knob every sample with NO smoothing. A FIXED-coefficient allpass is
// unity-gain; a TIME-VARYING one is not -- measured (§K.1), with a bounded
// +-1 `wet` input: 1.002 under free random phase, 4.15 under full-bank
// per-sample-random modulation (`knob()` refreshes every sample,
// FroggersAppCore.hpp:1008-1009 / tasks.md's "Audio-rate modulation"
// section -- this is not an edge case, it is what audio-rate noise
// modulation of this knob does continuously), and 50.5 under periodic
// phase/content coincidence (an LFO landing near the note's own period),
// bounded and plateauing but the largest blowout path measured in the
// instrument -- 50x dwarfs the comb's ~1.95 and the peak's 1.669.
//
// TWO-PART FIX, both measured against a standalone harness reproducing
// this struct's exact math (not assumed by analogy -- W2.2a's own glide
// constant was picked by analogy and measured 80% wrong, tasks.md):
//
// 1. SMOOTH THE COEFFICIENT (root cause, tried first per the brief). A
//    one-pole (`coeffSmoother` below, `dsp::OnePoleLowPass` reused, not a
//    new smoother) on `a` makes the allpass quasi-static, restoring
//    near-unity gain. Swept 0.0005-0.5 cycles/sample against two
//    adversarial patterns: (A) phaseKnob01 AND wet both redrawn uniform
//    per-sample-random (the §K.1 "full-bank" case) and (B) a periodic
//    phase/content coincidence (wet a sine, phaseKnob01 a synced
//    square-ish LFO at a rational-multiple frequency) that reproduces
//    §K.1's periodic-coincidence mechanism -- found up to 61.2x unsmoothed
//    (exceeds the recorded 50.5x; verified bounded/plateauing to 20M
//    samples, matching §K.1's own plateau finding, not divergent).
//    Result: smoothing alone drives pattern B to ~1.0x at any glide <=
//    ~0.05 (the periodic mechanism needs `a` to track the content in sync;
//    a lagging coefficient breaks the lock). Pattern A -- persistent full-
//    range per-sample redraws, not a one-off transient -- has a genuine
//    residual floor smoothing cannot close: minimized (10 seeds x 1M
//    samples/glide) at ~0.0035 cycles/sample (worst 1.405x), WORSE both
//    slower (a slow-moving coefficient dwelling near the +-0.98 pole for
//    many samples re-creates pattern B's own reinforcement, 1e-5 cyc/samp
//    -> 1.86x) and faster (approaches the unsmoothed case, 0.45 cyc/samp
//    -> 3.96x) -- so 0.0035 is a genuine measured minimum, not a monotone
//    tradeoff. Same class of fix as W2.2a's trim smoothing; costs nothing
//    tonally, since an unsmoothed per-sample-random coefficient was never
//    a musical control (static-Phase output is unchanged, see
//    FroggersDspParityTests.cpp's static-neutrality pin).
// 2. LIMIT THE STAGE OUTPUT (only because step 1 proved insufficient --
//    pattern A's ~1.4-1.7x residual, confirmed against 20 seeds x 2M
//    samples at the chosen glide, does not close on its own; same
//    structural finding as the peak branch, B1/B5, where a per-sample
//    scalar trim alone plateaued at 1.669x and needed its own limiter).
//    `dsp::OutputLimiter` (Limiter.hpp), five-argument `Configure()`.
//    Threshold/attack/release swept together (thresholds 0.7-0.9,
//    attacks 1us-1ms) against both patterns above (10-20 seeds x
//    500k-2M samples): attack must be MICROSECONDS, not the ~1ms §K
//    originally guessed for "sustained" excess -- confirming §K.4's
//    correction (mechanism-shape predicted attack wrong twice already;
//    measurement is what actually decides it, every stage so far has
//    needed microseconds). 1000x slower (1ms) leaves pattern A at 1.39x,
//    barely better than smoothing alone; 2us reaches 0.990x. Threshold
//    0.7 (below the master's 0.9, same headroom logic as the peak
//    branch's kPeakLimiterThreshold) with `kSharedCeiling`/
//    `kSharedReleaseSeconds` (Limiter.hpp, reused rather than
//    re-declared per OMNI §8) rounds out the tuning. FINAL, measured:
//    pattern A worst 0.990x (20 seeds x 2M samples), pattern B worst
//    0.896x (20M samples) -- both at or below the ~1.0 bound FrogBlock's
//    own output already respects (W2.1-MATH), so this stage no longer
//    forces the master limiter to engage on its own account.
// -------------------------------------------------------------------------
struct DriveBlendPhase
{
    // Measured minimum of the pattern-A sweep above; NOT chosen by analogy
    // to FilterFxChain's kTrimGlideCyclesPerSample (0.45, a different
    // problem -- smoothing an output-level TRIM, not a recursive filter's
    // own pole -- and measured far too fast here, see class comment).
    static constexpr float kPhaseCoeffGlideCyclesPerSample = 0.0035f;

    // Measured tuning (class comment): below the master's 0.9 threshold,
    // same headroom logic as the peak branch's kPeakLimiterThreshold.
    static constexpr float kOutputLimiterThreshold = 0.7f;
    // F2.1a/F2.1b: this limiter is Configure()'d against kStageCeiling (see
    // the outputLimiter.Configure(...) call below, retargeted off
    // kSharedCeiling by F2.1b), so that is the ceiling the invariant is
    // checked against -- unchanged threshold (0.7, MEASURED, class comment
    // above), already strictly below kStageCeiling (0.80). See
    // dsp::OutputLimiter::kDefaultThreshold's own static_assert
    // (dsp/Limiter.hpp) for why a negative headroom is both catastrophic and
    // invisible to every finiteness guard in the suite.
    static_assert(kOutputLimiterThreshold < kStageCeiling,
                  "threshold must stay strictly below ceiling; a negative headroom turns "
                  "DesiredMagnitude into an exponential amplifier -- see dsp/Limiter.hpp");
    static constexpr float kOutputLimiterAttackSeconds = 2.0e-6f;  // 2 microseconds -- measured, see class comment.

    float allpassX1 = 0.0f;
    float allpassY1 = 0.0f;

    // F3.3 SPEC CORRECTED 2026-08-07: Tier 2's per-unit sustained-over-
    // ceiling counter, owned here rather than in FroggersAppCore -- see
    // dsp::Vco::overCeilingSeconds's own comment (Vco.hpp) for the full
    // rationale.
    float overCeilingSeconds = 0.0f;

    // Smooths the allpass coefficient `a`, not the Phase knob or `wet`
    // themselves -- the excess is specifically a time-varying POLE, so the
    // coefficient feeding the recurrence is what must be made quasi-static
    // (class comment, fix 1).
    OnePoleLowPass coeffSmoother;

    // Class comment, fix 2: catches the residual smoothing alone cannot
    // close. Attack/release are sample-rate-dependent, so -- same reason
    // FilterFxChain::peakLimiter/Reverb::wetLimiter/Delay's wetLimiterL/R
    // all need their own Configure(sampleRate) -- this type gets one too,
    // called from the constructor (assumed-48kHz default, matching
    // FilterFxChain's/Reverb's own constructor-time Configure()) AND from
    // FroggersAppCore::PrepareToPlay() once the real host rate is known
    // (driveBlendPhase_.Configure(sampleRate_), mirroring
    // filterChain_.Configure(sampleRate_)/reverb_.Configure(sampleRate_)
    // there).
    OutputLimiter outputLimiter;

    DriveBlendPhase()
    {
        // Same assumed-48kHz constructor-time default FilterFxChain's/
        // Reverb's own constructors use for their sample-rate-dependent
        // limiters, for the identical reason (a direct `DriveBlendPhase bp;`
        // instantiation -- e.g. a test -- that never calls Configure() still
        // gets a harmless, finite tuning rather than an unconfigured one).
        constexpr float kDefaultAssumedSampleRate = 48000.0f;
        Configure(kDefaultAssumedSampleRate);
    }

    void Configure(float sampleRate)
    {
        coeffSmoother.SetAlphaFromNatFreq(kPhaseCoeffGlideCyclesPerSample);
        // Seeded to `a` at phaseKnob01 == 0 (the knob's own default, class
        // header note above) so a fresh instance produces no startup
        // transient -- same idiom as FilterFxChain's combTrimSmoother/
        // peakTrimSmoother seeding their `.output` to the untrimmed value
        // at each trim's own default knob position. `coeffSmoother`'s glide
        // is sample-rate-INdependent (SetAlphaFromNatFreq takes cycles/
        // sample, same idiom FilterFxChain's own trim smoothers use, class
        // header note), so re-running it here on a later Configure() call
        // is a harmless no-op re-derivation, not a behaviour change.
        coeffSmoother.output = -0.98f;
        // F2.1b: retargeted from kSharedCeiling to kStageCeiling -- see the
        // static_assert above.
        outputLimiter.Configure(sampleRate, kOutputLimiterThreshold, kStageCeiling, kOutputLimiterAttackSeconds,
                                 kSharedReleaseSeconds);
    }

    float Process(float dry, float wet, float blendKnob01, float phaseKnob01)
    {
        // Item 3 fix: 0.98x keeps |a| < 1 strictly across the whole knob
        // range, including both endpoints (phaseKnob01 == 0 -> a == -0.98,
        // phaseKnob01 == 1 -> a == 0.98), so the allpass's pole never sits
        // on the unit circle.
        const float aTarget = 0.98f * (2.0f * phaseKnob01 - 1.0f);  // authored mapping -> (-0.98, 0.98)
        // B7.2 fix 1: smooth the coefficient itself, not the knob input --
        // see class header comment for the measurement that picked this glide.
        const float a = coeffSmoother.Process(aTarget);
        const float phased = -a * wet + allpassX1 + a * allpassY1;
        allpassX1 = wet;
        allpassY1 = phased;
        const float blended = dry * (1.0f - blendKnob01) + phased * blendKnob01;
        // B7.2 fix 2: catches the residual smoothing alone cannot close.
        return outputLimiter.Process(blended);
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only the
    // allpass's own recursive history plus the two new stateful units this
    // fix adds (coeffSmoother/outputLimiter), each reset to the same
    // quiescent value their own type's Reset()/constructor already defines
    // as "no history" -- the coefficient smoother back to the default-knob
    // seed above, the limiter's envelope back to unity (OutputLimiter::Reset()).
    void Reset()
    {
        allpassX1 = 0.0f;
        allpassY1 = 0.0f;
        coeffSmoother.output = -0.98f;
        outputLimiter.Reset();
        overCeilingSeconds = 0.0f;
    }

    bool StateFinite() const
    {
        return std::isfinite(allpassX1) && std::isfinite(allpassY1) && std::isfinite(coeffSmoother.output) &&
               outputLimiter.StateFinite();
    }
    float StateMagnitude() const
    {
        return std::max(std::max(std::fabs(allpassX1), std::fabs(allpassY1)), std::fabs(coeffSmoother.output));
    }
};

}  // namespace synth_froggers::dsp

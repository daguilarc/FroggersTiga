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
struct DigitalReorganizer
{
    uint8_t flip = 0;
    uint8_t hashBits = 0;

    float Process(float input) const
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
struct DriveBlendPhase
{
    float allpassX1 = 0.0f;
    float allpassY1 = 0.0f;

    float Process(float dry, float wet, float blendKnob01, float phaseKnob01)
    {
        // Item 3 fix: 0.98x keeps |a| < 1 strictly across the whole knob
        // range, including both endpoints (phaseKnob01 == 0 -> a == -0.98,
        // phaseKnob01 == 1 -> a == 0.98), so the allpass's pole never sits
        // on the unit circle.
        const float a = 0.98f * (2.0f * phaseKnob01 - 1.0f);  // authored mapping -> (-0.98, 0.98)
        const float phased = -a * wet + allpassX1 + a * allpassY1;
        allpassX1 = wet;
        allpassY1 = phased;
        return dry * (1.0f - blendKnob01) + phased * blendKnob01;
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only the
    // allpass's own recursive history.
    void Reset()
    {
        allpassX1 = 0.0f;
        allpassY1 = 0.0f;
    }

    bool StateFinite() const { return std::isfinite(allpassX1) && std::isfinite(allpassY1); }
    float StateMagnitude() const { return std::max(std::fabs(allpassX1), std::fabs(allpassY1)); }
};

}  // namespace synth_froggers::dsp

#pragma once

// synth_froggers::dsp::{PadeSaturator, ResonantBump, Comb, PureDelay,
// FilterFxChain} -- packet 3 task 3.6 (DSP port).
// openspec/changes/froggers-sheaf-app/tasks.md section 3, item 3.6. A
// **copy** (design D3) of the cited Froggers formulas.
//
// Ported from:
//   - src/core/ResonantBump.hpp:44-71   RBJ peaking-biquad coefficients
//   - src/core/Comb.hpp:54              out = in + fb*sat(lp(delay[i-N]))
//   - src/core/Comb.hpp:63              N = 1/freq (GetDelaySamples)
//   - src/core/Comb.hpp:66-76           asymmetric +-1.1 feedback (GetFeedback --
//     THIS PORT DELIBERATELY DIVERGES to +-0.95; see GetFeedback's own comment
//     below, design.md A2/A2a, item 1)
//   - src/core/Comb.hpp:79-109          PureDelay (fractional interp :98-108;
//     the frozen file has no trailing newline after its closing `};`, so
//     `wc -l` reports 108 while the struct's real content runs through
//     line 109 -- confirmed by reading the file's raw bytes, not a citation
//     error)
//   - src/core/TanhSaturator.hpp:25-30  Pade rational approximation
//     x(27+x^2)/(27+9x^2), NOT std::tanh (formula :28, clamp :29)
//   - src/core/FroggersEngine.hpp:822-848 (ApplyOutputFx) -- specifically
//     the parallel branch :824-833 and serial branch :834-839. The reverb
//     stage and m_simFxInsert hook that follow (:840-847) are NOT part of
//     this task and are not ported.
//
// ============================================================================
// Packet 9 (tasks.md section "9. Bump/Comb transfer-function visualizers",
// task 9.1; design D10) -- ResonantBump and Comb each gain a
// `struct UIState : synth::TransferFunction` (the interface TYPE; the
// header is named DspTransferFunction.hpp,
// include/synth/DspTransferFunction.hpp:7,9,10), mirroring the exact
// in-struct placement Sheaf's own filters use (`OnePoleLowPass::UIState`,
// include/synth/DspFilters.hpp:22; `OnePoleHighPass::UIState`, :85;
// `ClassicStateVariableFilter::UIState`, :143) rather than a separate
// wrapper. As with app/dsp/Vco.hpp's task-7 scope UIState, this is a
// deliberate, minimal widening of this file's "no Sheaf dependency"
// convention: `synth/DspTransferFunction.hpp` is a two-method abstract
// interface with no further transitive Sheaf includes beyond `<complex>`,
// header-only, no link dependency. Flagged in the packet report;
// `app/Makefile`'s DSP_TEST_BIN rule gains Sheaf's -I include path
// accordingly.
//
// The comb's saturator is the Pade rational approximation (PadeSaturator
// above), not tanh -- its small-signal gain (the derivative at x=0) is
// exactly 1.0 (d/dx [x(27+x^2)/(27+9x^2)] at x=0 = 27/27 = 1), so the
// linearised comb response below treats the saturator as an identity and
// reduces Comb::Process's feedback loop to an ordinary linear
// delay-plus-lowpass-in-feedback comb filter (design D10's explicit
// instruction: "the small-signal gain is what the linearised response
// should assume").

#include "DspMath.hpp"

#include "synth/DspTransferFunction.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstddef>

namespace synth_froggers::dsp {

namespace transfer_function_detail {

// Task 9.4's "self-oscillating comb feedback produces only finite plot
// values, and the displayed magnitude is bounded rather than drawn out of
// frame": a self-oscillating comb (feedback up to +-0.95, Comb::GetFeedback
// above -- item 1, design.md A2, was +-1.1) can place this linearised
// system's loop gain arbitrarily close to
// (or, in principle, exactly on) a true unit-circle pole, which would make
// a bare `1/denominator` divide-by-(near)zero and stop being finite. This
// floors the denominator's MAGNITUDE (preserving its phase) before any
// division below, so every closed-form response this file computes is
// finite and bounded by construction -- not by coincidence of where sample
// points happen to land.
inline std::complex<float> SafeDenominator(std::complex<float> denominator)
{
    constexpr float kMinMagnitude = 1.0e-3f;  // bounds the worst-case response to 1/kMinMagnitude = 1000x.
    const float magnitude = std::abs(denominator);
    if (!std::isfinite(magnitude) || magnitude < kMinMagnitude)
    {
        const float phase = (std::isfinite(magnitude) && magnitude > 0.0f) ? std::arg(denominator) : 0.0f;
        return std::polar(kMinMagnitude, phase);
    }
    return denominator;
}

}  // namespace transfer_function_detail

// src/core/TanhSaturator.hpp:25-30. Despite the frozen struct's name, this
// is a Pade rational approximation, not std::tanh -- pinned exactly,
// including the clamp, per tasks.md 3.6.
struct PadeSaturator
{
    static float Saturate(float input)
    {
        const float input2 = input * input;
        const float output = input * (27.0f + input2) / (27.0f + 9.0f * input2);
        return std::max(-1.0f, std::min(1.0f, output));
    }
};

// src/core/ResonantBump.hpp:7-78. RBJ peaking EQ; coefficients at :44-71.
// The app's maximum peak-resonance gain, as a SHARED constant rather than a
// literal at each use (2026-07-29). It lives here, beside the unit it bounds,
// because two places need to agree on it: FroggersAppCore's
// `peak.SetHeight(ExpMapCompute(1.0f, kMaxResonantBumpHeight, knob))` and the
// parity test that pins the resulting gain.
//
// It is a shared constant because the test that was supposed to pin this
// ceiling did not: it computed `ExpMapCompute(1.0f, 4.0f, 1.0f)` with its own
// hardcoded 4.0f and asserted the answer was 4.0f -- self-referential, and it
// passed unchanged when the app's real ceiling moved 4.0 -> 2.0. Anything
// asserting this value must read it from here, never retype it.
//
// Value history: 10.0 (frozen-firmware parity) -> 4.0 (gross overload) ->
// 2.0 (operator, on hearing it modulated: "still too harsh ... very close to
// blowout territory"). See FroggersAppCore's own comment at the SetHeight
// call for why modulation is the case that governs this number.
inline constexpr float kMaxResonantBumpHeight = 2.0f;

struct ResonantBump
{
    // Task 9.1 (design D10): b0/b1/b2/a1/a2 (a0 already normalized to 1 by
    // UpdateCoefficients() below) are exactly what BiquadDf1::Process's
    // difference equation needs to reproduce the closed-form response --
    // capturing them directly (rather than freq/height/width) means
    // FrequencyResponse() reflects precisely the coefficients actually
    // driving Process(), with no risk of re-deriving them differently.
    struct UIState : synth::TransferFunction
    {
        std::atomic<float> b0{1.0f};
        std::atomic<float> b1{0.0f};
        std::atomic<float> b2{0.0f};
        std::atomic<float> a1{0.0f};
        std::atomic<float> a2{0.0f};

        float FrequencyResponse(float normalizedFrequency) const override
        {
            return std::abs(TransferFunctionValue(normalizedFrequency));
        }

        std::complex<float> TransferFunctionValue(float normalizedFrequency) const override
        {
            return ResonantBump::ComputeTransferFunctionValue(
                b0.load(), b1.load(), b2.load(), a1.load(), a2.load(), normalizedFrequency);
        }
    };

    BiquadDf1 biquad;

    float freq = 1000.0f;
    float height = 1.0f;
    float width = 1.0f;

    ResonantBump() { UpdateCoefficients(); }

    void SetFreq(float f)
    {
        freq = f;
        UpdateCoefficients();
    }

    void SetHeight(float h)
    {
        height = h;
        UpdateCoefficients();
    }

    void SetWidth(float w)
    {
        width = w;
        UpdateCoefficients();
    }

    // src/core/ResonantBump.hpp:44-71, verbatim formula.
    void UpdateCoefficients()
    {
        const float omega = 2.0f * static_cast<float>(M_PI) * freq;
        const float cosw = std::cos(omega);
        const float sinw = std::sin(omega);

        const float a = std::sqrt(height);
        const float q = width;
        const float alpha = sinw / (2.0f * q);

        const float a0 = 1.0f + alpha / a;
        const float a1 = -2.0f * cosw;
        const float a2 = 1.0f - alpha / a;
        const float b0 = 1.0f + alpha * a;
        const float b1 = -2.0f * cosw;
        const float b2 = 1.0f - alpha * a;

        biquad.b0 = b0 / a0;
        biquad.b1 = b1 / a0;
        biquad.b2 = b2 / a0;
        biquad.a1 = a1 / a0;
        biquad.a2 = a2 / a0;
    }

    float Process(float input) { return biquad.Process(input); }

    // Task 9.1: BiquadDf1's own difference equation --
    // y[n] = b0 x[n] + b1 x[n-1] + b2 x[n-2] - a1 y[n-1] - a2 y[n-2]
    // (DspMath.hpp's BiquadDf1::Process) -- gives the standard direct-form-1
    // closed form H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2),
    // evaluated at z = e^{j*2*pi*normalizedFrequency} (normalizedFrequency in
    // cycles/sample, matching this struct's own `freq` convention).
    static std::complex<float> ComputeTransferFunctionValue(
        float b0Value, float b1Value, float b2Value, float a1Value, float a2Value, float normalizedFrequency)
    {
        const float omega = 2.0f * static_cast<float>(M_PI) * normalizedFrequency;
        const std::complex<float> zInv(std::cos(omega), -std::sin(omega));
        const std::complex<float> zInv2 = zInv * zInv;
        const std::complex<float> numerator = b0Value + b1Value * zInv + b2Value * zInv2;
        const std::complex<float> denominator = std::complex<float>(1.0f, 0.0f) + a1Value * zInv + a2Value * zInv2;
        return numerator / transfer_function_detail::SafeDenominator(denominator);
    }

    void PopulateUIState(UIState& state) const
    {
        state.b0.store(biquad.b0);
        state.b1.store(biquad.b1);
        state.b2.store(biquad.b2);
        state.a1.store(biquad.a1);
        state.a2.store(biquad.a2);
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only
    // biquad's recursive x1/x2/y1/y2 history -- NOT b0/b1/b2/a1/a2 (those
    // are coefficients, recomputed from freq/height/width by
    // UpdateCoefficients() on the very next SetFreq/SetHeight/SetWidth call,
    // not signal state) and NOT freq/height/width themselves (tuning, set by
    // the caller every block regardless). This is exactly the recovery
    // scoopNotch's own self-oscillation bug (see FroggersAppCore.hpp's
    // RouteAudioSample comment) needed: the biquad's y1/y2 can diverge to
    // non-finite from a bad coefficient set and then stay non-finite forever
    // afterward even once freq/height/width return to sane values, because
    // the difference equation (DspMath.hpp's BiquadDf1::Process) keeps
    // feeding y1/y2 back into every future output.
    void Reset()
    {
        biquad.x1 = 0.0f;
        biquad.x2 = 0.0f;
        biquad.y1 = 0.0f;
        biquad.y2 = 0.0f;
    }

    // Tasks 2.3/2.4 (Tier 1/Tier 2 recovery): checks recursive state only
    // (x1/x2/y1/y2), not the coefficients -- a coefficient set computed from
    // a momentarily extreme freq/height/width is not itself "poisoned",
    // only a diverged y1/y2 that keeps propagating regardless of the
    // coefficients is.
    bool StateFinite() const
    {
        return std::isfinite(biquad.x1) && std::isfinite(biquad.x2) && std::isfinite(biquad.y1) &&
               std::isfinite(biquad.y2);
    }

    float StateMagnitude() const
    {
        return std::max({std::fabs(biquad.x1), std::fabs(biquad.x2), std::fabs(biquad.y1), std::fabs(biquad.y2)});
    }
};

// src/core/Comb.hpp:9-77.
struct Comb
{
    static constexpr size_t kSize = 8192;

    // Task 9.1 (design D10): the comb's *linearised* response (saturator
    // treated as identity, see this file's header comment) is a feedback
    // loop of "delay by delaySamples, then one-pole lowpass, then scale by
    // feedback" around unity input gain:
    //   Y(z) = X(z) + feedback * z^-N * Hlp(z) * Y(z)
    //   H(z) = Y(z)/X(z) = 1 / (1 - feedback * z^-N * Hlp(z))
    // -- captured state is exactly what that closed form needs: the
    // feedback coefficient, the lowpass's alpha (OnePoleLowPass::alpha, the
    // same state Sheaf's own OnePoleLowPass::UIState captures,
    // DspFilters.hpp:22-23), and the integer delay length.
    struct UIState : synth::TransferFunction
    {
        std::atomic<float> feedback{0.0f};
        std::atomic<float> lowPassAlpha{0.0f};
        std::atomic<std::size_t> delaySamples{0};

        float FrequencyResponse(float normalizedFrequency) const override
        {
            return std::abs(TransferFunctionValue(normalizedFrequency));
        }

        std::complex<float> TransferFunctionValue(float normalizedFrequency) const override
        {
            return Comb::ComputeLinearizedTransferFunctionValue(
                feedback.load(), lowPassAlpha.load(), delaySamples.load(), normalizedFrequency);
        }
    };

    OnePoleLowPass filter;
    float delayLine[kSize]{};
    size_t index = 0;
    size_t delaySamples = 0;
    float feedback = 0.0f;
    PadeSaturator saturator;

    void SetFeedback(float fb) { feedback = fb; }
    void SetCutoffAlpha(float cutoff) { filter.alpha = cutoff; }

    // src/core/Comb.hpp:54, verbatim: out = in + fb*sat(lp(delay[i-N])).
    float Process(float input)
    {
        const float tapped = delayLine[(index + kSize - delaySamples) % kSize];
        const float output = input + feedback * PadeSaturator::Saturate(filter.Process(tapped));
        delayLine[index] = output;
        index = (index + 1) % kSize;
        return output;
    }

    // Task 9.1: the closed form derived in this struct's header comment.
    // `Hlp(z) = alpha / (1 - (1-alpha) z^-1)` matches
    // OnePoleLowPass::Process's own recurrence
    // (`output = alpha*input + (1-alpha)*output`, app/dsp/DspMath.hpp)
    // exactly. `SafeDenominator` (this file's top) keeps every division
    // finite and bounded even when feedback*Hlp's magnitude approaches or
    // exceeds 1 (a genuinely self-oscillating configuration, task 9.4).
    static std::complex<float> ComputeLinearizedTransferFunctionValue(
        float feedbackValue, float lowPassAlpha, size_t delaySamplesValue, float normalizedFrequency)
    {
        const float omega = 2.0f * static_cast<float>(M_PI) * normalizedFrequency;
        const std::complex<float> zInv(std::cos(omega), -std::sin(omega));
        const std::complex<float> lowPassResponse =
            std::complex<float>(lowPassAlpha, 0.0f) /
            transfer_function_detail::SafeDenominator(
                std::complex<float>(1.0f, 0.0f) - (1.0f - lowPassAlpha) * zInv);
        // z^-N = e^{-j*N*omega}; computed directly from N*omega (exact, no
        // accumulated per-sample multiplication error) since |zInv| == 1.
        const float omegaN = omega * static_cast<float>(delaySamplesValue);
        const std::complex<float> zInvN(std::cos(omegaN), -std::sin(omegaN));
        const std::complex<float> loopGain = feedbackValue * zInvN * lowPassResponse;
        return std::complex<float>(1.0f, 0.0f) /
               transfer_function_detail::SafeDenominator(std::complex<float>(1.0f, 0.0f) - loopGain);
    }

    void PopulateUIState(UIState& state) const
    {
        state.feedback.store(feedback);
        state.lowPassAlpha.store(filter.alpha);
        state.delaySamples.store(delaySamples);
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only the
    // comb's own recursive state -- the delay line, its write index, and
    // the one-pole lowpass's output -- NOT feedback/delaySamples (config,
    // reassigned every block from the Filter bank's knobs by
    // RouteAudioSample) and NOT filter.alpha (same: reassigned by
    // SetCutoffAlpha every block). Mirrors dsp::Reverb::Reset()'s own
    // "clears state, does not reconfigure" convention (Reverb.hpp).
    void Reset()
    {
        filter.output = 0.0f;
        std::fill(delayLine, delayLine + kSize, 0.0f);
        index = 0;
    }

    // Tasks 2.3/2.4 (Tier 1/Tier 2 recovery). O(kSize) per call by
    // necessity (the entire recirculating delay line is state a poisoned
    // sample could be sitting in) -- called once per block (not per
    // sample), so this is 8192 float compares per block per Comb instance,
    // negligible next to a typical audio block's own per-sample DSP cost.
    bool StateFinite() const
    {
        if (!std::isfinite(filter.output))
        {
            return false;
        }
        for (size_t i = 0; i < kSize; ++i)
        {
            if (!std::isfinite(delayLine[i]))
            {
                return false;
            }
        }
        return true;
    }

    float StateMagnitude() const
    {
        float magnitude = std::fabs(filter.output);
        for (size_t i = 0; i < kSize; ++i)
        {
            magnitude = std::max(magnitude, std::fabs(delayLine[i]));
        }
        return magnitude;
    }

    // src/core/Comb.hpp:61-64 (GetDelaySamples): N = 1/freq.
    static float GetDelaySamples(float freq) { return 1.0f / freq; }

    // src/core/Comb.hpp:66-76 (GetFeedback): asymmetric feedback, magnitude
    // scaled by ZeroedExpCompute's 0..1 curve.
    //
    // DELIBERATE PARITY DIVERGENCE (item 1, design.md A1/A1a/A2, 2026-07-29,
    // operator-approved -- same treatment as Fuegoize.hpp's own D6
    // divergence note): the frozen firmware scales by +-1.1. This port
    // scales by +-0.95 instead. The predecessor's analysis (and an earlier
    // statement to the operator) claimed |fb| > 1 makes the comb diverge
    // exponentially -- that is FALSE. `Process()` above is
    // `out = in + fb*Saturate(lp(delayed))`, and `PadeSaturator::Saturate`
    // sits INSIDE the feedback path, so the fed-back term can never exceed
    // |fb|*1.0 no matter how large |fb| is -- there is no exponential
    // divergence possible here. What |fb| > 1 actually does is hold the
    // loop in permanent, undecaying self-oscillation pinned at the
    // saturator's limit: `Reset()`-free silence never arrives, because each
    // pass regenerates (>=1.0)*saturator-bounded-output faster than it can
    // decay. |fb| < 1 is the mathematical definition of a loop that DOES
    // decay once its input stops (each pass multiplies by a strictly
    // sub-unity factor -- see `PadeSaturator::Saturate`'s own comment: it is
    // compressive, `Saturate(y) <= y` for `y >= 0`, so `|fb|*Saturate(y) <=
    // |fb|*y` and the sequence is geometric with ratio `<= |fb| < 1`).
    // 0.95 keeps that decay slow enough to still ring for a long, musical
    // time while guaranteeing it eventually reaches silence. Parity was
    // explicitly deprioritized here by the operator ("parity is stupid");
    // this is the intended outcome, not a defect -- do not restore 1.1.
    static float GetFeedback(float knob)
    {
        constexpr float kMaxFeedbackMagnitude = 0.95f;
        if (knob < 0.5f)
        {
            return -kMaxFeedbackMagnitude * ZeroedExpCompute(0.25f, 2.0f * (0.5f - knob));
        }
        return kMaxFeedbackMagnitude * ZeroedExpCompute(0.25f, 2.0f * (knob - 0.5f));
    }
};

// src/core/Comb.hpp:79-109. Fractional interpolation at :98-108.
struct PureDelay
{
    static constexpr size_t kSize = 8192;

    float delayLine[kSize]{};
    size_t index = 0;
    float delaySamples = 0.0f;

    void SetDelaySeconds(float seconds, float sampleRate) { delaySamples = seconds * sampleRate; }

    float Process(float input)
    {
        delayLine[index] = input;
        const float lowExact = static_cast<float>(index) + static_cast<float>(kSize) - delaySamples;
        const float frac = lowExact - std::floor(lowExact);
        const size_t idx0 = static_cast<size_t>(lowExact) % kSize;
        const size_t idx1 = (idx0 + 1) % kSize;
        const float output = delayLine[idx0] * (1.0f - frac) + delayLine[idx1] * frac;
        index = (index + 1) % kSize;
        return output;
    }
};

// FroggersEngine.hpp:822-848 (ApplyOutputFx), the parallel (:824-833) and
// serial (:834-839) comb/peak/scoop routing only -- the reverb blend and
// m_simFxInsert hook that follow in the frozen function are out of scope.
struct FilterFxChain
{
    ResonantBump peak;
    ResonantBump scoopNotch;
    Comb comb;
    PureDelay pureDelay;

    // W2.2a (openspec/changes/frogg3rs-modulation-truth-and-voicing/
    // tasks.md): smooths the comb branch's exact output trim computed in
    // Process() below. `fb` (Comb::feedback) is a per-block cached knob
    // read (FroggersAppCore.hpp:1027) that steps at every block boundary
    // and every Crispy/randomize scramble -- smoothing the TRIM value here
    // (never `fb` itself, which would change the filter's behaviour, not
    // just its level) erases that zipper.
    OnePoleLowPass combTrimSmoother;

    FilterFxChain()
    {
        // Sample-rate-independent knob-glide idiom this codebase already
        // uses when a DSP unit has no sample-rate handle to convert a real
        // Hz cutoff into alpha (RandomShLane.hpp:183 SetAlphaFromNatFreq
        // call, :274-275 kFastCutoff/kSlowCutoff constants) -- FilterFxChain
        // has no SetSampleRate() of its own, so a fixed cycles/sample cutoff
        // is the only option without threading a new parameter down from
        // FroggersAppCore (W2.2a constraint). 0.01 cycles/sample sits
        // between RandomShLane's kFastCutoff (0.45, near-instant) and
        // kSlowCutoff (0.002, audible glide): fast enough to track knob
        // motion, slow enough to erase the per-block trim step.
        constexpr float kCombTrimGlideCyclesPerSample = 0.01f;
        combTrimSmoother.SetAlphaFromNatFreq(kCombTrimGlideCyclesPerSample);
        combTrimSmoother.output = 1.0f;  // unity at fb=0 -- matches the untrimmed branch (no initial fade-in).
    }

    // combPeakBlend/scoopMix are precomputed 0..1 control values (the
    // frozen engine derives them from smoothed knob reads via RuntimeParam,
    // which is parameter-smoothing infrastructure, not a DSP unit in scope
    // for task 3.6 -- callers pass the already-smoothed 0..1 value).
    float Process(float input, bool useParallel, float combPeakBlend, float scoopMix)
    {
        if (useParallel)
        {
            // :824-833
            const float combRaw = comb.Process(pureDelay.Process(input));
            // W2.2a: exact output trim `1/(1+|fb|)` on the comb branch
            // ONLY, before the blend with peakPath below. `|comb| <= A +
            // |fb|` (PadeSaturator bounds the fed-back term to +-1,
            // Comb::Process above), so this normalizes the worst case
            // (A=0, i.e. the comb's own decaying tail with no input) to
            // exactly 1.0 at |fb| == kMaxFeedbackMagnitude (0.95), while
            // leaving +3.5 dB of bloom across the rest of the feedback
            // travel. A scalar on a branch output moves level, not
            // frequency response -- the comb's peaks/notches/ring/decay
            // are unchanged. `fb` is read as a MAGNITUDE (GetFeedback is
            // asymmetric +-0.95) directly from Comb's own state, not
            // threaded as a new parameter.
            const float rawCombTrim = 1.0f / (1.0f + std::fabs(comb.feedback));
            const float combTrim = combTrimSmoother.Process(rawCombTrim);
            const float combPath = combRaw * combTrim;
            const float peakPath = peak.Process(input);
            const float mixed = peakPath * (1.0f - combPeakBlend) + combPath * combPeakBlend;
            const float scooped = scoopNotch.Process(mixed);
            return mixed * (1.0f - scoopMix) + scooped * scoopMix;
        }

        // :834-839
        float output = pureDelay.Process(input);
        output = comb.Process(output);
        output = peak.Process(output);
        return output;
    }
};

}  // namespace synth_froggers::dsp

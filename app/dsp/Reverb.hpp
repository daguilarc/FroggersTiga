#pragma once

// synth_froggers::dsp::Reverb -- packet 3 task 3.8 (extended DSP port,
// design D15). openspec/changes/froggers-sheaf-app/tasks.md section 3, item
// 3.8. A **copy** (design D3) of the cited Froggers formulas -- read
// directly from the frozen source before porting, not from memory.
//
// Ported (7 of the Reverb page's 9 params -- Wet/dry, Room size, Decay,
// Pre-delay, Damping, Stereo width, Diffusion) from:
//   - src/core/FroggersEngine.hpp:493-534  ProcessReverb, verbatim signal
//     path (pre-delay tap, twin comb-ish delay lines A/B with cross-feed
//     diffusion, shared damping low-pass, stereo width blend)
//   - src/core/FroggersEngine.hpp:455-461  param wiring:
//       :455 Wet/dry   = m_reverbParams->GetParam(0), direct passthrough
//       :456 Room size = ExpMap(0.05, 1.0, GetParam(1))
//       :457 Decay     = ExpMap(0.1, 0.98, GetParam(2))
//       :458 Pre-delay = ExpMap(1/sr, 100/sr, GetParam(3)) [see note below]
//       :459 Damping   = ExpMap(0.001, 0.2, 1 - GetParam(4)), fed directly
//            as the shared damping filter's alpha (:574 `m_rvDampFilter
//            .m_alpha = m_rvDamp.Process()`) -- NOT run through
//            SetAlphaFromNatFreq; the ExpMap output IS the alpha.
//       :460 Stereo width = GetParam(5), direct passthrough
//       :461 Diffusion    = GetParam(6), direct passthrough
//   - src/core/FroggersEngine.hpp:844-846 (ApplyOutputFx) -- the Wet/dry
//     blend `(1-rvMix)*output + rvMix*rvb` that actually consumes Wet/dry;
//     ProcessReverb itself never reads m_rvMix.
//   - src/core/FroggersEngine.hpp:65-69 (x_rvSize = 4096, the three
//     x_rvSize-length ring buffers, and m_rvDampFilter's type OPLowPassFilter,
//     ported here as the shared dsp::OnePoleLowPass).
//
// Pre-delay note: the frozen ExpMap divides both endpoints by sampleRate
// (`ExpMap(1.0f/sr, 100.0f/sr, knob)`) and ProcessReverb then re-multiplies
// the result by sampleRate to get a sample count (:497 `preNorm *
// m_sampleRate`). That double division/multiplication is unusual (it is
// NOT simply "1 to 100 samples" pre-scaled) but the sampleRate terms are
// exact algebraic inverses of one another around the exponential map, so
// it is ported verbatim rather than "simplified" -- an exponential map is
// not linear, so cancelling the /sr and *sr naively would change the
// result.
//
// Smoothing NOT ported: FroggersEngine.hpp reads every one of these seven
// knobs through a RuntimeParam (a one-pole smoothing filter on the raw
// 0..1 target, to avoid zipper noise on knob movement) before using the
// value. That smoothing is parameter-model infrastructure, not DSP, and
// per this port's established convention (see Vco.hpp, FilterFx.hpp) is
// owned by Sheaf's parameter model -- callers of this unit pass already-
// resolved 0..1 knob values per sample.
//
// NOT ported (deliberately, design D15): Mod depth and Hold (Reverb page
// slots 7 and 8). `m_reverbParams->GetParam(7)`/`GetParam(8)` are never
// read anywhere in FroggersEngine.hpp -- confirmed by grep -- so there is
// no frozen formula to pin. They are newly authored below, clearly marked,
// with behavioral (not parity) tests. At their neutral defaults
// (modDepthKnob=0, holdKnob=0) the authored stage is a no-op, so the seven
// ported params reproduce ProcessReverb + the Wet/dry blend exactly.

#include "DspMath.hpp"
#include "FilterFx.hpp"  // reuse dsp::PadeSaturator (S2a.1 below; same reuse Delay.hpp's B2 makes).
#include "Limiter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace synth_froggers::dsp {

// B6b (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md,
// "Group B outcomes" / operator: "why can't we just have limiters for
// reverb and delay?"): tuning for `Reverb::wetLimiter` below, a single
// `dsp::OutputLimiter` (dsp/Limiter.hpp) instance inserted on the value
// `Process()` returns, AFTER both tanks and the dry/wet mix -- NOT inside
// the feedback loop, and it never touches `fb`/Hold's own computation
// (:238 below is unchanged). Hold (W2.1-MATH-2: `fb` -> ~0.99998 at Hold's
// ceiling, ~50,000x steady-state gain, deliberately parked just under
// self-oscillation) keeps sustaining exactly as before; this stage only
// caps the LEVEL that escapes toward the master limiter, the same "bound
// what escapes, don't touch what persists" treatment B6a gives Delay.
//
// Chosen BY MEASUREMENT (scratch harness driving this exact struct with
// Hold pinned to 1.0/max, decay knob swept {0,0.5,1.0}, room size knob swept
// {0,0.3,0.6,1.0}, sustained full-scale input, fully wet (mix=1.0, isolates
// the tank's own bound from any diluting dry floor), 20000 samples per
// point):
//
//   - Raw (no limiter) growth is genuinely unbounded and, unlike Delay's
//     worst case, DOES build over hundreds of milliseconds to seconds at
//     moderate room sizes -- measured raw |output| reached ~7x input by
//     100ms, ~14x by 200ms, ~72x by 1s, ~290x by 4s of sustained input
//     (size=0.6, decay=1.0, Hold=1.0) -- confirming the "sustained
//     material" premise for the STEADY-STATE climb.
//   - But master's own tuning (0.9/1ms/100ms) is still NOT sufficient: the
//     measured worst-case TRANSIENT overshoot (at the smallest room size,
//     shortest tank round trip -- the reverb's analogue of Delay's
//     shortest-delay-time worst case) was 1.282757 against the 1.0
//     ceiling, at a 1ms attack. Same failure shape as B6a: the onset is
//     fast even though the long-run climb is slow.
//   - kReverbWetLimiterThreshold (0.9): kept AT the master's value, for the
//     identical reason B6a's comment gives (dsp/Delay.hpp) -- this stage
//     runs inside `Reverb::Process`, upstream of `SanitizeOutputSample`'s
//     master limiter in FroggersAppCore.hpp, so it always reduces first
//     regardless of where its threshold sits relative to the master's.
//   - kReverbWetLimiterAttackSeconds (2 microseconds): sweeping attack
//     alone (threshold 0.9, release 100ms, worst case taken across every
//     room-size/decay combination above) found the worst-case overshoot
//     crosses under the 1.0 ceiling between 3us (1.000006) and 2us
//     (1.000000); 2 microseconds sits at that measured crossing -- the
//     identical value B6a's own sweep found for Delay's own worst case
//     (dsp/Delay.hpp's own comment), not copied from it by analogy.
//   - kReverbWetLimiterReleaseSeconds (100ms, matches the master): as with
//     B6a, the worst-case peak is an attack-side effect; release does not
//     move it, so it stays at the value this codebase already uses for
//     "gain reduction that does not pump" (dsp::OutputLimiter::
//     kDefaultReleaseSeconds, dsp::kPeakLimiterReleaseSeconds,
//     dsp::kDelayWetLimiterReleaseSeconds) rather than inventing a fourth
//     number where measurement gave no reason to move it.
// F2.1b: retargeted from 0.9 to 0.72, preserving the ORIGINAL
// threshold/ceiling ratio (0.9/1.0 == 0.72/0.80) rather than picking a
// round number, so this stage's measured knee (attack/release above) keeps
// its character instead of being re-tuned by accident. 0.9 was strictly
// below kSharedCeiling (1.0) but is ABOVE the retargeted kStageCeiling
// (0.80) -- the negative-headroom exponential-amplifier trap F2.1a's
// static_assert below exists to catch -- so it comes down in this same edit.
inline constexpr float kReverbWetLimiterThreshold = 0.72f;
inline constexpr float kReverbWetLimiterCeiling = kStageCeiling;
// F2.1a: see dsp::OutputLimiter::kDefaultThreshold's own static_assert
// (dsp/Limiter.hpp).
static_assert(kReverbWetLimiterThreshold < kReverbWetLimiterCeiling,
              "threshold must stay strictly below ceiling; a negative headroom turns "
              "DesiredMagnitude into an exponential amplifier -- see dsp/Limiter.hpp");
inline constexpr float kReverbWetLimiterAttackSeconds = 2.0e-6f;   // 2 microseconds -- see comment above.
inline constexpr float kReverbWetLimiterReleaseSeconds = kSharedReleaseSeconds;  // shared; see Limiter.hpp.

struct Reverb
{
    // src/core/FroggersEngine.hpp:65 (x_rvSize).
    static constexpr size_t kSize = 4096;

    // Authored Mod depth constants (no frozen equivalent to cite) -- a slow
    // wow on the tank read-taps, deliberately small (well under one sample
    // at typical knob values) so it colors the tail without destabilising
    // the delay-line indexing.
    static constexpr float kModLfoHz = 0.35f;
    static constexpr float kModMaxOffsetSamples = 24.0f;

    float lineA[kSize]{};
    float lineB[kSize]{};
    float preLine[kSize]{};
    size_t indexA = 0;
    size_t indexB = 0;
    size_t preIndex = 0;

    // src/core/FroggersEngine.hpp:69, shared between the A and B taps just
    // as the frozen single m_rvDampFilter instance is (ported faithfully,
    // including the shared-state quirk of filtering A then B in sequence
    // through the same one-pole state each sample).
    OnePoleLowPass dampFilter;

    float wetL = 0.0f;
    float wetR = 0.0f;

    // Authored Mod depth's own LFO phase (no frozen equivalent).
    float modLfoPhase = 0.0f;

    // B6b: the stage's own output limiter, applied to what Process() (below)
    // returns -- after both tanks and the dry/wet mix, never inside the
    // feedback loop. See this file's header comment (above the struct) for
    // the tuning and its measurement.
    OutputLimiter wetLimiter;

    // B6b: unlike `dsp::StereoDelay` (which is always `SetSampleRate()`'d
    // before any `Process()` call, dsp/Delay.hpp), `Reverb` has no such
    // entry point of its own -- every caller passes `sampleRate` directly
    // into `Process()` each call instead, and several existing tests
    // default-construct a `dsp::Reverb` and call `Process()` immediately
    // with no configuration step at all. Mirrors `FilterFxChain`'s own
    // constructor (dsp/FilterFx.hpp, B5): pre-configure `wetLimiter` at an
    // assumed 48kHz here so a bare `dsp::Reverb rv;` still gets this stage's
    // intended threshold/attack/release rather than `OutputLimiter`'s
    // zero-initialized attack/release coefficients (unconfigured, `Process()`
    // would apply no smoothing at all). `Configure()` below re-configures at
    // the real sample rate once FroggersAppCore::PrepareToPlay() calls it
    // (mirrors `filterChain_.Configure(sampleRate_)`/`delay_.SetSampleRate(
    // sampleRate_)` there).
    Reverb()
    {
        constexpr float kDefaultAssumedSampleRate = 48000.0f;
        Configure(kDefaultAssumedSampleRate);
    }

    // B6b: sample-rate-dependent configuration for `wetLimiter`, separate
    // from the constructor above for the identical reason `FilterFxChain::
    // Configure()` is (dsp/FilterFx.hpp) -- the real sample rate is only
    // known once FroggersAppCore::PrepareToPlay() runs.
    void Configure(float sampleRate)
    {
        wetLimiter.Configure(sampleRate, kReverbWetLimiterThreshold, kReverbWetLimiterCeiling,
                              kReverbWetLimiterAttackSeconds, kReverbWetLimiterReleaseSeconds);
    }

    // Task 6.x (Stop-transport reset, app/FroggersAppCore.hpp's ProcessBlock
    // running->stopped edge): zero every member that carries signal energy
    // between calls to Process() -- the recursive comb-ish tank (lineA/
    // lineB), the pre-delay line, all three ring indices, the shared
    // damping filter's one-pole state (dampFilter.output; its `alpha`
    // coefficient is recomputed from dampKnob01 every Process() call, so
    // leaving it untouched is correct -- this clears state, it does not
    // reconfigure), and the last computed wet outputs. Decay (up to 0.98)
    // and Hold (:169-174, fb approaching but never reaching 1.0) make this
    // tank self-sustaining on its own, so without this the reverb keeps
    // ringing after the operator stops the transport.
    //
    // modLfoPhase: reset to 0 too, even though it carries no *signal*
    // energy (it only offsets which sample of the already-zeroed lineA/
    // lineB gets read back, Sine01(modLfoPhase) at :153) -- silence after
    // Reset() does not depend on its value. It is zeroed anyway so a
    // Reset()'d Reverb is in a single deterministic state regardless of how
    // long the instrument had been running before Stop, rather than
    // carrying over an arbitrary phase from the previous run.
    void Reset()
    {
        std::fill(lineA, lineA + kSize, 0.0f);
        std::fill(lineB, lineB + kSize, 0.0f);
        std::fill(preLine, preLine + kSize, 0.0f);
        indexA = 0;
        indexB = 0;
        preIndex = 0;
        dampFilter.output = 0.0f;
        wetL = 0.0f;
        wetR = 0.0f;
        modLfoPhase = 0.0f;
        // B6b: `wetLimiter` carries its own per-sample `envelope` state, so
        // a buffer clear -- Stop-transport reset or Tier 1 fault recovery,
        // both routed through this same Reset() -- resets it too, the same
        // treatment B6a gives `StereoDelay::wetLimiterL`/`R` (dsp/Delay.hpp).
        wetLimiter.Reset();
    }

    // Task 2.3 (Tier 1 recovery, app/FroggersAppCore.hpp): Reverb has NO
    // gate/bypass -- Process() unconditionally writes into lineA/lineB/
    // preLine and unconditionally feeds `input` through dampFilter's shared
    // one-pole state every call, regardless of any parameter -- so a single
    // non-finite sample arriving from an upstream fault propagates into this
    // unit's own state (first the pre-delay tap, typically within a handful
    // of samples at default settings, then the room delay taps once the
    // tainted ring slot is eventually read back) and, once dampFilter.output
    // itself goes non-finite, poisons every future sample this Reverb ever
    // produces -- permanently, since nothing else clears it. This is exactly
    // the "audio never comes back" failure mode Tier 1 exists to fix, and
    // Reverb is just as exposed to it as any Item-2 unit; reuses this
    // existing Reset() (added packet 3, task "Stop-transport reset") rather
    // than adding a duplicate.
    bool StateFinite() const
    {
        if (!std::isfinite(dampFilter.output) || !std::isfinite(wetL) || !std::isfinite(wetR) ||
            !std::isfinite(modLfoPhase))
        {
            return false;
        }
        // B6b: fold `wetLimiter`'s own finiteness into this unit's
        // aggregate -- `RecoverIfNonFinite(reverb_)` (FroggersAppCore.hpp)
        // calls this StateFinite()/the Reset() above uniformly, so a
        // poisoned limiter envelope must be visible here.
        if (!wetLimiter.StateFinite())
        {
            return false;
        }
        for (size_t i = 0; i < kSize; ++i)
        {
            if (!std::isfinite(lineA[i]) || !std::isfinite(lineB[i]) || !std::isfinite(preLine[i]))
            {
                return false;
            }
        }
        return true;
    }

    // F3.1 (frogg3rs-blowout-and-drilldown-repair): read-only diagnostic,
    // NOT wired into RecoverPoisonedUnitState's Tier 2, same reasoning as
    // dsp::StereoDelay::StateMagnitude()'s own comment (BIBO-stable
    // feedback loop, legitimately large-but-finite under sustained loud
    // input -- Tier 2's ceiling would misfire here). Exists so the
    // Stop-flush measurement harness can tell "cleared once and stayed
    // clear" apart from "cleared once, then refilled."
    float StateMagnitude() const
    {
        float magnitude = std::max({std::fabs(dampFilter.output), std::fabs(wetL), std::fabs(wetR)});
        for (size_t i = 0; i < kSize; ++i)
        {
            magnitude = std::max({magnitude, std::fabs(lineA[i]), std::fabs(lineB[i]), std::fabs(preLine[i])});
        }
        return magnitude;
    }

    // -- Ported formula helpers, exposed statically for direct pinning
    // (mirrors dsp::Vco's PitchToPhaseIncrement/PmDepthScale precedent) --

    // :456 Room size.
    static float RoomSizeFromKnob(float knob01) { return ExpMapCompute(0.05f, 1.0f, knob01); }

    // :457 Decay.
    static float DecayFeedbackFromKnob(float knob01) { return ExpMapCompute(0.1f, 0.98f, knob01); }

    // :458 Pre-delay (see file-header note on the /sr, *sr round trip).
    static float PreDelayNormFromKnob(float knob01, float sampleRate)
    {
        return ExpMapCompute(1.0f / sampleRate, 100.0f / sampleRate, knob01);
    }

    // :459, :574 Damping -- the ExpMap output IS the damping filter's alpha.
    static float DampAlphaFromKnob(float knob01) { return ExpMapCompute(0.001f, 0.2f, 1.0f - knob01); }

    // Returns the fully mixed (dry/wet-blended) output, i.e. what
    // ApplyOutputFx's `(1.0f - rvMix) * output + rvMix * rvb` computes
    // (FroggersEngine.hpp:844-846), folding the Wet/dry knob (:455) in here
    // since this is the port's only consumer of that ported parameter.
    float Process(float input,
                   float mixKnob01,
                   float sizeKnob01,
                   float decayKnob01,
                   float preKnob01,
                   float dampKnob01,
                   float widthKnob01,
                   float diffusionKnob01,
                   float sampleRate,
                   float modDepthKnob01 = 0.0f,
                   float holdKnob01 = 0.0f)
    {
        // :458, :497-504 -- pre-delay tap.
        const float preNorm = PreDelayNormFromKnob(preKnob01, sampleRate);
        size_t preDelay = static_cast<size_t>(std::round(preNorm * sampleRate));
        if (preDelay >= kSize)
        {
            preDelay = kSize - 1;
        }
        preLine[preIndex] = input;
        const size_t preRead = (preIndex + kSize - preDelay) % kSize;
        const float preOut = preLine[preRead];
        preIndex = (preIndex + 1) % kSize;

        // :456, :506-512 -- room size sets both tank delay lengths.
        const float sizeNorm = RoomSizeFromKnob(sizeKnob01);
        size_t baseA = static_cast<size_t>(180.0f + sizeNorm * 1300.0f);
        size_t baseB = static_cast<size_t>(260.0f + sizeNorm * 1800.0f);
        size_t dA = std::min(kSize - 1, std::max(static_cast<size_t>(1), baseA));
        size_t dB = std::min(kSize - 1, std::max(static_cast<size_t>(1), baseB));

        // Authored Mod depth: a small sinusoidal wow on the read taps.
        // modDepthKnob01 == 0 -> modOffset == 0 -> dA/dB unchanged, so this
        // never disturbs the parity case.
        modLfoPhase = WrapPhase(modLfoPhase + kModLfoHz / sampleRate);
        const float modOffset = modDepthKnob01 * kModMaxOffsetSamples * Sine01(modLfoPhase);
        const long offsetSamples = std::lround(modOffset);
        const auto applyMod = [&](size_t baseDelay) -> size_t {
            long modulated = static_cast<long>(baseDelay) + offsetSamples;
            modulated = std::max<long>(1, std::min<long>(static_cast<long>(kSize) - 1, modulated));
            return static_cast<size_t>(modulated);
        };
        dA = applyMod(dA);
        dB = applyMod(dB);

        const size_t readA = (indexA + kSize - dA) % kSize;
        const size_t readB = (indexB + kSize - dB) % kSize;

        const float valA = lineA[readA];
        const float valB = lineB[readB];

        // :457, :514-515 -- decay/feedback, folded with authored Hold.
        // holdKnob01 == 0 -> fb == decayFb exactly (parity default). Hold
        // is clamped strictly below 1.0 so the tail lengthens without ever
        // reaching true self-oscillation (bounded/finite requirement).
        const float decayFb = DecayFeedbackFromKnob(decayKnob01);
        const float fb = decayFb + (1.0f - decayFb) * std::min(holdKnob01, 0.999f);

        const float diffusion = diffusionKnob01;  // :461, direct passthrough
        const float cross = diffusion * 0.5f;
        const float aFb = valB * (1.0f - cross) + valA * cross;
        const float bFb = valA * (1.0f - cross) + valB * cross;

        // S2a.1 (openspec/changes/frogg3rs-parametric-slew-and-stop-root-cause/
        // tasks.md; OPERATOR DECISION 2026-08-07: "reverb tank should have
        // in-loop saturator anyway, the same one, for omni rule purposes").
        // Mirrors B2 (dsp/Delay.hpp, StereoDelay::Process's own comment)
        // exactly, same reasoning, same fix shape: `aFb`/`bFb` above are
        // unbounded reads straight off lineA/lineB (this tank's own cross-fed
        // taps), so the pre-fix `preOut + aFb * fb` fed a linear, unsaturated
        // loop -- steady state `preOut / (1 - fb)`, and with Hold maxed `fb`
        // (computed just above) -> ~0.99998, i.e. ~50,000x. Wraps each fed-back tap in
        // the SAME `PadeSaturator::Saturate` the comb's own loop
        // (FilterFx.hpp's Comb::Process) and the delay's B2 fix already use
        // -- reused, not reimplemented (OMNI §8) -- applied BEFORE the `fb`
        // multiply, exactly mirroring B2's `fbk * PadeSaturator::Saturate(fbL)`.
        // `Saturate` clamps to +-1 unconditionally, so every write to
        // lineA/lineB is now bounded by `|preOut| + fb` regardless of how
        // many round trips have already run -- a per-sample bound, not
        // merely a steady-state one.
        //
        // Hold's PERSISTENCE is untouched: PadeSaturator's small-signal gain
        // at x=0 is exactly 1.0 (FilterFx.hpp's own comment on the comb's
        // saturator, "the derivative at x=0 ... = 27/27 = 1"), so once a
        // tap's magnitude decays under the saturator's linear region the
        // fed-back term is again Saturate(x) ~= x and the tail keeps decaying
        // at the same fb-per-round-trip rate as before this fix -- this adds
        // a MAGNITUDE ceiling on a hot tank, not a change to the decay time
        // constant at ordinary levels. Same "bound what can blow up, don't
        // touch what legitimately persists" split B6b's wetLimiter already
        // keeps relative to `fb`'s own computation just above (also
        // untouched by that fix) -- this is that same split, one loop
        // further in, per the operator's own framing: "a structural guard,
        // not a tone change."
        const float aIn = preOut + fb * PadeSaturator::Saturate(aFb);
        const float bIn = preOut + fb * PadeSaturator::Saturate(bFb);

        dampFilter.alpha = DampAlphaFromKnob(dampKnob01);  // :459, :574
        const float aOut = dampFilter.Process(valA);
        const float bOut = dampFilter.Process(valB);

        lineA[indexA] = aIn;
        lineB[indexB] = bIn;
        indexA = (indexA + 1) % kSize;
        indexB = (indexB + 1) % kSize;

        const float mid = 0.5f * (aOut + bOut);
        const float width = widthKnob01;  // :460, direct passthrough
        wetL = mid + width * (aOut - mid);
        wetR = mid + width * (bOut - mid);
        const float wet = 0.5f * (wetL + wetR);

        const float mix = mixKnob01;  // :455, direct passthrough
        const float mixedOut = (1.0f - mix) * input + mix * wet;  // :846

        // B6b (tasks.md CONSOLIDATED PUSH table; "Group B outcomes" /
        // operator: "why can't we just have limiters for reverb and
        // delay?"): applied to the fully mixed dry/wet output, AFTER both
        // tanks (`lineA`/`lineB` already written above) and AFTER the mix
        // -- `fb`/Hold's own computation (`const float fb = ...` above) is
        // untouched [drive-by fix, S2a.1: this line previously cited a
        // stale ":341-342", already wrong before this edit -- symbol-
        // anchored per tasks.md's own "cite by SYMBOL, not by line" rule
        // now], so Hold keeps sustaining exactly as before; this only bounds the
        // LEVEL that escapes this stage toward the master limiter. See this
        // file's header comment (above the struct) for the tuning and its
        // measurement.
        return wetLimiter.Process(mixedOut);
    }
};

}  // namespace synth_froggers::dsp

#pragma once

// synth_froggers::dsp::OutputLimiter -- B5 (openspec/changes/
// archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md, CONSOLIDATED PUSH table
// and "Group B outcomes"): extracted out of app/FroggersAppCore.hpp (where
// it was Task 2.8/Item 3, design.md A2/A2a/A4's PRIVATE nested type) so a
// SECOND, independently-tuned instance can run on the Filter bank's peak
// branch (FilterFxChain, dsp/FilterFx.hpp) without duplicating the struct.
//
// WHY THIS FILE, NOT dsp/FilterFx.hpp AND NOT FroggersAppCore.hpp:
// `FroggersAppCore.hpp` includes `dsp/FilterFx.hpp` (never the reverse), so
// a peak-branch limiter living in `FilterFxChain` cannot reach UP into
// FroggersAppCore.hpp for the type without a circular include. It has to
// move DOWN into dsp/. A small dedicated header (rather than folding it
// into FilterFx.hpp beside PadeSaturator) keeps a general-purpose dynamics
// processor -- attack/release envelope follower, not a filter/saturator --
// out of a file whose own header comment scopes it to "comb/peak/scoop
// routing". `FilterFx.hpp` includes this header below; `FroggersAppCore.hpp`
// uses `dsp::OutputLimiter` transitively through that include (and directly,
// for its own `outputLimiter_` member).
//
// CRITICAL, B5's own constraint: the MASTER output limiter's behaviour must
// not change by one sample. Before this move, `kThreshold` (0.9) /
// `kCeiling` (1.0) / `kHeadroom` / `kAttackSeconds` (1ms) / `kReleaseSeconds`
// (100ms) were `static constexpr`, so every instance of the type shared one
// tuning -- exactly why a second instance (the peak branch's own limiter)
// could not just be dropped in verbatim; it would duck identically to the
// master and be useless there (W2.2-PREP already found this, tasks.md).
// Converted to instance fields here. The single-argument `Configure(
// sampleRate)` overload below reproduces the master's ORIGINAL tuning via
// the exact same formula, same operand order, same float literals, so
// FroggersAppCore.hpp's one pre-existing call site
// (`outputLimiter_.Configure(sampleRate_)`, PrepareToPlay()) is textually
// unchanged and bit-identical to before this extraction -- the existing
// master-limiter tests (FroggersAudioRoutingTests.cpp) are the proof.

#include <algorithm>
#include <cmath>

namespace synth_froggers::dsp {

// Shared across EVERY per-stage limiter instance (peak branch, delay wet,
// reverb wet, and the master). These two values are identical at all four
// sites by design, not by coincidence, so they live here once rather than
// being re-declared per stage (OMNI §8: 2+ occurrences must be abstracted;
// they were duplicated 4x and 3x respectively before this consolidation).
//
// What is DELIBERATELY NOT shared: each stage's `threshold` and
// `attackSeconds`. Both are MEASURED per stage and legitimately differ --
// peak 0.7/5us (single-sample transients from stored biquad energy), delay
// and reverb 0.9/2us (fast onset at the short-round-trip extreme), master
// 0.9/1ms (sustained material only). §K.4 records that inferring attack
// from mechanism shape was wrong twice; it is per-stage evidence, not a
// shared constant, and must not be folded in here.
//
// `kSharedCeiling` is full scale everywhere: a limiter's ceiling is what it
// must never let through, and that is 1.0 for every stage regardless of
// where the stage sits.
inline constexpr float kSharedCeiling = 1.0f;
// `kSharedReleaseSeconds`: 100ms at all four sites. B5 derived it from the
// peak's measured residual decay (-60dB in 11.79ms at max Q, so ~8.5x
// margin) and B6 measured it as correct for delay and reverb too; each
// stage's own comment already said "matches the master". One value.
inline constexpr float kSharedReleaseSeconds = 0.1f;

// NOTE for B7.1 (§K): the per-stage THRESHOLDS are currently scattered
// (0.7 peak, 0.9 delay/reverb/master). B7.1 retargets every stage to the
// measured shared ceiling C = 0.80. When that lands, the per-stage
// threshold constants collapse into one shared value here too -- and the
// master's stays 0.9, since it is the backstop that must fire LAST.

// VST/PLUGIN NOTE (operator, 2026-07-29, design.md A2a -- required comment,
// carried from this struct's original definition site): this stage does NOT
// need to live inside a future VST/plugin build. A plugin host owns final
// gain staging on its own output bus and typically supplies its own limiter
// there, so in a plugin context the MASTER instance of this stage is
// redundant -- a candidate to bypass or compile out rather than run twice.
// (The peak-branch instance this struct also now serves is a different
// concern -- in-chain gain staging, not final output gain -- and is not
// covered by this note.)
struct OutputLimiter
{
    // Pinned defaults for the single-argument Configure(sampleRate) overload
    // below -- the MASTER output limiter's ORIGINAL tuning (design.md A2a),
    // UNCHANGED by this extraction. Do not retune these; an independently-
    // tuned instance (e.g. the peak-branch limiter, B5) calls the five-
    // argument Configure() overload instead of touching these.
    static constexpr float kDefaultThreshold = 0.9f;
    static constexpr float kDefaultCeiling = kSharedCeiling;
    static constexpr float kDefaultAttackSeconds = 0.001f;  // fast: 1ms, per design.md A2a.
    static constexpr float kDefaultReleaseSeconds = kSharedReleaseSeconds;  // shared; design.md A2a ("so it does not pump").

    // Per-instance tuning (B5: was `static constexpr`, shared by every
    // instance of the type -- see this file's header comment for why that
    // had to change). Defaults match the master's original tuning so a
    // freshly-constructed-but-not-yet-`Configure()`'d instance is harmless
    // (identity below `kDefaultThreshold`, same as before).
    float threshold = kDefaultThreshold;
    float ceiling = kDefaultCeiling;
    float headroom = kDefaultCeiling - kDefaultThreshold;  // 0.1f.
    float attackSeconds = kDefaultAttackSeconds;
    float releaseSeconds = kDefaultReleaseSeconds;

    float attackCoeff = 0.0f;   // (re)computed by Configure(); one-pole coeff exp(-1/(t*sampleRate)).
    float releaseCoeff = 0.0f;
    float envelope = 1.0f;      // current gain multiplier; 1.0f == no reduction (also Reset()'s value).

    // Full configuration: sample-rate-dependent coefficients PLUS this
    // instance's own threshold/ceiling/attack/release. A second,
    // independently-tuned instance (the peak-branch limiter, FilterFx.hpp)
    // calls this overload; the master keeps calling the single-argument
    // overload below, unchanged.
    void Configure(float sampleRate, float thresholdIn, float ceilingIn, float attackSecondsIn,
                   float releaseSecondsIn)
    {
        threshold = thresholdIn;
        ceiling = ceilingIn;
        headroom = ceiling - threshold;
        attackSeconds = attackSecondsIn;
        releaseSeconds = releaseSecondsIn;
        const float sr = std::max(1.0f, sampleRate);
        attackCoeff = std::exp(-1.0f / (attackSeconds * sr));
        releaseCoeff = std::exp(-1.0f / (releaseSeconds * sr));
    }

    // Master-limiter convenience overload (BEHAVIOUR-PRESERVING): sample-rate
    // only, pinned to the original tuning above -- the exact call this
    // struct's one pre-existing caller (FroggersAppCore.hpp's
    // `outputLimiter_.Configure(sampleRate_)`) makes, textually unchanged by
    // this extraction.
    void Configure(float sampleRate)
    {
        Configure(sampleRate, kDefaultThreshold, kDefaultCeiling, kDefaultAttackSeconds, kDefaultReleaseSeconds);
    }

    // Instantaneous desired output magnitude for a given input magnitude:
    // identity below `threshold` (so a below-threshold sample's own target
    // gain is always exactly 1.0f), and above it an exponential-saturation
    // curve that asymptotes toward `ceiling` as `absX -> infinity` without
    // ever reaching or exceeding it for any finite input. Continuous AND
    // C1-continuous (equal value AND slope) at `absX == threshold`, so there
    // is no audible knee discontinuity between the two branches.
    float DesiredMagnitude(float absX) const
    {
        if (absX <= threshold) {
            return absX;
        }
        return threshold + headroom * (1.0f - std::exp(-(absX - threshold) / headroom));
    }

    // `targetGain == DesiredMagnitude(|x|) / |x|`: for any `|x| <= threshold`
    // this is exactly `|x| / |x| == 1.0f` (guarded at `absX == 0` to avoid a
    // 0/0). `envelope` then one-pole-smooths toward `targetGain`, fast
    // (`attackCoeff`) when MORE reduction is needed (`targetGain <
    // envelope`) and slow (`releaseCoeff`) when easing back off, so recovery
    // from a loud transient does not pump.
    //
    // Bit-identical claim for an entirely-below-threshold signal (unchanged
    // by this extraction -- `threshold`/`headroom` are read from instance
    // fields now instead of `static constexpr`, but hold the identical
    // values for the master instance, so the Sterbenz-lemma argument below
    // still applies bit for bit): if `envelope` is already exactly 1.0f and
    // `targetGain` is exactly 1.0f, then `coeff*1.0f + (1.0f-coeff)*1.0f` is
    // exactly 1.0f in IEEE-754 float, not merely in real-number arithmetic --
    // both `attackCoeff` and `releaseCoeff` sit in (0.5, 1) for any
    // musically reasonable sample rate (`exp(-1/N)` for `N` on the order of
    // tens of samples or more), so Sterbenz's lemma makes `1.0f - coeff`
    // exact, `coeff*1.0f`/`(1.0f-coeff)*1.0f` exact (multiplying by 1.0f is
    // always exact), and their sum exactly reconstructs the representable
    // value 1.0f with zero rounding error. So `envelope` never moves off
    // exactly 1.0f while the signal stays under threshold, and `x * 1.0f ==
    // x` bit for bit.
    float Process(float x)
    {
        const float absX = std::fabs(x);
        const float targetGain = absX > 0.0f ? DesiredMagnitude(absX) / absX : 1.0f;
        const float coeff = targetGain < envelope ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * targetGain;
        return x * envelope;
    }

    // Task 2.2-style per-unit recovery: unity gain is this unit's quiescent
    // state, the same convention `dsp::DriveBlendPhase::Reset()`
    // (Drive.hpp) uses for its own allpass history -- "no reduction" is this
    // stage's equivalent of "no recursive history". `envelope` cannot
    // legitimately leave `(0, 1]` from finite input (`targetGain` is always
    // in that range), so `StateFinite()` only exists to satisfy
    // `RecoverIfNonFinite<Unit>`'s template contract and defend against an
    // unforeseen path in.
    void Reset() { envelope = 1.0f; }
    bool StateFinite() const { return std::isfinite(envelope); }
};

}  // namespace synth_froggers::dsp

#pragma once

// synth_froggers::dsp::{VcoAdsrState, MixOscVoices} -- packet 3 task 3.2
// (DSP port). openspec/changes/froggers-sheaf-app/tasks.md section 3, item
// 3.2. A **copy** (design D3) of the cited Froggers formulas.
//
// Ported from:
//   - src/core/VcoAdsrState.hpp (whole file, verbatim -- it already has no
//     #include of src/, sim/, or desktop-v2/ paths of its own, so this is a
//     line-for-line copy under app/, not a shared header)
//   - src/core/FroggersEngine.hpp:772-809 (MixOscVoices) -- specifically
//     the `m_vcoAdsr && m_adsrParams` branch at :774-784, plus the plain
//     average return at :786-788.
//
// NOT ported (deliberately, v1 legacy per tasks.md 3.2):
//   - the `m_pairAr` fallback branch, FroggersEngine.hpp:789-808 (pair
//     attack/release smoothing + copysign recombination). This app's port
//     always has an ASR state (VcoAdsrState is unconditional here, unlike
//     the frozen engine's optional m_vcoAdsr pointer), so control permanently
//     takes the :774-784 branch and then the plain-average return at
//     :786-788 -- the exact code path the citation says to keep.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace synth_froggers::dsp {

// src/core/VcoAdsrState.hpp, verbatim.
struct VcoAdsrState
{
    static constexpr size_t kNumVoices = 3;
    static constexpr float kMinTimeSeconds = 0.0005f;
    // Operator, 2026-08-07: "the sustain minimum value is too low. i'm
    // concerned that audio rate modulation in some of the envelope parameters
    // would result in silence."
    //
    // This is the MISSING SIBLING of kMinTimeSeconds. Attack and release are
    // both floored (mapAttack/mapRelease below) precisely so modulation cannot
    // drive them to a degenerate zero; sustain -- the third ASR parameter, and
    // the only per-voice LEVEL control -- was read straight through as
    // `clamp(knob, 0, 1)` with no floor at all. Two of three had the guard,
    // one did not (OMNI §1: duplication is symmetric, go find the sibling).
    //
    // Degenerate at zero is not merely quiet, it is structurally broken:
    // `attackStep` is `sustainLevel / attackTime`, so at sustainLevel 0 the
    // step is 0 AND `m_level >= sustainLevel` is immediately true, dropping the
    // voice straight to Hold at silence with no attack at all. Worse, Attack's
    // own `m_level = min(sustainLevel, m_level + attackStep)` CLAMPS DOWN to a
    // falling sustain, so audio-rate modulation does not ride the envelope --
    // it hard-gates it to zero on every trough.
    //
    // 0.10 is about -20 dB: audible, unmistakably quiet, and far enough above
    // zero that a full-depth modulation trough still passes signal.
    // TRADE-OFF, operator-accepted: sustain 0 is no longer a hard mute for a
    // voice, it is this floor.
    static constexpr float kMinSustainLevel = 0.10f;
    // ITEM 4 (design.md A2, 2026-07-29, operator judgement -- deliberate
    // parity divergence, same treatment as Fuegoize.hpp's own D6 note):
    // lowered from 2.5s to 1.0s. 1.0s still comfortably covers a slow pad
    // swell; 2.5s was judged unnecessarily long.
    static constexpr float kMaxAttackSeconds = 1.0f;
    // Operator decision 2026-07-29, same deliberate parity divergence: lowered
    // from the frozen firmware's 10.0s to 5.0s. Note this does NOT by itself
    // make Stop responsive -- 5s of release after pressing Stop still reads as
    // broken, which is why FroggersAppCore's stop path forces a ~50ms fade
    // independent of this ceiling (see kStopFadeReleaseKnob there).
    //
    // B3 (tasks.md CONSOLIDATED PUSH table, 2026-08-05): operator, "sustain
    // and release maxima are simply too long." Traced before changing
    // anything (§0's "no fix before the recorded root cause"): `sustainKnob`
    // is clamped to a LEVEL in [0,1] by `stepVoice` below, not mapped through
    // any seconds-ceiling constant -- there is no `kMaxSustainSeconds` in
    // this file, and none is buildable from a knob that is a level, not a
    // time. "Sustain maximum too long" therefore cannot literally mean
    // sustain; it is release read through the perceptual lens of "how long
    // the note keeps ringing after I let go" (release governs exactly that,
    // Stage::Release above), so this is treated as a release complaint.
    // Halved as a starting point, ear-tuned by the operator like the 5.0s
    // value it replaces -- this number is not derived from anything.
    static constexpr float kMaxReleaseSeconds = 2.5f;
    // Strict-executor packet (Envelope slot 1/5/9, Decay). No design-doc
    // value exists for Decay's own time ceiling (tasks.md T1.1 only
    // specifies the peak/numerator fix, not a Decay time range) -- this is
    // an implementer judgment call, reported as such rather than silently
    // asserted. Mirrors kMaxAttackSeconds (1.0f) rather than
    // kMaxReleaseSeconds: Decay, like Attack, is a per-note transient stage
    // that normally completes WHILE the gate is still held, so
    // kMaxReleaseSeconds's own halving rationale ("note keeps ringing after
    // I let go") does not apply here.
    static constexpr float kMaxDecaySeconds = 1.0f;
    // Strict-executor packet (Envelope slot 13, Grace). Another
    // implementer judgment call (tasks.md T1.4: "needs its own design pass
    // at implementation time -- this proposal does not fully specify it").
    // 1.0f matches kMaxAttackSeconds/kMaxDecaySeconds's own scale (a
    // generous but bounded per-note floor, not an open-ended hang).
    static constexpr float kMaxGraceSeconds = 1.0f;

    enum class Stage : uint8_t
    {
        Idle = 0,
        Attack,
        Decay,
        Hold,
        Release,
    };

    void init(float sampleRate)
    {
        setSampleRate(sampleRate);
        m_gateHigh = false;
        for (size_t i = 0; i < kNumVoices; ++i)
        {
            m_stage[i] = Stage::Idle;
            m_level[i] = 0.0f;
            m_releasePending[i] = false;
            m_graceRemaining[i] = -1.0f;  // sentinel: grace countdown not started
        }
    }

    // C1 (openspec/changes/archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md
    // F8.1): this struct's only production caller is
    // FroggersAppCore::PrepareToPlay() (audioAdsr_.init(sampleRate_), which
    // calls this), and that method now validates the host's sample rate
    // ONCE before any downstream use (see its own §12 trace) -- so the
    // `44100.0f` re-guard this used to duplicate is unreachable. The bare
    // `m_sampleRate = 44100.0f` default below is a different concern (a
    // harmless pre-`init()` placeholder, not a re-guard of validated input)
    // and is left as is.
    void setSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate;
    }

    void setGate(bool high)
    {
        if (high == m_gateHigh)
        {
            return;
        }
        m_gateHigh = high;
        for (size_t i = 0; i < kNumVoices; ++i)
        {
            if (high)
            {
                // Hard retrigger, unchanged from before Grace existed: every
                // voice snaps to Attack regardless of its current stage/level
                // (m_level itself is untouched, so this does not click if a
                // voice was mid-release). Grace (slot 13) ITEM B: a retrigger
                // also cancels any deferred release from the previous gate-low
                // period outright -- chosen because a fresh note-on is a
                // stronger, more recent intent than an old pending release,
                // and leaving the old timer running could force an
                // unrelated, unexpected cutoff mid-way through the new note.
                m_stage[i] = Stage::Attack;
                m_releasePending[i] = false;
                m_graceRemaining[i] = -1.0f;
            }
            else
            {
                // Grace (slot 13) ITEM B: do NOT force Stage::Release here.
                // stepVoice() below resolves the deferred transition, because
                // only it has the live attack/decay/grace knob values needed
                // to decide (setGate() is never given them). Marking intent
                // here and resolving in stepVoice() keeps the resolution
                // synchronous with the very next stepVoice() call, so the
                // grace-inactive (default) case still transitions to Release
                // on the SAME sample setGate(false) was called on, exactly
                // like the old unconditional force did.
                m_releasePending[i] = true;
            }
        }
    }

    // True per-voice ASR. sustainKnob is not a time -- it is the target
    // level (0..1, clamped) that Attack/Decay ramp toward and Hold holds at.
    // curveKnob/graceKnob default to 0.0f (their exact neutral values -- see
    // mapGrace's own comment for why Grace has no floor) so existing callers
    // that only care about attack/decay/sustain/release keep compiling and
    // keep today's linear-shape, no-deferral behaviour unchanged.
    float apply(size_t voiceIndex,
                float input,
                float attackKnob,
                float decayKnob,
                float sustainKnob,
                float releaseKnob,
                float curveKnob = 0.0f,
                float graceKnob = 0.0f)
    {
        if (voiceIndex >= kNumVoices)
        {
            return input;
        }
        stepVoice(voiceIndex, attackKnob, decayKnob, sustainKnob, releaseKnob, curveKnob, graceKnob);
        return input * m_level[voiceIndex];
    }

    // ITEM 1 (app/FroggersAppCore.hpp's Stop-transport delay/reverb reset):
    // true once every voice has fully released and stopped producing
    // signal, i.e. no voice can still be re-exciting a downstream feedback
    // structure (delay/reverb). Const, read-only -- does not step or alter
    // any voice's stage/level.
    bool AllIdle() const
    {
        for (size_t i = 0; i < kNumVoices; ++i)
        {
            if (m_stage[i] != Stage::Idle)
            {
                return false;
            }
        }
        return true;
    }

    // The third of the three ASR range maps, and the one that was missing.
    // Same shape as mapAttack/mapRelease below: clamp the knob, then map onto
    // [floor, max] so the floor cannot be modulated through. See
    // kMinSustainLevel's own comment for why zero is degenerate here.
    //
    // PUBLIC and static, unlike its two private siblings, for one reason: the
    // parity tests assert the settled level a given sustain KNOB produces, and
    // they must read that from this one function rather than re-deriving
    // `kMinSustainLevel + knob * (1 - kMinSustainLevel)` themselves. A test
    // that retypes the formula is a second definition site of it, and would go
    // on passing against the old shape if this map were ever changed (e.g. to
    // an exponential curve). dsp::kMaxResonantBumpHeight's own comment records
    // this project already shipping exactly that bug: a test that computed its
    // expectation with its own hardcoded copy of the constant and therefore
    // passed unchanged when the real value moved.
    static constexpr float MapSustain(float knob)
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return kMinSustainLevel + clamped * (1.0f - kMinSustainLevel);
    }

private:
    float mapAttack(float knob) const
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return kMinTimeSeconds + clamped * (kMaxAttackSeconds - kMinTimeSeconds);
    }

    float mapRelease(float knob) const
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return kMinTimeSeconds + clamped * (kMaxReleaseSeconds - kMinTimeSeconds);
    }

    // Decay (Envelope slot 1/5/9). Same floored shape as mapAttack/mapRelease
    // -- see kMaxDecaySeconds's own comment for why that constant's value is
    // an implementer judgment call, not a recorded design number.
    float mapDecay(float knob) const
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return kMinTimeSeconds + clamped * (kMaxDecaySeconds - kMinTimeSeconds);
    }

    // Grace (Envelope slot 13). Deliberately NOT the mapAttack/mapRelease/
    // mapDecay shape: those floor at kMinTimeSeconds so a modulated knob can
    // never drive a ramp to a degenerate zero-length step. Grace is the
    // opposite: knob==0 (the registered default, FroggersParameters.hpp) must
    // map to EXACTLY 0.0f seconds, because the "Grace at default is a no-op"
    // requirement depends on this being bit-exact zero, not a small floor.
    float mapGrace(float knob) const
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return clamped * kMaxGraceSeconds;
    }

    // Curve (Envelope slot 12), applied to Attack/Decay/Release alike (all
    // three share this one idiom -- reused 3x, isolates the shape decision
    // from the stage-transition logic, and keeps that decision traceable in
    // one place: OMNI SS6's 2-of-4 helper test). curveAmount<=0.0f (the
    // registered default) takes the FIRST branch, which is the original
    // linear formula verbatim -- no curve arithmetic is even evaluated, so
    // the default is bit-identical to the pre-Curve ramp by construction,
    // not by a formula that merely happens to reduce to it.
    float ComputeRampStep(float from, float target, float stepMagnitude, float curveAmount) const
    {
        if (curveAmount <= 0.0f)
        {
            return (target >= from) ? std::min(target, from + stepMagnitude)
                                     : std::max(target, from - stepMagnitude);
        }
        const float remaining = target - from;
        const float absRemaining = std::fabs(remaining);
        if (absRemaining <= stepMagnitude)
        {
            return target;  // finishing step, same snap-to-target the linear path uses.
        }
        // Shape: blend the constant linear step with a one-pole step whose
        // rate is proportional to the remaining distance -- small steps
        // while far from target, growing toward stepMagnitude as the ramp
        // closes in (an ease-in, slow-start/fast-finish curve).
        // curveAmount in (0,1] scales how much of that shaping is mixed in.
        const float onePoleCoefficient = std::min(1.0f, stepMagnitude / absRemaining);
        const float linearNext = from + std::copysign(stepMagnitude, remaining);
        const float curvedNext = from + std::copysign(stepMagnitude * onePoleCoefficient, remaining);
        const float blended = linearNext + curveAmount * (curvedNext - linearNext);
        return (remaining >= 0.0f) ? std::min(target, blended) : std::max(target, blended);
    }

    void stepVoice(size_t voiceIndex,
                    float attackKnob,
                    float decayKnob,
                    float sustainKnob,
                    float releaseKnob,
                    float curveKnob,
                    float graceKnob)
    {
        const float sustainLevel = MapSustain(sustainKnob);
        // Decay (task A): Attack's ceiling is now an independent peak
        // (1.0f, the same ceiling Vco's own output already has -- no named
        // constant per the packet brief's explicit instruction) instead of
        // sustainLevel, so attackStep's NUMERATOR must be 1.0f too, or the
        // realized attack time silently becomes attackTime/sustainLevel (up
        // to 10x longer at kMinSustainLevel=0.10f).
        const float attackStep = 1.0f / std::max(mapAttack(attackKnob) * m_sampleRate, 1.0f);
        // Decay ramps DOWN from that same 1.0f peak to sustainLevel; its
        // numerator is that range, following attackStep/releaseStep's own
        // divide-by-mapped-time idiom.
        const float decayStep = (1.0f - sustainLevel) / std::max(mapDecay(decayKnob) * m_sampleRate, 1.0f);
        const float releaseStep = 1.0f / std::max(mapRelease(releaseKnob) * m_sampleRate, 1.0f);
        const float curveAmount = std::min(std::max(curveKnob, 0.0f), 1.0f);

        // Grace (task B): resolve any deferred gate-false transition BEFORE
        // the stage switch below, so the switch this same call already sees
        // the resolved stage -- this is what keeps the grace-inactive
        // (default) case landing on the SAME sample as the old eager
        // setGate(false) force (see setGate()'s own comment).
        if (m_releasePending[voiceIndex])
        {
            const float graceSeconds = mapGrace(graceKnob);
            const bool graceInactive = graceSeconds <= 0.0f;
            const Stage stage = m_stage[voiceIndex];
            if (graceInactive && (stage == Stage::Attack || stage == Stage::Decay || stage == Stage::Hold))
            {
                // Neutral default: identical to the old unconditional force
                // -- forcibly cut short whatever stage this voice was in.
                m_stage[voiceIndex] = Stage::Release;
                m_releasePending[voiceIndex] = false;
            }
            else if (!graceInactive && stage == Stage::Hold)
            {
                // Minimum-hold countdown, started the first sample Hold is
                // reached with a release still pending (Attack/Decay are
                // left alone above, so they always run to completion first
                // -- "at minimum Attack (and Decay) has completed").
                if (m_graceRemaining[voiceIndex] < 0.0f)
                {
                    m_graceRemaining[voiceIndex] = graceSeconds * m_sampleRate;
                }
                if (m_graceRemaining[voiceIndex] <= 0.0f)
                {
                    m_stage[voiceIndex] = Stage::Release;
                    m_releasePending[voiceIndex] = false;
                    m_graceRemaining[voiceIndex] = -1.0f;
                }
                else
                {
                    m_graceRemaining[voiceIndex] -= 1.0f;
                }
            }
            // else: grace active and still in Attack/Decay -- keep
            // progressing toward Hold untouched; Release/Idle -- pending is
            // stale (already resolved), harmless, cleared on the next
            // setGate() edge.
        }

        switch (m_stage[voiceIndex])
        {
            case Stage::Idle:
                m_level[voiceIndex] = 0.0f;
                break;
            case Stage::Attack:
                m_level[voiceIndex] = ComputeRampStep(m_level[voiceIndex], 1.0f, attackStep, curveAmount);
                if (m_level[voiceIndex] >= 1.0f)
                {
                    m_stage[voiceIndex] = Stage::Decay;
                }
                break;
            case Stage::Decay:
                m_level[voiceIndex] = ComputeRampStep(m_level[voiceIndex], sustainLevel, decayStep, curveAmount);
                if (m_level[voiceIndex] <= sustainLevel)
                {
                    m_stage[voiceIndex] = Stage::Hold;
                }
                break;
            case Stage::Hold:
                // Gate-high hold and gate-low-but-still-deferred hold are the
                // same visible state (sustainLevel); the grace resolution
                // above already decided whether/when this stage moves on.
                m_level[voiceIndex] = sustainLevel;
                break;
            case Stage::Release:
                m_level[voiceIndex] = ComputeRampStep(m_level[voiceIndex], 0.0f, releaseStep, curveAmount);
                if (m_level[voiceIndex] <= 0.0f)
                {
                    m_stage[voiceIndex] = Stage::Idle;
                }
                break;
        }
    }

    float m_sampleRate = 44100.0f;
    bool m_gateHigh = false;
    std::array<float, kNumVoices> m_level{};
    std::array<Stage, kNumVoices> m_stage{};
    // Grace (task B) per-voice state. m_releasePending: true from a
    // gate-false edge until stepVoice() actually forces Stage::Release (see
    // setGate()/stepVoice()'s own comments -- this is the only new
    // information setGate() itself records, since it lacks the knob values
    // needed to decide). m_graceRemaining: minimum-hold countdown in
    // samples, -1.0f sentinel meaning "not started yet"; only used while a
    // release is pending AND the voice is in Hold.
    std::array<bool, kNumVoices> m_releasePending{};
    std::array<float, kNumVoices> m_graceRemaining{};
};

// UI-rework ITEM 3 (design.md A3d, tasks.md B.3, 2026-07-29): the three
// GATED (post-`adsr.apply`) per-voice values, for a caller that needs them
// for something other than the mixed average below -- specifically,
// FroggersAppCore::RouteAudioSample()'s post-gate scope tap (see this
// struct's own header comment and Vco.hpp's `Process()` comment for why the
// pre-gate write it used to do there was wrong). A plain struct, not
// `std::array<float, 3>`, so call sites read `.v1`/`.v2`/`.v3` instead of
// index magic numbers that would have to match VcoAdsrState's own voice
// index convention (0/1/2) by coincidence rather than by name.
struct GatedVoices
{
    float v1 = 0.0f;
    float v2 = 0.0f;
    float v3 = 0.0f;
};

// Task C (VCO balance, Audio slot 13): a normalized 3-point crossfade that
// tilts emphasis VCO1 -> VCO2 -> VCO3 as knob01 sweeps 0 -> 1, replacing
// MixOscVoices's old hardcoded equal-thirds average. BINDING INVARIANT
// (operator ruling): the three weights sum to exactly 1 and each stays
// within [0.10, 0.80] at EVERY knob01 -- guaranteed BY CONSTRUCTION, not
// checked after the fact:
//   - Below knob01 0.5, the result is a convex combination ((1-u), u for
//     u in [0,1]) of two anchor triples: (0.80, 0.10, 0.10) at knob01==0
//     and (1/3, 1/3, 1/3) at knob01==0.5.
//   - Above knob01 0.5, it is a convex combination of (1/3, 1/3, 1/3) at
//     knob01==0.5 and (0.10, 0.10, 0.80) at knob01==1.
// Both anchor triples in each half sum to exactly 1, and a convex
// combination of two vectors that each sum to 1 always sums to 1 (the
// weights (1-u)+u==1 factor straight through the sum). Both anchor triples
// in each half also lie within [0.10, 0.80] component-wise, and a convex
// combination of two points inside an interval never leaves that interval
// -- so every weight is bounded the same way, for the same reason,
// regardless of knob01. At knob01==0.5 (centre default) every weight is
// exactly kThird, i.e. exactly the equal-thirds mix this replaces (same
// constant MixOscVoices's old `(v1+v2+v3)*(1.0f/3.0f)` used).
inline void ComputeVcoBalanceWeights(float knob01, float& w1, float& w2, float& w3)
{
    constexpr float kFloor = 0.10f;
    constexpr float kCap = 0.80f;
    constexpr float kThird = 1.0f / 3.0f;
    const float knob = std::min(std::max(knob01, 0.0f), 1.0f);
    if (knob <= 0.5f)
    {
        const float u = knob / 0.5f;  // 0 at knob01==0, 1 at knob01==0.5.
        w1 = kCap * (1.0f - u) + kThird * u;
        w2 = kFloor * (1.0f - u) + kThird * u;
        w3 = kFloor * (1.0f - u) + kThird * u;
    }
    else
    {
        const float v = (knob - 0.5f) / 0.5f;  // 0 at knob01==0.5, 1 at knob01==1.
        w1 = kThird * (1.0f - v) + kFloor * v;
        w2 = kThird * (1.0f - v) + kFloor * v;
        w3 = kThird * (1.0f - v) + kCap * v;
    }
}

// FroggersEngine.hpp:772-809 (MixOscVoices), the m_vcoAdsr && m_adsrParams
// branch (:774-784) applied unconditionally -- this app always has an ASR
// state, so the frozen engine's `if (m_vcoAdsr && m_adsrParams)` guard has
// no off-state here -- followed by the plain average return at :786-788.
// adsr[v] rows are (attack, sustain, release) per voice, matching
// V2EngineSetup::configureAdsrPage's per-VCO triplet row order.
//
// `balanceKnob01` (task C, Audio slot 13): drives ComputeVcoBalanceWeights
// above, replacing the old hardcoded equal-thirds average. Defaults to 0.5f
// (this mapping's own centre, which reproduces the exact 1/3-each weights
// the old average used) so callers that pass neither this nor `outGated`
// keep their pre-packet mixed-average behaviour unchanged.
//
// `outGated` (UI-rework ITEM 3, design.md A3d, tasks.md B.3, 2026-07-29):
// optional out-parameter carrying the three post-gate voice values, added
// so FroggersAppCore::RouteAudioSample() (this function's single
// production call site) can tap the GATED signal for the scope instead of
// the pre-gate raw VCO output Vco::Process() used to write directly (OMNI
// §8: extending this existing single call site rather than re-applying
// `adsr.apply` a second time at a new call site, which would duplicate a
// ported formula and create two sites that could drift). Defaults to
// `nullptr` so the two existing test call sites
// (FroggersCrunchyBlowupRepro.cpp, FroggersDspParityTests.cpp) that
// only need the mixed average keep compiling unchanged. `outGated` still
// carries the raw per-voice GATED values (unweighted) -- balanceKnob01
// only changes how the three are combined into the single mixed return.
// `decay1/2/3` (Envelope slot 1/5/9): per-voice Decay knobs, added alongside
// attack/sustain/release. `curveKnob`/`graceKnob` (Envelope slot 12/13) are
// SHARED across all three voices (matching the spec delta's "applying to all
// three voices'" / single-knob-per-bank framing for these two, unlike the
// per-VCO attack/decay/sustain/release quads) and default to 0.0f -- their
// exact neutral values -- so callers that only care about decay keep
// compiling and keep today's shape/no-deferral behaviour.
inline float MixOscVoices(VcoAdsrState& adsr,
                           float v1,
                           float v2,
                           float v3,
                           float attack1,
                           float decay1,
                           float sustain1,
                           float release1,
                           float attack2,
                           float decay2,
                           float sustain2,
                           float release2,
                           float attack3,
                           float decay3,
                           float sustain3,
                           float release3,
                           float curveKnob = 0.0f,
                           float graceKnob = 0.0f,
                           float balanceKnob01 = 0.5f,
                           GatedVoices* outGated = nullptr)
{
    v1 = adsr.apply(0, v1, attack1, decay1, sustain1, release1, curveKnob, graceKnob);
    v2 = adsr.apply(1, v2, attack2, decay2, sustain2, release2, curveKnob, graceKnob);
    v3 = adsr.apply(2, v3, attack3, decay3, sustain3, release3, curveKnob, graceKnob);
    if (outGated != nullptr)
    {
        outGated->v1 = v1;
        outGated->v2 = v2;
        outGated->v3 = v3;
    }
    float w1 = 0.0f;
    float w2 = 0.0f;
    float w3 = 0.0f;
    ComputeVcoBalanceWeights(balanceKnob01, w1, w2, w3);
    return w1 * v1 + w2 * v2 + w3 * v3;
}

}  // namespace synth_froggers::dsp

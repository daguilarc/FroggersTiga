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
    // 0.05 is about -26 dB: audible, unmistakably quiet, and far enough above
    // zero that a full-depth modulation trough still passes signal.
    // TRADE-OFF, operator-accepted: sustain 0 is no longer a hard mute for a
    // voice, it is this floor.
    static constexpr float kMinSustainLevel = 0.05f;
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

    enum class Stage : uint8_t
    {
        Idle = 0,
        Attack,
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
        }
    }

    // C1 (openspec/changes/frogg3rs-blowout-and-drilldown-repair/tasks.md
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
            m_stage[i] = high ? Stage::Attack : Stage::Release;
        }
    }

    // True per-voice ASR. sustainKnob is not a time -- it is the target
    // level (0..1, clamped) that Attack ramps toward and Hold holds at.
    float apply(size_t voiceIndex, float input, float attackKnob, float sustainKnob, float releaseKnob)
    {
        if (voiceIndex >= kNumVoices)
        {
            return input;
        }
        stepVoice(voiceIndex, attackKnob, sustainKnob, releaseKnob);
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

    void stepVoice(size_t voiceIndex, float attackKnob, float sustainKnob, float releaseKnob)
    {
        const float sustainLevel = MapSustain(sustainKnob);
        const float attackStep = sustainLevel / std::max(mapAttack(attackKnob) * m_sampleRate, 1.0f);
        const float releaseStep = 1.0f / std::max(mapRelease(releaseKnob) * m_sampleRate, 1.0f);

        switch (m_stage[voiceIndex])
        {
            case Stage::Idle:
                m_level[voiceIndex] = 0.0f;
                break;
            case Stage::Attack:
                m_level[voiceIndex] = std::min(sustainLevel, m_level[voiceIndex] + attackStep);
                if (m_level[voiceIndex] >= sustainLevel)
                {
                    m_stage[voiceIndex] = Stage::Hold;
                }
                break;
            case Stage::Hold:
                if (m_gateHigh)
                {
                    m_level[voiceIndex] = sustainLevel;
                }
                else
                {
                    m_stage[voiceIndex] = Stage::Release;
                }
                break;
            case Stage::Release:
                m_level[voiceIndex] = std::max(0.0f, m_level[voiceIndex] - releaseStep);
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

// FroggersEngine.hpp:772-809 (MixOscVoices), the m_vcoAdsr && m_adsrParams
// branch (:774-784) applied unconditionally -- this app always has an ASR
// state, so the frozen engine's `if (m_vcoAdsr && m_adsrParams)` guard has
// no off-state here -- followed by the plain average return at :786-788.
// adsr[v] rows are (attack, sustain, release) per voice, matching
// V2EngineSetup::configureAdsrPage's per-VCO triplet row order.
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
// only need the mixed average keep compiling unchanged.
inline float MixOscVoices(VcoAdsrState& adsr,
                           float v1,
                           float v2,
                           float v3,
                           float attack1,
                           float sustain1,
                           float release1,
                           float attack2,
                           float sustain2,
                           float release2,
                           float attack3,
                           float sustain3,
                           float release3,
                           GatedVoices* outGated = nullptr)
{
    v1 = adsr.apply(0, v1, attack1, sustain1, release1);
    v2 = adsr.apply(1, v2, attack2, sustain2, release2);
    v3 = adsr.apply(2, v3, attack3, sustain3, release3);
    if (outGated != nullptr)
    {
        outGated->v1 = v1;
        outGated->v2 = v2;
        outGated->v3 = v3;
    }
    return (v1 + v2 + v3) * (1.0f / 3.0f);
}

}  // namespace synth_froggers::dsp

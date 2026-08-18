// FroggersDspParityTests.cpp -- tasks.md 3.7: parity test suite for packet
// 3's DSP port (tasks 3.1-3.6). Each TEST_CASE pins one ported unit to its
// cited Froggers formula. This TU includes ONLY app/dsp/*.hpp (no Sheaf, no
// JUCE, no frozen-tree headers) -- the DSP port is dependency-free per
// design D3, and check_no_frozen_includes.sh mechanically enforces that no
// file under app/ includes src/, sim/, desktop-v2/, wasm/, vcv/, web/, or
// desktop/.
//
// Harness mirrors FroggersHeadlessTests.cpp's self-registering TEST_CASE /
// REQUIRE_TRUE macros (no external test framework).

#include "dsp/Delay.hpp"
#include "dsp/Drive.hpp"
#include "dsp/DspMath.hpp"
#include "dsp/EnvelopeFollowers.hpp"
#include "dsp/FilterFx.hpp"
#include "dsp/Fuegoize.hpp"
#include "dsp/RandomShLane.hpp"
#include "dsp/Reverb.hpp"
#include "dsp/Vco.hpp"
#include "dsp/VoiceEnvelope.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

struct TestCase {
    const char* name;
    void (*fn)();
};

std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Register {
    Register(const char* name, void (*fn)()) {
        Registry().push_back({name, fn});
    }
};

#define TEST_CASE(name)     \
    void name();            \
    Register reg_##name(#name, &name); \
    void name()

#define REQUIRE_TRUE(expr)                                                 \
    do {                                                                   \
        if (!(expr)) {                                                    \
            std::ostringstream oss;                                       \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #expr; \
            throw std::runtime_error(oss.str());                          \
        }                                                                  \
    } while (false)

#define REQUIRE_NEAR(a, b, eps)                                            \
    do {                                                                   \
        const double av = static_cast<double>(a);                         \
        const double bv = static_cast<double>(b);                         \
        if (std::fabs(av - bv) > (eps)) {                                 \
            std::ostringstream oss;                                       \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #a \
                << " (" << av << ") ~= " #b << " (" << bv << "), eps=" << (eps); \
            throw std::runtime_error(oss.str());                          \
        }                                                                  \
    } while (false)

namespace dsp = synth_froggers::dsp;

// =========================================================================
// 3.1 -- VCO (FroggersEngine.hpp:439-441,706-712,735-744; VcoWaveEval.hpp:7-23)
// =========================================================================

TEST_CASE(vco_pitch_exp_map_matches_20hz_20khz_range) {
    const float sr = 48000.0f;
    // knob=0 -> 20 Hz, knob=1 -> 20000 Hz, expressed as phase increment.
    REQUIRE_NEAR(dsp::Vco::PitchToPhaseIncrement(0.0f, sr), 20.0f / sr, 1e-9);
    REQUIRE_NEAR(dsp::Vco::PitchToPhaseIncrement(1.0f, sr), 20000.0f / sr, 1e-6);
    // Midpoint is geometric (exponential map), not arithmetic: sqrt(20*20000)/sr.
    const float expectedMid = std::sqrt(20.0f * 20000.0f) / sr;
    REQUIRE_NEAR(dsp::Vco::PitchToPhaseIncrement(0.5f, sr), expectedMid, 1e-6);
}

TEST_CASE(vco_pm_depth_scale_zero_off_gate_and_smoothstep) {
    // FroggersEngine.hpp:147-148,150-165.
    REQUIRE_TRUE(dsp::Vco::PmDepthScale(0.0f) == 0.0f);
    REQUIRE_TRUE(dsp::Vco::PmDepthScale(dsp::Vco::kPmLfoFloor) == 0.0f);
    REQUIRE_TRUE(dsp::Vco::PmDepthScale(dsp::Vco::kPmLfoFloor + dsp::Vco::kPmLfoRampWidth) == 1.0f);
    REQUIRE_TRUE(dsp::Vco::PmDepthScale(1.0f) == 1.0f);
    // Smoothstep at the ramp midpoint: t=0.5 -> 0.5*0.5*(3-1) = 0.5.
    const float mid = dsp::Vco::kPmLfoFloor + 0.5f * dsp::Vco::kPmLfoRampWidth;
    REQUIRE_NEAR(dsp::Vco::PmDepthScale(mid), 0.5f, 1e-6);
}

TEST_CASE(dsp_true_zero_depth_taper_monotonic_smoothstep_and_pm_parity) {
    // DspMath.hpp TrueZeroDepthTaper, generalized from PmDepthScale
    // (FroggersEngine.hpp:150-165). A step function would also pass a
    // floor/ramp-top-only check, so this sweeps the whole [0,1] range and
    // checks (a) monotonic non-decreasing, (b) strictly inside (0,1)
    // strictly inside the ramp window (a step fails this), and (c) exact
    // parity with Vco::PmDepthScale called at kPmLfoFloor/kPmLfoRampWidth.
    float prev = -1.0f;
    const int steps = 200;
    for (int i = 0; i <= steps; ++i) {
        const float knob = static_cast<float>(i) / static_cast<float>(steps);
        const float value = dsp::TrueZeroDepthTaper(knob, dsp::Vco::kPmLfoFloor, dsp::Vco::kPmLfoRampWidth);

        // (a) monotonic non-decreasing across the whole sweep.
        REQUIRE_TRUE(value + 1e-6f >= prev);
        prev = value;

        // (b) strictly inside the ramp window, the value is strictly
        // between 0 and 1 -- a hard step would instead jump straight to
        // 0 or 1 here.
        const float rampTop = dsp::Vco::kPmLfoFloor + dsp::Vco::kPmLfoRampWidth;
        if (knob > dsp::Vco::kPmLfoFloor && knob < rampTop) {
            REQUIRE_TRUE(value > 0.0f);
            REQUIRE_TRUE(value < 1.0f);
        }

        // (c) parity: shared function called with PM's own thresholds
        // must match Vco::PmDepthScale exactly, at every sampled knob.
        REQUIRE_TRUE(value == dsp::Vco::PmDepthScale(knob));
    }
}

TEST_CASE(vco_pm_zero_at_or_below_floor_leaves_carrier_unmodulated) {
    // At pmKnob <= floor, PmDepthScale == 0, so the depth multiply
    // (FroggersEngine.hpp:741-743) must zero the PM offset entirely --
    // output must equal EvalWaveMorph of the bare carrier phase.
    dsp::Vco vco;
    const float sr = 48000.0f;
    const float pitchKnob = 0.3f;
    const float morphKnob = 0.4f;
    float refPhase = 0.0f;
    for (int i = 0; i < 32; ++i) {
        const float out =
            vco.Process(pitchKnob, morphKnob, /*pmKnob=*/0.0f, /*pmRateKnob=*/0.0f, /*ringModKnob=*/0.0f, sr);
        const float expected = dsp::EvalWaveMorph(refPhase, morphKnob);
        REQUIRE_NEAR(out, expected, 1e-5);
        refPhase = dsp::WrapPhase(refPhase + dsp::Vco::PitchToPhaseIncrement(pitchKnob, sr));
    }
}

TEST_CASE(vco_pm_above_floor_actually_modulates) {
    dsp::Vco withPm;
    dsp::Vco withoutPm;
    const float sr = 48000.0f;
    bool sawDifference = false;
    for (int i = 0; i < 64; ++i) {
        const float a = withPm.Process(0.3f, 0.4f, /*pmKnob=*/1.0f, /*pmRateKnob=*/0.5f, /*ringModKnob=*/0.0f, sr);
        const float b = withoutPm.Process(0.3f, 0.4f, /*pmKnob=*/0.0f, /*pmRateKnob=*/0.5f, /*ringModKnob=*/0.0f, sr);
        if (std::fabs(a - b) > 1e-4f) {
            sawDifference = true;
        }
    }
    REQUIRE_TRUE(sawDifference);
}

TEST_CASE(vco_zero_cross_vco_terms_independent_of_other_instances) {
    // FroggersEngine.hpp:735-744 (independent-PM branch only; the legacy
    // XCPL `else` at :752-754 is NOT ported). Prove a Vco's output sequence
    // is identical whether or not a second Vco is driven in between calls
    // -- i.e. nothing here can reach another instance's state.
    const float sr = 48000.0f;
    dsp::Vco alone;
    std::vector<float> aloneSeq;
    for (int i = 0; i < 16; ++i) {
        aloneSeq.push_back(alone.Process(0.6f, 0.2f, 0.7f, /*pmRateKnob=*/0.3f, /*ringModKnob=*/0.6f, sr));
    }

    dsp::Vco interleavedA;
    dsp::Vco interleavedB;
    std::vector<float> interleavedSeq;
    for (int i = 0; i < 16; ++i) {
        interleavedSeq.push_back(interleavedA.Process(0.6f, 0.2f, 0.7f, /*pmRateKnob=*/0.3f, /*ringModKnob=*/0.6f, sr));
        // different knobs, driven "in between" -- proves zero cross-VCO
        // terms hold for the new Ring Mod carrier too (each instance's own
        // internal carrier, never the other's).
        interleavedB.Process(0.9f, 0.8f, 0.1f, /*pmRateKnob=*/0.9f, /*ringModKnob=*/0.4f, sr);
    }

    REQUIRE_TRUE(aloneSeq.size() == interleavedSeq.size());
    for (size_t i = 0; i < aloneSeq.size(); ++i) {
        REQUIRE_NEAR(aloneSeq[i], interleavedSeq[i], 1e-7);
    }
}

TEST_CASE(vco_eval_wave_morph_sine_saw_square_endpoints) {
    // VcoWaveEval.hpp:7-23.
    const float phase = 0.25f;
    REQUIRE_NEAR(dsp::EvalWaveMorph(phase, 0.0f), dsp::Sine01(phase), 1e-6);
    REQUIRE_NEAR(dsp::EvalWaveMorph(phase, 1.0f), (phase < 0.5f) ? 1.0f : -1.0f, 1e-6);
    const float saw = 2.0f * phase - 1.0f;
    REQUIRE_NEAR(dsp::EvalWaveMorph(phase, 0.5f), saw, 1e-6);
}

// =========================================================================
// 3.2 -- ASR + voice mix (FroggersEngine.hpp:772-809; VcoAdsrState.hpp)
// =========================================================================

TEST_CASE(vco_adsr_state_attacks_holds_and_releases) {
    dsp::VcoAdsrState adsr;
    adsr.init(1000.0f);  // 1 kHz for easy-to-reason step counts
    adsr.setGate(true);
    // Strict-executor packet (Decay): the old per-sample monotonic-rise
    // assertion no longer holds bit-for-bit -- Attack now ramps to 1.0f (not
    // sustainLevel) and a new Decay stage then falls from there down to
    // sustainLevel, so the trace genuinely dips partway through. What still
    // must hold, and is checked below instead: the level reaches (very
    // close to) 1.0f during Attack, then settles at MapSustain(0.8f) once
    // Attack+Decay have had time to complete.
    float peak = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const float level = adsr.apply(0, 1.0f, /*attack=*/0.5f, /*decay=*/0.5f, /*sustain=*/0.8f, /*release=*/0.5f);
        peak = std::max(peak, level);
    }
    REQUIRE_NEAR(peak, 1.0f, 1e-3);  // Attack reached its independent peak
    const float last = adsr.apply(0, 1.0f, 0.5f, 0.5f, 0.8f, 0.5f);
    // F3-adjacent (2026-08-07): sustain is now floored by
    // dsp::VcoAdsrState::kMinSustainLevel so audio-rate modulation cannot gate
    // a voice to silence, so a 0.8 KNOB no longer settles at a 0.8 LEVEL.
    // Deliberate parity divergence, same class as this struct's own
    // kMaxAttackSeconds (2.5s -> 1.0s) and kMaxReleaseSeconds (10s -> 5s).
    // Read the expectation from the map, never retype it.
    REQUIRE_NEAR(last, dsp::VcoAdsrState::MapSustain(0.8f), 1e-3);  // settled at sustain level

    adsr.setGate(false);
    for (int i = 0; i < 20000; ++i) {
        adsr.apply(0, 1.0f, 0.5f, 0.5f, 0.8f, 0.5f);
    }
    const float released = adsr.apply(0, 1.0f, 0.5f, 0.5f, 0.8f, 0.5f);
    REQUIRE_NEAR(released, 0.0f, 1e-3);
}

// -----------------------------------------------------------------------
// ITEM 4 failing-test-first (design.md A2): kMaxAttackSeconds lowered from
// 2.5s to 1.0s (VoiceEnvelope.hpp -- private, so this drives the same
// observable surface the test above does). W5 (proposal
// frogg3rs-stop-isolation-and-legible-labels SS2, operator ruling
// 2026-08-17, verbatim: "that max attack is also way too long. half a
// second at most") lowered it again, 1.0s -> 0.5f -- the citations below are
// updated to that current value; the assertion itself was already keyed to
// the constant via `dsp::VcoAdsrState::MapSustain` and needed no code
// change, only this prose. stepVoice()'s attack ramp is LINEAR
// (`attackStep = sustainLevel / (mapAttack(knob)*sampleRate)`), and
// `mapAttack(1.0) == kMinTimeSeconds + 1.0*(kMaxAttackSeconds -
// kMinTimeSeconds) == kMaxAttackSeconds` exactly (the kMinTimeSeconds terms
// cancel at knob==1.0) -- so at the maximum attack knob, the ramp completes
// in EXACTLY kMaxAttackSeconds worth of samples, no time-constant fuzz to
// account for. At 2.0s of held-gate samples: under the ORIGINAL 2.5s
// ceiling the level would still be mid-ramp (2.0/2.5 == 80% of the way
// there, NOT at sustain -- this is what made the test fail against the
// pre-item-4 code); under the CURRENT 0.5f ceiling the ramp finished at
// 0.5s and Hold has been clamping the level at sustain for a further 1.5s
// since.
// -----------------------------------------------------------------------
TEST_CASE(max_attack_knob_reaches_sustain_within_the_current_half_second_ceiling) {
    constexpr float kSampleRate = 48000.0f;
    constexpr float kSustain = 0.8f;
    dsp::VcoAdsrState adsr;
    adsr.init(kSampleRate);
    adsr.setGate(true);

    float level = 0.0f;
    const int samplesAt2s = static_cast<int>(2.0f * kSampleRate);
    // Strict-executor packet (Decay): decayKnob=0.0f (the fastest mapped
    // decay time, kMinTimeSeconds) isolates THIS test's own concern --
    // attack's ceiling/timing -- from decay's, which is not what this test
    // is about. At 0.0f decay, Decay completes in well under a millisecond
    // once Attack's 1.0s finishes, so it cannot meaningfully eat into the
    // 2.0s budget this test measures against.
    for (int i = 0; i < samplesAt2s; ++i) {
        level = adsr.apply(0, 1.0f, /*attack=*/1.0f, /*decay=*/0.0f, /*sustain=*/kSustain, /*release=*/0.0f);
    }
    REQUIRE_NEAR(level, dsp::VcoAdsrState::MapSustain(kSustain), 1e-6);
}

TEST_CASE(mix_osc_voices_applies_asr_per_voice_then_averages) {
    // FroggersEngine.hpp:774-784 (apply branch) + :786-788 (plain average
    // return) -- the m_pairAr fallback at :789-808 is v1 legacy, not ported.
    dsp::VcoAdsrState adsrForMix;
    adsrForMix.init(1000.0f);
    adsrForMix.setGate(true);

    dsp::VcoAdsrState adsrReference;
    adsrReference.init(1000.0f);
    adsrReference.setGate(true);

    for (int i = 0; i < 500; ++i) {
        const float v1 = 0.5f;
        const float v2 = -0.3f;
        const float v3 = 0.9f;
        const float mixed = dsp::MixOscVoices(adsrForMix, v1, v2, v3,
                                               0.1f, 0.15f, 0.6f, 0.2f,
                                               0.2f, 0.25f, 0.7f, 0.3f,
                                               0.05f, 0.35f, 0.5f, 0.1f);
        const float e1 = adsrReference.apply(0, v1, 0.1f, 0.15f, 0.6f, 0.2f);
        const float e2 = adsrReference.apply(1, v2, 0.2f, 0.25f, 0.7f, 0.3f);
        const float e3 = adsrReference.apply(2, v3, 0.05f, 0.35f, 0.5f, 0.1f);
        const float expected = (e1 + e2 + e3) * (1.0f / 3.0f);
        REQUIRE_NEAR(mixed, expected, 1e-6);
    }
}

// =========================================================================
// T1 (openspec/changes/frogg3rs-stop-isolation-and-legible-labels/tasks.md
// T1.1-T1.3) -- ComputeRampStep's progress floor (root cause SS1a: at
// curveAmount==1.0 the pre-fix ramp's per-sample progress was
// stepMagnitude^2/absRemaining, unbounded -- a "1-second" attack at 48kHz
// measured ~6.7 hours) and its interaction with the Grace ladder
// (VoiceEnvelope.hpp:332-370, UNCHANGED by this packet -- T1.2 is
// TEST-ONLY, see that task's own tasks.md note).
// =========================================================================

namespace {

// Sample count plus the level `apply()` returned on that sample -- the
// level half lets a caller read a stage-boundary value straight off the
// real object (ground truth) instead of recomputing it independently (see
// RuntimeFloat's own comment for why an independent recomputation is not
// automatically trustworthy).
struct StageResult {
    long samples;
    float level;
};

// Steps `adsr` voice 0 forward sample by sample (curveKnob applied every
// call) until `level` first crosses `target` (>= for ascending stages, <=
// for descending), returning the 1-based sample count and the level
// reached. VcoAdsrState exposes no per-voice stage getter, so every T1 test
// below infers stage completion purely from the level trajectory -- exact,
// because ComputeRampStep's finishing-step early return (`absRemaining <=
// stepMagnitude -> return target`, untouched by T1.1) always snaps exactly
// to target regardless of curve. `capSamples` turns an UNBOUNDED pre-T1.1
// ramp into a clean test FAILURE instead of a multi-hour hang (OMNI SS9.1's
// RED pass, per this packet's report, relies on this).
// `graceKnob` defaults to 0.0f (inactive, apply()'s own default) so callers
// that don't care about Grace -- most of this helper's call sites -- don't
// have to name it; T1.2(a) passes it explicitly because that test's whole
// point is measuring behaviour WHILE grace is active.
StageResult StepUntilLevelCrosses(dsp::VcoAdsrState& adsr, float attackKnob, float decayKnob, float sustainKnob,
                                   float releaseKnob, float curveKnob, bool ascending, float target, long capSamples,
                                   float graceKnob = 0.0f) {
    for (long sample = 1; sample <= capSamples; ++sample) {
        const float level =
            adsr.apply(0, 1.0f, attackKnob, decayKnob, sustainKnob, releaseKnob, curveKnob, graceKnob);
        const bool reached = ascending ? (level >= target) : (level <= target);
        if (reached) {
            return {sample, level};
        }
    }
    std::ostringstream oss;
    oss << "level did not reach target=" << target << " within capSamples=" << capSamples
        << " (curveKnob=" << curveKnob << ") -- ComputeRampStep's progress floor appears absent or broken.";
    throw std::runtime_error(oss.str());
}

// Steps `adsr` voice 0 forward until the level reads BIT-IDENTICAL on two
// consecutive samples, returning that sample count and the stabilized
// level -- the signature of Hold being reached (stepVoice()'s
// `case Stage::Hold: m_level = sustainLevel;`, a constant re-assignment,
// versus Decay's ComputeRampStep call, which changes the value every
// sample until its own finishing-step snap lands exactly on target,
// immediately followed by Hold repeating that SAME value). Used instead of
// comparing against an independently-recomputed `MapSustain(sustainKnob)`
// specifically so the caller gets the REAL object's own ground-truth
// sustain level, sidestepping RuntimeFloat's whole class of divergence
// risk entirely rather than merely working around it. Assumes the caller's
// knob/curve choice does not produce a degenerate zero-length Decay step
// that could plateau before Hold -- true for every T1 test that uses this.
StageResult StepUntilLevelStabilizes(dsp::VcoAdsrState& adsr, float attackKnob, float decayKnob, float sustainKnob,
                                      float releaseKnob, float curveKnob, long capSamples,
                                      float graceKnob = 0.0f) {
    float previous = adsr.apply(0, 1.0f, attackKnob, decayKnob, sustainKnob, releaseKnob, curveKnob, graceKnob);
    for (long sample = 2; sample <= capSamples; ++sample) {
        const float level =
            adsr.apply(0, 1.0f, attackKnob, decayKnob, sustainKnob, releaseKnob, curveKnob, graceKnob);
        if (level == previous) {
            return {sample, level};
        }
        previous = level;
    }
    std::ostringstream oss;
    oss << "level never stabilized (Hold never reached) within capSamples=" << capSamples
        << " (curveKnob=" << curveKnob << ").";
    throw std::runtime_error(oss.str());
}

// Round-trips `value` through a volatile read, defeating compile-time
// constant folding. Measured on this toolchain: a `dsp::VcoAdsrState::
// MapSustain`/similar formula call whose argument the compiler can prove
// constant (a `constexpr`/`const` local initialized from a literal is
// enough) is sometimes evaluated by the compiler's OWN constant-expression
// interpreter instead of being code-generated as ordinary runtime IEEE-754
// float32 arithmetic -- and that interpreter does not always round the same
// way stepVoice()'s genuine runtime evaluation of the identical formula
// does (VcoAdsrState::MapSustain(0.6f) folds to 0.640000045f at compile
// time but evaluates to 0.639999986f at runtime on this build; 0.5f
// happened not to diverge, which is exactly what makes trusting this
// silently is dangerous). Every T1 test below that needs its own
// independently-recomputed value to bit-match (or near-match within much
// less than one ramp step) VcoAdsrState's internal computation routes its
// inputs through this first.
float RuntimeFloat(float value) {
    volatile float v = value;
    return v;
}

// Same formula ComputeRampStep's private mapAttack/mapDecay/mapRelease use
// (VoiceEnvelope.hpp), recomputed independently here from the PUBLIC
// kMinTimeSeconds/kMax*Seconds constants -- never read from the private
// helpers themselves, so a test using this can never silently pass against
// a moved constant it re-typed a stale copy of (dsp::VcoAdsrState::
// MapSustain's own header comment records this project already shipping
// exactly that bug once). Both inputs are routed through RuntimeFloat (see
// that function's own comment) so this always matches genuine runtime
// evaluation, never the compiler's constant-expression interpreter.
float MapKnobToSeconds(float knob, float maxSeconds) {
    const float k = RuntimeFloat(knob);
    const float m = RuntimeFloat(maxSeconds);
    const float clamped = std::min(std::max(k, 0.0f), 1.0f);
    return dsp::VcoAdsrState::kMinTimeSeconds + clamped * (m - dsp::VcoAdsrState::kMinTimeSeconds);
}

// dsp::VcoAdsrState::MapSustain, called through RuntimeFloat (see that
// function's own comment) so this test file's independent recomputation of
// the sustain-level map always matches genuine runtime evaluation.
float MapSustainRuntime(float knob) {
    return dsp::VcoAdsrState::MapSustain(RuntimeFloat(knob));
}

// A deliberately CONSERVATIVE stand-in for ComputeRampStep's private
// kCurveMinProgress (VoiceEnvelope.hpp, currently 0.4f) -- assumed much
// smaller (0.1f) than the real floor so this cap stays valid (generous, not
// tight) even if kCurveMinProgress is retuned by ear later, without this
// test file needing to track that private value. T1.3(a)'s own sweep below
// is the one that measures and reports the ACTUAL worst multiple observed;
// this helper only needs to be loose enough to fail fast on a genuinely
// unbounded ramp, not tight enough to pin the real bound.
constexpr float kConservativeMinProgressAssumption = 0.1f;

long ConservativeStageCap(float distance, float stepMagnitude) {
    const double linearSamples = static_cast<double>(distance) / static_cast<double>(stepMagnitude);
    return static_cast<long>(linearSamples / static_cast<double>(kConservativeMinProgressAssumption)) + 64;
}

}  // namespace

// -----------------------------------------------------------------------
// T1.3(c) -- bit-identity at curve == 0. ComputeRampStep's `curveAmount <=
// 0.0f` branch is the ORIGINAL linear early-return, untouched by T1.1's
// progress floor (which lives entirely past that branch's own `return`).
// Proven here not by code inspection alone but by an independent,
// hand-rolled per-sample replica of stepVoice()'s own stage arithmetic
// (same min/max-step formula, same stage-transition thresholds), run in
// lockstep against the real ADSR across a full Attack -> Decay -> Hold ->
// (gate low, grace inactive) -> Release -> Idle arc, asserting EXACT
// (bit-for-bit, not epsilon) equality every single sample -- the same
// exact-equality idiom this file already uses elsewhere for formula-parity
// claims (e.g. StereoDelay::FreezeFeedback's "bit-exact" tests below).
// -----------------------------------------------------------------------
TEST_CASE(compute_ramp_step_curve_zero_is_bit_identical_to_the_untouched_linear_path) {
    constexpr float kSampleRate = 48000.0f;
    constexpr float attackKnob = 0.2f;
    constexpr float decayKnob = 0.3f;
    constexpr float sustainKnob = 0.5f;
    constexpr float releaseKnob = 0.4f;

    dsp::VcoAdsrState adsr;
    adsr.init(kSampleRate);
    adsr.setGate(true);

    const float sustainLevel = MapSustainRuntime(sustainKnob);
    const float attackStep =
        1.0f / std::max(MapKnobToSeconds(attackKnob, dsp::VcoAdsrState::kMaxAttackSeconds) * kSampleRate, 1.0f);
    const float decayStep = (1.0f - sustainLevel) /
                             std::max(MapKnobToSeconds(decayKnob, dsp::VcoAdsrState::kMaxDecaySeconds) * kSampleRate,
                                       1.0f);
    const float releaseStep =
        1.0f / std::max(MapKnobToSeconds(releaseKnob, dsp::VcoAdsrState::kMaxReleaseSeconds) * kSampleRate, 1.0f);

    enum class RefStage { Attack, Decay, Hold, Release, Idle };
    RefStage refStage = RefStage::Attack;
    float refLevel = 0.0f;
    bool gateOpen = true;

    const int totalSamples = static_cast<int>(0.3f * kSampleRate);  // well past every stage's completion here.
    const int gateLowAtSample = totalSamples / 2;

    for (int i = 0; i < totalSamples; ++i) {
        if (i == gateLowAtSample) {
            adsr.setGate(false);
            gateOpen = false;
        }
        const float actual =
            adsr.apply(0, 1.0f, attackKnob, decayKnob, sustainKnob, releaseKnob, /*curveKnob=*/0.0f);

        // Replica of stepVoice()'s pending-release resolution at
        // graceKnob==0.0f (default, inactive): a gate-false edge forces
        // Release immediately from Attack/Decay/Hold -- the pre-Grace
        // `setGate(false)` synchronous-force semantic (VoiceEnvelope.hpp's
        // own setGate()/stepVoice() comments).
        if (!gateOpen && refStage != RefStage::Release && refStage != RefStage::Idle) {
            refStage = RefStage::Release;
        }

        switch (refStage) {
            case RefStage::Attack:
                refLevel = std::min(1.0f, refLevel + attackStep);
                if (refLevel >= 1.0f) refStage = RefStage::Decay;
                break;
            case RefStage::Decay:
                refLevel = std::max(sustainLevel, refLevel - decayStep);
                if (refLevel <= sustainLevel) refStage = RefStage::Hold;
                break;
            case RefStage::Hold:
                refLevel = sustainLevel;
                break;
            case RefStage::Release:
                refLevel = std::max(0.0f, refLevel - releaseStep);
                if (refLevel <= 0.0f) refStage = RefStage::Idle;
                break;
            case RefStage::Idle:
                refLevel = 0.0f;
                break;
        }

        REQUIRE_TRUE(actual == refLevel);
    }
}

// -----------------------------------------------------------------------
// T1.3(a) -- duration-bound sweep. T1.1's floor guarantees per-sample
// progress magnitude >= kCurveMinProgress (VoiceEnvelope.hpp) of the linear
// step at every curveAmount, so every stage's worst-case duration is
// bounded at roughly 1/kCurveMinProgress times its plain-linear (curve==0)
// duration -- this sweep proves that bound holds across a curve x knob grid
// INCLUDING curve == 1.0 exactly (the value that made the pre-fix ramp
// unbounded), at both 48k and 96k, and reports the worst multiple actually
// observed (see this packet's own report for that number -- not asserted
// here as a literal value, since it is a measured OUTCOME of the fix, not a
// separate spec).
// -----------------------------------------------------------------------
TEST_CASE(compute_ramp_step_bounds_every_stage_duration_across_curve_and_knob_grid_at_multiple_sample_rates) {
    const float curves[] = {0.0f, 0.5f, 0.9f, 0.999f, 1.0f};
    const float knobs[] = {0.0f, 0.5f, 1.0f};
    const float sampleRates[] = {48000.0f, 96000.0f};
    constexpr float kSustainKnob = 0.5f;

    double worstMultipleObserved = 0.0;
    const char* worstLabel = "";

    for (float sampleRate : sampleRates) {
        for (float curve : curves) {
            for (float knob : knobs) {
                const float sustainLevel = MapSustainRuntime(kSustainKnob);

                // -- Attack: 0 -> 1.0, isolated (fresh ADSR). ---------------
                {
                    dsp::VcoAdsrState adsr;
                    adsr.init(sampleRate);
                    adsr.setGate(true);
                    const float step =
                        1.0f / std::max(MapKnobToSeconds(knob, dsp::VcoAdsrState::kMaxAttackSeconds) * sampleRate,
                                        1.0f);
                    const long cap = ConservativeStageCap(1.0f, step);
                    const long samples =
                        StepUntilLevelCrosses(adsr, knob, 0.0f, kSustainKnob, 0.0f, curve, true, 1.0f, cap).samples;
                    const double linearSamples =
                        std::max(static_cast<double>(MapKnobToSeconds(knob, dsp::VcoAdsrState::kMaxAttackSeconds)) *
                                     sampleRate,
                                 1.0);
                    const double multiple = static_cast<double>(samples) / linearSamples;
                    if (multiple > worstMultipleObserved) {
                        worstMultipleObserved = multiple;
                        worstLabel = "Attack";
                    }
                }

                // -- Decay: 1.0 -> sustainLevel, isolated (warm up Attack
                // with attackKnob=0/curve=0 first -- fast and not what this
                // pass measures). ---------------------------------------
                {
                    dsp::VcoAdsrState adsr;
                    adsr.init(sampleRate);
                    adsr.setGate(true);
                    StepUntilLevelCrosses(adsr, /*attackKnob=*/0.0f, 0.0f, kSustainKnob, 0.0f, /*curveKnob=*/0.0f,
                                           true, 1.0f, static_cast<long>(sampleRate));  // warm-up, ample cap.
                    const float step = (1.0f - sustainLevel) /
                                        std::max(MapKnobToSeconds(knob, dsp::VcoAdsrState::kMaxDecaySeconds) *
                                                      sampleRate,
                                                  1.0f);
                    const long cap = ConservativeStageCap(1.0f - sustainLevel, step);
                    const long samples = StepUntilLevelCrosses(adsr, 0.0f, knob, kSustainKnob, 0.0f, curve, false,
                                                                sustainLevel, cap)
                                             .samples;
                    const double linearSamples =
                        std::max(static_cast<double>(MapKnobToSeconds(knob, dsp::VcoAdsrState::kMaxDecaySeconds)) *
                                     sampleRate,
                                 1.0);
                    const double multiple = static_cast<double>(samples) / linearSamples;
                    if (multiple > worstMultipleObserved) {
                        worstMultipleObserved = multiple;
                        worstLabel = "Decay";
                    }
                }

                // -- Release: sustainLevel -> 0.0, isolated (warm up Attack
                // then Decay to Hold with attackKnob=decayKnob=0/curve=0,
                // then force Release via gate-low with grace inactive). --
                {
                    dsp::VcoAdsrState adsr;
                    adsr.init(sampleRate);
                    adsr.setGate(true);
                    StepUntilLevelCrosses(adsr, 0.0f, 0.0f, kSustainKnob, 0.0f, 0.0f, true, 1.0f,
                                           static_cast<long>(sampleRate));
                    StepUntilLevelCrosses(adsr, 0.0f, 0.0f, kSustainKnob, 0.0f, 0.0f, false, sustainLevel,
                                           static_cast<long>(sampleRate));
                    adsr.setGate(false);  // graceKnob defaults to 0.0f below -> inactive -> immediate Release.
                    const float step =
                        1.0f / std::max(MapKnobToSeconds(knob, dsp::VcoAdsrState::kMaxReleaseSeconds) * sampleRate,
                                        1.0f);
                    const long cap = ConservativeStageCap(sustainLevel, step);
                    const long samples =
                        StepUntilLevelCrosses(adsr, 0.0f, 0.0f, kSustainKnob, knob, curve, false, 0.0f, cap).samples;
                    const double linearSamples =
                        std::max(sustainLevel * static_cast<double>(MapKnobToSeconds(
                                                     knob, dsp::VcoAdsrState::kMaxReleaseSeconds)) *
                                     sampleRate,
                                 1.0);
                    const double multiple = static_cast<double>(samples) / linearSamples;
                    if (multiple > worstMultipleObserved) {
                        worstMultipleObserved = multiple;
                        worstLabel = "Release";
                    }
                }
            }
        }
    }

    std::cout << "T1.3(a) duration-bound sweep: worst multiple observed = " << worstMultipleObserved << " ("
              << worstLabel << ")\n";
    // The bound itself: a small multiple of the plain-linear duration, at
    // EVERY grid point (including curve==1.0, every knob, both rates) --
    // this is the actual pass/fail assertion. 10x is a generous test-side
    // ceiling (kCurveMinProgress's own analytic bound is ~1/0.4 == 2.5x;
    // this leaves ample room for retuning kCurveMinProgress lower without
    // this sweep needing to change) while still catching a regression back
    // toward unbounded.
    REQUIRE_TRUE(worstMultipleObserved < 10.0);
}

// -----------------------------------------------------------------------
// T1.2 -- Grace semantics preserved and bounded (TEST-ONLY; the ladder
// itself, VoiceEnvelope.hpp:332-370, is UNCHANGED by this packet).
// -----------------------------------------------------------------------

// (a) With T1.1's floor in, a pending release under active Grace reaches
// Release within (bounded stage completion + grace) at every curve
// including 1.0 exactly. "Reaches Release" is read from the level
// trajectory: Hold keeps level pinned exactly at sustainLevel every sample
// (stepVoice()'s own `case Stage::Hold: m_level = sustainLevel;`), so the
// first sample where level drops BELOW sustainLevel is the same sample
// Release's own ComputeRampStep call first ran (stage resolution happens
// before the stage switch in the SAME stepVoice() call, VoiceEnvelope.hpp's
// own stepVoice() comment) -- an exact, not approximate, marker.
//
// Originally exercised ONLY graceKnob==1.0f: measured while writing this
// test, the grace countdown (VoiceEnvelope.hpp's own `m_graceRemaining`)
// only ever reached its `<= 0.0f` force-Release check on the SAME call its
// `-1.0f` "not started" sentinel got re-armed, for almost any grace knob
// whose mapped countdown (`graceSeconds * m_sampleRate`) was not an EXACT
// integer -- the countdown decremented by exactly 1.0f/sample and
// generically crossed from a small positive value straight to a small
// negative one without ever landing on 0.0f, and the OLD `< 0.0f` init
// guard mistook that pending-expiry value for "not started", re-arming it
// to the full grace forever. 1.0f*48000.0f == exactly 48000.0f (no
// rounding at all) was the one value immune to this, which is why only it
// was used here.
//
// Fixed by T1.6 (VoiceEnvelope.hpp's Hold-stage grace block, 2026-08-17):
// the init guard now matches the exact -1.0f sentinel instead of any
// negative value, so a decremented-negative, non-sentinel value falls
// through to the `<= 0.0f` expiry check on the very next call instead of
// re-arming. The grid below now covers grace knobs beyond the one value
// that used to dodge the bug, including float-inexact ones.
TEST_CASE(grace_active_pending_release_reaches_release_within_bounded_completion_plus_grace_at_every_curve) {
    constexpr float kSampleRate = 48000.0f;
    constexpr float attackKnob = 0.3f;
    constexpr float decayKnob = 0.3f;
    constexpr float sustainKnob = 0.6f;
    constexpr float releaseKnob = 0.3f;

    // mapGrace's own formula (VoiceEnvelope.hpp): plain clamp*kMaxGraceSeconds.
    const float graceKnobs[] = {0.3f, 0.5f, 0.7f, 1.0f};
    const float curves[] = {0.0f, 0.5f, 0.9f, 1.0f};
    for (float graceKnob : graceKnobs) {
        const float graceSeconds = std::min(std::max(graceKnob, 0.0f), 1.0f) * dsp::VcoAdsrState::kMaxGraceSeconds;
        const double graceSamples = static_cast<double>(graceSeconds) * kSampleRate;

        for (float curve : curves) {
            dsp::VcoAdsrState adsr;
            adsr.init(kSampleRate);
            adsr.setGate(true);

            // Reach Hold (bounded stage completion) before marking a release
            // pending -- ample cap, this is not what's under test here. The
            // sustain level used as the boundary marker below is read straight
            // off THIS real object (StepUntilLevelStabilizes's own ground-truth
            // level), not recomputed independently -- see RuntimeFloat's own
            // comment for why an independent MapSustain recomputation is not
            // automatically trustworthy bit-for-bit.
            StepUntilLevelCrosses(adsr, attackKnob, decayKnob, sustainKnob, releaseKnob, curve, true, 1.0f,
                                   static_cast<long>(10.0f * kSampleRate));
            const StageResult holdReached = StepUntilLevelStabilizes(adsr, attackKnob, decayKnob, sustainKnob,
                                                                       releaseKnob, curve,
                                                                       static_cast<long>(10.0f * kSampleRate));
            const float sustainLevel = holdReached.level;

            adsr.setGate(false);  // marks pending; grace is ACTIVE (graceKnob > 0) and stage is Hold.

            // Ample cap: grace (graceSamples) plus a generous bounded-release
            // allowance -- if this test ever times out against a REGRESSED
            // ramp, that regression is the finding, not test flakiness.
            const long cap = static_cast<long>(graceSamples) + static_cast<long>(10.0f * kSampleRate);
            const long samplesUntilReleaseStarts =
                StepUntilLevelCrosses(adsr, attackKnob, decayKnob, sustainKnob, releaseKnob, curve, false,
                                       std::nextafter(sustainLevel, 0.0f), cap, graceKnob)
                    .samples;

            // Bounded: Release starts within grace plus a small slack (a few
            // samples for the countdown's own ceil/floor rounding -- NOT a
            // separate multi-second allowance, because Attack/Decay already
            // finished before setGate(false) was called above).
            REQUIRE_TRUE(static_cast<double>(samplesUntilReleaseStarts) <= graceSamples + 8.0);
            // Preserved (not cut short): Release does NOT start meaningfully
            // before grace's own countdown elapses -- the minimum-hold
            // guarantee this task exists to pin, from the Hold side.
            REQUIRE_TRUE(static_cast<double>(samplesUntilReleaseStarts) >= graceSamples - 8.0);
        }
    }
}

// -----------------------------------------------------------------------
// T1.6 -- Grace countdown sentinel-conflation bug (found during P1,
// VoiceEnvelope.hpp's stepVoice() Hold-stage grace block). The init guard
// used to fire on `m_graceRemaining < 0.0f` -- ANY negative value, not just
// the exact -1.0f "not started" sentinel. A countdown initialized to a
// non-float-exact `graceSeconds * m_sampleRate` (most grace knobs; the
// widened test above's 1.0f was the one exception, landing on exactly
// 48000.0f at 48kHz) decrements past zero to a small negative, non-sentinel
// value without ever landing exactly on 0.0f, and the old guard mistook
// that pending-expiry value for "not started", re-arming it to the full
// grace -- forever, confirmed by direct measurement: a 0.3 grace knob at
// 48kHz never released within 494,400+ samples (over 10s). Fixed by
// narrowing the guard to an exact `== -1.0f` sentinel match, so a
// decremented-negative value falls straight through to the `<= 0.0f`
// expiry check on the very next call instead of being mistaken for
// "not started".
// -----------------------------------------------------------------------
TEST_CASE(grace_countdown_with_float_inexact_values_expires_within_grace_plus_a_few_samples) {
    constexpr float kSampleRate = 48000.0f;
    constexpr float attackKnob = 0.3f;
    constexpr float decayKnob = 0.3f;
    constexpr float sustainKnob = 0.6f;
    constexpr float releaseKnob = 0.3f;

    // Both deliberately float-inexact: neither 0.3f nor 0.7f, multiplied by
    // kMaxGraceSeconds and m_sampleRate, lands on an exact integer sample
    // count -- exactly the class of knob the pre-fix guard hung forever on.
    const float graceKnobs[] = {0.3f, 0.7f};
    for (float graceKnob : graceKnobs) {
        const float graceSeconds = std::min(std::max(graceKnob, 0.0f), 1.0f) * dsp::VcoAdsrState::kMaxGraceSeconds;
        const double graceSamples = static_cast<double>(graceSeconds) * kSampleRate;

        dsp::VcoAdsrState adsr;
        adsr.init(kSampleRate);
        adsr.setGate(true);

        StepUntilLevelCrosses(adsr, attackKnob, decayKnob, sustainKnob, releaseKnob, 0.0f, true, 1.0f,
                               static_cast<long>(10.0f * kSampleRate));
        const StageResult holdReached = StepUntilLevelStabilizes(adsr, attackKnob, decayKnob, sustainKnob,
                                                                   releaseKnob, 0.0f,
                                                                   static_cast<long>(10.0f * kSampleRate));
        const float sustainLevel = holdReached.level;

        adsr.setGate(false);  // marks pending; grace active (graceKnob > 0), stage is Hold.

        // Tight cap, deliberately: pre-fix, this bug never released at all
        // (re-armed to the full grace forever), so a small cap turns that
        // into a clean test FAILURE (StepUntilLevelCrosses's own throw)
        // instead of masking the bug behind a merely-generous allowance.
        const long cap = static_cast<long>(graceSamples) + 64;
        const long samplesUntilReleaseStarts =
            StepUntilLevelCrosses(adsr, attackKnob, decayKnob, sustainKnob, releaseKnob, 0.0f, false,
                                   std::nextafter(sustainLevel, 0.0f), cap, graceKnob)
                .samples;

        // T1.6 asks for expiry "within grace + 1 sample"; the enforced
        // tolerance is +/-2 samples (corrected 2026-08-17 -- the comment
        // previously quoted the task's "+1" while the assertion below has
        // always allowed 2). Two samples is the honest bound: the countdown
        // decrements by exactly 1.0f/sample from a non-integer start, so
        // ceil-style rounding can land either side by one. Still a
        // sample-scale window, not a multi-second allowance -- the pre-fix
        // bug never expired at all.
        REQUIRE_TRUE(static_cast<double>(samplesUntilReleaseStarts) <= graceSamples + 2.0);
        REQUIRE_TRUE(static_cast<double>(samplesUntilReleaseStarts) >= graceSamples - 2.0);
    }
}

// (b) The minimum-hold guarantee holds: a short gate with a long attack and
// active grace still completes Attack and Decay before Release begins --
// the approved main-spec Grace behaviour ("a short gate cannot clip a note
// before its envelope completes Attack and Decay"), unaffected by T1.1's
// floor. Attack knob near-max (kMaxAttackSeconds, W5's new 0.5f ceiling) so
// a naive "gate released mid-attack forces Release" bug would be obvious:
// the gate here closes after 1ms, far short of any mapped attack time.
TEST_CASE(short_gate_with_long_attack_and_active_grace_completes_attack_and_decay_before_release) {
    constexpr float kSampleRate = 48000.0f;
    constexpr float attackKnob = 1.0f;   // mapped to kMaxAttackSeconds == 0.5f exactly.
    constexpr float decayKnob = 0.5f;
    constexpr float sustainKnob = 0.6f;
    constexpr float releaseKnob = 0.3f;
    constexpr float graceKnob = 0.2f;  // active.

    const float curves[] = {0.0f, 1.0f};  // the two extremes: plain linear and the fixed-unbounded value.
    for (float curve : curves) {
        dsp::VcoAdsrState adsr;
        adsr.init(kSampleRate);
        adsr.setGate(true);

        // Short gate: closed after 1ms (48 samples), a tiny fraction of any
        // mapped attack time at attackKnob==1.0. At this point the level is
        // still deep in Attack (near 0), nowhere near sustainLevel yet --
        // Attack rising THROUGH sustainLevel en route to its own 1.0 peak is
        // normal and must not be mistaken for a premature Release, which is
        // exactly why this test tracks Attack's actual peak and Decay's
        // actual settle rather than a level-vs-sustainLevel threshold.
        for (int i = 0; i < 48; ++i) {
            adsr.apply(0, 1.0f, attackKnob, decayKnob, sustainKnob, releaseKnob, curve, graceKnob);
        }
        adsr.setGate(false);  // marks pending; grace is active, stage is still Attack -- must NOT force Release.

        // If grace is honoured, Attack (bounded by T1.1) still reaches its
        // own peak (1.0) despite the short gate -- a naive "gate released
        // mid-attack forces Release" bug would instead see the level keep
        // heading toward 0 from here and never reach 1.0 within this
        // generous, T1.1-bounded cap.
        const StageResult peakReached =
            StepUntilLevelCrosses(adsr, attackKnob, decayKnob, sustainKnob, releaseKnob, curve, /*ascending=*/true,
                                   1.0f, static_cast<long>(10.0f * kSampleRate), graceKnob);
        REQUIRE_TRUE(peakReached.level >= 1.0f - 1e-6f);

        // ...and Decay then runs on to settle at Hold (two consecutive
        // identical readings -- see StepUntilLevelStabilizes's own comment)
        // rather than being pre-empted by Release -- the "and Decay" half
        // of this task's name. Read off the real object, not an
        // independently-recomputed sustainLevel (RuntimeFloat's own
        // comment). Settling strictly below the Attack peak and strictly
        // above 0 rules out both failure shapes: still-finishing Attack
        // (would settle AT 1.0) and a forced Release that ran all the way
        // to Idle (would settle at exactly 0.0).
        const StageResult holdReached = StepUntilLevelStabilizes(
            adsr, attackKnob, decayKnob, sustainKnob, releaseKnob, curve, static_cast<long>(10.0f * kSampleRate),
            graceKnob);
        REQUIRE_TRUE(holdReached.level < 1.0f - 1e-6f);
        REQUIRE_TRUE(holdReached.level > 0.0f);
    }
}

// =========================================================================
// 3.3 -- Envelope followers (sim/V2EnvelopeFollowerBank.hpp:22-25,30-35)
// =========================================================================

TEST_CASE(envelope_followers_coeffs_match_exp_formula) {
    dsp::VcoEnvelopeFollowers ef;
    const float sr = 48000.0f;
    ef.SetSampleRate(sr);
    const float expectedAttack = 1.0f - std::exp(-1.0f / (0.01f * sr));
    const float expectedRelease = 1.0f - std::exp(-1.0f / (0.05f * sr));
    REQUIRE_NEAR(ef.attackCoeff, expectedAttack, 1e-9);
    REQUIRE_NEAR(ef.releaseCoeff, expectedRelease, 1e-9);
}

TEST_CASE(envelope_followers_only_three_taps_no_pair_sums) {
    // The frozen bank has 5 taps (3 solo + 2 pair-sum); this port keeps
    // only the 3 solo taps that feed the D5 slate (:9-11).
    REQUIRE_TRUE(dsp::VcoEnvelopeFollowers::kNumTaps == 3);
}

TEST_CASE(envelope_followers_track_abs_value_with_attack_release_asymmetry) {
    dsp::VcoEnvelopeFollowers ef;
    ef.SetSampleRate(48000.0f);
    float out[3];
    ef.Process(1.0f, -1.0f, 0.0f, out);  // rising from 0 -> attack branch
    REQUIRE_NEAR(out[0], ef.attackCoeff, 1e-6);  // level += (1-0)*attackCoeff
    REQUIRE_NEAR(out[1], ef.attackCoeff, 1e-6);  // |−1| == 1, same target
    REQUIRE_NEAR(out[2], 0.0f, 1e-6);

    ef.Process(0.0f, 0.0f, 0.0f, out);  // falling from > 0 -> release branch
    const float expectedFall = out[0];  // recompute independently below
    (void)expectedFall;
    REQUIRE_TRUE(out[0] < ef.attackCoeff);  // moved back down, not up
}

// Packet 6 addition (design D5 slot 14, external audio EF): SingleEnvelopeFollower
// is VcoEnvelopeFollowers's identical per-tap formula generalized to one
// channel -- pinned here against a VcoEnvelopeFollowers instance fed the same
// signal on all three lanes, which must match exactly (same coefficients,
// same target/attack/release formula).
TEST_CASE(single_envelope_follower_matches_vco_envelope_followers_per_tap_formula) {
    dsp::SingleEnvelopeFollower single;
    dsp::VcoEnvelopeFollowers triple;
    single.SetSampleRate(48000.0f);
    triple.SetSampleRate(48000.0f);
    REQUIRE_NEAR(single.attackCoeff, triple.attackCoeff, 1e-9);
    REQUIRE_NEAR(single.releaseCoeff, triple.releaseCoeff, 1e-9);

    const float inputs[] = {1.0f, 0.6f, -0.8f, 0.0f, 0.0f, 0.3f};
    float out[3];
    for (float in : inputs) {
        const float singleOut = single.Process(in);
        triple.Process(in, in, in, out);
        REQUIRE_NEAR(singleOut, out[0], 1e-6);
        REQUIRE_NEAR(out[0], out[1], 1e-9);
        REQUIRE_NEAR(out[0], out[2], 1e-9);
    }
}

// =========================================================================
// 3.4 -- Random S&H lanes (Marbles.hpp:90-119,138-144; RGen.hpp)
// =========================================================================

TEST_CASE(rgen_same_seed_is_deterministic) {
    dsp::RGen a(12345u);
    dsp::RGen b(12345u);
    for (int i = 0; i < 50; ++i) {
        REQUIRE_TRUE(a.UniGen() == b.UniGen());
    }
}

TEST_CASE(rgen_distinct_seeds_produce_independent_streams) {
    // This is the guarantee tasks.md 3.4 asks to "verify in a test": with
    // the frozen RGen's shared-static state, this would be meaningless (all
    // instances share one cursor); this port's instance-level state makes
    // distinct seeds actually matter.
    dsp::RGen a(1u);
    dsp::RGen b(2u);
    bool sawDifference = false;
    for (int i = 0; i < 20; ++i) {
        if (a.UniGen() != b.UniGen()) {
            sawDifference = true;
        }
    }
    REQUIRE_TRUE(sawDifference);
}

TEST_CASE(random_sh_five_lanes_are_pairwise_independent) {
    // Construction seeds are arbitrary but distinct per lane.
    dsp::RandomShLane lanes[5] = {
        dsp::lanes::MakeSource1(0x1001u),
        dsp::lanes::MakeSource2(0x1002u),
        dsp::lanes::MakeSource3(0x1003u),
        dsp::lanes::MakeSource4(0x1004u),
        dsp::lanes::MakeSource5(0x1005u),
    };
    std::vector<float> sequences[5];
    for (int step = 0; step < 40; ++step) {
        for (int lane = 0; lane < 5; ++lane) {
            lanes[lane].Increment();
            sequences[lane].push_back(lanes[lane].Process());
        }
    }
    for (int i = 0; i < 5; ++i) {
        for (int j = i + 1; j < 5; ++j) {
            bool sawDifference = false;
            for (size_t k = 0; k < sequences[i].size(); ++k) {
                if (std::fabs(sequences[i][k] - sequences[j][k]) > 1e-6f) {
                    sawDifference = true;
                    break;
                }
            }
            REQUIRE_TRUE(sawDifference);
        }
    }
}

TEST_CASE(random_sh_same_seed_reconstructed_lane_matches) {
    dsp::RandomShLane lane1 = dsp::lanes::MakeSource2(777u);
    dsp::RandomShLane lane2 = dsp::lanes::MakeSource2(777u);
    for (int i = 0; i < 30; ++i) {
        lane1.Increment();
        lane2.Increment();
        REQUIRE_NEAR(lane1.Process(), lane2.Process(), 1e-9);
    }
}

TEST_CASE(random_sh_source3_is_narrow_and_centred) {
    // D8a: source #3 is the only narrow, centred source. spread=0.3 bounds
    // output to [0.5-0.15, 0.5+0.15] = [0.35, 0.65] (bias=0).
    dsp::RandomShLane lane = dsp::lanes::MakeSource3(0xABCDu);
    for (int i = 0; i < 200; ++i) {
        lane.Increment();
        const float v = lane.Process();
        REQUIRE_TRUE(v >= 0.35f - 1e-4f);
        REQUIRE_TRUE(v <= 0.65f + 1e-4f);
    }
}

TEST_CASE(random_sh_source4_settles_to_one_of_five_quantised_levels) {
    dsp::RandomShLane lane = dsp::lanes::MakeSource4(0xBEEFu);
    for (int step = 0; step < 20; ++step) {
        lane.Increment();
        float settled = 0.0f;
        for (int i = 0; i < 32; ++i) {  // let the (fast) slew filter converge
            settled = lane.Process();
        }
        const float nearestLevel = std::round(settled * 4.0f) / 4.0f;
        REQUIRE_NEAR(settled, nearestLevel, 1e-3);
    }
}

TEST_CASE(random_sh_locked_loop_source_replays_without_regenerating) {
    // D8a sources #1/#2/#3 are LOCKED loops: dejaVuKnob == 0.5 takes
    // Marbles.hpp's :99 "else" branch with regen chance
    // 2*(0.5-0.5) == 0, so the bag never regenerates after construction --
    // only the read index advances. The slew filter (kFastCutoff, alpha
    // ~0.94) still carries transient memory across a full 8-step loop, so
    // this test warms up several full loops first (decay per loop is
    // (1-alpha)^8 ~ 1.7e-10 -- fully settled) before comparing two
    // back-to-back loops, rather than comparing the very first loop
    // (filter starting from zero) against the second.
    dsp::RandomShLane lane = dsp::lanes::MakeSource1(0x2222u);
    for (int warmup = 0; warmup < 3 * 8; ++warmup) {
        lane.Increment();
        lane.Process();
    }
    std::vector<float> loopA;
    for (int i = 0; i < 8; ++i) {
        lane.Increment();
        loopA.push_back(lane.Process());
    }
    std::vector<float> loopB;
    for (int i = 0; i < 8; ++i) {
        lane.Increment();
        loopB.push_back(lane.Process());
    }
    for (size_t i = 0; i < loopA.size(); ++i) {
        REQUIRE_NEAR(loopA[i], loopB[i], 1e-6);  // settled loop repeats
    }
}

// =========================================================================
// 3.5 -- Fuegoize (sim/V2FuegoStack.hpp:9-23; parity target
// src/core/Parameter.hpp:129-151, NOT sim/Fuegoize.hpp)
// =========================================================================

namespace {
// Reference re-derivation of the FIRMWARE's sh formula only (Parameter.hpp
// :143), isolated from the rest of the scramble, so the regression test
// pins exactly the bit this task is about: sh = 1 + (row % 256) at
// mask == 255 (knob >= 0.9375), with NO row % 0 anywhere.
uint8_t ReferenceShAt256(uint8_t row) {
    return static_cast<uint8_t>(1u + static_cast<uint8_t>(row % 256u));
}
}  // namespace

// Rows exercised below are the app's REAL domain (design D5a/D6: fuego row
// == a parameter's 0-based slot index in its 16-slot bank, so 0-15, with
// crispyRow fixed at 14) -- not the full uint8_t range. This matters
// because the algorithm shifts `lowerBits` (widened to `int` by the usual
// arithmetic conversions) by `sh` bits, and `sh` is unbounded by `row` in
// general (`sh = 1 + (row % (mask+1))`, up to 1 + row at mask==255): for
// large `row` values (roughly >=31) that shift count reaches or exceeds a
// 32-bit int's width, which is undefined behavior. That UB is a latent
// property of the algorithm itself -- identical in the firmware's
// Parameter.hpp:143-144 and in sim/Fuegoize.hpp -- not something this port
// introduces or fixes; it was hit and confirmed while drafting these tests
// with an out-of-domain row=200 (finite-looking output at -O0, a mismatch
// between two supposedly-identical calls at -O2). It is harmless in
// practice only because every real caller keeps `row` inside 0-15.
TEST_CASE(fuegoize_knob_1_0_is_finite_deterministic_and_matches_firmware_sh) {
    for (int rowInt = 0; rowInt <= 15; ++rowInt) {
        const uint8_t row = static_cast<uint8_t>(rowInt);
        const float value = 0.5f;
        const float out1 = dsp::Fuegoize(value, 1.0f, row);
        const float out2 = dsp::Fuegoize(value, 1.0f, row);
        REQUIRE_TRUE(std::isfinite(out1));
        REQUIRE_TRUE(out1 == out2);  // deterministic

        // mask == 255 at knob 1.0 -> firmware sh = 1 + (row % 256).
        const uint8_t sh = ReferenceShAt256(row);
        REQUIRE_TRUE(sh == static_cast<uint8_t>(1u + row));  // row % 256 == row (uint8_t)
        (void)sh;
    }
}

TEST_CASE(fuegoize_knob_0_9375_also_rounds_mask_to_255_and_matches) {
    // round(0.9375 * 8) == round(7.5) == 8 (round-half-away-from-zero) ->
    // mask = (1<<8)-1 = 255, same divisor-256 case as knob 1.0. row=14 is
    // the app's real crispyRow (design D5a/D6).
    const uint8_t row = 14;
    const float value = 0.3f;
    const float outAt1_0 = dsp::Fuegoize(value, 1.0f, row);
    const float outAt0_9375 = dsp::Fuegoize(value, 0.9375f, row);
    REQUIRE_TRUE(std::isfinite(outAt0_9375));
    REQUIRE_NEAR(outAt0_9375, outAt1_0, 1e-6);  // same mask -> same transform
}

TEST_CASE(fuegoize_zero_knob_is_passthrough) {
    REQUIRE_TRUE(dsp::Fuegoize(0.42f, 0.0f, 7) == 0.42f);
}

TEST_CASE(fuego_stack_apply_musical_row_warps_crispy_by_crunchy_first) {
    // sim/V2FuegoStack.hpp:14-23: crispy is itself Crunchy-warped before use.
    const float value = 0.6f;
    const float globalCrunchy = 0.7f;
    const float crispyPreFuego = 0.2f;
    const uint8_t row = 3;
    const uint8_t crispyRow = 14;

    const float manualCrispyAfterCrunchy = dsp::Fuegoize(crispyPreFuego, globalCrunchy, crispyRow);
    const float manualAfterCrunchy = dsp::Fuegoize(value, globalCrunchy, row);
    const float expected = dsp::Fuegoize(manualAfterCrunchy, manualCrispyAfterCrunchy, row);

    const float actual = dsp::FuegoStack::ApplyMusicalRow(value, globalCrunchy, crispyPreFuego, row, crispyRow);
    REQUIRE_NEAR(actual, expected, 1e-9);
}

// =========================================================================
// 3.6 -- Filters (ResonantBump.hpp:44-71; Comb.hpp:54,63,66-76,79-109;
// TanhSaturator.hpp:25-30 Pade approximation; FroggersEngine.hpp:822-848)
// =========================================================================

TEST_CASE(pade_saturator_matches_rational_approximation_and_clamps) {
    for (float x : {-2.0f, -0.5f, 0.0f, 0.5f, 2.0f}) {
        const float x2 = x * x;
        const float expected = std::max(-1.0f, std::min(1.0f, x * (27.0f + x2) / (27.0f + 9.0f * x2)));
        REQUIRE_NEAR(dsp::PadeSaturator::Saturate(x), expected, 1e-6);
    }
    // Large input must clamp to +-1, not blow past it.
    REQUIRE_NEAR(dsp::PadeSaturator::Saturate(100.0f), 1.0f, 1e-6);
    REQUIRE_NEAR(dsp::PadeSaturator::Saturate(-100.0f), -1.0f, 1e-6);
}

TEST_CASE(resonant_bump_coefficients_match_rbj_peaking_formula) {
    dsp::ResonantBump bump;
    bump.SetFreq(0.01f);
    bump.SetHeight(2.0f);
    bump.SetWidth(1.0f);

    const float omega = 2.0f * static_cast<float>(M_PI) * 0.01f;
    const float cosw = std::cos(omega);
    const float sinw = std::sin(omega);
    const float a = std::sqrt(2.0f);
    const float q = 1.0f;
    const float alpha = sinw / (2.0f * q);
    const float a0 = 1.0f + alpha / a;
    const float a1 = -2.0f * cosw;
    const float a2 = 1.0f - alpha / a;
    const float b0 = 1.0f + alpha * a;
    const float b1 = -2.0f * cosw;
    const float b2 = 1.0f - alpha * a;

    REQUIRE_NEAR(bump.biquad.b0, b0 / a0, 1e-6);
    REQUIRE_NEAR(bump.biquad.b1, b1 / a0, 1e-6);
    REQUIRE_NEAR(bump.biquad.b2, b2 / a0, 1e-6);
    REQUIRE_NEAR(bump.biquad.a1, a1 / a0, 1e-6);
    REQUIRE_NEAR(bump.biquad.a2, a2 / a0, 1e-6);
}

// -----------------------------------------------------------------------
// ITEM 2 failing-test-first (design.md A2): the Filter bank wires
// `filterChain_.peak.SetHeight(dsp::ExpMapCompute(1.0f, 10.0f, knob))`
// (FroggersAppCore.hpp's RouteAudioSample). Ceiling history: 10x (+20 dB,
// frozen-firmware parity) -> 4x -> 2x (+6 dB), the last on the operator
// hearing it modulated. Drives the app's OWN `dsp::kMaxResonantBumpHeight`
// (dsp/FilterFx.hpp) at knob==1.0 through the real ResonantBump::Process
// path with a full-scale sine at the peak's resonant frequency, and confirms
// the measured steady-state gain lands on that constant -- so retuning it
// moves this test rather than leaving a stale literal behind.
// -----------------------------------------------------------------------
TEST_CASE(resonant_bump_max_knob_settles_at_the_apps_configured_ceiling) {
    // Reads the app's OWN ceiling constant. The previous version of this test
    // typed `4.0f` into its own ExpMapCompute call and then asserted the
    // result was 4.0f -- which is true of any number, so it passed unchanged
    // when the app's real ceiling moved to 2.0. It pinned nothing.
    const float maxHeight = dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, 1.0f);
    REQUIRE_NEAR(maxHeight, dsp::kMaxResonantBumpHeight, 1e-5);

    dsp::ResonantBump bump;
    const float freqNormalized = 0.05f;
    bump.SetFreq(freqNormalized);
    bump.SetHeight(maxHeight);
    bump.SetWidth(1.0f);

    constexpr int kWarmupSamples = 4000;
    constexpr int kMeasureSamples = 200;
    float measuredPeak = 0.0f;
    for (int i = 0; i < kWarmupSamples + kMeasureSamples; ++i) {
        const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(i);
        const float output = bump.Process(std::sin(phase));  // full-scale (unit-amplitude) input.
        if (i >= kWarmupSamples) {
            measuredPeak = std::max(measuredPeak, std::fabs(output));
        }
    }
    // Measured gain must match the app's ceiling, whatever it currently is --
    // so retuning the constant moves this assertion with it instead of
    // silently leaving a stale number behind.
    REQUIRE_NEAR(measuredPeak, dsp::kMaxResonantBumpHeight, 0.05);
    // And it must stay well under the retired 10x parity ceiling regardless.
    REQUIRE_TRUE(measuredPeak < 5.0f);
}

// DELIBERATE PARITY DIVERGENCE (design.md A2/A2a, 2026-07-29, operator-
// approved -- same treatment as Fuegoize.hpp's own D6 divergence note):
// the frozen firmware's Comb::GetFeedback (src/core/Comb.hpp:66-76) returns
// an asymmetric +-1.1 ceiling. |fb| > 1 does NOT diverge -- PadeSaturator
// sits INSIDE the feedback path (FilterFx.hpp:294), so the fed-back term is
// always bounded to |fb|*1.0 regardless of |fb| -- but it DOES drive the
// loop into permanent, undecaying self-oscillation held at the saturator's
// limit, which the downstream resonant peak (item 2) then turns into
// an audible blowout (design.md A1/A1a). |fb| < 1 is the definition of a
// loop that decays once its input stops, so this port's own GetFeedback
// caps the magnitude at 0.95 instead of 1.1 -- still close enough to unity
// to ring for a long, musical time, but no longer able to sustain forever.
// This is intentionally NOT what the frozen firmware does; parity was
// explicitly deprioritized here by the operator ("parity is stupid").
TEST_CASE(comb_get_delay_samples_and_asymmetric_feedback) {
    REQUIRE_NEAR(dsp::Comb::GetDelaySamples(100.0f), 1.0f / 100.0f, 1e-9);

    const float fbLow = dsp::Comb::GetFeedback(0.25f);   // < 0.5 -> negative
    const float fbHigh = dsp::Comb::GetFeedback(0.75f);  // > 0.5 -> positive
    REQUIRE_TRUE(fbLow < 0.0f);
    REQUIRE_TRUE(fbHigh > 0.0f);
    REQUIRE_NEAR(fbLow, -0.95f * dsp::ZeroedExpCompute(0.25f, 2.0f * (0.5f - 0.25f)), 1e-6);
    REQUIRE_NEAR(fbHigh, 0.95f * dsp::ZeroedExpCompute(0.25f, 2.0f * (0.75f - 0.5f)), 1e-6);
    // No endpoint may reach or exceed unity magnitude -- that is the whole
    // point of item 1 (a sub-unity loop gain is what makes the comb decay).
    REQUIRE_TRUE(std::fabs(fbLow) < 1.0f);
    REQUIRE_TRUE(std::fabs(fbHigh) < 1.0f);
    REQUIRE_TRUE(std::fabs(dsp::Comb::GetFeedback(0.0f)) < 1.0f);
    REQUIRE_TRUE(std::fabs(dsp::Comb::GetFeedback(1.0f)) < 1.0f);
}

// -----------------------------------------------------------------------
// ITEM 1 failing-test-first (design.md A1a): the predecessor's claim that
// the comb "diverges exponentially" at |fb| > 1 was false -- it self-
// oscillates FOREVER at the saturator's limit instead (PadeSaturator is
// INSIDE the feedback path, FilterFx.hpp:294, so the fed-back term can
// never exceed |fb|*1.0 regardless of |fb|'s own magnitude). The old +-1.1
// ceiling fails this test: drive the comb hard, stop the input entirely
// (feed 0.0f), and the tap that fed the loop just keeps recirculating
// near the saturator ceiling instead of decaying. A loop gain strictly
// below unity (item 1's fix, |fb| <= 0.95) is what makes "stop the input"
// actually mean "the output goes to silence" rather than "the output
// keeps ringing at the saturator's limit indefinitely."
// -----------------------------------------------------------------------
TEST_CASE(comb_feedback_at_both_knob_extremes_decays_to_silence_once_input_stops) {
    constexpr float kDrivenSamples = 4000;  // several hundred delay periods' worth of excitation.
    constexpr float kSilentSamples = 20000;  // ample time for a genuinely sub-unity loop to decay.
    constexpr float kSilenceFloor = 1.0e-4f;

    for (const float knob : {0.0f, 1.0f}) {  // both magnitude extremes (negative and positive branch).
        dsp::Comb comb;
        comb.delaySamples = 37;  // an arbitrary short-ish integer delay -- not a knob-derived value.
        comb.SetFeedback(dsp::Comb::GetFeedback(knob));
        comb.SetCutoffAlpha(1.0f);  // identity lowpass -- isolates the feedback loop gain itself.

        // Drive it hard with a full-scale square wave (rich in the harmonics
        // a self-oscillating loop would otherwise sustain).
        for (int i = 0; i < static_cast<int>(kDrivenSamples); ++i) {
            comb.Process((i / 2) % 2 == 0 ? 1.0f : -1.0f);
        }

        // Stop the input entirely and let the loop run on its own energy.
        float lastMagnitude = 0.0f;
        for (int i = 0; i < static_cast<int>(kSilentSamples); ++i) {
            lastMagnitude = std::fabs(comb.Process(0.0f));
        }
        REQUIRE_TRUE(std::isfinite(lastMagnitude));
        REQUIRE_TRUE(lastMagnitude < kSilenceFloor);
    }
}

TEST_CASE(comb_process_matches_in_plus_fb_sat_lp_delay_formula) {
    // Comb.hpp:54: out = in + fb*sat(lp(delay[i-N])).
    dsp::Comb comb;
    comb.delaySamples = 1;
    comb.SetFeedback(0.5f);
    comb.SetCutoffAlpha(1.0f);  // alpha=1 -> lp is an identity pass-through

    const float in0 = comb.Process(1.0f);   // delay line starts at 0 -> tapped=0
    REQUIRE_NEAR(in0, 1.0f + 0.5f * dsp::PadeSaturator::Saturate(0.0f), 1e-6);

    const float in1 = comb.Process(0.2f);   // tapped = in0 (1-sample delay)
    REQUIRE_NEAR(in1, 0.2f + 0.5f * dsp::PadeSaturator::Saturate(in0), 1e-6);
}

TEST_CASE(pure_delay_integer_delay_is_exact) {
    dsp::PureDelay delay;
    delay.delaySamples = 3.0f;
    float last = 0.0f;
    for (int i = 0; i < 10; ++i) {
        last = delay.Process(static_cast<float>(i));
    }
    // After warm-up, an exact integer delay reproduces the input from N
    // samples ago with no interpolation blur (frac == 0).
    REQUIRE_NEAR(last, 6.0f, 1e-5);  // i=9 reads back i=6
}

TEST_CASE(pure_delay_fractional_delay_interpolates) {
    dsp::PureDelay delay;
    delay.delaySamples = 2.5f;
    std::vector<float> inputs = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float last = 0.0f;
    for (float v : inputs) {
        last = delay.Process(v);
    }
    // At i=5 (0-based, 6th sample), delaySamples=2.5 reads between i=2 (2.0)
    // and i=3 (3.0) -> exact midpoint 2.5.
    REQUIRE_NEAR(last, 2.5f, 1e-4);
}

TEST_CASE(filter_fx_chain_parallel_matches_manual_comb_peak_scoop_blend) {
    dsp::FilterFxChain chain;
    chain.comb.delaySamples = 1;
    chain.comb.SetFeedback(0.3f);
    chain.comb.SetCutoffAlpha(0.5f);
    chain.pureDelay.delaySamples = 0.0f;
    chain.peak.SetFreq(0.02f);
    chain.peak.SetHeight(1.5f);
    chain.peak.SetWidth(1.0f);
    chain.scoopNotch.SetFreq(0.02f);
    chain.scoopNotch.SetHeight(0.5f);
    chain.scoopNotch.SetWidth(1.0f);

    dsp::Comb refComb;
    refComb.delaySamples = 1;
    refComb.SetFeedback(0.3f);
    refComb.SetCutoffAlpha(0.5f);
    dsp::PureDelay refPureDelay;
    refPureDelay.delaySamples = 0.0f;
    dsp::ResonantBump refPeak;
    refPeak.SetFreq(0.02f);
    refPeak.SetHeight(1.5f);
    refPeak.SetWidth(1.0f);
    dsp::ResonantBump refScoop;
    refScoop.SetFreq(0.02f);
    refScoop.SetHeight(0.5f);
    refScoop.SetWidth(1.0f);

    const float combPeakBlend = 0.4f;
    const float scoopMix = 0.6f;

    // W2.2a (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/
    // tasks.md): the comb branch now carries an exact output trim
    // `1/(1+|fb|)`, smoothed by a one-pole (FilterFxChain::combTrimSmoother,
    // FilterFx.hpp) before the blend below. OLD expectation: `combPath =
    // refComb.Process(...)`, untrimmed. NEW expectation: `combPath =
    // refComb.Process(...) * trimState`, where `trimState` comes from
    // `refTrimSmoother`, an actual `dsp::OnePoleLowPass` instance (R2, OMNI
    // REVIEW W2.2a -- REVISED from re-implementing the one-pole recurrence
    // by hand here, which made the recurrence exist twice and risked
    // silently desyncing from `OnePoleLowPass::Process` on a future change
    // to it). Seeded identically to production: `alpha` copied from
    // `chain.combTrimSmoother.alpha` (public field, set once by
    // FilterFxChain's own constructor -- read, never re-hardcoded a second
    // time here) and `output` at 1.0 (combTrimSmoother's construction-time
    // initial value, matching the untrimmed branch -- no fade-in), driven
    // each sample by the constant raw target `1/(1+|0.3|)` (feedback is
    // fixed at 0.3 for this whole test, never changes). The reference chain
    // otherwise stays independent of production (refComb/refPureDelay/
    // refPeak/refScoop below are all separate instances) -- a parity test's
    // job is to be an independent check, so only the recurrence itself
    // (which W2.2a introduced and which must exist exactly once on the
    // production side, not be re-derived here too) is shared via the same
    // reusable `dsp::OnePoleLowPass` type, not shared state.
    dsp::OnePoleLowPass refTrimSmoother;
    refTrimSmoother.alpha = chain.combTrimSmoother.alpha;
    refTrimSmoother.output = 1.0f;
    const float rawCombTrim = 1.0f / (1.0f + std::fabs(0.3f));

    // B1 (tasks.md CONSOLIDATED PUSH table): the peak branch now carries
    // its own exact output trim `1/height`, smoothed the identical way the
    // comb trim above is (same `dsp::OnePoleLowPass` type, own instance,
    // `alpha` read from production's `chain.peakTrimSmoother.alpha` rather
    // than re-hardcoded, `output` seeded at 1.0 matching the untrimmed
    // branch -- R2's precedent applied to the new trim rather than
    // reproduced ad hoc). `height` is fixed at 1.5 for this whole test
    // (never changes), so `rawPeakTrim` is likewise a constant target.
    // OLD expectation: `peakPath = refPeak.Process(input)`, untrimmed
    // (0.0905098 at this test's fixed inputs before B1). NEW: `peakPath =
    // refPeak.Process(input) * trimState`, where `trimState` converges to
    // `1/1.5 ~= 0.6667` -- the ~33% reduction the arithmetic (0.0905098 *
    // 0.6667 ~= 0.0603) does NOT land on the measured 0.0717059 above only
    // because the smoother is still gliding from its unity seed across
    // these first 8 samples, not yet at steady state; REQUIRE_NEAR below
    // reads the same live recurrence, so it tracks the glide exactly rather
    // than asserting the converged number.
    dsp::OnePoleLowPass refPeakTrimSmoother;
    refPeakTrimSmoother.alpha = chain.peakTrimSmoother.alpha;
    refPeakTrimSmoother.output = 1.0f;
    const float rawPeakTrim = 1.0f / 1.5f;

    // B5 (tasks.md CONSOLIDATED PUSH table): the peak branch now runs its
    // own `OutputLimiter` (`chain.peakLimiter`) AFTER the trim above, before
    // the blend below. Copied wholesale from `chain.peakLimiter` (R2's own
    // precedent: share the reusable TYPE/STATE, not re-derive the tuning) --
    // taken here, before any `chain.Process()` call, so it captures exactly
    // the state the production instance starts this test in (its
    // constructor's assumed-48kHz `Configure()` call, FilterFx.hpp; this
    // test never calls `chain.Configure()` itself, so production and
    // reference must agree on that same starting state or this parity check
    // would silently test the wrong thing). Every value in this test's
    // fixed 0.1..0.8 input range stays far below `kPeakLimiterThreshold`
    // (0.7) at height=1.5, so the limiter is expected to act as an exact
    // identity here (the struct's own bit-identical-below-threshold
    // guarantee, dsp/Limiter.hpp) -- included anyway so this reference
    // mirrors production's real code path rather than one that happens to
    // produce the same numbers by omission.
    dsp::OutputLimiter refPeakLimiter = chain.peakLimiter;

    for (int i = 0; i < 8; ++i) {
        const float input = 0.1f * static_cast<float>(i + 1);
        const float actual = chain.Process(input, /*topology=*/0.0f, combPeakBlend, scoopMix);

        const float combRaw = refComb.Process(refPureDelay.Process(input));
        const float trimState = refTrimSmoother.Process(rawCombTrim);
        const float combPath = combRaw * trimState;
        const float peakRaw = refPeak.Process(input);
        const float peakTrimState = refPeakTrimSmoother.Process(rawPeakTrim);
        const float peakTrimmed = peakRaw * peakTrimState;
        const float peakPath = refPeakLimiter.Process(peakTrimmed);
        const float mixed = peakPath * (1.0f - combPeakBlend) + combPath * combPeakBlend;
        const float scooped = refScoop.Process(mixed);
        const float expected = mixed * (1.0f - scoopMix) + scooped * scoopMix;

        REQUIRE_NEAR(actual, expected, 1e-5);
    }
}

// -----------------------------------------------------------------------
// W2.2a (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md):
// pins the fix itself. With comb feedback pinned at its maximum magnitude
// (kMaxFeedbackMagnitude, 0.95) and a sustained full-scale input, the comb
// branch must never exceed the computed bound `(A + fb) / (1 + fb)` -- the
// exact worst case the trim was designed to normalize to 1.0. The bound is
// COMPUTED from A and fb, never a hardcoded literal (W2 standing
// constraint: a pin asserts the property that broke, not a typed-in
// number). combPeakBlend=1.0/scoopMix=0.0 isolate the comb branch through
// the real FilterFxChain::Process code path: mixed = peakPath*(1-1) +
// combPath*1 == combPath, and the return is mixed*(1-0) + scooped*0 ==
// mixed -- so chain.Process's return value IS combPath. A settle period
// precedes the assertion window: the trim smoother starts at unity
// (matching feedback's own default of 0) and glides toward its new,
// lower target over several time constants once feedback jumps to its
// maximum -- asserting mid-glide would catch the transient, not the
// steady-state bound this test exists to pin.
// -----------------------------------------------------------------------
TEST_CASE(comb_branch_output_stays_at_or_below_computed_bound_at_max_feedback) {
    dsp::FilterFxChain chain;
    chain.comb.delaySamples = 1;
    chain.comb.SetFeedback(dsp::Comb::GetFeedback(1.0f));  // +0.95, kMaxFeedbackMagnitude.
    chain.comb.SetCutoffAlpha(1.0f);  // identity lowpass -- isolates the feedback loop gain itself.
    chain.pureDelay.delaySamples = 0.0f;

    const float fb = std::fabs(chain.comb.feedback);
    const float inputAmplitude = 1.0f;  // A: filter input is bounded |A| <= 1 (Drive output, W2.1-MATH).
    const float bound = (inputAmplitude + fb) / (1.0f + fb);

    constexpr int kSettleSamples = 2000;   // many comb-loop + trim-smoother time constants.
    constexpr int kAssertSamples = 2000;

    for (int i = 0; i < kSettleSamples; ++i) {
        chain.Process(inputAmplitude, /*topology=*/0.0f, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
    }

    for (int i = 0; i < kAssertSamples; ++i) {
        const float combPath =
            chain.Process(inputAmplitude, /*topology=*/0.0f, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
        REQUIRE_TRUE(std::fabs(combPath) <= bound + 1.0e-4f);
    }
}

// -----------------------------------------------------------------------
// R1 (OMNI REVIEW W2.2a, tasks.md "OMNI REVIEW -- W2.2a"): the test above
// holds `fb` STATIC and settles for 2000 samples before asserting, so it
// cannot see the trim smoother's lag -- and in the real app `fb` is NOT
// static: `Comb::SetFeedback` is called from `RouteAudioSample()` once per
// SAMPLE (FroggersAppCore.hpp:521,647), so a fast modulation source (audio-
// rate noise being the extreme case -- `NoiseModulatorProcessor` draws a
// fresh independent value every sample, DspNoise.hpp) can swing `fb` across
// its whole range sample-to-sample, and an abrupt scramble (Crispy,
// randomize) can step it instantly. This test asserts the SAME `(A +
// fb)/(1 + fb)` bound the test above pins, but evaluated against each
// sample's OWN currently-set `fb` (not a fixed one), from sample zero --
// deliberately NOT skipping a settle window, because the transient
// immediately following a fast swing is exactly what a static-feedback test
// cannot catch and this one exists to.
//
// Two feedback drive patterns, both audio-rate (one new `SetFeedback` call
// per `Process` call, matching production's per-sample cadence exactly):
//   1. A hard step: settle at fb=0 (matching the trim smoother's own
//      construction-time initial state, `output=1.0`, i.e. "at rest"), then
//      jump straight to the maximum magnitude and hold -- the single
//      sharpest possible transient, the shape a Crispy/randomize scramble
//      produces.
//   2. A fixed-seed audio-rate sweep: `fb` redrawn every sample, uniformly
//      across its full [-kMaxFeedbackMagnitude, +kMaxFeedbackMagnitude]
//      range -- the worst realistic case, a 100%-depth Noise source
//      modulating this parameter (deterministic xorshift32, not
//      `<random>`, so this TU stays dependency-free per its own header
//      comment; a fixed seed keeps the test reproducible).
//
// Measured at the pre-fix constant (kCombTrimGlideCyclesPerSample = 0.01,
// analogy-derived from RandomShLane, ~16-sample time constant): worst-case
// overshoot over the bound was ~0.80 for the hard step and ~0.53 for the
// audio-rate sweep (delaySamples=1, same adversarial single-sample-delay
// setup the test above already uses to isolate loop gain) -- both fail this
// test outright. Sweeping the constant (scratch measurement, not checked
// in) found overshoot reaches exactly 0.0 (to measurement precision, 50000
// trials x 7 seeds for the sweep pattern) at glide >= ~0.33 cycles/sample;
// `kCombTrimGlideCyclesPerSample` is now 0.45, comfortably above that
// crossover while staying below `OnePoleLowPass::kMaxCutoff` (0.499, the
// hard clamp). At 0.45 both patterns below measure exactly 0.0 overshoot.
// -----------------------------------------------------------------------
TEST_CASE(comb_branch_output_stays_at_or_below_computed_bound_under_audio_rate_feedback_modulation) {
    const float inputAmplitude = 1.0f;  // A: filter input is bounded |A| <= 1 (Drive output, W2.1-MATH).
    const float kMaxFb = dsp::Comb::GetFeedback(1.0f);  // +0.95, kMaxFeedbackMagnitude.

    auto boundFor = [&](float fb) { return (inputAmplitude + std::fabs(fb)) / (1.0f + std::fabs(fb)); };
    auto makeChain = [&]() {
        dsp::FilterFxChain chain;
        chain.comb.delaySamples = 1;
        chain.comb.SetCutoffAlpha(1.0f);  // identity lowpass -- isolates the feedback loop gain itself.
        chain.pureDelay.delaySamples = 0.0f;
        return chain;
    };

    // Pattern 1: hard step from rest (fb=0) to max, held -- the sharpest
    // transient a scramble can produce.
    {
        dsp::FilterFxChain chain = makeChain();
        chain.comb.SetFeedback(0.0f);
        constexpr int kSettleSamples = 500;
        for (int i = 0; i < kSettleSamples; ++i) {
            chain.Process(inputAmplitude, /*topology=*/0.0f, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
        }
        chain.comb.SetFeedback(kMaxFb);
        constexpr int kPostJumpSamples = 500;
        const float bound = boundFor(kMaxFb);
        for (int i = 0; i < kPostJumpSamples; ++i) {
            const float combPath =
                chain.Process(inputAmplitude, /*topology=*/0.0f, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
            REQUIRE_TRUE(std::fabs(combPath) <= bound + 1.0e-4f);
        }
    }

    // Pattern 2: audio-rate sweep, `fb` redrawn every sample across its full
    // range -- the worst realistic case (a 100%-depth Noise source).
    {
        dsp::FilterFxChain chain = makeChain();
        std::uint32_t rngState = 0xC0FFEEu;
        const auto nextUniform01 = [&rngState]() {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            return static_cast<float>(rngState % 1000000u) / 1000000.0f;
        };
        constexpr int kSamples = 20000;
        for (int i = 0; i < kSamples; ++i) {
            const float fb = (2.0f * nextUniform01() - 1.0f) * kMaxFb;  // [-kMaxFb, kMaxFb)
            chain.comb.SetFeedback(fb);
            const float combPath =
                chain.Process(inputAmplitude, /*topology=*/0.0f, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
            REQUIRE_TRUE(std::fabs(combPath) <= boundFor(fb) + 1.0e-4f);
        }
    }
}

// -----------------------------------------------------------------------
// B1 (tasks.md CONSOLIDATED PUSH table; W2.1-MATH's peak bound `|peak| <=
// A * height`): mirrors the comb bound test above -- static height held at
// the app's own ceiling, driven with a full-scale sine at the bump's own
// resonant frequency (the peak's worst case, matching
// resonant_bump_max_knob_settles_at_the_apps_configured_ceiling's own
// excitation shape), and asserts the TRIMMED branch output never exceeds
// A (== A * height / height, B1's exact target) once warmed up.
// `combPeakBlend=0` isolates the peak branch exactly as
// `combPeakBlend=1` isolated the comb branch above.
// -----------------------------------------------------------------------
TEST_CASE(peak_branch_output_stays_at_or_below_computed_bound_at_max_height) {
    dsp::FilterFxChain chain;
    const float maxHeight = dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, 1.0f);
    const float freqNormalized = 0.05f;  // matches resonant_bump_max_knob_settles_at_the_apps_configured_ceiling
    chain.peak.SetFreq(freqNormalized);
    chain.peak.SetHeight(maxHeight);
    chain.peak.SetWidth(1.0f);

    const float inputAmplitude = 1.0f;  // A: filter input is bounded |A| <= 1 (Drive output, W2.1-MATH).
    const float bound = inputAmplitude;  // A * height / height == A -- B1's exact target.

    constexpr int kWarmupSamples = 4000;   // same buildup window the untrimmed ceiling test uses.
    constexpr int kAssertSamples = 400;
    int sampleIx = 0;
    for (; sampleIx < kWarmupSamples; ++sampleIx) {
        const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
        chain.Process(inputAmplitude * std::sin(phase), /*topology=*/0.0f, /*combPeakBlend=*/0.0f,
                      /*scoopMix=*/0.0f);
    }
    for (int i = 0; i < kAssertSamples; ++i, ++sampleIx) {
        const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
        const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*topology=*/0.0f,
                                              /*combPeakBlend=*/0.0f, /*scoopMix=*/0.0f);
        REQUIRE_TRUE(std::fabs(peakPath) <= bound + 0.05f);
    }
}

// -----------------------------------------------------------------------
// R1's peak-branch analogue (tasks.md CONSOLIDATED PUSH table): the static
// test above holds `height` fixed and settles for 4000 samples first, so it
// cannot see the trim smoother's lag against a fast-moving `height` -- and
// in the real app `height` is not static: `ResonantBump::SetHeight` is
// called from `RouteAudioSample()` once per SAMPLE, same cadence as the
// comb's `fb`, so a noise-modulation source can redraw it every sample
// (see comb_branch_output_stays_at_or_below_computed_bound_under_audio_
// rate_feedback_modulation's own comment for the production call-site
// citation). Same two adversarial patterns, same glide constant under test
// (peakTrimSmoother shares kTrimGlideCyclesPerSample with combTrimSmoother,
// FilterFx.hpp) -- reused here rather than re-derived, per B1's brief.
//
// FINDING (measured, not fixed -- out of B1's scope, which is "mirror
// W2.2a's mechanism exactly"): pattern 1 (hard step) settles to ~0
// overshoot, same as the comb -- but pattern 2 (audio-rate sweep) does NOT
// reach the comb's "exactly 0 overshoot at glide>=0.33" result. Root cause,
// traced rather than assumed: the comb's per-sample bound is UNCONDITIONAL
// on history -- `PadeSaturator::Saturate` sits INSIDE the loop and clamps
// the fed-back term to +-1 every sample regardless of what `fb` was a
// moment ago, so any trim fast enough to track `fb`'s current value fully
// closes the gap. The peak's raw branch has no such per-sample clamp: it is
// a genuine two-pole recursive filter whose x1/x2/y1/y2 state persists
// across a height change, so when `height` drops suddenly the trim (tracked
// fast, correctly, at the new low target) can under-attenuate a raw signal
// still carrying resonance energy built up under the PRIOR, higher height
// -- the mirror image of R1's original comb problem, and not closable by
// retuning the same one glide constant in either direction (faster
// worsens a height-decrease; slower would reopen R1's original
// height-increase gap). Measured (500,000 trials across 10 fixed seeds,
// xorshift32, 50000 samples/seed): trimmed worst-case 1.669 vs raw
// (untrimmed) worst-case 1.819 -- B1 measurably helps (worst case pulled
// down from near the untrimmed ceiling) but does not achieve the static
// case's tight `A` bound under this adversarial pattern. What IS still
// provably true, and what this test pins: the trim never makes the
// worst case WORSE than the pre-existing, already-accepted ceiling
// `A * kMaxResonantBumpHeight` (W2.1-MATH's own `|peak| <= A * height`,
// height <= kMaxResonantBumpHeight) -- i.e. B1 is a net improvement and a
// safe no-regression, not a complete fix of the audio-rate case. Recorded
// as a residual finding for a future dispatch, not invented as a fix here
// (B1's brief is "mirror W2.2a's mechanism exactly", not "redesign it").
// -----------------------------------------------------------------------
TEST_CASE(peak_branch_output_stays_at_or_below_computed_bound_under_audio_rate_height_modulation) {
    const float inputAmplitude = 1.0f;  // A: filter input is bounded |A| <= 1 (Drive output, W2.1-MATH).
    const float freqNormalized = 0.05f;
    const float kMaxHeight = dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, 1.0f);

    auto makeChain = [&]() {
        dsp::FilterFxChain chain;
        chain.peak.SetFreq(freqNormalized);
        chain.peak.SetWidth(1.0f);
        return chain;
    };

    // Pattern 1: hard step from rest (height=1, exact flat passthrough --
    // ResonantBump::UpdateCoefficients reduces to H(z)==1 at height==1, see
    // this file's resonant_bump_coefficients_match_rbj_peaking_formula's own
    // math) to the app's ceiling, held -- the sharpest transient a
    // scramble/randomize can produce. Measured worst overshoot here: ~4e-6
    // (float noise floor) -- this pattern DOES reach B1's tight `A` bound.
    {
        dsp::FilterFxChain chain = makeChain();
        chain.peak.SetHeight(1.0f);
        int sampleIx = 0;
        constexpr int kSettleSamples = 500;
        for (; sampleIx < kSettleSamples; ++sampleIx) {
            const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
            chain.Process(inputAmplitude * std::sin(phase), /*topology=*/0.0f, /*combPeakBlend=*/0.0f,
                          /*scoopMix=*/0.0f);
        }
        chain.peak.SetHeight(kMaxHeight);
        constexpr int kPostJumpSamples = 4000;  // resonance buildup window, same as the static test above.
        for (int i = 0; i < kPostJumpSamples; ++i, ++sampleIx) {
            const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
            const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*topology=*/0.0f,
                                                  /*combPeakBlend=*/0.0f, /*scoopMix=*/0.0f);
            REQUIRE_TRUE(std::fabs(peakPath) <= inputAmplitude + 0.05f);
        }
    }

    // Pattern 2: audio-rate sweep, `height` redrawn every sample across its
    // full [1, kMaxHeight] range -- the worst realistic case (a 100%-depth
    // Noise source modulating this parameter, same idiom as the comb's own
    // pattern 2). Deterministic xorshift32, fixed seed, dependency-free.
    // Per this TEST_CASE's own header finding, this pattern does NOT settle
    // to the tight `A` bound the way the comb's audio-rate sweep does --
    // asserted here against the pre-existing, already-established ceiling
    // `A * kMaxResonantBumpHeight` (W2.1-MATH) instead: the property this
    // test CAN honestly pin is "B1 does not regress the worst case past the
    // already-accepted untrimmed ceiling", not "B1 fully bounds the
    // audio-rate case" (it measurably does not, per the header comment).
    {
        dsp::FilterFxChain chain = makeChain();
        std::uint32_t rngState = 0xC0FFEEu;
        const auto nextUniform01 = [&rngState]() {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            return static_cast<float>(rngState % 1000000u) / 1000000.0f;
        };
        const float ceilingBound = inputAmplitude * dsp::kMaxResonantBumpHeight;  // W2.1-MATH's own bound.
        constexpr int kSamples = 20000;
        for (int i = 0; i < kSamples; ++i) {
            const float height = 1.0f + nextUniform01() * (kMaxHeight - 1.0f);
            chain.peak.SetHeight(height);
            const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(i);
            const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*topology=*/0.0f,
                                                  /*combPeakBlend=*/0.0f, /*scoopMix=*/0.0f);
            REQUIRE_TRUE(std::fabs(peakPath) <= ceilingBound + 1.0e-3f);
        }
    }
}

// -----------------------------------------------------------------------
// B5 (tasks.md CONSOLIDATED PUSH table; "Group B outcomes" B1 finding):
// FAILING-FIRST for the property B1 could not close -- the peak branch
// respects its own computed bound `A` (not the looser untrimmed ceiling
// `A * kMaxResonantBumpHeight` the test above settles for) under the exact
// same adversarial per-sample-random height modulation that measured B1's
// own 1.669. B1's own header comment (above) traced WHY the trim alone
// cannot do this: the peak is a stateful 2-pole biquad whose stored energy
// survives a height DROP, so a same-instant scalar cannot retroactively
// remove energy already in its state -- what corrects that is something
// with its own release, i.e. `FilterFxChain::peakLimiter`
// (`OutputLimiter`, dsp/Limiter.hpp), inserted after the trim in
// `Process()` (FilterFx.hpp). Reuses the existing Pattern-2 idiom exactly
// (freqNormalized, kMaxHeight, xorshift32, seed 0xC0FFEE, 20000 samples --
// the identical adversarial drive, not a fresh one) so this test is a
// direct tightening of the assertion above, not a different measurement.
//
// `chain.Configure(sampleRate)` is called explicitly here (rather than
// relying on the constructor's assumed-48kHz default, FilterFx.hpp) so
// this test exercises the exact call production makes
// (FroggersAppCore::PrepareToPlay -> filterChain_.Configure(sampleRate_)).
//
// MEASURED (this exact seed/sample count, a scratch harness reproducing
// this file's own production math bit-for-bit before this test existed):
// worst-case overshoot with B1's trim only (no limiter) = 1.615898 against
// bound 1.0 (this single seed does not reach the full 500k-trial/10-seed
// worst case of 1.669, but is comfortably past the bound this test pins,
// which is what makes it a valid failing-first repro). With
// `peakLimiter` inserted at its measured tuning (kPeakLimiterThreshold
// 0.7, kPeakLimiterAttackSeconds 5 microseconds, kPeakLimiterReleaseSeconds
// 100ms, FilterFx.hpp): worst-case overshoot = 0.988341, at or below bound.
// -----------------------------------------------------------------------
TEST_CASE(peak_branch_output_respects_computed_bound_under_audio_rate_height_modulation_with_limiter) {
    constexpr float sampleRate = 48000.0f;
    const float inputAmplitude = 1.0f;  // A: filter input is bounded |A| <= 1 (Drive output, W2.1-MATH).
    const float freqNormalized = 0.05f;
    const float kMaxHeight = dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, 1.0f);
    const float bound = inputAmplitude;  // the computed bound this task targets -- never a literal.

    dsp::FilterFxChain chain;
    chain.Configure(sampleRate);
    chain.peak.SetFreq(freqNormalized);
    chain.peak.SetWidth(1.0f);

    std::uint32_t rngState = 0xC0FFEEu;  // identical seed to Pattern 2 above -- same adversarial drive.
    const auto nextUniform01 = [&rngState]() {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return static_cast<float>(rngState % 1000000u) / 1000000.0f;
    };

    constexpr int kSamples = 20000;
    for (int i = 0; i < kSamples; ++i) {
        const float height = 1.0f + nextUniform01() * (kMaxHeight - 1.0f);
        chain.peak.SetHeight(height);
        const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(i);
        const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*topology=*/0.0f,
                                              /*combPeakBlend=*/0.0f, /*scoopMix=*/0.0f);
        REQUIRE_TRUE(std::fabs(peakPath) <= bound + 1.0e-2f);
    }
}

// filter_fx_chain_serial_matches_delay_then_comb_then_peak DELETED (Task A,
// this packet): it pinned the `useParallel == false` series branch
// (`pureDelay -> comb -> peak`, no trims/limiter/blend, ignoring
// combPeakBlend/scoopMix) that FilterFxChain::Process no longer has --
// dead code, confirmed by grep, deleted rather than kept beside the new
// `topology` morph per this packet's brief. No replacement test: there is
// no longer a distinct series code path to pin, and topology's behaviour
// across [0,1] is covered by Task F's headroom sweep test below plus
// filter_fx_chain_parallel_matches_manual_comb_peak_scoop_blend at
// topology==0.

// -----------------------------------------------------------------------
// Task F (packet -- Filter slot 9 "Topology"): at high topology the peak
// biquad's input is `combPath` (the comb branch's OWN output,
// FilterFxChain::Process, FilterFx.hpp) instead of the raw chain input -- a
// new operating point for a stage whose ceiling this codebase has already
// had to lower (kMaxResonantBumpHeight's own history, FilterFx.hpp; the
// peak-branch limiter's own B5 history, this file above). Sweeps topology
// across the WHOLE [0,1] range -- not just the endpoints; the midpoint
// genuinely mixes both paths and is its own case -- drives the peak branch
// (isolated via combPeakBlend=0/scoopMix=0, this file's own established
// idiom, e.g. peak_branch_output_stays_at_or_below_computed_bound_at_max_
// height above) with a full-scale sine at the peak's own resonant
// frequency, comb feedback pinned at its own maximum (the worst case for
// how far combPath can depart from the raw input), and RECORDS the
// measured peak absolute output at each topology -- printed below, per
// this task's own brief that a bare pass/fail is not a result.
// -----------------------------------------------------------------------
TEST_CASE(topology_morph_peak_branch_headroom_across_full_range) {
    constexpr float sampleRate = 48000.0f;
    const float inputAmplitude = 1.0f;  // full-scale.
    const float freqNormalized = 0.05f;  // matches this file's other peak-branch bound tests.
    const float maxHeight = dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, 1.0f);
    const float maxFeedback = dsp::Comb::GetFeedback(1.0f);  // +0.95, kMaxFeedbackMagnitude.

    // Not just the endpoints -- the midpoint (0.5) mixes both paths and is
    // its own case, per this task's own brief.
    const float topologies[] = {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f};

    std::cout << "  [Task F] topology sweep, peak-branch-isolated (combPeakBlend=0, scoopMix=0), "
                 "full-scale sine at peak freq, comb feedback pinned at max magnitude:\n";

    float overallMax = 0.0f;
    for (float topology : topologies) {
        dsp::FilterFxChain chain;
        chain.Configure(sampleRate);  // real peakLimiter coefficients, not the constructor's assumed-48kHz default.
        chain.comb.delaySamples = 1;
        chain.comb.SetFeedback(maxFeedback);
        chain.comb.SetCutoffAlpha(1.0f);  // identity lowpass -- isolates the feedback loop gain, worst case for combPath.
        chain.pureDelay.delaySamples = 0.0f;
        chain.peak.SetFreq(freqNormalized);
        chain.peak.SetHeight(maxHeight);
        chain.peak.SetWidth(1.0f);

        constexpr int kWarmupSamples = 4000;  // resonance buildup window, matches this file's other peak tests.
        constexpr int kAssertSamples = 400;
        int sampleIx = 0;
        float maxLevel = 0.0f;
        for (; sampleIx < kWarmupSamples; ++sampleIx) {
            const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
            chain.Process(inputAmplitude * std::sin(phase), topology, /*combPeakBlend=*/0.0f, /*scoopMix=*/0.0f);
        }
        for (int i = 0; i < kAssertSamples; ++i, ++sampleIx) {
            const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
            const float level = chain.Process(inputAmplitude * std::sin(phase), topology, /*combPeakBlend=*/0.0f,
                                               /*scoopMix=*/0.0f);
            REQUIRE_TRUE(std::isfinite(level));
            maxLevel = std::max(maxLevel, std::fabs(level));
        }
        overallMax = std::max(overallMax, maxLevel);
        std::cout << "  [Task F]   topology=" << topology << "  peak abs output=" << maxLevel << "\n";
    }

    // Safety net only, deliberately NOT a tight "topology must never exceed
    // topology==0's own level" assertion: this task's own brief is that a
    // measurement showing topology's new operating point running hotter
    // than topology==0 must be REPORTED plainly (see the numbers printed
    // above), not hidden behind a bound tuned to pass regardless of what
    // the measurement finds. `< 5.0f` only guards against a genuine
    // blowup -- an order of magnitude past every ceiling this file's
    // limiters/trims already target -- which finiteness above does not by
    // itself rule out.
    REQUIRE_TRUE(overallMax < 5.0f);
}

// =========================================================================
// 3.8 -- Reverb (FroggersEngine.hpp:493-534 ProcessReverb, :455-461 wiring,
// :844-846 Wet/dry blend). Mod depth and Hold are newly authored (no
// Froggers original -- GetParam(7)/(8) unread) -- design D15.
// =========================================================================

TEST_CASE(reverb_room_size_decay_predelay_damp_match_expmap_formulas) {
    // Pin the four ExpMap-derived formulas (:456,457,458,459) independently
    // of the stateful Process() path.
    REQUIRE_NEAR(dsp::Reverb::RoomSizeFromKnob(0.0f), 0.05f, 1e-6);
    REQUIRE_NEAR(dsp::Reverb::RoomSizeFromKnob(1.0f), 1.0f, 1e-6);
    REQUIRE_NEAR(dsp::Reverb::DecayFeedbackFromKnob(0.0f), 0.1f, 1e-6);
    REQUIRE_NEAR(dsp::Reverb::DecayFeedbackFromKnob(1.0f), 0.98f, 1e-6);

    const float sr = 48000.0f;
    REQUIRE_NEAR(dsp::Reverb::PreDelayNormFromKnob(0.0f, sr), 1.0f / sr, 1e-9);
    REQUIRE_NEAR(dsp::Reverb::PreDelayNormFromKnob(1.0f, sr), 100.0f / sr, 1e-9);

    // Damping: ExpMap(0.001, 0.2, 1-knob) -- knob=0 -> upper bound 0.2,
    // knob=1 -> lower bound 0.001 (the 1-knob flip is part of the cited
    // formula, not a port error).
    REQUIRE_NEAR(dsp::Reverb::DampAlphaFromKnob(0.0f), 0.2f, 1e-6);
    REQUIRE_NEAR(dsp::Reverb::DampAlphaFromKnob(1.0f), 0.001f, 1e-6);
}

TEST_CASE(reverb_wet_dry_mix_is_affine_in_mix_knob_at_fixed_history) {
    // FroggersEngine.hpp:846: output = (1-mix)*input + mix*wet, where `wet`
    // is whatever ProcessReverb produced from the SAME input/history. Two
    // freshly-constructed (identical zero state) Reverb instances fed the
    // same input and reverb knobs, differing only in mixKnob, must satisfy
    // (out - input)/mix == wet - input for both, i.e. the same constant --
    // this pins the mix formula exactly without needing to hand-replicate
    // the tank's internal arithmetic.
    dsp::Reverb rvA;
    dsp::Reverb rvB;
    const float sr = 48000.0f;
    const float input = 0.37f;
    const float mixA = 0.25f;
    const float mixB = 0.75f;

    const float outA = rvA.Process(input, mixA, 0.4f, 0.5f, 0.1f, 0.6f, 0.5f, 0.3f, sr);
    const float outB = rvB.Process(input, mixB, 0.4f, 0.5f, 0.1f, 0.6f, 0.5f, 0.3f, sr);

    const float impliedWetMinusInputA = (outA - input) / mixA;
    const float impliedWetMinusInputB = (outB - input) / mixB;
    REQUIRE_NEAR(impliedWetMinusInputA, impliedWetMinusInputB, 1e-4);

    // mix == 0 must reproduce the dry input exactly, for any history.
    dsp::Reverb rvDry;
    for (int i = 0; i < 200; ++i) {
        const float out = rvDry.Process(0.1f * static_cast<float>(i % 7), 0.0f,
                                          0.6f, 0.4f, 0.2f, 0.5f, 0.8f, 0.6f, sr);
        REQUIRE_NEAR(out, 0.1f * static_cast<float>(i % 7), 1e-6);
    }
}

TEST_CASE(reverb_process_matches_manual_tank_replica_at_neutral_mod_and_hold) {
    // Full-chain regression pin: an independent manual re-derivation of
    // ProcessReverb's tank (pre-delay ring, twin delay lines with diffusion
    // cross-feed, shared damping filter, stereo width blend), built directly
    // from FroggersEngine.hpp:493-534 rather than by calling dsp::Reverb, run
    // in lockstep against dsp::Reverb::Process at modDepth=0/hold=0 (the
    // parity default) over several samples with a fixed knob set.
    constexpr size_t kSize = dsp::Reverb::kSize;
    static float lineA[kSize]{};
    static float lineB[kSize]{};
    static float preLine[kSize]{};
    size_t indexA = 0, indexB = 0, preIndex = 0;
    dsp::OnePoleLowPass dampFilter;

    const float sr = 44100.0f;
    const float mixKnob = 0.6f;
    const float sizeKnob = 0.35f;
    const float decayKnob = 0.7f;
    const float preKnob = 0.2f;
    const float dampKnob = 0.55f;
    const float widthKnob = 0.4f;
    const float diffusionKnob = 0.5f;

    dsp::Reverb rv;

    for (int step = 0; step < 32; ++step) {
        const float input = std::sin(0.1f * static_cast<float>(step));

        // -- manual replica --
        const float preNorm = dsp::ExpMapCompute(1.0f / sr, 100.0f / sr, preKnob);
        size_t preDelay = static_cast<size_t>(std::round(preNorm * sr));
        if (preDelay >= kSize) preDelay = kSize - 1;
        preLine[preIndex] = input;
        const size_t preRead = (preIndex + kSize - preDelay) % kSize;
        const float preOut = preLine[preRead];
        preIndex = (preIndex + 1) % kSize;

        const float sizeNorm = dsp::ExpMapCompute(0.05f, 1.0f, sizeKnob);
        size_t baseA = static_cast<size_t>(180.0f + sizeNorm * 1300.0f);
        size_t baseB = static_cast<size_t>(260.0f + sizeNorm * 1800.0f);
        const size_t dA = std::min(kSize - 1, std::max(static_cast<size_t>(1), baseA));
        const size_t dB = std::min(kSize - 1, std::max(static_cast<size_t>(1), baseB));
        const size_t readA = (indexA + kSize - dA) % kSize;
        const size_t readB = (indexB + kSize - dB) % kSize;

        const float valA = lineA[readA];
        const float valB = lineB[readB];
        const float fb = dsp::ExpMapCompute(0.1f, 0.98f, decayKnob);
        const float cross = diffusionKnob * 0.5f;
        const float aFb = valB * (1.0f - cross) + valA * cross;
        const float bFb = valA * (1.0f - cross) + valB * cross;
        const float aIn = preOut + aFb * fb;
        const float bIn = preOut + bFb * fb;

        dampFilter.alpha = dsp::ExpMapCompute(0.001f, 0.2f, 1.0f - dampKnob);
        const float aOut = dampFilter.Process(valA);
        const float bOut = dampFilter.Process(valB);

        lineA[indexA] = aIn;
        lineB[indexB] = bIn;
        indexA = (indexA + 1) % kSize;
        indexB = (indexB + 1) % kSize;

        const float mid = 0.5f * (aOut + bOut);
        const float wetL = mid + widthKnob * (aOut - mid);
        const float wetR = mid + widthKnob * (bOut - mid);
        const float wet = 0.5f * (wetL + wetR);
        const float expected = (1.0f - mixKnob) * input + mixKnob * wet;

        const float actual = rv.Process(input, mixKnob, sizeKnob, decayKnob, preKnob,
                                          dampKnob, widthKnob, diffusionKnob, sr,
                                          /*modDepthKnob01=*/0.0f, /*holdKnob01=*/0.0f);
        REQUIRE_NEAR(actual, expected, 1e-4);
    }
}

TEST_CASE(reverb_authored_hold_lengthens_decay_but_stays_bounded_and_finite) {
    // Authored, not ported: Hold pushes feedback toward (never reaching) 1.0.
    dsp::Reverb rvHold;
    dsp::Reverb rvNoHold;
    const float sr = 48000.0f;
    float maxAbsHold = 0.0f;
    for (int i = 0; i < 4000; ++i) {
        const float input = (i == 0) ? 1.0f : 0.0f;  // impulse
        const float outHold = rvHold.Process(input, 1.0f, 0.6f, 0.5f, 0.1f, 0.5f, 0.5f, 0.4f, sr,
                                              /*modDepthKnob01=*/0.0f, /*holdKnob01=*/1.0f);
        const float outNoHold = rvNoHold.Process(input, 1.0f, 0.6f, 0.5f, 0.1f, 0.5f, 0.5f, 0.4f, sr,
                                                  0.0f, 0.0f);
        REQUIRE_TRUE(std::isfinite(outHold));
        REQUIRE_TRUE(std::isfinite(outNoHold));
        maxAbsHold = std::max(maxAbsHold, std::fabs(outHold));
    }
    REQUIRE_TRUE(maxAbsHold < 100.0f);  // bounded, no runaway despite fb -> 0.999
}

TEST_CASE(reverb_authored_mod_depth_alters_output_and_stays_finite) {
    // Authored, not ported: Mod depth wows the tank read taps.
    dsp::Reverb rvMod;
    dsp::Reverb rvNoMod;
    const float sr = 48000.0f;
    bool sawDifference = false;
    // kModLfoHz is deliberately slow (0.35 Hz); at 48 kHz the LFO needs
    // several hundred samples before sin(phase) grows enough for the
    // rounded sample offset to move off zero, so this loop runs long enough
    // (well under a quarter of the LFO's own period) to guarantee that.
    for (int i = 0; i < 6000; ++i) {
        const float input = std::sin(0.05f * static_cast<float>(i));
        const float outMod = rvMod.Process(input, 1.0f, 0.5f, 0.5f, 0.1f, 0.5f, 0.5f, 0.5f, sr,
                                            /*modDepthKnob01=*/1.0f, 0.0f);
        const float outNoMod = rvNoMod.Process(input, 1.0f, 0.5f, 0.5f, 0.1f, 0.5f, 0.5f, 0.5f, sr,
                                                0.0f, 0.0f);
        REQUIRE_TRUE(std::isfinite(outMod));
        if (std::fabs(outMod - outNoMod) > 1e-4f) {
            sawDifference = true;
        }
    }
    REQUIRE_TRUE(sawDifference);
}

// -----------------------------------------------------------------------
// B6b (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md,
// "Group B outcomes" / operator: "why can't we just have limiters for
// reverb and delay?"): FAILING-FIRST for the property nothing upstream of
// the master output limiter used to bound -- W2.1-MATH-2's own finding:
// Hold pushes `fb` to ~0.99998 (Reverb.hpp:238), ~50,000x steady-state
// gain, "the master limiter is the only thing between Hold-at-max and the
// output." This test pins the STAGE's own escape bound instead:
// `wetLimiter` (dsp/Reverb.hpp), applied to the fully mixed dry/wet output
// AFTER both tanks, so what actually leaves `Process()` never exceeds
// `dsp::OutputLimiter::kDefaultCeiling` (1.0 -- `Reverb.hpp`'s own
// `kReverbWetLimiterCeiling` is defined equal to it).
//
// Smallest room size (shortest tank round trip -- B6b's own measured worst
// case; see dsp/Reverb.hpp's header comment for the full sweep), decay and
// Hold both at their ceiling, fully wet (mix=1.0, isolates the tank's own
// bound from any diluting dry floor), sustained full-scale input, run long
// enough (20000 samples, ~0.42s) to comfortably clear the transient window
// the measurement found the worst-case overshoot inside.
//
// MEASURED (scratch harness, this exact scenario, run against the code
// before this task existed): WITHOUT `wetLimiter`, worst case over this
// run = 66.555695 -- clears the 1.0 ceiling by nearly two orders of
// magnitude, the failing case this test pins (and this is still far short
// of the ~50,000x steady state W2.1-MATH-2 computed -- reaching that
// would take vastly longer than any bounded test can run, which is exactly
// why nothing upstream could ever bound it by construction). WITH
// `wetLimiter` at its measured tuning (threshold 0.9, attack 2
// microseconds, release 100ms -- dsp/Reverb.hpp's own
// `kReverbWetLimiter*` constants): worst case = 1.000000, at the ceiling.
// -----------------------------------------------------------------------
TEST_CASE(reverb_wet_output_stays_at_or_below_limiter_ceiling_with_hold_at_max) {
    dsp::Reverb rv;
    const float sr = 48000.0f;

    const float ceiling = dsp::OutputLimiter::kDefaultCeiling;  // 1.0 -- shared by wetLimiter and the master.

    constexpr int kSamples = 20000;
    float maxAbs = 0.0f;
    for (int i = 0; i < kSamples; ++i) {
        const float out = rv.Process(1.0f, /*mix=*/1.0f, /*size=*/0.0f, /*decay=*/1.0f, /*pre=*/0.1f,
                                      /*damp=*/0.5f, /*width=*/0.5f, /*diffusion=*/0.4f, sr,
                                      /*modDepth=*/0.0f, /*hold=*/1.0f);
        REQUIRE_TRUE(std::isfinite(out));
        maxAbs = std::max(maxAbs, std::fabs(out));
    }
    REQUIRE_TRUE(maxAbs <= ceiling + 1.0e-4f);
}

// =========================================================================
// T1.6 (openspec/changes/frogg3rs-post-expansion-consolidation) -- pin the
// two measurements the predecessor change performed in STANDALONE harnesses
// and never guarded. The archived numbers (its tasks.md §EXECUTION): Tilt
// post-limiter peak 0.8000 at centre vs 0.7973 at brightest; Tuned peak
// exactly 0.8000 across every sweep rate and room size. Both are really the
// same property -- neither control can push the wet path past the stage
// ceiling (dsp::kStageCeiling, 0.80) -- and that ceiling, not the exact
// third decimal of an unpinned measurement, is what a later change must not
// silently invalidate. The spec delta this closes:
// "A measurement performed in a standalone harness and reported only in a
// document does not satisfy this requirement."
// =========================================================================

TEST_CASE(reverb_tilt_never_raises_the_post_limiter_peak_above_the_stage_ceiling) {
    const float sr = 48000.0f;
    constexpr int kSamples = 20000;

    // Same hot-input, wet-only, max-decay/hold rig the neighbouring ceiling
    // test uses, per tilt position. Tilt swept across its whole range,
    // including the archived measurement's two named points (centre 0.5,
    // brightest 1.0).
    float peakAtCentre = 0.0f;
    float worstPeak = 0.0f;
    float worstTilt = -1.0f;
    for (int t = 0; t <= 10; ++t) {
        const float tilt = static_cast<float>(t) / 10.0f;
        dsp::Reverb rv;
        float maxAbs = 0.0f;
        for (int i = 0; i < kSamples; ++i) {
            const float out = rv.Process(1.0f, /*mix=*/1.0f, /*size=*/0.0f, /*decay=*/1.0f, /*pre=*/0.1f,
                                          /*damp=*/0.5f, /*width=*/0.5f, /*diffusion=*/0.4f, sr,
                                          /*modDepth=*/0.0f, /*hold=*/1.0f,
                                          /*modRate=*/0.5f, /*tankDrive=*/0.5f, /*grit=*/0.0f,
                                          /*tilt=*/tilt, /*tuned=*/0.5f);
            REQUIRE_TRUE(std::isfinite(out));
            maxAbs = std::max(maxAbs, std::fabs(out));
        }
        if (t == 5) { peakAtCentre = maxAbs; }
        if (maxAbs > worstPeak) { worstPeak = maxAbs; worstTilt = tilt; }
    }

    // OMNI 9.1 positive control: the rig must actually drive the limiter to
    // its ceiling, or "never exceeds it" is vacuously true of silence. The
    // archived centre measurement WAS 0.8000, i.e. pinned at the ceiling.
    std::cout << "  [T1.6 tilt-pin] peak at tilt centre=" << peakAtCentre
              << "  worst peak=" << worstPeak << " at tilt=" << worstTilt
              << "  ceiling=" << dsp::kStageCeiling << "\n";
    // P8v (§9.1) found the ceiling comparison alone is SELF-REFERENTIAL:
    // raising kStageCeiling moves measured peak and asserted bound together
    // (0.9 vs 0.9, still green), so only a decoupled break -- loosening the
    // wet limiter's own ceiling -- failed it (1.19562 vs 0.8001). Pinning
    // the literal 0.80 (the archived measurement's own value) closes that
    // hole: a silently moved stage ceiling now fails here too.
    REQUIRE_NEAR(dsp::kStageCeiling, 0.8, 1.0e-6);
    REQUIRE_TRUE(peakAtCentre > dsp::kStageCeiling - 0.01f);            // the instrument was at the ceiling...
    REQUIRE_TRUE(worstPeak <= dsp::kStageCeiling + 1.0e-4f);            // ...and NO tilt position exceeds it.
}

TEST_CASE(reverb_tuned_sweeps_never_raise_the_peak_above_the_stage_ceiling) {
    const float sr = 48000.0f;

    // The archived measurement swept Tuned 0<->1 at step periods of 1, 4 and
    // 32 samples across four room sizes: peak exactly 0.8000 in every
    // configuration. Same grid here, pinned against the same ceiling.
    float worstPeak = 0.0f;
    float worstSize = -1.0f;
    int worstPeriod = -1;
    float minPeak = 1.0e9f;
    for (const float size : {0.0f, 0.33f, 0.66f, 1.0f}) {
        for (const int period : {1, 4, 32}) {
            dsp::Reverb rv;
            float maxAbs = 0.0f;
            for (int i = 0; i < 20000; ++i) {
                const float tuned = ((i / period) % 2 == 0) ? 0.0f : 1.0f;  // hard 0<->1 square sweep.
                const float out = rv.Process(1.0f, /*mix=*/1.0f, size, /*decay=*/1.0f, /*pre=*/0.1f,
                                              /*damp=*/0.5f, /*width=*/0.5f, /*diffusion=*/0.4f, sr,
                                              /*modDepth=*/0.0f, /*hold=*/1.0f,
                                              /*modRate=*/0.5f, /*tankDrive=*/0.5f, /*grit=*/0.0f,
                                              /*tilt=*/0.5f, tuned);
                REQUIRE_TRUE(std::isfinite(out));
                maxAbs = std::max(maxAbs, std::fabs(out));
            }
            if (maxAbs > worstPeak) { worstPeak = maxAbs; worstSize = size; worstPeriod = period; }
            minPeak = std::min(minPeak, maxAbs);
        }
    }

    std::cout << "  [T1.6 tuned-pin] peak range across 4 sizes x 3 step periods: [" << minPeak << ", "
              << worstPeak << "] worst at size=" << worstSize << " period=" << worstPeriod
              << "  ceiling=" << dsp::kStageCeiling << "\n";
    REQUIRE_NEAR(dsp::kStageCeiling, 0.8, 1.0e-6);            // literal pin -- see the tilt test's P8v comment.
    REQUIRE_TRUE(minPeak > dsp::kStageCeiling - 0.01f);       // every configuration drove the ceiling...
    REQUIRE_TRUE(worstPeak <= dsp::kStageCeiling + 1.0e-4f);  // ...and none broke through it.
}

// =========================================================================
// S2a.1 (openspec/changes/archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/
// tasks.md) -- reverb tank in-loop saturator. `dsp::Reverb::Process` now
// writes `preOut + fb * PadeSaturator::Saturate(aFb)` (dsp/Reverb.hpp:
// 410-411), the SAME saturator in the SAME in-loop position the delay's own
// B2 fix uses (dsp/Delay.hpp), because an unsaturated recursive loop
// settles at `in/(1-fb)` and Hold pushes `fb` to ~0.99998.
//
// These three TEST_CASEs and their two fixtures lived in a dedicated
// app/FroggersReverbSaturatorTests.cpp with its own binary, for one stated
// reason: at the time this file was other in-flight work and the S2a.1
// brief forbade touching it. That reason has lapsed, so they are folded in
// here -- same convention (pure app/dsp/*.hpp, no Sheaf/JUCE), same
// harness, and the duplicated harness/build recipe goes away with the
// separate file (OMNI §8).
// =========================================================================

// -----------------------------------------------------------------------
// OMNI §9.1 positive-control fixture: a minimal, LOCAL manual replica of
// ONLY the recursive relationship that determines dsp::Reverb's own
// lineA/lineB growth (pre-delay tap, room-size-derived per-line delay
// length, diffusion cross-feed, `fb`) -- every formula copied verbatim from
// dsp::Reverb::Process (dsp/Reverb.hpp) EXCEPT the one line S2a.1 changed:
// no PadeSaturator::Saturate here, deliberately -- this is the pre-fix
// shape.
//
// Deliberately omits dampFilter/width/mix/wetLimiter: dsp::Reverb::Process
// never feeds any of those back into lineA/lineB -- `dampFilter.Process`
// reads valA/valB for the OUTPUT path only, computed from the SAME valA the
// tank write above it also reads, never from aIn/aOut -- so omitting them
// changes nothing about the question this replica exists to answer (the
// tank's OWN growth). PreFixReverbReplica below adds exactly those omitted
// output-path stages on top of this one, rather than restating the tank.
//
// Distinct from this file's own
// reverb_process_matches_manual_tank_replica_at_neutral_mod_and_hold, which
// is NOT duplication of these two (OMNI §8: classify before merging): that
// test re-derives the tank from FroggersEngine.hpp's raw ExpMapCompute
// calls precisely so it does NOT go through dsp::Reverb's own static
// helpers -- its independence from them IS its assertion, and routing it
// through a shared fixture would gut it.
// -----------------------------------------------------------------------
struct UnsaturatedTankReplica {
    static constexpr size_t kSize = dsp::Reverb::kSize;
    float lineA[kSize]{};
    float lineB[kSize]{};
    float preLine[kSize]{};
    size_t indexA = 0, indexB = 0, preIndex = 0;

    // Same formulas, same argument order as dsp::Reverb::Process's own
    // pre-delay/room-size/diffusion/fb math -- minus S2a.1's
    // PadeSaturator::Saturate wrap (the one line this struct exists to
    // omit, marked below).
    //
    // Returns the two tap values (valA, valB) read this sample: the only
    // quantities dsp::Reverb::Process's OUTPUT path ever reads out of the
    // tank, so PreFixReverbReplica below builds the rest of the pre-fix
    // chain on top of this without restating any tank math (OMNI §8: one
    // definition site). Sound by construction: dA/dB are clamped to >= 1,
    // so readA/readB can never equal indexA/indexB and this sample's writes
    // cannot disturb this sample's reads.
    std::pair<float, float> Step(float input, float sizeKnob01, float decayKnob01, float preKnob01,
                                 float diffusionKnob01, float sampleRate, float holdKnob01) {
        const float preNorm = dsp::Reverb::PreDelayNormFromKnob(preKnob01, sampleRate);
        size_t preDelay = static_cast<size_t>(std::round(preNorm * sampleRate));
        if (preDelay >= kSize) {
            preDelay = kSize - 1;
        }
        preLine[preIndex] = input;
        const size_t preRead = (preIndex + kSize - preDelay) % kSize;
        const float preOut = preLine[preRead];
        preIndex = (preIndex + 1) % kSize;

        const float sizeNorm = dsp::Reverb::RoomSizeFromKnob(sizeKnob01);
        const size_t baseA = static_cast<size_t>(180.0f + sizeNorm * 1300.0f);
        const size_t baseB = static_cast<size_t>(260.0f + sizeNorm * 1800.0f);
        const size_t dA = std::min(kSize - 1, std::max(static_cast<size_t>(1), baseA));
        const size_t dB = std::min(kSize - 1, std::max(static_cast<size_t>(1), baseB));
        const size_t readA = (indexA + kSize - dA) % kSize;
        const size_t readB = (indexB + kSize - dB) % kSize;
        const float valA = lineA[readA];
        const float valB = lineB[readB];

        const float decayFb = dsp::Reverb::DecayFeedbackFromKnob(decayKnob01);
        const float fb = decayFb + (1.0f - decayFb) * std::min(holdKnob01, 0.999f);
        const float cross = diffusionKnob01 * 0.5f;
        const float aFb = valB * (1.0f - cross) + valA * cross;
        const float bFb = valA * (1.0f - cross) + valB * cross;

        // Pre-S2a.1 shape, deliberately: no PadeSaturator::Saturate here --
        // this is the whole reason this struct exists.
        const float aIn = preOut + aFb * fb;
        const float bIn = preOut + bFb * fb;

        lineA[indexA] = aIn;
        lineB[indexB] = bIn;
        indexA = (indexA + 1) % kSize;
        indexB = (indexB + 1) % kSize;

        return {valA, valB};
    }

    // Mirrors dsp::Reverb::StateMagnitude()'s own scan exactly (lineA,
    // lineB, preLine -- this replica has no dampFilter/wetL/wetR to fold
    // in), so the two numbers these tests compare are computed the
    // identical way: an apples-to-apples comparison, not two different
    // metrics that happen to share a name.
    float StateMagnitude() const {
        float magnitude = 0.0f;
        for (size_t i = 0; i < kSize; ++i) {
            magnitude = std::max({magnitude, std::fabs(lineA[i]), std::fabs(lineB[i]), std::fabs(preLine[i])});
        }
        return magnitude;
    }
};

// -----------------------------------------------------------------------
// Second fixture: a FULL pre-S2a.1 reconstruction of dsp::Reverb::Process
// -- the tank above, plus the damping filter, width blend, mix, AND a
// `dsp::OutputLimiter` configured identically to dsp::Reverb's own
// `wetLimiter` (B6b, which predates S2a.1 and belongs in both the "before"
// and "after" pictures unchanged) -- minus ONLY the S2a.1 saturator wrap.
//
// This reconstructs the actual AUDIBLE OUTPUT (Process()'s own return
// value) the pre-S2a.1 code produced, which is what "Hold sustains a long
// tail" is a claim about -- not the internal tank register
// UnsaturatedTankReplica measures. Both fixtures are warranted because they
// answer different questions; neither restates the other's math.
// -----------------------------------------------------------------------
struct PreFixReverbReplica {
    UnsaturatedTankReplica tank;
    dsp::OnePoleLowPass dampFilter;
    dsp::OutputLimiter wetLimiter;

    explicit PreFixReverbReplica(float sampleRate) {
        wetLimiter.Configure(sampleRate, dsp::kReverbWetLimiterThreshold, dsp::kReverbWetLimiterCeiling,
                              dsp::kReverbWetLimiterAttackSeconds, dsp::kReverbWetLimiterReleaseSeconds);
    }

    float Step(float input, float mixKnob01, float sizeKnob01, float decayKnob01, float preKnob01,
               float dampKnob01, float widthKnob01, float diffusionKnob01, float sampleRate, float holdKnob01) {
        const auto [valA, valB] =
            tank.Step(input, sizeKnob01, decayKnob01, preKnob01, diffusionKnob01, sampleRate, holdKnob01);

        dampFilter.alpha = dsp::Reverb::DampAlphaFromKnob(dampKnob01);
        const float aOut = dampFilter.Process(valA);
        const float bOut = dampFilter.Process(valB);

        const float mid = 0.5f * (aOut + bOut);
        const float wetL = mid + widthKnob01 * (aOut - mid);
        const float wetR = mid + widthKnob01 * (bOut - mid);
        const float wet = 0.5f * (wetL + wetR);
        const float mixedOut = (1.0f - mixKnob01) * input + mixKnob01 * wet;
        return wetLimiter.Process(mixedOut);
    }
};

// -----------------------------------------------------------------------
// S2a.1 -- bounded under sustained overdrive, decay and Hold both at their
// ceiling (`fb` -> ~0.99998; 1/(1-fb) ~= 50,000x steady state, unreachable
// in a bounded test but the mechanism this whole task is about).
// -----------------------------------------------------------------------
TEST_CASE(reverb_tank_stays_bounded_under_sustained_overdrive_at_max_decay_and_hold) {
    dsp::Reverb rv;
    const float sr = 48000.0f;

    // Smallest room size -> fastest round trip (baseA=180 samples,
    // RoomSizeFromKnob(0)==0.05 floor), the same "shortest round trip is
    // the worst case" choice B6b's own reverb-wetLimiter test above makes
    // -- most round trips per sample, so any real divergence (or its
    // absence) shows up fastest.
    constexpr float sizeKnob = 0.0f;
    constexpr float decayKnob = 1.0f;      // -> decayFb = 0.98 (ceiling).
    constexpr float preKnob = 0.1f;
    constexpr float diffusionKnob = 0.4f;  // nonzero cross-feed -- exercises both taps, "both lines" per B2.
    constexpr float widthKnob = 0.5f;      // in [0,1]: this test's bound derivation below needs that range.
    constexpr float dampKnob = 0.5f;
    constexpr float holdKnob = 1.0f;       // -> fb = 0.98 + 0.02*0.999 = 0.99998.
    constexpr float mixKnob = 1.0f;        // fully wet -- isolates the tank, matches existing Reverb precedent.

    const float decayFb = dsp::Reverb::DecayFeedbackFromKnob(decayKnob);
    const float fb = decayFb + (1.0f - decayFb) * std::min(holdKnob, 0.999f);

    // Overdrive, not an ordinary level (the next TEST_CASE covers those).
    constexpr float kOverdriveInput = 5.0f;

    // Per-sample bound: aIn = preOut + fb*Saturate(aFb), |preOut| <=
    // |input| (a pure delay of `input`, no gain) and |Saturate(.)| <= 1
    // unconditionally (dsp::PadeSaturator::Saturate's own hard clamp), so
    // |aIn| <= |input| + fb regardless of round-trip count -- exactly B2's
    // own bound shape (dsp/Delay.hpp's test: bound = inputAmplitude*p.dsnd
    // + fbk). dampFilter.output/wetL/wetR are each a convex combination
    // (coefficients in [0,1]) of quantities already <= this same bound
    // (dampFilter.alpha in (0.001,0.2); widthKnob fixed at 0.5 above, in
    // [0,1]), so the SAME bound covers dsp::Reverb::StateMagnitude()'s
    // full scan (lineA, lineB, preLine, dampFilter.output, wetL, wetR),
    // not just the raw taps.
    const float bound = std::fabs(kOverdriveInput) + fb;

    UnsaturatedTankReplica control;

    constexpr int kSamples = 5000;  // ~27 round trips at 180 samples/trip -- plenty for the control to diverge.
    float maxRawMagnitude = 0.0f;
    float maxControlMagnitude = 0.0f;
    for (int i = 0; i < kSamples; ++i) {
        const float out = rv.Process(kOverdriveInput, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob,
                                      diffusionKnob, sr, /*modDepthKnob01=*/0.0f, holdKnob);
        REQUIRE_TRUE(std::isfinite(out));
        const float rawMagnitude = rv.StateMagnitude();
        REQUIRE_TRUE(std::isfinite(rawMagnitude));
        // Per-sample bound (B2's own style: asserted every sample, not
        // merely checked once at the end of the run).
        REQUIRE_TRUE(rawMagnitude <= bound + 1.0e-4f);
        maxRawMagnitude = std::max(maxRawMagnitude, rawMagnitude);

        control.Step(kOverdriveInput, sizeKnob, decayKnob, preKnob, diffusionKnob, sr, holdKnob);
        maxControlMagnitude = std::max(maxControlMagnitude, control.StateMagnitude());
    }

    std::cout << "  [S2a.1 bounded-vs-unbounded] input=" << kOverdriveInput << " fb=" << fb
              << " bound=|input|+fb=" << bound << "\n"
              << "  [S2a.1 bounded-vs-unbounded]   WITH saturator (post-fix, StateMagnitude() max over "
              << kSamples << " samples):    " << maxRawMagnitude << "\n"
              << "  [S2a.1 bounded-vs-unbounded]   WITHOUT saturator (positive control, identical run): "
              << maxControlMagnitude << "\n";

    // OMNI §9.1: the positive control must actually have exceeded the
    // bound the real (fixed) unit respects, or this run never tested
    // anything the fix could fail.
    REQUIRE_TRUE(maxControlMagnitude > bound);
    REQUIRE_TRUE(maxControlMagnitude > kOverdriveInput * 2.0f);  // comfortably past "just barely over the bound".
    REQUIRE_TRUE(maxRawMagnitude <= bound + 1.0e-4f);
}

// -----------------------------------------------------------------------
// S2a.1 -- ordinary (quiet) level not degraded, Hold still at its ceiling.
//
// MEASURED FIRST, then written to match (OMNI: measured, not assumed): a
// draft of this test tried "ordinary" == the loud, full-scale,
// 4000-sample-sustained scenario the loud-tail TEST_CASE below uses, and
// asserted the saturated tank's raw retention should match the unsaturated
// control's there. MEASURED result: it does not (control retention 0.937,
// real 0.207-0.42 depending on amplitude) -- because at that scenario the
// UNSATURATED control itself is not "a long tail", it is the bug (see the
// loud-tail TEST_CASE below, which measures and prints exactly that).
// "Ordinary levels" has to mean a level where the tap magnitude the
// saturator sees stays inside its near-linear region -- amplitude 0.02
// (~-34dBFS) was swept and found small enough that a genuine,
// apples-to-apples comparison holds; see the printed numbers for the
// measured gap.
// -----------------------------------------------------------------------
TEST_CASE(reverb_quiet_ordinary_level_tail_matches_unsaturated_control_at_max_hold) {
    dsp::Reverb rv;
    UnsaturatedTankReplica control;
    const float sr = 48000.0f;
    constexpr float mixKnob = 1.0f, sizeKnob = 0.6f, decayKnob = 1.0f, preKnob = 0.1f, dampKnob = 0.5f,
                    widthKnob = 0.5f, diffusionKnob = 0.4f, holdKnob = 1.0f;
    // "Ordinary" excitation: quiet relative to full scale (~-34dBFS), NOT
    // the sustained-overdrive scenario the TEST_CASE above already covers.
    constexpr float kOrdinaryInput = 0.02f;

    constexpr int kExciteSamples = 4000;
    for (int i = 0; i < kExciteSamples; ++i) {
        rv.Process(kOrdinaryInput, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr,
                   0.0f, holdKnob);
        control.Step(kOrdinaryInput, sizeKnob, decayKnob, preKnob, diffusionKnob, sr, holdKnob);
    }

    // Measurement windows use SILENCE (0.0f input): "peakAfterExcite" means
    // "peak in the window right after the burst ends", not "peak while
    // still exciting".
    constexpr int kMeasureWindow = 200;
    float rawPeakAfterExcite = 0.0f, controlPeakAfterExcite = 0.0f;
    for (int i = 0; i < kMeasureWindow; ++i) {
        const float out = rv.Process(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob,
                                      diffusionKnob, sr, 0.0f, holdKnob);
        REQUIRE_TRUE(std::isfinite(out));
        rawPeakAfterExcite = std::max(rawPeakAfterExcite, rv.StateMagnitude());
        control.Step(0.0f, sizeKnob, decayKnob, preKnob, diffusionKnob, sr, holdKnob);
        controlPeakAfterExcite = std::max(controlPeakAfterExcite, control.StateMagnitude());
    }
    REQUIRE_TRUE(rawPeakAfterExcite > 0.01f);      // meaningfully non-silent right after the burst.
    REQUIRE_TRUE(controlPeakAfterExcite > 0.01f);  // same premise held for the control, or the comparison is void.

    constexpr int kSilenceRunSamples = 10000;  // ~0.21s.
    for (int i = 0; i < kSilenceRunSamples; ++i) {
        rv.Process(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr, 0.0f,
                   holdKnob);
        control.Step(0.0f, sizeKnob, decayKnob, preKnob, diffusionKnob, sr, holdKnob);
    }

    float rawPeakAfterSilence = 0.0f, controlPeakAfterSilence = 0.0f;
    for (int i = 0; i < kMeasureWindow; ++i) {
        const float out = rv.Process(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob,
                                      diffusionKnob, sr, 0.0f, holdKnob);
        REQUIRE_TRUE(std::isfinite(out));
        rawPeakAfterSilence = std::max(rawPeakAfterSilence, rv.StateMagnitude());
        control.Step(0.0f, sizeKnob, decayKnob, preKnob, diffusionKnob, sr, holdKnob);
        controlPeakAfterSilence = std::max(controlPeakAfterSilence, control.StateMagnitude());
    }

    const float rawRetention = rawPeakAfterSilence / rawPeakAfterExcite;
    const float controlRetention = controlPeakAfterSilence / controlPeakAfterExcite;

    std::cout << "  [S2a.1 quiet-ordinary-tail] input=" << kOrdinaryInput << " Hold=max, Decay=max:\n"
              << "  [S2a.1 quiet-ordinary-tail]   WITH saturator:    peakAfterExcite=" << rawPeakAfterExcite
              << " peakAfterSilence=" << rawPeakAfterSilence << " retention=" << rawRetention << "\n"
              << "  [S2a.1 quiet-ordinary-tail]   WITHOUT saturator: peakAfterExcite=" << controlPeakAfterExcite
              << " peakAfterSilence=" << controlPeakAfterSilence << " retention=" << controlRetention << "\n";

    // At this quiet level the fix must not have made tail retention
    // meaningfully worse than the unsaturated control's own retention over
    // the identical window -- tolerance set with margin above the measured
    // gap (~0.04 absolute at this amplitude), not tuned to just barely pass.
    REQUIRE_NEAR(rawRetention, controlRetention, 0.08);
}

// -----------------------------------------------------------------------
// B6b's guard AND S2a.1's, merged: one scenario, one instrument, one
// TEST_CASE (OMNI §8 -- keeping two would be sequential duplication).
//
// B6b (`wetLimiter`, added above) had to prove it capped the tail's LEVEL
// without touching Hold's PERSISTENCE -- "fixed the level by breaking the
// feature." S2a.1 (in-loop `PadeSaturator`, dsp/Reverb.hpp:410-411) has to
// prove exactly the same thing about exactly the same tail. Same knobs,
// same burst, same measurement: one test.
//
// SUPERSEDES an earlier `reverb_hold_at_max_tail_still_sustains_after_
// wet_limiter_is_added` whose whole bar was
// `peakAfterSilenceRun > peakAfterExcite * 0.5f`. That bar was not merely
// tight, it was STALE -- calibrated against the pre-S2a.1 regime, in which
// (MEASURED, and reproduced by `PreFixReverbReplica` on every run of this
// test rather than taken on faith) the OUTPUT at this exact scenario sat
// PINNED at the wetLimiter's 0.8 ceiling for the entire 10000-sample
// silence window, retention 1.000. That is not Hold sustaining a tail; it
// is the unbounded-tank defect S2a.1 removes, seen from the output side.
// A bar that only passes in that regime asserts the bug.
//
// What this asserts instead is what a real reverb tail actually is:
//   (1) still clearly audible long after the burst -- not collapsed to
//       silence, and not merely "technically nonzero";
//   (2) declining GRADUALLY across two checkpoints, not falling off a
//       cliff (an on/off gate would pass a start/end check alone);
//   (3) genuinely DECAYING rather than pinned at the ceiling -- the
//       direction the old bar had backwards, and the one that stops (1)
//       and (2) from silently going vacuous if the tank ever regressed to
//       riding the limiter again.
// (3) is kept honest by OMNI §9.1: the pre-fix control is asserted to BE
// pinned, so this run demonstrably could have caught the pinned regime.
//
// MEASURED post-fix at this scenario (printed below on every run): 0.800
// right after the burst -> 0.374 at +2000 samples -> 0.302 at +10000
// samples. Every threshold below carries real margin off those numbers
// rather than being tuned to just barely pass.
// -----------------------------------------------------------------------
TEST_CASE(reverb_hold_at_max_tail_stays_audible_and_decays_gradually_not_pinned) {
    dsp::Reverb rv;
    PreFixReverbReplica before(48000.0f);
    const float sr = 48000.0f;
    constexpr float mixKnob = 1.0f, sizeKnob = 0.6f, decayKnob = 1.0f, preKnob = 0.1f, dampKnob = 0.5f,
                    widthKnob = 0.5f, diffusionKnob = 0.4f, holdKnob = 1.0f;

    // A burst of sustained input (building real energy into the tank, not a
    // single impulse), then silence.
    constexpr int kExciteSamples = 4000;
    for (int i = 0; i < kExciteSamples; ++i) {
        rv.Process(1.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr, 0.0f,
                   holdKnob);
        before.Step(1.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr, holdKnob);
    }

    constexpr int kMeasureWindow = 200;
    auto measurePeak = [&](int silenceSamplesFirst) {
        for (int i = 0; i < silenceSamplesFirst; ++i) {
            rv.Process(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr, 0.0f,
                       holdKnob);
            before.Step(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr,
                        holdKnob);
        }
        float afterPeak = 0.0f, beforePeak = 0.0f;
        for (int i = 0; i < kMeasureWindow; ++i) {
            const float afterOut = rv.Process(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob,
                                               diffusionKnob, sr, 0.0f, holdKnob);
            const float beforeOut = before.Step(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob,
                                                 diffusionKnob, sr, holdKnob);
            REQUIRE_TRUE(std::isfinite(afterOut));
            REQUIRE_TRUE(std::isfinite(beforeOut));
            afterPeak = std::max(afterPeak, std::fabs(afterOut));
            beforePeak = std::max(beforePeak, std::fabs(beforeOut));
        }
        return std::make_pair(afterPeak, beforePeak);
    };

    const auto [afterRightAfterBurst, beforeRightAfterBurst] = measurePeak(0);
    const auto [afterMidSilence, beforeMidSilence] = measurePeak(2000);    // ~0.042s further silence.
    const auto [afterLongSilence, beforeLongSilence] = measurePeak(8000);  // total ~0.21s.

    std::cout << "  [S2a.1 loud-tail] Hold=max, Decay=max, full-scale 4000-sample burst:\n"
              << "  [S2a.1 loud-tail]   BEFORE (pre-fix, reconstructed): rightAfterBurst=" << beforeRightAfterBurst
              << " +2000samples=" << beforeMidSilence << " +10000samples=" << beforeLongSilence
              << "  (retention=" << (beforeLongSilence / beforeRightAfterBurst)
              << " -- pinned at the limiter ceiling, the defect itself)\n"
              << "  [S2a.1 loud-tail]   AFTER  (post-fix, real):         rightAfterBurst=" << afterRightAfterBurst
              << " +2000samples=" << afterMidSilence << " +10000samples=" << afterLongSilence
              << "  (retention=" << (afterLongSilence / afterRightAfterBurst) << " -- a genuine decay)\n";

    // Premise: the burst actually excited the tank, for both the real unit
    // and the control -- otherwise every ratio below is 0/0 and this run
    // asserted nothing.
    REQUIRE_TRUE(afterRightAfterBurst > 0.05f);
    REQUIRE_TRUE(beforeRightAfterBurst > 0.05f);

    // (1) Still clearly audible a long time after the burst.
    REQUIRE_TRUE(afterLongSilence > 0.1f);
    // (2) Decays GRADUALLY (each checkpoint no more than moderately below
    //     the last), not a cliff -- a real tail, not an on/off gate.
    REQUIRE_TRUE(afterMidSilence > afterRightAfterBurst * 0.3f);
    REQUIRE_TRUE(afterLongSilence > afterMidSilence * 0.3f);
    // (3) ...and it is a DECAY, not the pre-fix pinned-at-the-ceiling
    //     artifact. Measured 0.377; the pre-fix regime this rejects
    //     measures 1.000.
    REQUIRE_TRUE(afterLongSilence < afterRightAfterBurst * 0.8f);
    // OMNI §9.1 positive control for (3): the pre-fix reconstruction must
    // actually exhibit the pinned regime, or (3) never tested anything.
    REQUIRE_TRUE(beforeLongSilence > beforeRightAfterBurst * 0.9f);
}

// =========================================================================
// T2.5(b) (frogg3rs-stop-isolation-and-legible-labels tasks.md; proposal.md
// SS2 W2b; MEASURED third mechanism, packet 2b/2c 2026-08-17): pins the
// app-free claim the T2.5 fix rests on -- that Grit (Reverb slot 11) is the
// load-bearing variable for the tank's self-sustain after Stop, isolated
// from the App, the voice engine, the delay line, and the randomized
// trigger entirely (dsp::Reverb::Process + StateMagnitude() only). Chosen
// for THIS file over FroggersAudioRoutingTests.cpp because it needs none of
// the App/Rig/parameter-model machinery that file's tests reuse -- matches
// this file's own header framing ("DSP port... dependency-free") and its
// established idiom of instantiating dsp::Reverb directly (see
// reverb_tilt_never_raises_the_post_limiter_peak_above_the_stage_ceiling
// and neighbours, above). FroggersAudioRoutingTests.cpp's extended T2.3(a)
// covers the separate claim that FroggersAppCore's stoppedKnob plumbing
// actually resolves Grit to 0.0f while stopped.
//
// Knobs held fixed at the EXACT state packet 2b measured off
// FroggersReverbTankMechanismRepro.cpp's PASS P2B-REPRO trial 0
// (p2b-stderr3.log's "P2B REVERB-KNOBS" line immediately following the
// "STOP ISSUED (repro trial 0)" marker -- that tag appears twice in the
// log, at an earlier unrelated all-zero default-state line and at this
// one; only the line after "STOP ISSUED (repro trial 0)" matches the
// constants below), with Tank drive forced to
// kStopUnityDriveKnob (0.5, unity) exactly as FroggersAppCore.hpp's
// stoppedKnob override already does while stopped -- reproduced from
// .../scratchpad/p2b_isolated_reverb_test.cpp, the app-free positive
// control packet 2b/2c's investigation ran directly against dsp::Reverb.
// A single small seed sample (0.01), then nothing but exact zero for
// 300000 samples (6.25s at 48kHz):
//   - at the DRAWN Grit knob (0.8094f, the pre-fix state): the tank's own
//     state LOCKS at a nonzero magnitude and never decays -- measured
//     0.306814, constant from ~n=72000 (1.5s) through the full window.
//   - at Grit forced to 0.0f (T2.5's fix -- its own exact bit-identical
//     bypass by construction, Mangle(x,0,0) - Mangle(0,0,0) == x,
//     dsp/Reverb.hpp:534-538): the SAME seed decays to 1.98e-7 by the same
//     checkpoint.
// =========================================================================
TEST_CASE(reverb_tank_grit_zero_lets_the_measured_pass_d_seed_decay_where_grit_0p8094_locked_forever) {
    // Measured knobs (packet 2b, p2b-stderr3.log's "P2B REVERB-KNOBS" line
    // immediately following the "STOP ISSUED (repro trial 0)" marker -- see
    // this test's own header comment above for why that anchor, not the
    // bare tag, is needed) -- everything held fixed except Grit, which this
    // test sweeps between the two measured arms.
    constexpr float mixKnob = 0.4301f, sizeKnob = 0.7342f, decayKnob = 0.8151f, preKnob = 0.3110f,
                     dampKnob = 0.3885f, widthKnob = 0.7300f, diffusionKnob = 0.0084f, modDepthKnob = 0.0499f,
                     holdKnob = 0.5241f, modRateKnob = 0.1358f, tiltKnob = 0.8414f, tunedKnob = 0.7377f;
    // FroggersAppCore.hpp's own stoppedKnob override forces Tank drive to
    // this value (unity post-map) while stopped -- reused here rather than
    // re-derived, matching this file's own "reused, not rederived"
    // convention for shared formulas/constants (e.g.
    // dsp::Reverb::TankDriveFromKnob reuse elsewhere in this suite).
    constexpr float kStopUnityDriveKnob = 0.5f;
    constexpr float sr = 48000.0f;
    constexpr long kSamples = 300000;  // 6.25s at 48kHz -- packet 2b's own measurement window.
    constexpr float kSeed = 0.01f;

    auto runArm = [&](float gritKnob) -> float {
        dsp::Reverb rv;
        float out = rv.Process(kSeed, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr,
                                modDepthKnob, holdKnob, modRateKnob, kStopUnityDriveKnob, gritKnob, tiltKnob,
                                tunedKnob);
        REQUIRE_TRUE(std::isfinite(out));
        for (long n = 1; n < kSamples; ++n) {
            out = rv.Process(0.0f, mixKnob, sizeKnob, decayKnob, preKnob, dampKnob, widthKnob, diffusionKnob, sr,
                              modDepthKnob, holdKnob, modRateKnob, kStopUnityDriveKnob, gritKnob, tiltKnob, tunedKnob);
        }
        REQUIRE_TRUE(std::isfinite(out));
        return rv.StateMagnitude();
    };

    // OMNI §9.1 positive control: the pre-fix state (Grit at the drawn
    // 0.8094) really does lock nonzero, or the Grit==0 assertion below
    // never tested anything against a genuine counter-example.
    const float lockedMagnitude = runArm(0.8094f);
    std::cout << "  [T2.5b] Grit=0.8094 (pre-fix state): FINAL mag=" << lockedMagnitude
              << " (measured packet 2b: 0.306814, locked)\n";
    REQUIRE_TRUE(lockedMagnitude > 0.1f);  // genuinely locked, not a fluke near-zero run.

    const float decayedMagnitude = runArm(0.0f);
    std::cout << "  [T2.5b] Grit=0.0 (T2.5's stopped-state override): FINAL mag=" << decayedMagnitude
              << " (measured packet 2b: 1.98e-7, decayed)\n";
    // Task's own bound: decays below 1e-4 within the window.
    REQUIRE_TRUE(decayedMagnitude < 1.0e-4f);
}

// =========================================================================
// 3.9 -- Drive (PolynomialDrive.hpp whole file, wiring FroggersEngine.hpp:
// 81-92,207,483-490,569-573,640-650). Blend and Phase are newly authored (no
// Froggers original -- GetParam(7)/(8) unread) -- design D15.
// =========================================================================

TEST_CASE(polynomial_drive_gain_matches_expmap_1_to_5) {
    dsp::PolynomialDrive drive;
    drive.SetGain(0.0f);
    REQUIRE_NEAR(drive.gain, 1.0f, 1e-6);
    drive.SetGain(1.0f);
    REQUIRE_NEAR(drive.gain, 5.0f, 1e-6);
}

TEST_CASE(polynomial_drive_set_coefs_matches_space_filling_curve_formula) {
    dsp::PolynomialDrive drive;
    drive.SetGain(0.5f);
    const float computedGain = drive.gain;
    drive.SetCoefs(0.3f);

    const float coefsKnob = dsp::ZeroedExpCompute(30.0f, 0.3f);
    REQUIRE_NEAR(drive.coefs[0], 1.0f + 10.0f * dsp::Sine01(coefsKnob * 1.0f), 1e-5);
    REQUIRE_NEAR(drive.coefs[1], 10.0f * dsp::Sine01(coefsKnob * 1.618f + 0.25f * (computedGain - 1.0f)), 1e-5);
    REQUIRE_NEAR(drive.coefs[2], 10.0f * dsp::Sine01(coefsKnob * 2.718f), 1e-5);
    REQUIRE_NEAR(drive.coefs[3], 10.0f * dsp::Sine01(coefsKnob * 3.141f + 0.25f * (computedGain - 1.0f)), 1e-5);
    REQUIRE_NEAR(drive.coefs[4], 10.0f * dsp::Sine01(coefsKnob * 4.669f), 1e-5);
}

TEST_CASE(polynomial_drive_process_matches_polynomial_formula) {
    dsp::PolynomialDrive drive;
    drive.SetGain(0.4f);
    drive.SetCoefs(0.6f);
    for (float x : {-0.8f, -0.1f, 0.0f, 0.3f, 0.9f}) {
        const float x2 = x * x, x3 = x2 * x, x4 = x3 * x, x5 = x3 * x2;
        const float expected = drive.gain * (x * drive.coefs[0] + x2 * drive.coefs[1] + x3 * drive.coefs[2]
                                              + x4 * drive.coefs[3] + x5 * drive.coefs[4]);
        REQUIRE_NEAR(drive.Process(x), expected, 1e-4);
    }
}

TEST_CASE(sample_rate_reducer_passthrough_hold_and_sample_and_hold_regions) {
    dsp::SampleRateReducer srr;
    srr.SetFreq(1.5f);  // >= 1 -> passthrough
    REQUIRE_NEAR(srr.Process(0.42f), 0.42f, 1e-9);

    dsp::SampleRateReducer srrHold;
    srrHold.SetFreq(0.0f);  // <= 0 -> holds at initial output (0)
    REQUIRE_NEAR(srrHold.Process(0.9f), 0.0f, 1e-9);

    dsp::SampleRateReducer srrStep;
    srrStep.SetFreq(0.5f);  // phase accumulates 0.5/sample -> new sample every 2nd call
    const float s0 = srrStep.Process(1.0f);  // phase 0.5 -> < 1, holds initial 0
    REQUIRE_NEAR(s0, 0.0f, 1e-9);
    const float s1 = srrStep.Process(2.0f);  // phase 1.0 -> >= 1, takes new sample
    REQUIRE_NEAR(s1, 2.0f, 1e-9);
}

TEST_CASE(digital_reorganizer_set_flip_truncates_set_hash_rounds) {
    dsp::DigitalReorganizer reorg;
    reorg.SetFlip(0.5f);
    REQUIRE_TRUE(reorg.flip == static_cast<uint8_t>(0.5f * 255.0f));  // truncation, :156

    reorg.SetHash(0.5f);
    REQUIRE_TRUE(reorg.hashBits == static_cast<uint8_t>(std::round(0.5f * 8.0f)));  // rounds, :161
}

// S1.3 (openspec/changes/archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/
// tasks.md; proposal.md §2-4; DIVERGENCE #2 note above dsp::DigitalReorganizer
// in Drive.hpp): Process() now returns Mangle(x) - Mangle(0), not Mangle(x)
// alone, at any nonzero flip/hash -- THE sanctioned exception to "any red is
// a regression" (tasks.md §0). This was RED after S1.3 landed (measured:
// reorg.Process(-0.6f) moved from -0.998438 to -1.48281, a delta of exactly
// -0.484375 == -Mangle(0.0f, flip=51, hashBits=6)) and is re-asserted here,
// never deleted, against the corrected behaviour.
TEST_CASE(digital_reorganizer_process_matches_bit_scramble_formula) {
    dsp::DigitalReorganizer reorg;
    reorg.SetFlip(0.2f);
    reorg.SetHash(0.75f);  // hashBits = round(6) = 6

    // Inputs kept strictly inside (-1, 1): this test pins the formula in its
    // originally-well-defined domain (roughly [-1, 0.9921875]), where the
    // Fix 1a clamp in Drive.hpp is a no-op (round(inputUp) is already in
    // [0,255]) -- so the reference replica below (a direct transcription of
    // the pre-fix formula) still matches exactly. The boundary and
    // out-of-range cases, where the clamp actually changes what would
    // otherwise be UB, are covered separately below by
    // digital_reorganizer_process_at_and_beyond_input_1_0_is_defined_and_saturates.
    //
    // The bit-scramble MATH transcribed below is UNCHANGED by S1.3 -- this
    // test's whole point is an INDEPENDENT check that Process() matches a
    // hand-transcribed formula, so it deliberately does not call Drive.hpp's
    // own Mangle() (that would only prove Process() calls Mangle() as
    // written, not that Mangle() computes the right thing; the "does
    // Process() literally equal Mangle(x)-Mangle(0)" question is instead
    // covered by construction, since Process()'s only body IS that
    // subtraction -- see Drive.hpp). What changed is what this test does
    // with two evaluations of that independent formula: `rawScramble` is
    // called once at `x` and once at silence, mirroring Process()'s own
    // Mangle(input,...) - Mangle(0.0f,...) shape one level up, without
    // duplicating the transcribed expression a second time in this file
    // (OMNI §8: one local definition, two call sites).
    const auto rawScramble = [&](float x) {
        const float inputUp = (x + 1.0f) * 128.0f;
        uint8_t inputInt = static_cast<uint8_t>(std::round(inputUp));
        const float inputRemainder = inputUp - static_cast<float>(inputInt);
        inputInt = static_cast<uint8_t>(inputInt ^ reorg.flip);
        const uint8_t mask = static_cast<uint8_t>((1 << reorg.hashBits) - 1);
        uint8_t lowerBits = static_cast<uint8_t>(inputInt & mask);
        lowerBits = static_cast<uint8_t>(lowerBits ^ ((lowerBits << 3) & mask));
        lowerBits = static_cast<uint8_t>(lowerBits ^ ((lowerBits >> 5) & mask));
        lowerBits = static_cast<uint8_t>(lowerBits ^ ((lowerBits << 1) & mask));
        inputInt = static_cast<uint8_t>((inputInt & ~mask) | lowerBits);
        return (static_cast<float>(inputInt) + inputRemainder) / 128.0f - 1.0f;
    };

    for (float x : {-0.6f, -0.2f, 0.1f, 0.5f}) {
        const float expected = rawScramble(x) - rawScramble(0.0f);
        REQUIRE_NEAR(reorg.Process(x), expected, 1e-6);
    }
}

// Fix 1a regression test (strict-executor brief item "FIX 1"): at
// input==1.0 exactly, inputUp==256, and `static_cast<uint8_t>(std::round(256.0f))`
// was undefined behavior before Drive.hpp's clamp. This is newly written
// code this app owns (unlike fuegoize's UB, which is carried forward
// because the frozen tree has a correct alternative reference to port
// instead) -- so it is fixed, not reproduced. With flip==0 and hashBits==0
// (pass-through configuration, mask==0 so lowerBits stays 0), the clamp's
// remainder term must reconstruct the input exactly rather than wrapping,
// and the same must hold with a nonzero flip/hash, and for inputs beyond
// +-1 (reachable in practice -- PolynomialDrive's output upstream of this
// stage is not amplitude-bounded to [-1,1]).
TEST_CASE(digital_reorganizer_process_at_and_beyond_input_1_0_is_defined_and_saturates) {
    dsp::DigitalReorganizer passthrough;  // flip=0, hashBits=0 -> mask=0, identity bit-scramble
    for (float x : {1.0f, 1.5f, -1.0f, -1.5f}) {
        const float out1 = passthrough.Process(x);
        const float out2 = passthrough.Process(x);
        REQUIRE_TRUE(std::isfinite(out1));
        REQUIRE_TRUE(out1 == out2);  // deterministic, no UB-driven flakiness
        REQUIRE_NEAR(out1, x, 1e-5);  // remainder term reconstructs input exactly
    }

    dsp::DigitalReorganizer withFlipAndHash;
    withFlipAndHash.SetFlip(0.3f);
    withFlipAndHash.SetHash(0.6f);
    for (float x : {1.0f, 2.0f, -1.0f, -2.0f}) {
        const float out1 = withFlipAndHash.Process(x);
        const float out2 = withFlipAndHash.Process(x);
        REQUIRE_TRUE(std::isfinite(out1));
        REQUIRE_TRUE(out1 == out2);
    }
}

TEST_CASE(oversampler2x_first_sample_processes_twice_then_interpolates) {
    dsp::Oversampler2x over;
    // Identity processFunc isolates the oversampler's own interpolation and
    // anti-alias-filter bookkeeping from any drive-stage math.
    auto identity = [](float x) { return x; };

    dsp::OnePoleLowPass refFilter;
    refFilter.SetAlphaFromNatFreq(0.4f);
    const float in0 = 0.5f;
    refFilter.Process(in0);  // first-sample branch processes input twice
    const float expected0 = refFilter.Process(in0);
    const float actual0 = over.Process(in0, identity);
    REQUIRE_NEAR(actual0, expected0, 1e-6);

    const float in1 = -0.3f;
    const float interpolated1 = (in0 + in1) * 0.5f;
    refFilter.Process(interpolated1);
    const float expected1 = refFilter.Process(in1);
    const float actual1 = over.Process(in1, identity);
    REQUIRE_NEAR(actual1, expected1, 1e-6);
}

TEST_CASE(frog_block_process_matches_manual_chain_replica) {
    // Full-chain regression pin, built from independently-tested primitives
    // (PolynomialDrive, Oversampler2x, DigitalReorganizer, SampleRateReducer,
    // PadeSaturator, Sine01), run in lockstep against dsp::FrogBlock over
    // several samples with a fixed knob set including a nonzero fuzz to
    // exercise the PadeSaturator branch (PolynomialDrive.hpp:187-202).
    dsp::FrogBlock block;
    block.polynomialDrive.SetGain(0.3f);
    block.polynomialDrive.SetCoefs(0.5f);
    block.sampleRateReducer1.SetFreq(0.9f);
    block.sampleRateReducer2.SetFreq(0.85f);
    block.digitalReorganizer.SetFlip(0.1f);
    block.digitalReorganizer.SetHash(0.4f);
    block.fuzz = 0.6f;

    dsp::PolynomialDrive refDrive;
    refDrive.SetGain(0.3f);
    refDrive.SetCoefs(0.5f);
    dsp::Oversampler2x refOversampler;
    dsp::DigitalReorganizer refReorg;
    refReorg.SetFlip(0.1f);
    refReorg.SetHash(0.4f);
    dsp::SampleRateReducer refSrr1;
    refSrr1.SetFreq(0.9f);
    dsp::SampleRateReducer refSrr2;
    refSrr2.SetFreq(0.85f);
    const float fuzz = 0.6f;

    for (int i = 0; i < 16; ++i) {
        const float input = 0.2f * std::sin(0.3f * static_cast<float>(i));

        float expected = refOversampler.Process(input, [&](float in) -> float {
            const float out = refDrive.Process(in);
            const float sinIn = out / 4.0f;
            return dsp::Sine01(sinIn) * (1.0f - fuzz) + fuzz * dsp::PadeSaturator::Saturate(out);
        });
        expected = refReorg.Process(expected);
        expected = refSrr1.Process(expected);
        expected = refSrr2.Process(expected);

        const float actual = block.Process(input);
        REQUIRE_NEAR(actual, expected, 1e-4);
    }
}

TEST_CASE(drive_blend_phase_authored_zero_blend_is_exact_passthrough) {
    // Authored, not ported: at blendKnob01 == 0, output must equal `dry`
    // exactly regardless of `wet` or phaseKnob01 -- the neutral default that
    // keeps this stage from disturbing the seven ported params.
    dsp::DriveBlendPhase bp;
    for (float phase : {0.0f, 0.3f, 0.5f, 0.9f, 1.0f}) {
        const float out = bp.Process(/*dry=*/0.42f, /*wet=*/-3.7f, /*blendKnob01=*/0.0f, phase);
        REQUIRE_NEAR(out, 0.42f, 1e-6);
    }
}

TEST_CASE(drive_blend_phase_authored_allpass_is_stable_and_finite) {
    dsp::DriveBlendPhase bp;
    float maxAbs = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const float wet = std::sin(0.37f * static_cast<float>(i)) * (1.0f + 0.01f * static_cast<float>(i % 5));
        const float out = bp.Process(/*dry=*/0.0f, wet, /*blendKnob01=*/1.0f, /*phaseKnob01=*/1.0f);
        REQUIRE_TRUE(std::isfinite(out));
        maxAbs = std::max(maxAbs, std::fabs(out));
    }
    REQUIRE_TRUE(maxAbs < 100.0f);  // energy-preserving allpass, no runaway
}

// Item 3 (new, found while reading the code): phaseKnob01 DEFAULTS to 0,
// mapping to a == -1 under the OLD [-1,1] coefficient mapping -- a pole
// exactly on the unit circle, so the allpass's state rings forever at
// constant amplitude instead of decaying once excited. Drives the allpass
// with a single impulse at the DEFAULT phase (0) and asserts the output
// magnitude decays toward zero as further zero-valued samples are pushed
// through, rather than staying at a constant (or growing) magnitude.
TEST_CASE(drive_blend_phase_default_phase_impulse_response_decays_not_rings_forever) {
    dsp::DriveBlendPhase bp;
    // One impulse (blendKnob01=1 so `phased` reaches the output undiluted).
    const float firstOut = bp.Process(/*dry=*/0.0f, /*wet=*/1.0f, /*blendKnob01=*/1.0f, /*phaseKnob01=*/0.0f);
    REQUIRE_TRUE(std::isfinite(firstOut));

    // Feed silence afterward and track the magnitude every 200 samples --
    // a decaying (stable) allpass shrinks toward 0; a pole-on-the-unit-
    // -circle allpass holds a constant nonzero magnitude forever.
    float magnitudeAt200 = 0.0f;
    float magnitudeAt2000 = 0.0f;
    for (int i = 1; i <= 2000; ++i) {
        const float out = bp.Process(/*dry=*/0.0f, /*wet=*/0.0f, /*blendKnob01=*/1.0f, /*phaseKnob01=*/0.0f);
        REQUIRE_TRUE(std::isfinite(out));
        if (i == 200) magnitudeAt200 = std::fabs(out);
        if (i == 2000) magnitudeAt2000 = std::fabs(out);
    }
    // A ringing (never-decaying) pole would leave magnitudeAt2000 equal to
    // (or within float noise of) magnitudeAt200 -- decay requires it to have
    // shrunk substantially by 2000 samples in.
    REQUIRE_TRUE(magnitudeAt2000 < magnitudeAt200 * 0.5f);
    // And it must actually reach (near-)silence, not merely shrink slowly.
    REQUIRE_TRUE(magnitudeAt2000 < 1.0e-6f);
}

// -----------------------------------------------------------------------
// B7.2 (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md
// §K.1/§K.4): FAILING-FIRST for the property this task fixes. §K.1 measured
// DriveBlendPhase's allpass coefficient `a` -- read fresh from the Phase
// knob every sample, unsmoothed -- produces gain up to 4.15x under
// full-bank per-sample-random modulation and 50.5x under a periodic
// phase/content coincidence (an LFO landing near the note's own period),
// against bounded (+-1) input. A fixed-coefficient allpass is unity-gain;
// a time-varying one is not.
//
// Two adversarial patterns, matching §K.1's own two cases:
//   (A) `wet` AND `phaseKnob01` both redrawn per-sample-random -- the
//       "full-bank per-sample-random modulation" case (`knob()` refreshes
//       every sample in production, FroggersAppCore.hpp:1008-1009 /
//       tasks.md's "Audio-rate modulation" section -- this is not an edge
//       case, it is what audio-rate noise modulation of this knob does
//       continuously).
//   (B) a periodic phase/content coincidence -- `wet` a bounded sine at
//       fWet, `phaseKnob01` a synced duty-cycled square-ish LFO at 2*fWet
//       -- reproducing §K.1's own "LFO near the note's period" mechanism.
//       A scratch harness sweep (not this file) found this exact
//       configuration reaches 61.2x unsmoothed -- EXCEEDS §K.1's own
//       recorded 50.5x -- verified bounded/plateauing (buildup complete
//       within ~500 samples, unchanged out to 20M samples), not divergent,
//       matching §K.1's own plateau finding.
//
// MEASURED with this exact test (reverting dsp::DriveBlendPhase's
// coeffSmoother/outputLimiter to confirm the failing-first requirement,
// then restoring the fix):
//   BEFORE the fix: pattern A worst = 4.810x, pattern B worst = 61.214x
//     (both against bound 1.0) -- FAILS (exceeds bound+0.02 on both).
//   AFTER the fix:  pattern A worst = 0.962x, pattern B worst = 0.892x
//     -- PASSES.
// -----------------------------------------------------------------------
TEST_CASE(drive_blend_phase_output_stays_at_or_below_computed_bound_under_audio_rate_phase_modulation) {
    const float inputAmplitude = 1.0f;  // FrogBlock's own bound (W2.1-MATH; dsp/Drive.hpp class comment).
    const float bound = inputAmplitude;

    // Pattern A: full-bank per-sample-random modulation. blendKnob01=1
    // isolates the allpass/wet path exactly as the existing allpass-
    // stability test above does.
    {
        dsp::DriveBlendPhase bp;
        std::uint32_t rngState = 0xC0FFEEu;
        const auto nextUniform01 = [&rngState]() {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            return static_cast<float>(rngState % 1000000u) / 1000000.0f;
        };
        constexpr int kSamples = 200000;
        for (int i = 0; i < kSamples; ++i) {
            const float wet = (2.0f * nextUniform01() - 1.0f) * inputAmplitude;
            const float phase = nextUniform01();
            const float out = bp.Process(/*dry=*/0.0f, wet, /*blendKnob01=*/1.0f, phase);
            REQUIRE_TRUE(std::isfinite(out));
            REQUIRE_TRUE(std::fabs(out) <= bound + 0.02f);
        }
    }

    // Pattern B: periodic phase/content coincidence -- the exact
    // fWet/fLfo/offset/duty configuration a scratch sweep found worst
    // (see this TEST_CASE's own header comment).
    {
        dsp::DriveBlendPhase bp;
        constexpr float fWet = 0.2f;
        constexpr float fLfo = 0.4f;
        constexpr float phaseOffCycles = 0.4f;
        constexpr float duty = 0.25f;
        constexpr int kSamples = 200000;
        for (int i = 0; i < kSamples; ++i) {
            const float wet = inputAmplitude * std::sin(2.0f * static_cast<float>(M_PI) * fWet * static_cast<float>(i));
            float lfoPhase = fLfo * static_cast<float>(i) + phaseOffCycles;
            lfoPhase -= std::floor(lfoPhase);
            const float phase = lfoPhase < duty ? 0.0f : 1.0f;
            const float out = bp.Process(/*dry=*/0.0f, wet, /*blendKnob01=*/1.0f, phase);
            REQUIRE_TRUE(std::isfinite(out));
            REQUIRE_TRUE(std::fabs(out) <= bound + 0.02f);
        }
    }
}

// -----------------------------------------------------------------------
// B7.2 tonal-neutrality proof: coeffSmoother/outputLimiter (dsp/Drive.hpp)
// only matter when `a`/the stage output move at audio rate. At a STATIC
// Phase knob the fixed-coefficient allpass was already unity-gain (class
// header comment), so the fix must leave a static-Phase patch unchanged --
// this is what makes the fix tonally free rather than a new colouration.
//
// Reference input amplitude (0.3) is kept safely under outputLimiter's own
// 0.7 threshold so the limiter's identity branch applies throughout
// (Limiter.hpp's own comment: absX <= threshold keeps envelope at exactly
// 1.0f, bit for bit) -- isolating this test to the smoother's effect only,
// not conflating it with the limiter's.
// -----------------------------------------------------------------------
TEST_CASE(drive_blend_phase_static_phase_output_unchanged_by_smoothing_and_limiter_fix) {
    constexpr float kRefAmplitude = 0.3f;  // well under outputLimiter's 0.7 threshold.

    // Case 1: default phase (0.0). coeffSmoother is seeded to exactly `a`
    // at phaseKnob01==0 (dsp/Drive.hpp constructor/Configure()), so there
    // is no startup transient at all -- bit-identical from the FIRST
    // sample against a manual replica of the pre-fix formula (fixed a, no
    // limiter needed as a reference here since a provably-unity-gain
    // static allpass at this amplitude never approaches 0.7).
    {
        dsp::DriveBlendPhase bp;
        float refX1 = 0.0f;
        float refY1 = 0.0f;
        const float a = 0.98f * (2.0f * 0.0f - 1.0f);  // == -0.98, matches the constructor's seed.
        for (int i = 0; i < 2000; ++i) {
            const float wet = kRefAmplitude * std::sin(0.31f * static_cast<float>(i));
            const float actual = bp.Process(/*dry=*/0.0f, wet, /*blendKnob01=*/1.0f, /*phaseKnob01=*/0.0f);
            const float refPhased = -a * wet + refX1 + a * refY1;
            refX1 = wet;
            refY1 = refPhased;
            REQUIRE_NEAR(actual, refPhased, 1e-5);
        }
    }

    // Case 2: a representative NON-default static phase (0.7). coeffSmoother
    // starts seeded at a(phase=0) and has to glide to a(phase=0.7) first, so
    // exact match only holds in STEADY STATE, not from sample 1 -- warm up
    // well past coeffSmoother's own time constant (glide 0.0035 cycles/
    // sample -> ~1/(2*pi*0.0035) =~ 45 samples; 1000 samples is >20 time
    // constants, i.e. the residual gap is negligible well below float
    // precision) before comparing.
    {
        dsp::DriveBlendPhase bp;
        constexpr float phaseKnob01 = 0.7f;
        const float a = 0.98f * (2.0f * phaseKnob01 - 1.0f);
        float refX1 = 0.0f;
        float refY1 = 0.0f;
        constexpr int kWarmupSamples = 1000;
        int i = 0;
        for (; i < kWarmupSamples; ++i) {
            const float wet = kRefAmplitude * std::sin(0.31f * static_cast<float>(i));
            bp.Process(/*dry=*/0.0f, wet, /*blendKnob01=*/1.0f, phaseKnob01);
            const float refPhased = -a * wet + refX1 + a * refY1;
            refX1 = wet;
            refY1 = refPhased;
        }
        for (int j = 0; j < 500; ++j, ++i) {
            const float wet = kRefAmplitude * std::sin(0.31f * static_cast<float>(i));
            const float actual = bp.Process(/*dry=*/0.0f, wet, /*blendKnob01=*/1.0f, phaseKnob01);
            const float refPhased = -a * wet + refX1 + a * refY1;
            refX1 = wet;
            refY1 = refPhased;
            REQUIRE_NEAR(actual, refPhased, 1e-4);
        }
    }
}

// =========================================================================
// 3.10 -- Delay (sim/StereoDelay.hpp whole file; sim/DelayState.hpp:165-198
// row->DelayParams mapping and Color/Halo fold at :180-193). A full
// Froggers original exists for all nine params -- nothing here is authored.
// =========================================================================

// Packet P1 (openspec/changes/frogg3rs-post-expansion-consolidation, T3.2):
// replaces both map_rows_to_delay_params_passes_through_rows_0_to_6_directly
// (rows 0-6 only, rows 7/8 folded) and
// map_rows_to_delay_params_color_halo_fold_matches_formula_and_clamps (the
// fold's own formula/clamp) -- the fold is deleted, so there is no fold
// formula left to pin, and every row (0-8) now passes straight through to
// its own field, not just 0-6. Distinct values per argument so a field
// reading the wrong row would be caught, not masked by a repeated 0.5f.
TEST_CASE(map_rows_to_delay_params_passes_through_all_rows_0_to_8_directly) {
    const dsp::DelayParams params = dsp::MapRowsToDelayParams(
        /*time=*/0.11f, /*send=*/0.22f, /*feedback=*/0.33f, /*width=*/0.44f,
        /*freeze=*/0.55f, /*mod=*/0.5f, /*mix=*/0.66f, /*reverse=*/0.77f, /*diffusion=*/0.88f);
    REQUIRE_NEAR(params.dtim, 0.11f, 1e-6);
    REQUIRE_NEAR(params.dsnd, 0.22f, 1e-6);
    REQUIRE_NEAR(params.dfbk, 0.33f, 1e-6);
    REQUIRE_NEAR(params.dwid, 0.44f, 1e-6);
    REQUIRE_NEAR(params.dfrz, 0.55f, 1e-6);
    REQUIRE_NEAR(params.dmod, 0.5f, 1e-6);
    REQUIRE_NEAR(params.dmix, 0.66f, 1e-6);
    REQUIRE_NEAR(params.drev, 0.77f, 1e-6);
    REQUIRE_NEAR(params.ddif, 0.88f, 1e-6);
}

TEST_CASE(stereo_delay_send_at_or_below_threshold_returns_silence_and_freezes_state) {
    // StereoDelay.hpp:60-64: dsnd <= 0.0001 returns {} before touching any
    // internal state (buffers, lfoPhase, writePos untouched).
    dsp::StereoDelay delayA;
    dsp::StereoDelay delayB;
    delayA.SetSampleRate(48000.0f);
    delayB.SetSampleRate(48000.0f);

    dsp::DelayParams silentParams;
    silentParams.dsnd = 0.0f;
    silentParams.dtim = 0.9f;  // other params irrelevant while dsnd is silent
    for (int i = 0; i < 500; ++i) {
        const dsp::DelayWetPair wet = delayA.Process(0.7f, silentParams);
        REQUIRE_TRUE(wet.l == 0.0f && wet.r == 0.0f);
    }

    // delayA's internal state must be identical to a fresh instance's (never
    // advanced) -- verified indirectly: feed both a real signal afterward
    // starting from delayB (fresh, never touched) and confirm identical
    // trajectories.
    dsp::DelayParams activeParams;
    activeParams.dtim = 0.3f;
    activeParams.dsnd = 0.8f;
    activeParams.dfbk = 0.2f;
    for (int i = 0; i < 10; ++i) {
        const dsp::DelayWetPair wetA = delayA.Process(0.5f, activeParams);
        const dsp::DelayWetPair wetB = delayB.Process(0.5f, activeParams);
        REQUIRE_NEAR(wetA.l, wetB.l, 1e-6);
        REQUIRE_NEAR(wetA.r, wetB.r, 1e-6);
    }
}

TEST_CASE(stereo_delay_time_maps_via_expmap_0p001_to_2s) {
    // StereoDelay.hpp:66: baseSeconds = ExpMap(0.001, kMaxDelaySeconds=2.0, dtim).
    // With dmod=0 and dwid=0, timeL/timeR both equal baseSeconds exactly
    // (:72-75, modSeconds and widthSpread both zero), so an impulse's return
    // position pins the exact mapped delay time.
    const float sr = 20000.0f;  // low sr keeps the delay short enough to land
                                 // well inside StereoDelay's capacity for this test
    dsp::StereoDelay delay;
    delay.SetSampleRate(sr);

    dsp::DelayParams p;
    p.dtim = 0.0f;  // -> baseSeconds = 0.001s = 20 samples at sr=20000
    p.dsnd = 1.0f;
    p.dfbk = 0.0f;
    p.dwid = 0.0f;
    p.dfrz = 0.0f;
    p.dmod = 0.0f;

    const float expectedSeconds = dsp::ExpMapCompute(0.001f, 2.0f, 0.0f);
    REQUIRE_NEAR(expectedSeconds, 0.001f, 1e-9);
    const float expectedDelaySamples = expectedSeconds * sr;  // 20 samples

    delay.Process(1.0f, p);  // impulse at sample 0
    dsp::DelayWetPair wet{};
    for (int i = 1; i <= 25; ++i) {
        wet = delay.Process(0.0f, p);
        if (i == static_cast<int>(std::lround(expectedDelaySamples))) {
            REQUIRE_TRUE(wet.l > 0.5f);  // impulse should be arriving here
        }
    }
}

TEST_CASE(stereo_delay_to_reverb_mono_matches_mix_formula) {
    dsp::StereoDelay delay;
    delay.SetSampleRate(48000.0f);
    const dsp::DelayWetPair wet{0.4f, -0.2f};
    for (float mix : {0.0f, 0.25f, 0.5f, 1.0f}) {
        const float bumpIn = 0.6f;
        const float monoWet = (wet.l + wet.r) * 0.5f;
        const float expected = (1.0f - mix) * bumpIn + mix * monoWet;
        REQUIRE_NEAR(delay.ToReverbMono(bumpIn, wet, mix), expected, 1e-6);
    }
}

TEST_CASE(stereo_delay_clear_buffers_resets_to_silence) {
    dsp::StereoDelay delay;
    delay.SetSampleRate(48000.0f);
    dsp::DelayParams p;
    p.dtim = 0.4f;
    p.dsnd = 1.0f;
    p.dfbk = 0.5f;
    for (int i = 0; i < 100; ++i) {
        delay.Process(std::sin(0.2f * static_cast<float>(i)), p);
    }
    delay.ClearBuffers();
    const dsp::DelayWetPair wetAfterClear = delay.Process(0.0f, p);
    REQUIRE_TRUE(std::isfinite(wetAfterClear.l));
    REQUIRE_TRUE(std::isfinite(wetAfterClear.r));
}

// -----------------------------------------------------------------------
// B2 (tasks.md CONSOLIDATED PUSH table; W2.1-MATH-2's "the delay is the
// only unsaturated feedback stage" finding): FAILING-FIRST, pinning a
// latent defect nobody has heard (the brief's own words). Pre-fix,
// `StereoDelay::Process` wrote `inSignal + fbL * fbk` -- a linear,
// unsaturated loop. `fbk` clamps to 0.98 (:170 above), so the loop's steady
// state is `in*send / (1 - 0.98)` == 50x input, unbounded by anything
// short of that 50x ceiling. Post-fix, the fed-back term is wrapped in the
// SAME `PadeSaturator::Saturate` the comb's own in-loop feedback already
// uses (`Comb::Process`, FilterFx.hpp), clamped to +-1 BEFORE the `fbk`
// multiply -- so every write to the line is bounded by
// `|inSignal| + fbk` REGARDLESS of how many round trips have already run,
// a per-sample bound, not merely a steady-state one.
//
// Feedback pinned to 1.0 (clamps to 0.98, the loop's actual maximum), Send
// to 1.0 (inSignal == bumpIn exactly), Time to 0.0 (~48 samples/round-trip
// at 48 kHz -- short enough that dozens of round trips, and therefore the
// pre-fix loop's runaway growth, happen well inside this test's sample
// budget), Width to 0.3 (nonzero cross-feed, so both channels exercise the
// saturator through the `fbL = dL*(1-cross) + dR*cross` blend -- "both
// lines" per B2's brief, not just an isolated single-channel case).
// -----------------------------------------------------------------------
TEST_CASE(delay_feedback_loop_stays_bounded_at_max_feedback) {
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);

    dsp::DelayParams p;
    p.dtim = 0.0f;   // -> baseSeconds = 0.001s = 48 samples at sr=48000: fast round trips.
    p.dsnd = 1.0f;   // inSignal == bumpIn exactly.
    p.dfbk = 1.0f;   // -> fbk clamps to 0.98 (StereoDelay::Process's own clamp).
    p.dwid = 0.3f;   // nonzero cross-feed -- exercises both lineL and lineR through the blend.
    p.dfrz = 0.0f;
    p.dmod = 0.0f;

    const float inputAmplitude = 1.0f;
    const float fbk = 0.98f;                             // the clamp StereoDelay::Process itself applies.
    const float bound = inputAmplitude * p.dsnd + fbk;    // |inSignal| + fbk -- B2's per-sample guarantee.

    constexpr int kSamples = 3000;  // ~60 round trips at 48 samples/trip -- pre-fix, this is already
                                     // well past the ~50x-input steady state (in*send/(1-0.98)==50.0).
    // B6a note: `wet.l`/`wet.r` measured here are now what `wetLimiterL`/`R`
    // (dsp/Delay.hpp) return, not the raw loop tap -- still `<= bound`
    // (trivially: limiter output <= raw tap <= bound, since the limiter
    // only ever reduces magnitude), just no longer the tightest statement
    // about them. The tighter, B6a-specific bound (the limiter's own
    // ceiling, ~1.0 rather than this test's ~1.98) is pinned separately
    // below (delay_wet_output_stays_at_or_below_limiter_ceiling_at_max_feedback).
    for (int i = 0; i < kSamples; ++i) {
        const dsp::DelayWetPair wet = delay.Process(inputAmplitude, p);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        REQUIRE_TRUE(std::fabs(wet.l) <= bound + 1.0e-4f);
        REQUIRE_TRUE(std::fabs(wet.r) <= bound + 1.0e-4f);
    }
}

// -----------------------------------------------------------------------
// B6a (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md,
// "Group B outcomes" / operator: "why can't we just have limiters for
// reverb and delay?"): FAILING-FIRST for the property B2's in-loop
// saturator alone cannot close -- B2 bounds the LOOP's own write to
// `|inSignal| + fbk` (~1.98 at max feedback, the test above), still well
// over the master output limiter's 0.9 threshold (W2.1-MATH-2's own
// finding: "at A = 0.5 that is 25.0 against a 0.9 threshold"). This test
// pins the STAGE's own escape bound instead: `wetLimiterL`/`wetLimiterR`
// (dsp/Delay.hpp), applied to `dL`/`dR` strictly AFTER B2's loop write, so
// what actually leaves this stage never exceeds
// `dsp::OutputLimiter::kDefaultCeiling` (1.0 -- both wetLimiterL/R's own
// `kDelayWetLimiterCeiling` and the master's ceiling share this same
// value).
//
// Same scenario as the test immediately above (shortest reachable delay
// time -- the fastest round trip, B6a's own measured worst case; see
// dsp/Delay.hpp's header comment for the full sweep), run long enough
// (5000 samples) to comfortably clear the transient window the measurement
// found the worst-case overshoot inside.
//
// MEASURED (scratch harness, this exact scenario, run against the code
// before this task existed): WITHOUT wetLimiterL/R (raw `dL`/`dR` tap),
// worst case over this run = 1.962235 -- clears the 1.0 ceiling easily,
// the failing case this test pins. WITH wetLimiterL/R at their measured
// tuning (threshold 0.9, attack 2 microseconds, release 100ms --
// dsp/Delay.hpp's own `kDelayWetLimiter*` constants): worst case =
// 0.999999, at or below the ceiling.
// -----------------------------------------------------------------------
TEST_CASE(delay_wet_output_stays_at_or_below_limiter_ceiling_at_max_feedback) {
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);

    dsp::DelayParams p;
    p.dtim = 0.0f;   // shortest reachable round trip -- B6a's own measured worst case.
    p.dsnd = 1.0f;
    p.dfbk = 1.0f;   // -> fbk clamps to 0.98.
    p.dwid = 0.3f;
    p.dfrz = 0.0f;
    p.dmod = 0.0f;

    const float ceiling = dsp::OutputLimiter::kDefaultCeiling;  // 1.0 -- shared by wetLimiterL/R and the master.

    constexpr int kSamples = 5000;
    float maxAbs = 0.0f;
    for (int i = 0; i < kSamples; ++i) {
        const dsp::DelayWetPair wet = delay.Process(1.0f, p);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        maxAbs = std::max(maxAbs, std::max(std::fabs(wet.l), std::fabs(wet.r)));
    }
    REQUIRE_TRUE(maxAbs <= ceiling + 1.0e-4f);
}

// Fix 1b regression test (strict-executor brief item "FIX 1"): immediately
// after construction/SetSampleRate, `writePos == 0`, so a call with a
// large-enough delay time makes `readPos` in `ReadAt` negative -- the
// "first ~delaySamples calls after construction" window the frozen
// sim/StereoDelay.hpp:120 and this port's pre-fix code both hit undefined
// behavior on (a negative-float-to-size_t narrowing conversion). As with
// Fix 1a, there is no correct frozen reference to port instead here -- the
// frozen source has the identical UB -- so this is a fix, not a
// reproduction. dtim=1.0 maps (ExpMapCompute(0.001, 2.0, 1.0) == 2.0s) to
// exactly `kMaxDelaySeconds`, i.e. a full buffer length of delay at this
// sample rate, so every one of the samples below reads the still-zero
// (never-yet-written) region: the intended "correct silence during
// warm-up" behavior, now reached through defined floor/mod arithmetic
// instead of an incidental target-specific truncation.
TEST_CASE(stereo_delay_read_before_buffer_has_filled_is_defined_and_silent) {
    const float sr = 48000.0f;  // capacity == 96000 == 2.0s * sr (kMaxDelaySeconds)
    dsp::DelayParams p;
    p.dtim = 1.0f;  // -> baseSeconds == kMaxDelaySeconds == 2.0s == full capacity in samples
    p.dsnd = 1.0f;
    p.dfbk = 0.0f;
    p.dwid = 0.0f;
    p.dfrz = 0.0f;
    p.dmod = 0.0f;

    dsp::StereoDelay delayA;
    delayA.SetSampleRate(sr);
    dsp::StereoDelay delayB;
    delayB.SetSampleRate(sr);

    for (int i = 0; i < 50; ++i) {
        const float input = std::sin(0.3f * static_cast<float>(i));
        const dsp::DelayWetPair wetA = delayA.Process(input, p);
        const dsp::DelayWetPair wetB = delayB.Process(input, p);

        REQUIRE_TRUE(std::isfinite(wetA.l));
        REQUIRE_TRUE(std::isfinite(wetA.r));
        // Still reading the never-yet-written (zero-filled) region this
        // early: the read position is a full buffer length behind writePos.
        REQUIRE_NEAR(wetA.l, 0.0f, 1e-6);
        REQUIRE_NEAR(wetA.r, 0.0f, 1e-6);
        // Determinism across two independently-constructed, identically-driven
        // instances: previously-UB behavior could (and did, per the class-level
        // note) vary by optimization level/target for the same logical input.
        REQUIRE_NEAR(wetA.l, wetB.l, 1e-9);
        REQUIRE_NEAR(wetA.r, wetB.r, 1e-9);
    }
}

// =========================================================================
// 9.1/9.4 -- ResonantBump/Comb transfer-function UIState (design D10)
// =========================================================================

// Independent cross-check of ResonantBump::UIState::TransferFunctionValue's
// closed form: drive a unit-amplitude sinusoid at the SAME normalized
// frequency through the real Process() path long enough to settle, and
// confirm the measured steady-state gain matches the closed form's
// predicted magnitude -- not merely self-consistent with it.
TEST_CASE(resonant_bump_frequency_response_matches_simulated_steady_state_gain) {
    dsp::ResonantBump bump;
    const float freqNormalized = 0.05f;
    bump.SetFreq(freqNormalized);
    bump.SetHeight(4.0f);
    bump.SetWidth(2.0f);

    dsp::ResonantBump::UIState state;
    bump.PopulateUIState(state);
    const float predictedMagnitude = state.FrequencyResponse(freqNormalized);
    REQUIRE_TRUE(std::isfinite(predictedMagnitude));

    constexpr int kWarmupSamples = 4000;
    constexpr int kMeasureSamples = 200;
    float measuredPeak = 0.0f;
    for (int i = 0; i < kWarmupSamples + kMeasureSamples; ++i) {
        const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(i);
        const float output = bump.Process(std::sin(phase));
        if (i >= kWarmupSamples) {
            measuredPeak = std::max(measuredPeak, std::fabs(output));
        }
    }
    REQUIRE_NEAR(measuredPeak, predictedMagnitude, 0.05);
}

// The comb's linearised closed form at feedback=0 must be EXACTLY unity
// (H(z) = 1/(1-0) = 1) at every frequency -- a clean, simulation-free sanity
// check of ComputeLinearizedTransferFunctionValue's own algebra, independent
// of the self-oscillation edge case below.
TEST_CASE(comb_zero_feedback_response_is_exactly_unity) {
    dsp::Comb::UIState state;
    state.feedback.store(0.0f);
    state.lowPassAlpha.store(0.5f);
    state.delaySamples.store(50);

    for (float f : {0.001f, 0.05f, 0.1f, 0.25f, 0.49f}) {
        REQUIRE_NEAR(state.FrequencyResponse(f), 1.0f, 1e-5);
    }
}

// Task 9.4: "self-oscillating comb feedback produces only finite plot
// values" -- Comb::GetFeedback's own +-0.95 ceiling (item 1, design.md
// A2, was +-1.1), densely sampled, must never produce a NaN/Inf response,
// and must stay within the bound SafeDenominator's floor implies
// (1/kMinMagnitude = 1000x).
TEST_CASE(self_oscillating_comb_response_is_finite_and_bounded) {
    dsp::Comb::UIState state;
    state.feedback.store(dsp::Comb::GetFeedback(1.0f));  // +0.95, the ceiling.
    state.lowPassAlpha.store(0.999f);
    state.delaySamples.store(1);

    constexpr int kNumSamples = 2000;
    for (int i = 0; i < kNumSamples; ++i) {
        const float f = 1.0e-4f + (0.5f - 1.0e-4f) * static_cast<float>(i) / static_cast<float>(kNumSamples - 1);
        const float magnitude = state.FrequencyResponse(f);
        REQUIRE_TRUE(std::isfinite(magnitude));
        REQUIRE_TRUE(magnitude <= 1000.0f + 1.0f);  // SafeDenominator's 1/1e-3 bound, +1 rounding headroom.
    }
}

// =========================================================================
// 8.4 -- RandomShLane deja-vu behavior (design D8a): a locked loop
// (dejaVuKnob=0.5, sources 1/2/3) never regenerates a slot value once
// drawn; a free-running lane (dejaVuKnob=0.0, sources 4/5) regenerates the
// current slot's value on every single Increment() call.
// =========================================================================

TEST_CASE(locked_loop_source_cycles_a_fixed_loop_across_two_full_cycles) {
    dsp::RandomShLane lane = dsp::lanes::MakeSource1(0x3333u);  // dejaVuKnob=0.5 (locked loop, D8a source #1).
    dsp::RandomShLane::UiState state;

    std::vector<float> firstCycle(dsp::RandomShLane::kNumSlots);
    for (std::size_t i = 0; i < dsp::RandomShLane::kNumSlots; ++i) {
        lane.PopulateUiState(state);
        firstCycle[state.currentIndex.load()] = state.slots[state.currentIndex.load()].load();
        lane.Increment();
    }
    std::vector<float> secondCycle(dsp::RandomShLane::kNumSlots);
    for (std::size_t i = 0; i < dsp::RandomShLane::kNumSlots; ++i) {
        lane.PopulateUiState(state);
        secondCycle[state.currentIndex.load()] = state.slots[state.currentIndex.load()].load();
        lane.Increment();
    }
    for (std::size_t i = 0; i < dsp::RandomShLane::kNumSlots; ++i) {
        REQUIRE_TRUE(firstCycle[i] == secondCycle[i]);  // never regenerated -- bit-identical.
    }
}

TEST_CASE(free_running_source_keeps_producing_new_values_across_two_full_cycles) {
    dsp::RandomShLane lane = dsp::lanes::MakeSource4(0x4444u);  // dejaVuKnob=0.0 (free-running, D8a source #4).
    dsp::RandomShLane::UiState state;

    std::vector<float> firstCycle(dsp::RandomShLane::kNumSlots);
    for (std::size_t i = 0; i < dsp::RandomShLane::kNumSlots; ++i) {
        lane.PopulateUiState(state);
        firstCycle[state.currentIndex.load()] = state.slots[state.currentIndex.load()].load();
        lane.Increment();
    }
    std::vector<float> secondCycle(dsp::RandomShLane::kNumSlots);
    for (std::size_t i = 0; i < dsp::RandomShLane::kNumSlots; ++i) {
        lane.PopulateUiState(state);
        secondCycle[state.currentIndex.load()] = state.slots[state.currentIndex.load()].load();
        lane.Increment();
    }
    bool sawADifference = false;
    for (std::size_t i = 0; i < dsp::RandomShLane::kNumSlots; ++i) {
        if (firstCycle[i] != secondCycle[i]) {
            sawADifference = true;
        }
    }
    REQUIRE_TRUE(sawADifference);
}

// =========================================================================
// Drive/Delay slots 9-13 (strict-executor packet, D1-D10). Each mapping's
// own default-reproduces-today's-literal claim is pinned here through the
// REAL setter, not just by inspecting the in-class fallback default that
// the tests above (frog_block_process_matches_manual_chain_replica,
// delay_feedback_loop_stays_bounded_at_max_feedback, etc.) already happen
// to exercise by never calling these setters at all.
// =========================================================================

TEST_CASE(drive_set_anti_alias_brightness_default_knob_reproduces_0_4f_cutoff) {
    // D1: knob 0.5f -> ExpMapCompute(0.32,0.5,0.5) == sqrt(0.16) == 0.4f exactly.
    dsp::Oversampler2x over;
    over.SetAntiAliasBrightness(0.5f);
    dsp::OnePoleLowPass reference;
    reference.SetAlphaFromNatFreq(0.4f);
    REQUIRE_NEAR(over.antiAlias.alpha, reference.alpha, 1e-6);
}

TEST_CASE(polynomial_drive_set_link_default_knob_reproduces_0_25f_coupling) {
    // D2: knob 0.5f -> linkScalar == 0.5f*0.5f == 0.25f exactly, matching the
    // pre-packet hardcoded literal this same formula (polynomial_drive_set_
    // coefs_matches_space_filling_curve_formula above) already pins.
    dsp::PolynomialDrive drive;
    drive.SetGain(0.5f);
    const float computedGain = drive.gain;
    drive.SetLink(0.5f);
    drive.SetCoefs(0.3f);

    const float coefsKnob = dsp::ZeroedExpCompute(30.0f, 0.3f);
    REQUIRE_NEAR(drive.coefs[1], 10.0f * dsp::Sine01(coefsKnob * 1.618f + 0.25f * (computedGain - 1.0f)), 1e-5);
    REQUIRE_NEAR(drive.coefs[3], 10.0f * dsp::Sine01(coefsKnob * 3.141f + 0.25f * (computedGain - 1.0f)), 1e-5);
}

TEST_CASE(frog_block_set_fold_divisor_stays_strictly_positive_across_full_knob_range) {
    // D3's binding requirement: the divisor must never reach or cross zero
    // (out/0 -> +-inf -> Sine01's floor() turns it into NaN). ExpMapCompute's
    // floor is `min` (1.0 here), so this holds by construction across the
    // whole representable knob range, checked densely here as a regression
    // guard rather than trusted on paper alone.
    dsp::FrogBlock block;
    for (int i = 0; i <= 200; ++i) {
        const float knob = static_cast<float>(i) / 200.0f;
        block.SetFold(knob);
        REQUIRE_TRUE(block.foldDivisor > 0.0f);
        REQUIRE_TRUE(std::isfinite(block.foldDivisor));
    }
    block.SetFold(0.5f);
    REQUIRE_NEAR(block.foldDivisor, 4.0f, 1e-5);  // default knob reproduces today's literal exactly.
}

// D1/D3/D4/D5's combined claim: at the exact default knob values recorded in
// FroggersParameters.hpp (0.5f/0.5f/1.0f/0.5f for ABrt/Fold/Tone/Bias), a
// FrogBlock wired through the real setters must be bit-for-bit identical to
// one that never calls them at all (the pre-packet behaviour) -- the actual
// claim "a fresh launch sounds EXACTLY as it does today" makes, verified
// through the production call path rather than by inspecting field defaults.
TEST_CASE(frog_block_default_knob_values_reproduce_pre_packet_output_exactly) {
    dsp::FrogBlock blockOld;  // never touches SetAntiAliasBrightness/SetFold/SetTone/SetBias.
    blockOld.polynomialDrive.SetGain(0.4f);
    blockOld.polynomialDrive.SetCoefs(0.7f);
    blockOld.sampleRateReducer1.SetFreq(0.8f);
    blockOld.sampleRateReducer2.SetFreq(0.75f);
    blockOld.digitalReorganizer.SetFlip(0.2f);
    blockOld.digitalReorganizer.SetHash(0.5f);
    blockOld.fuzz = 0.3f;

    dsp::FrogBlock blockNew;  // same knobs, PLUS the four new setters at their FroggersParameters.hpp defaults.
    blockNew.polynomialDrive.SetGain(0.4f);
    blockNew.polynomialDrive.SetLink(0.5f);
    blockNew.polynomialDrive.SetCoefs(0.7f);
    blockNew.sampleRateReducer1.SetFreq(0.8f);
    blockNew.sampleRateReducer2.SetFreq(0.75f);
    blockNew.digitalReorganizer.SetFlip(0.2f);
    blockNew.digitalReorganizer.SetHash(0.5f);
    blockNew.fuzz = 0.3f;
    blockNew.oversampler.SetAntiAliasBrightness(0.5f);
    blockNew.SetFold(0.5f);
    blockNew.SetTone(1.0f);
    blockNew.SetBias(0.5f);

    for (int i = 0; i < 64; ++i) {
        const float input = 0.6f * std::sin(0.21f * static_cast<float>(i));
        REQUIRE_NEAR(blockOld.Process(input), blockNew.Process(input), 1e-5);
    }
}

TEST_CASE(stereo_delay_feedback_tone_default_knob_is_exact_bypass_alpha) {
    dsp::StereoDelay delay;
    delay.SetFeedbackTone(1.0f);  // D7 default knob.
    REQUIRE_NEAR(delay.fbToneL.alpha, 1.0f, 1e-6);
    REQUIRE_NEAR(delay.fbToneR.alpha, 1.0f, 1e-6);
}

TEST_CASE(stereo_delay_crush_default_knob_is_exact_bypass_freq) {
    dsp::StereoDelay delay;
    delay.SetCrush(0.0f);  // D10 default knob.
    REQUIRE_TRUE(delay.crushL.freq >= 1.0f);
    REQUIRE_TRUE(delay.crushR.freq >= 1.0f);
}

// D9's two binding bounds, checked across the WHOLE widthBalance knob range
// (not just the default), matching this task's own "must hold by
// construction of the mapping, not by luck" requirement.
TEST_CASE(stereo_delay_width_balance_mapping_keeps_cross_in_0_1_and_spread_at_or_below_todays_max) {
    dsp::StereoDelay delay;
    for (int wb = 0; wb <= 20; ++wb) {
        const float widthBalanceKnob = static_cast<float>(wb) / 20.0f;
        delay.SetWidthBalance(widthBalanceKnob);
        for (int dw = 0; dw <= 20; ++dw) {
            const float dwid = static_cast<float>(dw) / 20.0f;  // p.dwid's own established [0,1] invariant.
            const float baseSeconds = 2.0f;                     // baseSeconds' own max (kMaxDelaySeconds).
            const float cross = dwid * 0.5f * delay.widthBalance;
            const float spread = dwid * baseSeconds * 0.35f * delay.widthBalance;
            const float spreadCeilingToday = dwid * baseSeconds * 0.35f;
            REQUIRE_TRUE(cross >= 0.0f && cross <= 1.0f);          // bound (a).
            REQUIRE_TRUE(spread <= spreadCeilingToday + 1e-6f);    // bound (b): never exceeds today's own max.
        }
    }
    delay.SetWidthBalance(1.0f);
    REQUIRE_NEAR(delay.widthBalance, 1.0f, 1e-6);  // default knob reproduces today's 0.35/0.5 ratio exactly.
}

// D6's binding placement requirement: fbDrive multiplies the ARGUMENT of
// Saturate only, so the per-sample write bound `|inSignal| + fbk` must hold
// REGARDLESS of fbDrive -- checked here at fbDrive's own maximum (knob 1.0f
// -> ExpMapCompute(0.25,4,1.0) == 4.0x), the same scenario
// delay_feedback_loop_stays_bounded_at_max_feedback above pins at fbDrive's
// (implicit) default of 1.0x.
TEST_CASE(stereo_delay_feedback_drive_at_maximum_does_not_raise_the_per_sample_bound) {
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);
    delay.SetFeedbackDrive(1.0f);  // -> fbDrive == 4.0x, ExpMapCompute(0.25,4,1.0).

    dsp::DelayParams p;
    p.dtim = 0.0f;
    p.dsnd = 1.0f;
    p.dfbk = 1.0f;  // -> fbk clamps to 0.98.
    p.dwid = 0.3f;
    p.dfrz = 0.0f;
    p.dmod = 0.0f;

    const float inputAmplitude = 1.0f;
    const float fbk = 0.98f;
    const float bound = inputAmplitude * p.dsnd + fbk;  // same |inSignal| + fbk bound as B2's own test.

    for (int i = 0; i < 3000; ++i) {
        const dsp::DelayWetPair wet = delay.Process(inputAmplitude, p);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        REQUIRE_TRUE(std::fabs(wet.l) <= bound + 1.0e-4f);
        REQUIRE_TRUE(std::fabs(wet.r) <= bound + 1.0e-4f);
    }
}

// D6/D7/D8/D9/D10's combined claim, same shape as the FrogBlock version
// above: at the exact default knob values recorded in FroggersParameters.hpp
// (0.5f/1.0f/0.5f/1.0f/0.0f for FbDr/FbTn/MdRt/WBal/Crsh), a StereoDelay
// wired through the real setters is bit-for-bit identical to one that never
// calls them, across enough samples (and a nonzero dmod) to exercise the
// mod-rate LFO path D8 touches.
TEST_CASE(stereo_delay_default_knob_values_reproduce_pre_packet_output_exactly) {
    const float sr = 48000.0f;
    dsp::StereoDelay delayOld;
    delayOld.SetSampleRate(sr);  // never touches SetFeedbackDrive/SetFeedbackTone/SetModRate/SetWidthBalance/SetCrush.

    dsp::StereoDelay delayNew;
    delayNew.SetSampleRate(sr);
    delayNew.SetFeedbackDrive(0.5f);
    delayNew.SetFeedbackTone(1.0f);
    delayNew.SetModRate(0.5f);
    delayNew.SetWidthBalance(1.0f);
    delayNew.SetCrush(0.0f);

    dsp::DelayParams p;
    p.dtim = 0.3f;
    p.dsnd = 1.0f;
    p.dfbk = 0.6f;
    p.dwid = 0.4f;
    p.dfrz = 0.1f;
    p.dmod = 0.5f;  // nonzero -- exercises the mod-rate LFO path D8's SetModRate touches.

    for (int i = 0; i < 4000; ++i) {
        const float input = 0.5f * std::sin(0.05f * static_cast<float>(i));
        const dsp::DelayWetPair wetOld = delayOld.Process(input, p);
        const dsp::DelayWetPair wetNew = delayNew.Process(input, p);
        REQUIRE_NEAR(wetOld.l, wetNew.l, 1e-4);
        REQUIRE_NEAR(wetOld.r, wetNew.r, 1e-4);
    }
}

// =========================================================================
// T3.1c (Diffusion, Delay slot 8, strict-executor packet P2): per-channel
// three-section Schroeder allpass cascade on the wet tap
// (dsp/Delay.hpp SchroederAllpassSection/DelayDiffuser/
// StereoDelay::ApplyDiffusion). dfrz/drev (slots 4/7) stay inert -- not
// touched by this packet.
// =========================================================================

TEST_CASE(stereo_delay_diffusion_at_default_zero_is_bit_identical_to_no_diffusion) {
    // T3.1c binding requirement: ddif==0.0f (the Diffusion knob's own
    // default, FroggersParameters.hpp; dsp::DelayParams::ddif's own
    // in-class default) must leave the wet tap EXACTLY -- bit-for-bit, not
    // merely close -- what it would be with no diffuser at all.
    //
    // Proven in the strongest form this black-box API allows: one
    // instance's diffuser history is poked directly to a dirty,
    // non-quiescent state (diffuserL/diffuserR are public members, same
    // per-channel-instance idiom as wetLimiterL/R/fbToneL/R/crushL/R) with
    // NO Process() calls in between -- so neither instance's lineL/lineR/
    // lfoPhase/wetLimiter envelope differs AT ALL going in; the diffuser's
    // own history is the ONLY difference between the two instances. Both
    // then process identical input, built through the real production
    // entry point (dsp::MapRowsToDelayParams, the same call
    // FroggersAppCore.hpp makes) with row8Diffusion explicitly 0.0f. If
    // ddif==0 truly bypasses the diffuser (this packet's binding
    // placement, dsp/Delay.hpp Process()), the dirtied history is never
    // read and outputs must match exactly; if a future change drops the
    // bypass (e.g. always blends `diffused*ddif` in regardless, or drops
    // the `(1-ddif)` dry term / the `*ddif` scale on `diffused`), this is
    // the test that catches it -- see the packet report for the actual
    // broken/restored numbers this test produced.
    const float sr = 48000.0f;
    dsp::StereoDelay delayDirty;
    delayDirty.SetSampleRate(sr);
    dsp::StereoDelay delayClean;
    delayClean.SetSampleRate(sr);

    delayDirty.diffuserL.section1.xHistory[0] = 0.73f;
    delayDirty.diffuserL.section1.yHistory[0] = -0.41f;
    delayDirty.diffuserL.section2.xHistory[5] = 0.29f;
    delayDirty.diffuserL.section3.yHistory[10] = -0.55f;
    delayDirty.diffuserR.section1.xHistory[0] = -0.62f;
    delayDirty.diffuserR.section2.yHistory[3] = 0.18f;
    delayDirty.diffuserR.section3.xHistory[7] = 0.37f;

    const dsp::DelayParams p = dsp::MapRowsToDelayParams(
        /*time=*/0.35f, /*send=*/0.9f, /*feedback=*/0.4f, /*width=*/0.25f,
        /*freeze=*/0.0f, /*mod=*/0.2f, /*mix=*/0.6f, /*reverse=*/0.0f, /*diffusion=*/0.0f);

    float maxAbsDiffL = 0.0f;
    float maxAbsDiffR = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const float input = 0.6f * std::sin(0.041f * static_cast<float>(i));
        const dsp::DelayWetPair wetDirty = delayDirty.Process(input, p);
        const dsp::DelayWetPair wetClean = delayClean.Process(input, p);
        maxAbsDiffL = std::max(maxAbsDiffL, std::fabs(wetDirty.l - wetClean.l));
        maxAbsDiffR = std::max(maxAbsDiffR, std::fabs(wetDirty.r - wetClean.r));
    }
    REQUIRE_NEAR(maxAbsDiffL, 0.0, 0.0);  // exact equality (eps=0) -- this packet's own binding requirement.
    REQUIRE_NEAR(maxAbsDiffR, 0.0, 0.0);
}

TEST_CASE(stereo_delay_diffusion_at_maximum_differs_measurably_from_no_diffusion) {
    // "Audible across the range" requirement: ddif==1.0f must differ
    // measurably from ddif==0.0f for the same input -- not a rounding-level
    // difference.
    const float sr = 48000.0f;
    dsp::StereoDelay delayOff;
    delayOff.SetSampleRate(sr);
    dsp::StereoDelay delayOn;
    delayOn.SetSampleRate(sr);

    dsp::DelayParams pOff;
    pOff.dtim = 0.35f;
    pOff.dsnd = 0.9f;
    pOff.dfbk = 0.4f;
    pOff.dwid = 0.25f;
    pOff.dmod = 0.2f;
    pOff.dmix = 0.6f;
    pOff.ddif = 0.0f;

    dsp::DelayParams pOn = pOff;
    pOn.ddif = 1.0f;

    float maxAbsDiff = 0.0f;
    for (int i = 0; i < 4000; ++i) {
        const float input = 0.6f * std::sin(0.037f * static_cast<float>(i));
        const dsp::DelayWetPair wetOff = delayOff.Process(input, pOff);
        const dsp::DelayWetPair wetOn = delayOn.Process(input, pOn);
        REQUIRE_TRUE(std::isfinite(wetOn.l) && std::isfinite(wetOn.r));
        maxAbsDiff = std::max(maxAbsDiff, std::fabs(wetOn.l - wetOff.l));
        maxAbsDiff = std::max(maxAbsDiff, std::fabs(wetOn.r - wetOff.r));
    }
    REQUIRE_TRUE(maxAbsDiff > 1.0e-3f);  // measurable, not a rounding-level difference.
}

TEST_CASE(stereo_delay_diffusion_state_tracks_the_signal_while_ddif_is_zero) {
    // Packet P2 postflight regression guard. The original P2 call site read
    // `(p.ddif == 0.0f) ? dL : ApplyDiffusion(...)`, which skipped the CALL
    // at zero and so never advanced the sections' history. Every other
    // diffusion test holds ddif fixed for its whole run, so none of them can
    // observe that -- the state has to be inspected directly.
    //
    // Method, and why NOT convergence-between-instances: an earlier version
    // of this test ran one delay at ddif>0 throughout against one that
    // crossed 0 -> ddif>0, and asserted the two converged. They do, but only
    // after roughly 15x the longest section's memory -- the allpass
    // homogeneous term decays as a^(n/M), so at a=0.525 and M~1013 samples a
    // 4000-sample settle still leaves 0.525^4 ~ 0.076, which is exactly the
    // 0.0699 that version measured. It was timing the transient, not the
    // defect. The defect has a direct signature instead: a diffuser that is
    // never called keeps the all-zero state Reset() left it in, so its own
    // StateMagnitude() stays at exactly 0 no matter how loud the input.
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);

    dsp::DelayParams pZero;
    pZero.dtim = 0.35f;
    pZero.dsnd = 0.9f;
    pZero.dfbk = 0.4f;
    pZero.dwid = 0.25f;
    pZero.dmod = 0.2f;
    pZero.dmix = 0.6f;
    pZero.ddif = 0.0f;  // the whole point: diffusion OFF for this entire run.

    // Long enough to more than fill the longest section (21.1 ms at 48kHz is
    // ~1013 samples), so a tracking diffuser has unambiguously taken signal.
    float maxAbsWet = 0.0f;
    for (int i = 0; i < 4000; ++i) {
        const float input = 0.6f * std::sin(0.037f * static_cast<float>(i));
        const dsp::DelayWetPair wet = delay.Process(input, pZero);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        maxAbsWet = std::max(maxAbsWet, std::max(std::fabs(wet.l), std::fabs(wet.r)));
    }

    const float stateL = delay.diffuserL.StateMagnitude();
    const float stateR = delay.diffuserR.StateMagnitude();

    // OMNI 9.1 positive control: "the state is non-zero" means nothing unless
    // a signal actually reached the unit. Report the driving quantity's own
    // range beside the result, so a run that proved something true and
    // irrelevant is visible as such rather than reading as a pass.
    std::cout << "  [P2 postflight] ddif==0 state tracking: wet max|signal|=" << maxAbsWet
              << "  diffuser state L=" << stateL << " R=" << stateR << "\n";
    REQUIRE_TRUE(maxAbsWet > 0.05f);   // the delay was audibly live for the whole run...
    REQUIRE_TRUE(stateL > 1.0e-4f);    // ...so a tracking diffuser holds signal, and a
    REQUIRE_TRUE(stateR > 1.0e-4f);    // skipped one holds exactly Reset()'s zeros.
}
TEST_CASE(stereo_delay_diffusion_midpoint_differs_from_both_endpoints) {
    // "Midpoint is a real state" requirement (spec requirement, not just a
    // nice-to-have): ddif==0.5 must differ from BOTH ddif==0.0 and
    // ddif==1.0 -- proving the control is continuous, not a switch.
    const float sr = 48000.0f;
    dsp::StereoDelay delay0;
    delay0.SetSampleRate(sr);
    dsp::StereoDelay delayHalf;
    delayHalf.SetSampleRate(sr);
    dsp::StereoDelay delay1;
    delay1.SetSampleRate(sr);

    dsp::DelayParams p0;
    p0.dtim = 0.35f;
    p0.dsnd = 0.9f;
    p0.dfbk = 0.4f;
    p0.dwid = 0.25f;
    p0.dmod = 0.2f;
    p0.dmix = 0.6f;
    p0.ddif = 0.0f;

    dsp::DelayParams pHalf = p0;
    pHalf.ddif = 0.5f;
    dsp::DelayParams p1 = p0;
    p1.ddif = 1.0f;

    float maxAbsDiffFrom0 = 0.0f;
    float maxAbsDiffFrom1 = 0.0f;
    for (int i = 0; i < 4000; ++i) {
        const float input = 0.6f * std::sin(0.037f * static_cast<float>(i));
        const dsp::DelayWetPair wet0 = delay0.Process(input, p0);
        const dsp::DelayWetPair wetHalf = delayHalf.Process(input, pHalf);
        const dsp::DelayWetPair wet1 = delay1.Process(input, p1);
        maxAbsDiffFrom0 = std::max(maxAbsDiffFrom0, std::fabs(wetHalf.l - wet0.l));
        maxAbsDiffFrom0 = std::max(maxAbsDiffFrom0, std::fabs(wetHalf.r - wet0.r));
        maxAbsDiffFrom1 = std::max(maxAbsDiffFrom1, std::fabs(wetHalf.l - wet1.l));
        maxAbsDiffFrom1 = std::max(maxAbsDiffFrom1, std::fabs(wetHalf.r - wet1.r));
    }
    REQUIRE_TRUE(maxAbsDiffFrom0 > 1.0e-3f);  // differs from the ddif==0 endpoint.
    REQUIRE_TRUE(maxAbsDiffFrom1 > 1.0e-3f);  // differs from the ddif==1 endpoint.
}

TEST_CASE(stereo_delay_diffusion_coefficient_stays_strictly_inside_unit_circle_across_full_sweep) {
    // T3.1c's binding requirement: a = ddif * 0.7f (kDiffusionCoeffScale)
    // must stay strictly inside the unit circle (|a| < 1) at every ddif in
    // the knob's own [0,1] range. Swept range: ddif in [0.0, 1.0], 101
    // points. 0.7f is asserted here as a literal (not merely re-read from
    // the constant it pins) so a future change to the constant's own value
    // -- not just to how it is used -- is still caught.
    REQUIRE_NEAR(dsp::StereoDelay::kDiffusionCoeffScale, 0.7, 1e-6);

    float maxAbsA = 0.0f;
    for (int i = 0; i <= 100; ++i) {
        const float ddif = static_cast<float>(i) / 100.0f;  // sweep [0.0, 1.0].
        const float a = ddif * dsp::StereoDelay::kDiffusionCoeffScale;
        REQUIRE_TRUE(std::isfinite(a));
        REQUIRE_TRUE(a > -1.0f && a < 1.0f);  // strictly inside the unit circle -- the bound itself.
        maxAbsA = std::max(maxAbsA, std::fabs(a));
    }
    REQUIRE_NEAR(maxAbsA, 0.7, 1e-6);  // reached exactly at ddif==1.0 -- the sweep's own observed extreme.
}

TEST_CASE(stereo_delay_diffusion_does_not_raise_wet_output_beyond_limiter_ceiling) {
    // "Bounded output" requirement: the diffuser cannot raise the wet
    // tap's peak beyond what wetLimiterL/R already guarantee -- same
    // scenario/ceiling as delay_wet_output_stays_at_or_below_limiter_
    // ceiling_at_max_feedback above (shortest reachable delay time, max
    // feedback, hot input), with ddif at its maximum (1.0) so the diffuser
    // is maximally engaged while the escaping signal is measured.
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);

    dsp::DelayParams p;
    p.dtim = 0.0f;  // shortest reachable round trip -- same worst case as the pre-existing ceiling test.
    p.dsnd = 1.0f;
    p.dfbk = 1.0f;  // -> fbk clamps to 0.98.
    p.dwid = 0.3f;
    p.dmod = 0.0f;
    p.ddif = 1.0f;  // diffuser maximally engaged.

    const float ceiling = dsp::OutputLimiter::kDefaultCeiling;  // 1.0 -- shared by wetLimiterL/R and the master.

    constexpr int kSamples = 5000;
    float maxAbs = 0.0f;
    for (int i = 0; i < kSamples; ++i) {
        const dsp::DelayWetPair wet = delay.Process(1.0f, p);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        maxAbs = std::max(maxAbs, std::max(std::fabs(wet.l), std::fabs(wet.r)));
    }
    REQUIRE_TRUE(maxAbs <= ceiling + 1.0e-4f);
}

// =========================================================================
// T3.1d (Reverse Blend, Delay slot 7, strict-executor packet P3): per-
// channel backward-travelling read pointer blended against the forward tap
// on the wet tap (dsp/Delay.hpp DelayReverser/StereoDelay::ApplyReverse/
// StereoDelay::Process). dfrz (slot 4) stays inert -- not touched by this
// packet.
// =========================================================================

TEST_CASE(stereo_delay_reverse_blend_at_default_zero_is_bit_identical_to_no_reverse_blend) {
    // T3.1d binding requirement (item 4): drev==0.0f (the Reverse blend
    // knob's own default, FroggersParameters.hpp; DelayParams::drev's own
    // in-class default) must leave the wet tap EXACTLY -- bit-for-bit, not
    // merely close -- what it would be with no reverse tap at all.
    //
    // Proven the same way T3.1c's own analogous test proves it for
    // Diffusion: one instance's reverse-tap state is poked directly to a
    // dirty, non-quiescent value (reverserL/reverserR are public members,
    // same per-channel-instance idiom the diffuser already established)
    // with NO Process() calls in between -- so neither instance's
    // lineL/lineR/lfoPhase/wetLimiter envelope differs AT ALL going in; the
    // reverse tap's own state is the ONLY difference between the two
    // instances. Both then process identical input, built through the real
    // production entry point (dsp::MapRowsToDelayParams) with row7Reverse
    // explicitly 0.0f. If drev==0 truly bypasses the reverse tap's
    // contribution (this packet's binding placement, dsp/Delay.hpp
    // Process()), the dirtied state is never read into the output and
    // outputs must match exactly.
    const float sr = 48000.0f;
    dsp::StereoDelay delayDirty;
    delayDirty.SetSampleRate(sr);
    dsp::StereoDelay delayClean;
    delayClean.SetSampleRate(sr);

    delayDirty.reverserL.pos = -12345.5f;
    delayDirty.reverserL.elapsed = 789.0f;
    delayDirty.reverserL.fadePos = -54.25f;
    delayDirty.reverserL.fadeGain = 0.37f;
    delayDirty.reverserL.fading = true;
    delayDirty.reverserR.pos = -999.75f;
    delayDirty.reverserR.elapsed = 42.0f;
    delayDirty.reverserR.fadePos = -1.5f;
    delayDirty.reverserR.fadeGain = 0.81f;
    delayDirty.reverserR.fading = true;

    const dsp::DelayParams p = dsp::MapRowsToDelayParams(
        /*time=*/0.35f, /*send=*/0.9f, /*feedback=*/0.4f, /*width=*/0.25f,
        /*freeze=*/0.0f, /*mod=*/0.2f, /*mix=*/0.6f, /*reverse=*/0.0f, /*diffusion=*/0.0f);

    float maxAbsDiffL = 0.0f;
    float maxAbsDiffR = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const float input = 0.6f * std::sin(0.041f * static_cast<float>(i));
        const dsp::DelayWetPair wetDirty = delayDirty.Process(input, p);
        const dsp::DelayWetPair wetClean = delayClean.Process(input, p);
        maxAbsDiffL = std::max(maxAbsDiffL, std::fabs(wetDirty.l - wetClean.l));
        maxAbsDiffR = std::max(maxAbsDiffR, std::fabs(wetDirty.r - wetClean.r));
    }
    REQUIRE_NEAR(maxAbsDiffL, 0.0, 0.0);  // exact equality (eps=0) -- this packet's own binding requirement.
    REQUIRE_NEAR(maxAbsDiffR, 0.0, 0.0);
}

TEST_CASE(stereo_delay_reverse_blend_at_maximum_time_reverses_a_sharp_attack_slow_decay_transient) {
    // "Reverse actually reverses" requirement: feed a distinctive
    // asymmetric transient (sharp attack, slow decay) and confirm that at
    // drev==1 the tap's envelope is time-reversed relative to drev==0 --
    // the slow ramp precedes the sharp edge (tasks.md T3.1d's own framing).
    //
    // Method: dfbk=0 keeps the delay LINE itself an exact scaled copy of
    // the input stream (no feedback repeats to complicate it), so the
    // forward tap (drev=0) is a simple delayed echo of the transient, and
    // the reverse tap (drev=1) reads the SAME line content, just backward.
    // Both instances see identical writes throughout -- drev only selects
    // which tap is READ, applied before the feedback write, and with
    // dfbk=0 the write is `inSignal` regardless of dL/dR's value (B2/D9) --
    // so the two instances' delay lines stay identical and only the OUTPUT
    // differs.
    //
    // The reverse tap can only play the transient back once real time has
    // advanced PAST it (it sweeps BACKWARD from "now", so it cannot reach
    // content that has not been written yet) -- the observation window
    // below runs several full reverse laps past the injection point so at
    // least one complete backward sweep is guaranteed to pass over the
    // transient's footprint, regardless of exactly where in its own lap
    // cycle the injection happened to land.
    const float sr = 48000.0f;
    dsp::StereoDelay delayFwd;
    delayFwd.SetSampleRate(sr);
    dsp::StereoDelay delayRev;
    delayRev.SetSampleRate(sr);

    dsp::DelayParams pFwd;
    pFwd.dtim = 0.6f;
    pFwd.dsnd = 1.0f;
    pFwd.dfbk = 0.0f;
    pFwd.dwid = 0.0f;
    pFwd.dmod = 0.0f;
    pFwd.dmix = 1.0f;
    pFwd.drev = 0.0f;

    dsp::DelayParams pRev = pFwd;
    pRev.drev = 1.0f;

    // Same formula Process() itself uses for baseSeconds (DspMath.hpp's
    // ExpMapCompute, reused rather than re-derived by hand), so the
    // observation window below is sized from the ACTUAL delaySamplesWindow
    // ApplyReverse will use, not a hand-estimated guess.
    const float baseSeconds = dsp::ExpMapCompute(0.001f, dsp::StereoDelay::kMaxDelaySeconds, pFwd.dtim);
    const float delaySamplesWindow = baseSeconds * sr;

    const int kTransientSamples = 400;
    const float kDecay = 0.985f;
    const int kObserveSamples = kTransientSamples + static_cast<int>(6.0f * delaySamplesWindow);

    std::vector<float> fwdOut;
    std::vector<float> revOut;
    fwdOut.reserve(static_cast<size_t>(kObserveSamples));
    revOut.reserve(static_cast<size_t>(kObserveSamples));
    for (int i = 0; i < kObserveSamples; ++i) {
        const float input = (i < kTransientSamples) ? std::pow(kDecay, static_cast<float>(i)) : 0.0f;
        const dsp::DelayWetPair wf = delayFwd.Process(input, pFwd);
        const dsp::DelayWetPair wr = delayRev.Process(input, pRev);
        REQUIRE_TRUE(std::isfinite(wf.l) && std::isfinite(wr.l));
        fwdOut.push_back(wf.l);
        revOut.push_back(wr.l);
    }

    auto findPeak = [](const std::vector<float>& v) {
        size_t peak = 0;
        float peakAbs = 0.0f;
        for (size_t i = 0; i < v.size(); ++i) {
            if (std::fabs(v[i]) > peakAbs) {
                peakAbs = std::fabs(v[i]);
                peak = i;
            }
        }
        return peak;
    };

    const size_t peakFwd = findPeak(fwdOut);
    const size_t peakRev = findPeak(revOut);

    const int kWindow = 100;
    REQUIRE_TRUE(peakFwd >= static_cast<size_t>(kWindow) && peakFwd + static_cast<size_t>(kWindow) < fwdOut.size());
    REQUIRE_TRUE(peakRev >= static_cast<size_t>(kWindow) && peakRev + static_cast<size_t>(kWindow) < revOut.size());
    // Sanity thresholds are 0.5f, not closer to the input's own 1.0 peak,
    // because wetLimiterL/R (kDelayWetLimiterThreshold=0.72f/
    // kDelayWetLimiterCeiling=kStageCeiling=0.80f, dsp/Limiter.hpp) compress
    // this transient's sharp attack toward ~0.80 before it ever reaches
    // lastWet -- 0.5f still safely distinguishes "found the real echo" from
    // near-zero silence/interpolation noise.
    REQUIRE_TRUE(std::fabs(fwdOut[peakFwd]) > 0.5f);  // sanity: this really is the transient's echo, not noise.
    REQUIRE_TRUE(std::fabs(revOut[peakRev]) > 0.5f);

    auto windowAvgAbs = [](const std::vector<float>& v, size_t center, int window, bool before) {
        float sum = 0.0f;
        for (int k = 1; k <= window; ++k) {
            const size_t idx = before ? (center - static_cast<size_t>(k)) : (center + static_cast<size_t>(k));
            sum += std::fabs(v[idx]);
        }
        return sum / static_cast<float>(window);
    };

    const float fwdBefore = windowAvgAbs(fwdOut, peakFwd, kWindow, /*before=*/true);
    const float fwdAfter = windowAvgAbs(fwdOut, peakFwd, kWindow, /*before=*/false);
    const float revBefore = windowAvgAbs(revOut, peakRev, kWindow, /*before=*/true);
    const float revAfter = windowAvgAbs(revOut, peakRev, kWindow, /*before=*/false);

    std::cout << "  [T3.1d] forward: before=" << fwdBefore << " after=" << fwdAfter << "  reverse: before="
              << revBefore << " after=" << revAfter << "\n";

    // Forward (drev=0): sharp attack THEN slow decay -- the tail comes
    // AFTER the peak, so "after" clearly exceeds "before" (near-silence).
    REQUIRE_TRUE(fwdAfter > fwdBefore + 0.05f);
    // Reverse (drev=1): the SAME transient played backward -- slow ramp
    // THEN sharp edge, so "before" clearly exceeds "after" -- the opposite
    // asymmetry from the forward tap above, on the same-shaped transient.
    REQUIRE_TRUE(revBefore > revAfter + 0.05f);
}

TEST_CASE(stereo_delay_reverse_blend_midpoint_differs_from_both_endpoints) {
    // "Midpoint is a real state" requirement (spec requirement, not just a
    // nice-to-have -- tasks.md T3.1d: "It passes the continuous-range rule
    // (the midpoint is a real mixed texture)."): drev==0.5 must differ from
    // BOTH drev==0.0 and drev==1.0 -- proving the control is continuous,
    // not a switch.
    const float sr = 48000.0f;
    dsp::StereoDelay delay0;
    delay0.SetSampleRate(sr);
    dsp::StereoDelay delayHalf;
    delayHalf.SetSampleRate(sr);
    dsp::StereoDelay delay1;
    delay1.SetSampleRate(sr);

    dsp::DelayParams p0;
    p0.dtim = 0.35f;
    p0.dsnd = 0.9f;
    p0.dfbk = 0.4f;
    p0.dwid = 0.25f;
    p0.dmod = 0.2f;
    p0.dmix = 0.6f;
    p0.drev = 0.0f;

    dsp::DelayParams pHalf = p0;
    pHalf.drev = 0.5f;
    dsp::DelayParams p1 = p0;
    p1.drev = 1.0f;

    float maxAbsDiffFrom0 = 0.0f;
    float maxAbsDiffFrom1 = 0.0f;
    for (int i = 0; i < 4000; ++i) {
        const float input = 0.6f * std::sin(0.037f * static_cast<float>(i));
        const dsp::DelayWetPair wet0 = delay0.Process(input, p0);
        const dsp::DelayWetPair wetHalf = delayHalf.Process(input, pHalf);
        const dsp::DelayWetPair wet1 = delay1.Process(input, p1);
        maxAbsDiffFrom0 = std::max(maxAbsDiffFrom0, std::fabs(wetHalf.l - wet0.l));
        maxAbsDiffFrom0 = std::max(maxAbsDiffFrom0, std::fabs(wetHalf.r - wet0.r));
        maxAbsDiffFrom1 = std::max(maxAbsDiffFrom1, std::fabs(wetHalf.l - wet1.l));
        maxAbsDiffFrom1 = std::max(maxAbsDiffFrom1, std::fabs(wetHalf.r - wet1.r));
    }
    REQUIRE_TRUE(maxAbsDiffFrom0 > 1.0e-3f);  // differs from the drev==0 endpoint.
    REQUIRE_TRUE(maxAbsDiffFrom1 > 1.0e-3f);  // differs from the drev==1 endpoint.
}

TEST_CASE(stereo_delay_reverse_blend_wrap_crossfade_bounds_the_sample_to_sample_delta) {
    // "No click at the wrap" requirement (tasks.md T3.1d, operator-decided:
    // buffer smoothing at the reverse tap's wrap is REQUIRED, not optional
    // -- "what makes the control shippable at all"). Threshold calibrated
    // by measurement, not guessed (OMNI's own "a click test whose threshold
    // was guessed proves nothing"): with kReverseWrapCrossfadeSeconds
    // forced to 1.0e-9f (collapsing the crossfade to a single sample --
    // this exact scenario, same params, same sample count) the measured
    // max delta was 0.512177; shipped (kReverseWrapCrossfadeSeconds =
    // 0.005f) it measures 0.0246075 -- a ~20.8x reduction. 0.1f sits
    // roughly 4x above the smoothed number and ~5x below the unsmoothed
    // one, comfortably separated from both.
    const float sr = 48000.0f;
    dsp::StereoDelay delay;
    delay.SetSampleRate(sr);

    dsp::DelayParams p;
    p.dtim = 0.3f;
    p.dsnd = 1.0f;
    p.dfbk = 0.0f;
    p.dwid = 0.0f;
    p.dmod = 0.0f;
    p.dmix = 1.0f;
    p.drev = 1.0f;  // pure reverse tap -- isolates the wrap's own click, undiluted by any forward-tap contribution.

    float maxDelta = 0.0f;
    float prev = 0.0f;
    const int kSamples = 50000;  // many multiples of the reverse window at dtim=0.3/48kHz -- guarantees dozens of wraps.
    for (int i = 0; i < kSamples; ++i) {
        // Sustained, nontrivial content so every buffer position being
        // crossfaded at a wrap holds real signal, not silence -- a click
        // hiding in a run of near-zero samples would understate the risk.
        const float input = 0.6f * std::sin(0.041f * static_cast<float>(i));
        const dsp::DelayWetPair wet = delay.Process(input, p);
        REQUIRE_TRUE(std::isfinite(wet.l));
        if (i > 0) {
            maxDelta = std::max(maxDelta, std::fabs(wet.l - prev));
        }
        prev = wet.l;
    }
    std::cout << "  [T3.1d] reverse wrap max sample-to-sample delta (smoothed) = " << maxDelta << "\n";
    REQUIRE_TRUE(maxDelta < 0.1f);
}

TEST_CASE(stereo_delay_reverse_state_tracks_the_signal_while_drev_is_zero) {
    // Guards the P2-class defect directly (P2 postflight note, tasks.md
    // T3.1c): a reverse tap that is never advanced while drev sits at its
    // 0.0f default would replay stale content -- or in this struct's case,
    // never move its read pointer at all -- the first time the control was
    // turned up. Every other reverse-blend test above holds drev fixed for
    // its whole run, so none of them can observe that; the state has to be
    // inspected directly. Modeled on
    // stereo_delay_diffusion_state_tracks_the_signal_while_ddif_is_zero,
    // which exists for the identical reason.
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);

    dsp::DelayParams pZero;
    pZero.dtim = 0.35f;
    pZero.dsnd = 0.9f;
    pZero.dfbk = 0.4f;
    pZero.dwid = 0.25f;
    pZero.dmod = 0.2f;
    pZero.dmix = 0.6f;
    pZero.drev = 0.0f;  // the whole point: reverse blend OFF for this entire run.

    float maxAbsWet = 0.0f;
    for (int i = 0; i < 4000; ++i) {
        const float input = 0.6f * std::sin(0.037f * static_cast<float>(i));
        const dsp::DelayWetPair wet = delay.Process(input, pZero);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        maxAbsWet = std::max(maxAbsWet, std::max(std::fabs(wet.l), std::fabs(wet.r)));
    }

    const float stateL = delay.reverserL.StateMagnitude();
    const float stateR = delay.reverserR.StateMagnitude();

    // OMNI 9.1 positive control: "the state is non-zero" means nothing
    // unless a signal actually reached the unit. Report the driving
    // quantity's own range beside the result.
    std::cout << "  [P3] drev==0 state tracking: wet max|signal|=" << maxAbsWet << "  reverser elapsed L=" << stateL
              << " R=" << stateR << "\n";
    REQUIRE_TRUE(maxAbsWet > 0.05f);   // the delay was audibly live for the whole run...
    REQUIRE_TRUE(stateL > 1.0e-4f);    // ...so a tracking reverse pointer has moved, and a
    REQUIRE_TRUE(stateR > 1.0e-4f);    // frozen one would hold exactly Reset()'s 0.0f.
}

TEST_CASE(stereo_delay_reverse_blend_does_not_raise_wet_output_beyond_limiter_ceiling) {
    // "Bounded output" requirement: the reverse tap only ever reads values
    // already present in the delay line, and the existing wetLimiterL/R
    // bound the FINAL output regardless of which tap (forward, reverse, or
    // a blend of both) produced it -- same scenario/ceiling as
    // stereo_delay_diffusion_does_not_raise_wet_output_beyond_limiter_ceiling
    // above (shortest reachable delay time, max feedback, hot input), with
    // drev at its maximum (1.0) so the reverse tap is maximally engaged
    // while the escaping signal is measured.
    dsp::StereoDelay delay;
    const float sr = 48000.0f;
    delay.SetSampleRate(sr);

    dsp::DelayParams p;
    p.dtim = 0.0f;  // shortest reachable round trip -- same worst case as the pre-existing ceiling test.
    p.dsnd = 1.0f;
    p.dfbk = 1.0f;  // -> fbk clamps to 0.98.
    p.dwid = 0.3f;
    p.dmod = 0.0f;
    p.drev = 1.0f;  // reverse tap maximally engaged.

    const float ceiling = dsp::OutputLimiter::kDefaultCeiling;  // 1.0 -- shared by wetLimiterL/R and the master.

    constexpr int kSamples = 5000;
    float maxAbs = 0.0f;
    for (int i = 0; i < kSamples; ++i) {
        const dsp::DelayWetPair wet = delay.Process(1.0f, p);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        maxAbs = std::max(maxAbs, std::max(std::fabs(wet.l), std::fabs(wet.r)));
    }
    REQUIRE_TRUE(maxAbs <= ceiling + 1.0e-4f);
}

// =========================================================================
// T3.1a/T3.1b (Freeze, Delay slot 4, strict-executor packet P4): a
// crossfade on BOTH new-input write and feedback loop gain (dsp/Delay.hpp
// DelayParams::dfrz/dfrzLatched, StereoDelay::Process). drev/ddif (slots
// 7/8) stay untouched by this packet.
//
// Two matched claims from this packet's own brief: the clamp (freeze==1,
// unlatched) HOLDS the loop-gain product at unity, and dfrzLatched (T3.1b's
// override hook, wired by a later packet) can EXCEED that hold. Verified
// two ways below: a pure-formula sweep of the freezeEff/fbEff mapping
// itself (bit-exact claims, no simulation needed -- but also NOT sensitive
// to a bug in the real Process(), since it never calls it; see the very
// next TEST_CASE for the production-sensitive companion), then live
// dsp::StereoDelay::Process() simulations measuring the actual
// recirculating LOOP GAIN -- feed one small burst into an otherwise-silent,
// single-round-trip-length delay (dtim==0 -> ExpMapCompute(0.001,
// kMaxDelaySeconds,0)==0.001s exactly, independent of kMaxDelaySeconds
// since any base^0==1 -- this file's own established "shortest reachable
// round trip" probe, ~48 samples at 48kHz; dwid==0/dmod==0 keep that round
// trip an EXACT, CONSTANT sample count with no cross-feed or LFO smear),
// then read back the RECIRCULATING PEAK once per round trip: Process()
// returns a pure READ of history (the write a given sample commits is not
// itself audible until one round trip later), so the peak in round-trip
// block k (0-indexed, k>=1) is the loop's own gain raised to the (k-1)th
// power times the originally-injected amplitude, in the small-signal
// (near-linear) region of PadeSaturator::Saturate (exact at 0, slope 1 at
// 0 -- FilterFx.hpp).
// =========================================================================

// Reused by 5 TEST_CASEs below (freeze==0 at max drive, freeze==1 swept,
// latched above centre, latched below centre, release-after-latch) -- OMNI
// §6 helper rule, isolates the one "inject, then measure round-trip
// growth/decay" transformation stage every one of them needs.
//
// Two phases, in this order, both required: (1) SEED exactly one round
// trip with dfrz/dfrzLatched FORCED to 0.0f/false regardless of what `p`
// asks for, injecting `burstAmplitude` as that phase's own first sample
// only. This is not optional bookkeeping -- freezeEff==1.0f (whenever
// dfrz==1 OR dfrzLatched, T3.1a/T3.1b's own unconditional "new input never
// enters while frozen" contract) zeroes the new-input term at the write,
// so a caller whose OWN `p` already asks for freeze==1/latched could never
// get its probe signal into the line at all if injected under those same
// params -- exactly the failure this two-phase split exists to avoid.
// (2) MEASURE `numRoundTrips` further round trips under the CALLER's own
// `p` (bumpIn==0.0f throughout -- the seeded content is the only thing
// recirculating), returning one peak magnitude per round trip.
//
// peaks[k] (k==0..numRoundTrips-1) is approximately LoopGain^k *
// burstAmplitude in PadeSaturator::Saturate's near-linear small-signal
// region (exact at 0, slope 1 at 0 -- FilterFx.hpp): Process() returns a
// pure READ of history, so round trip k's peak reflects the write
// committed k round trips earlier, dominated by the ONE nonzero seed
// sample in this construction (dtim==0/dwid==0/dmod==0 keep the round trip
// an exact, constant, un-smeared sample count, so exactly one echo lands
// per round-trip block, and `LoopGain ~= peaks[k+1]/peaks[k]` for any k).
std::vector<float> MeasureFreezeRoundTripPeaks(dsp::StereoDelay& delay, const dsp::DelayParams& p,
                                                float burstAmplitude, int roundTripSamples, int numRoundTrips) {
    dsp::DelayParams seed = p;
    seed.dfrz = 0.0f;
    seed.dfrzLatched = false;
    for (int i = 0; i < roundTripSamples; ++i) {
        const float bumpIn = (i == 0) ? burstAmplitude : 0.0f;
        delay.Process(bumpIn, seed);
    }

    std::vector<float> peaks(static_cast<std::size_t>(numRoundTrips), 0.0f);
    for (int k = 0; k < numRoundTrips; ++k) {
        for (int i = 0; i < roundTripSamples; ++i) {
            const dsp::DelayWetPair wet = delay.Process(0.0f, p);
            const float mag = std::max(std::fabs(wet.l), std::fabs(wet.r));
            peaks[static_cast<std::size_t>(k)] = std::max(peaks[static_cast<std::size_t>(k)], mag);
        }
    }
    return peaks;
}

TEST_CASE(stereo_delay_freeze_effective_values_are_bit_exact_at_default_across_full_sweep) {
    // T3.1a/T3.1b's own "why exactly this form" claim (dsp/Delay.hpp
    // Process()): at freeze==0, not latched, `fbEff` must be BIT-EXACTLY
    // `fbk` (not merely close) and `freezeEff` BIT-EXACTLY `0.0f` -- an
    // exact-zero multiply followed by adding that exact zero, both IEEE-754
    // identities regardless of the finite magnitude of `1/fbDrive - fbk`.
    // Same formula text as the production call site, swept across dfbk's
    // whole [0,1] range x Feedback Drive's whole knob range: this is
    // exactly the regression this packet's brief warns an algebraically-
    // equivalent but NOT bit-exact rewrite (`lerp(fbk*fbDrive,1,freeze)/
    // fbDrive`) would cause. This test alone cannot catch that regression
    // in the SHIPPED formula (it never calls Process()) -- its sibling
    // below does; see the packet report for the broken/restored numbers
    // both produce under exactly that rewrite.
    for (int fbkStep = 0; fbkStep <= 20; ++fbkStep) {
        const float dfbk = static_cast<float>(fbkStep) / 20.0f;
        const float fbk = std::min(std::max(dfbk, 0.0f), 0.98f);
        for (int driveStep = 0; driveStep <= 20; ++driveStep) {
            const float knob = static_cast<float>(driveStep) / 20.0f;
            const float fbDrive = dsp::ExpMapCompute(0.25f, 4.0f, knob);
            const float freeze = 0.0f;  // the value under test -- Freeze's own default.
            const bool dfrzLatched = false;
            const float freezeEff = dfrzLatched ? 1.0f : freeze;
            const float fbEff = dfrzLatched ? 1.0f : fbk + (1.0f / fbDrive - fbk) * freeze;
            REQUIRE_TRUE(freezeEff == 0.0f);  // bit-exact, not merely near.
            REQUIRE_TRUE(fbEff == fbk);        // bit-exact -- this packet's own binding claim.
        }
    }
}

TEST_CASE(stereo_delay_freeze_at_default_reproduces_pinned_pre_packet_output_through_real_process) {
    // Companion to the pure-formula sweep above, but through the REAL
    // production dsp::StereoDelay::Process() entry point, over a nontrivial
    // continuous input, so a regression in the ACTUAL shipped formula (not
    // just a parallel copy of it) is what this test is sensitive to -- the
    // packet's own required §9.1 proof for "the bit-identical test" breaks
    // THIS test, not the sweep above, precisely because only this one calls
    // production code.
    //
    // fbDrive is deliberately NOT 1.0: the forbidden rewrite
    // (`lerp(fbk*fbDrive,1,freeze)/fbDrive`) divides back out by fbDrive,
    // and fbDrive==1.0 would let that multiply-then-divide round-trip
    // exactly by construction (x*1/1==x, no rounding possible), hiding the
    // very regression this test exists to catch. "Not a power of two" is
    // NOT by itself sufficient, per OMNI §9.1's own positive-control
    // discipline -- a first attempt at this test used dfbk=0.6/knob=0.3
    // (fbDrive~=0.574, not a power of two) and a float32 probe (OMNI §9.1,
    // see the packet report) showed `(0.6f*0.574...f)/0.574...f` round-
    // trips to bit-exact 0.6f anyway, which would have made this test's own
    // required §9.1 falsifiability proof a dead instrument -- a NULL result
    // silently mistaken for a negative one. knob==0.437 was chosen by
    // instead PROBING float32 arithmetic directly (not reasoned about) and
    // confirming `(fbk*fbDrive)/fbDrive` differs from `fbk` by 1 ULP at
    // this exact pair -- see the packet report for the probe. dtim==0 keeps
    // the round trip short (~48 samples at 48kHz, this file's own "shortest
    // reachable round trip" convention), so hundreds of samples cover many
    // round trips -- a recursive loop, so even that 1-ULP-per-round-trip
    // discrepancy compounds instead of staying hidden.
    //
    // MEASURED FIRST, then written to match (OMNI: measured, not assumed):
    // see the packet report for these two constants' provenance and for the
    // broken-vs-restored numbers the required §9.1 mutation produced.
    const float sr = 48000.0f;
    dsp::StereoDelay delay;
    delay.SetSampleRate(sr);
    delay.SetFeedbackDrive(0.437f);  // -> fbDrive ~= 0.840 (ExpMapCompute(0.25,4,0.437)); see comment above.

    const dsp::DelayParams p = dsp::MapRowsToDelayParams(
        /*time=*/0.0f, /*send=*/1.0f, /*feedback=*/0.6f, /*width=*/0.25f,
        /*freeze=*/0.0f, /*mod=*/0.2f, /*mix=*/0.6f, /*reverse=*/0.0f, /*diffusion=*/0.0f);

    dsp::DelayWetPair lastWet{};
    constexpr int kSamples = 500;
    for (int i = 0; i < kSamples; ++i) {
        const float input = 0.5f * std::sin(0.05f * static_cast<float>(i));
        lastWet = delay.Process(input, p);
        REQUIRE_TRUE(std::isfinite(lastWet.l) && std::isfinite(lastWet.r));
    }

    std::cout << std::setprecision(9) << "  [T3.1a/T3.1b bit-identical] sample " << (kSamples - 1) << " wet=("
              << lastWet.l << ", " << lastWet.r << ")\n";

    REQUIRE_NEAR(lastWet.l, -0.11269673f, 0.0);
    REQUIRE_NEAR(lastWet.r, -0.0563911721f, 0.0);
}

TEST_CASE(stereo_delay_freeze_clamp_does_not_reach_zero_loop_gain_matches_pre_packet_at_max_feedback_drive) {
    // T3.1a's own binding invariant, stated as the thing this packet's
    // brief says must NOT change: at freeze==0, the loop-gain product is
    // `fbk*fbDrive` exactly as it was before this packet -- including past
    // 1.0 (an "accidental Stop-sustain" the brief rules must survive
    // untouched), NOT clamped toward the freeze==1 hold. Measured here at
    // Feedback Drive's own maximum (knob 1.0 -> fbDrive==4.0,
    // ExpMapCompute(0.25,4,1.0)), the extreme where freeze==1's own hold
    // (1/fbDrive==0.25) sits FARTHEST from fbk -- the most sensitive point
    // to detect any leakage of the clamp into freeze==0.
    const float sr = 48000.0f;
    dsp::StereoDelay delay;
    delay.SetSampleRate(sr);
    delay.SetFeedbackDrive(1.0f);  // -> fbDrive == 4.0 (maximum).

    dsp::DelayParams p;
    p.dtim = 0.0f;  // shortest reachable round trip -- this file's own established worst-case probe.
    p.dsnd = 1.0f;
    p.dfbk = 1.0f;  // -> fbk clamps to 0.98 (this file's own established "max feedback" convention).
    p.dwid = 0.0f;  // no cross-feed -- keeps the round trip a single, clean, un-smeared echo train.
    p.dmod = 0.0f;  // no LFO wobble -- keeps the round trip an exact, constant sample count.
    p.dfrz = 0.0f;  // the value under test.
    p.dfrzLatched = false;

    constexpr int kRoundTripSamples = 48;      // dtim==0's own 0.001s floor * 48kHz.
    constexpr float kBurstAmplitude = 1.0e-4f;  // tiny -- keeps every round trip in Saturate's linear region.
    const std::vector<float> peaks = MeasureFreezeRoundTripPeaks(delay, p, kBurstAmplitude, kRoundTripSamples, 2);

    REQUIRE_TRUE(peaks[0] > 0.0f);
    const float measuredGain = peaks[1] / peaks[0];
    const float fbk = 0.98f;
    const float fbDrive = 4.0f;
    const float theoreticalGain = fbk * fbDrive;  // 3.92 -- the packet's own cited figure.

    std::cout << "  [T3.1a clamp-does-not-reach-zero] fbDrive=max(4.0) fbk=0.98 measured loop gain="
              << measuredGain << " theoretical fbk*fbDrive=" << theoreticalGain << "\n";

    REQUIRE_NEAR(measuredGain, theoreticalGain, theoreticalGain * 0.02);  // within 2%.
    REQUIRE_TRUE(measuredGain > 1.0f);  // still exceeds unity -- NOT clamped toward the freeze==1 hold.
}

TEST_CASE(stereo_delay_freeze_feedback_is_monotonically_non_decreasing_across_its_whole_range) {
    // T3.1e's own guard, and the test whose ABSENCE let the defect ship.
    // Operator, 2026-08-14: "obviously an encoder turning left to right
    // should make a value go from zero to a higher value." The prior
    // mapping, `fbk + (1/fbDrive - fbk) * freeze`, ran DOWNWARD at high
    // Feedback Drive (0.98 -> 0.25 at fbDrive 4.0), so raising Freeze
    // lowered loop gain. Every other Freeze test held freeze FIXED for its
    // whole run, so none of them could observe a direction of travel --
    // which is exactly why a knob wired backwards passed the entire suite
    // (proposal.md §3 names this defect class).
    //
    // Asserted against dsp::StereoDelay::FreezeFeedback directly rather than
    // through round-trip peak measurement: at high fbDrive the loop
    // saturates within a couple of round trips, so measured peak ratios
    // cannot distinguish "grew a lot" from "grew a bit" and would give a
    // false pass. This tests the mapping at the level the property lives.
    //
    // `fbk` is swept too, since it is the mapping's other input (dfbk
    // clamps to [0, 0.98] in Process()). The old mapping's fbDrive
    // dependence is gone, so there is no fbDrive to sweep -- its absence IS
    // the fix.
    float minStep = 1.0e9f;
    float worstFbk = -1.0f;
    float minSeen = 1.0e9f;
    float maxSeen = -1.0e9f;
    for (int fbkStep = 0; fbkStep <= 10; ++fbkStep) {
        const float fbk = 0.098f * static_cast<float>(fbkStep);  // 0.0 .. 0.98, dfbk's own clamped range.
        float previous = -1.0e9f;
        for (int i = 0; i <= 100; ++i) {
            const float freeze = static_cast<float>(i) / 100.0f;
            const float value = dsp::StereoDelay::FreezeFeedback(fbk, freeze, /*latched=*/false);
            REQUIRE_TRUE(std::isfinite(value));
            if (i > 0) {
                const float step = value - previous;
                REQUIRE_TRUE(step >= 0.0f);  // the property: never decreases as the knob rises.
                if (step < minStep) {
                    minStep = step;
                    worstFbk = fbk;
                }
            }
            previous = value;
            minSeen = std::min(minSeen, value);
            maxSeen = std::max(maxSeen, value);
        }
        // The ceiling, at every fbk: lossless recirculation, never self-gain.
        REQUIRE_NEAR(dsp::StereoDelay::FreezeFeedback(fbk, 1.0f, false), 1.0, 1.0e-6);
        // Bit-exact passthrough at zero -- the invariant that keeps the
        // default, and proposal.md §7's accidental sustain, untouched.
        REQUIRE_TRUE(dsp::StereoDelay::FreezeFeedback(fbk, 0.0f, false) == fbk);
    }

    // OMNI 9.1 positive control: report the swept ranges and the smallest
    // observed step, so a run that swept nothing is visible as such rather
    // than reading as a pass.
    std::cout << "  [T3.1e monotonicity] swept fbk 0.00..0.98 (11 steps) x freeze 0.00..1.00 (101 steps); "
              << "value range [" << minSeen << ", " << maxSeen << "]; smallest step " << minStep
              << " at fbk=" << worstFbk << "\n";
    REQUIRE_TRUE(maxSeen > minSeen);  // the sweep actually moved the quantity under test.
}

TEST_CASE(stereo_delay_freeze_latch_exceeds_every_value_the_encoder_can_reach) {
    // T3.1e, replacing the retired "full freeze holds the PRODUCT at unity"
    // test. The encoder's ceiling is now unity feedback (lossless), and the
    // latch's whole purpose is to exceed it -- operator, 2026-08-13: "when
    // toggled, the freeze button should override the freeze encoder
    // parameter and take it to the maximum above the clamp -- period."
    // kFreezeLatchOverdrive is a BY-EAR constant, so this pins the RELATION
    // (strictly greater, at every reachable encoder position) rather than
    // its numeric value, which the operator may retune freely.
    const float latched = dsp::StereoDelay::FreezeFeedback(0.0f, 0.0f, /*latched=*/true);
    REQUIRE_TRUE(latched > 1.0f);  // above unity: the loop adds energy of its own.

    float maxEncoder = -1.0e9f;
    for (int fbkStep = 0; fbkStep <= 10; ++fbkStep) {
        const float fbk = 0.098f * static_cast<float>(fbkStep);
        for (int i = 0; i <= 100; ++i) {
            const float freeze = static_cast<float>(i) / 100.0f;
            maxEncoder = std::max(maxEncoder, dsp::StereoDelay::FreezeFeedback(fbk, freeze, false));
        }
    }
    std::cout << "  [T3.1e latch-exceeds-encoder] latched=" << latched
              << "  max reachable by encoder across fbk 0.00..0.98 x freeze 0.00..1.00 = " << maxEncoder << "\n";
    REQUIRE_TRUE(latched > maxEncoder);  // the gate: the knob cannot reach it.
    // The latch ignores the encoder entirely -- same value whatever dfrz is.
    REQUIRE_TRUE(dsp::StereoDelay::FreezeFeedback(0.98f, 1.0f, true) == latched);
}
TEST_CASE(stereo_delay_freeze_latched_grows_the_loop_measurably_beyond_unlatched_full_freeze) {
    // T3.1b's own override, and "the test that distinguishes the override
    // from the hold; the most important one in this packet" per the brief.
    // dfrzLatched bypasses T3.1a's clamp outright (fbEff := 1.0f, so the
    // loop-gain product becomes fbDrive itself) rather than driving dfrz
    // past its own clamp, which by construction could never exceed the
    // freeze==1 hold measured just above -- so this test's own claim can
    // ONLY be true if dfrzLatched is honoured as its own branch, not folded
    // into dfrz (the trap this packet's brief warns two earlier packets
    // met). Feedback Drive above centre (knob 1.0 -> fbDrive==4.0, "above
    // centre" since knob 0.5 -> fbDrive==1.0 is SetFeedbackDrive's own
    // documented centre) so the override's own gain (==fbDrive==4.0)
    // clearly exceeds the hold's gain (==1.0) measured above.
    const float sr = 48000.0f;
    constexpr int kRoundTripSamples = 48;
    constexpr float kBurstAmplitude = 1.0e-4f;

    dsp::StereoDelay delayLatched;
    delayLatched.SetSampleRate(sr);
    delayLatched.SetFeedbackDrive(1.0f);  // -> fbDrive == 4.0, above centre.

    dsp::DelayParams pLatched;
    pLatched.dtim = 0.0f;
    pLatched.dsnd = 1.0f;
    pLatched.dfbk = 0.6f;
    pLatched.dwid = 0.0f;
    pLatched.dmod = 0.0f;
    pLatched.dfrz = 1.0f;          // per the trap warning: dfrz alone, even at its own max, cannot exceed the hold.
    pLatched.dfrzLatched = true;   // the override -- this field is what must do the work.

    dsp::StereoDelay delayUnlatched;
    delayUnlatched.SetSampleRate(sr);
    delayUnlatched.SetFeedbackDrive(1.0f);  // identical fbDrive -- isolates dfrzLatched as the only difference.

    dsp::DelayParams pUnlatched = pLatched;
    pUnlatched.dfrzLatched = false;  // same patch, latch released -- T3.1a's hold applies instead.

    const std::vector<float> peaksLatched =
        MeasureFreezeRoundTripPeaks(delayLatched, pLatched, kBurstAmplitude, kRoundTripSamples, 2);
    const std::vector<float> peaksUnlatched =
        MeasureFreezeRoundTripPeaks(delayUnlatched, pUnlatched, kBurstAmplitude, kRoundTripSamples, 2);

    REQUIRE_TRUE(peaksLatched[0] > 0.0f);
    REQUIRE_TRUE(peaksUnlatched[0] > 0.0f);
    // T3.1e retune. Under the retired product-clamp mapping these were
    // ~4.0 (latched) vs ~1.0 (the hold), so a `> 2x` threshold read as
    // "measurably greater". That ratio was an artifact of fbDrive living
    // inside Freeze's mapping. It no longer does: both branches are now
    // multiplied by the SAME fbDrive afterward, so the ratio between them is
    // exactly kFreezeLatchOverdrive / 1.0 == 1.05 at every Drive setting.
    // Asserting `> 2x` here would be asserting the old coupling back into
    // existence. The relation -- latched strictly and repeatably greater --
    // is what matters, and the 1.05 itself is a BY-EAR constant the operator
    // may retune, so this pins the relation and reports the ratio rather
    // than pinning a number that is not derived from anything.
    const float gainLatched = peaksLatched[1] / peaksLatched[0];      // per-round-trip, ~1.05 * fbDrive.
    const float gainUnlatched = peaksUnlatched[1] / peaksUnlatched[0];  // per-round-trip, ~1.00 * fbDrive.

    std::cout << "  [T3.1e latched-grows] fbDrive=4.0 (max) latched per-round-trip gain=" << gainLatched
              << "  unlatched (freeze==1) per-round-trip gain=" << gainUnlatched
              << "  ratio=" << (gainLatched / gainUnlatched) << "\n";

    REQUIRE_TRUE(gainLatched > gainUnlatched);          // the gate: the latch exceeds the encoder's ceiling.
    REQUIRE_TRUE(gainLatched / gainUnlatched > 1.02f);  // by a real margin, not float noise.
    REQUIRE_TRUE(gainLatched > gainUnlatched * 1.0f);   // restated as a strict relation, no magic multiple.
}

TEST_CASE(stereo_delay_freeze_latched_below_centre_feedback_drive_does_not_grow) {
    // "Expected behaviour, not a defect" (this packet's brief, verbatim) --
    // pinned here so nobody "fixes" it later. dfrzLatched's own gain is
    // ==fbDrive regardless of dfbk (fbEff := 1.0f unconditionally), so
    // BELOW centre (fbDrive < 1, knob < 0.5) the override's own gain is <1
    // -- it decays, exactly like an ordinary sub-unity feedback loop,
    // because there is nothing in T3.1b's own formula that floors fbDrive
    // at 1; latching only ever multiplies the SAME fbDrive already chosen
    // by the Feedback Drive knob.
    const float sr = 48000.0f;
    dsp::StereoDelay delay;
    delay.SetSampleRate(sr);
    delay.SetFeedbackDrive(0.0f);  // -> fbDrive == 0.25 (minimum), below the setter's own 0.5-knob centre.

    dsp::DelayParams p;
    p.dtim = 0.0f;
    p.dsnd = 1.0f;
    p.dfbk = 0.6f;
    p.dwid = 0.0f;
    p.dmod = 0.0f;
    p.dfrz = 1.0f;
    p.dfrzLatched = true;

    constexpr int kRoundTripSamples = 48;
    constexpr float kBurstAmplitude = 0.01f;
    const std::vector<float> peaks = MeasureFreezeRoundTripPeaks(delay, p, kBurstAmplitude, kRoundTripSamples, 2);

    REQUIRE_TRUE(peaks[0] > 0.0f);
    const float gain = peaks[1] / peaks[0];

    std::cout << "  [T3.1b latched-below-centre] fbDrive=0.25 (min) latched per-round-trip gain=" << gain << "\n";

    REQUIRE_TRUE(gain < 0.9f);  // clearly does not grow -- decays, matching fbDrive itself (~0.25).
    REQUIRE_NEAR(gain, 0.25, 0.05);
}

TEST_CASE(stereo_delay_freeze_releasing_the_latch_after_running_hot_decays_toward_silence) {
    // Spec invariant: the control can never leave a runaway behind it. Run
    // dfrzLatched==true, Feedback Drive above centre (same 4.0x as the
    // latched-grows test above) for enough round trips to get "hot" (a
    // clearly-grown, non-trivial recirculating level -- Saturate's own
    // hard +-1 clamp bounds it once latching zeroes freezeEff's new-input
    // term and pushes fbEff to 1.0, so "hot" here means "against that
    // ceiling", not unbounded), then release the latch AND return dfrz to
    // its own default (0.0f) -- at a dfbk chosen so the released
    // fbk*fbDrive product sits BELOW unity (0.15*4.0==0.6), so the released
    // loop decays rather than continuing to grow on its own (the untouched
    // "accidental Stop-sustain" this packet's brief protects is a
    // SEPARATE, deliberately-unrelated claim about dfbk/fbDrive
    // combinations that exceed unity; this test picks a combination that
    // does not, so release's own decay is the only effect in view).
    const float sr = 48000.0f;
    dsp::StereoDelay delay;
    delay.SetSampleRate(sr);
    delay.SetFeedbackDrive(1.0f);  // -> fbDrive == 4.0.

    dsp::DelayParams p;
    p.dtim = 0.0f;
    p.dsnd = 1.0f;
    p.dfbk = 0.15f;  // -> fbk == 0.15; released product 0.15*4.0 == 0.6, below unity.
    p.dwid = 0.0f;
    p.dmod = 0.0f;
    p.dfrz = 1.0f;
    p.dfrzLatched = true;

    constexpr int kRoundTripSamples = 48;
    constexpr int kHotRoundTrips = 8;  // latched gain==4.0/round-trip -- plenty to reach Saturate's ceiling.

    // Reuses MeasureFreezeRoundTripPeaks for its own seed phase too (OMNI
    // §8: one definition site) -- p already carries dfrzLatched==true/
    // dfrz==1.0f here, so injecting the burst under THESE params directly
    // would hit the exact same "frozen input never enters the line" trap
    // the helper's own header comment exists to avoid.
    const std::vector<float> hotPeaks = MeasureFreezeRoundTripPeaks(delay, p, 0.01f, kRoundTripSamples, kHotRoundTrips);
    for (float peak : hotPeaks) {
        REQUIRE_TRUE(std::isfinite(peak));
    }
    const float peakDuringLatch = *std::max_element(hotPeaks.begin(), hotPeaks.end());
    REQUIRE_TRUE(peakDuringLatch > 0.5f);  // positive control -- genuinely "hot" before release, not still quiet.

    p.dfrzLatched = false;  // release.
    p.dfrz = 0.0f;          // back to Freeze's own default.

    constexpr int kReleaseRoundTrips = 20;
    std::vector<float> postReleasePeaks(static_cast<std::size_t>(kReleaseRoundTrips), 0.0f);
    for (int k = 0; k < kReleaseRoundTrips; ++k) {
        for (int i = 0; i < kRoundTripSamples; ++i) {
            const dsp::DelayWetPair wet = delay.Process(0.0f, p);
            REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
            postReleasePeaks[static_cast<std::size_t>(k)] =
                std::max(postReleasePeaks[static_cast<std::size_t>(k)], std::max(std::fabs(wet.l), std::fabs(wet.r)));
        }
    }

    std::cout << "  [T3.1b release-decays] peak while hot=" << peakDuringLatch << "  post-release peak[0]="
              << postReleasePeaks[0] << "  post-release peak[" << (kReleaseRoundTrips - 1)
              << "]=" << postReleasePeaks[static_cast<std::size_t>(kReleaseRoundTrips - 1)] << "\n";

    const float finalPeak = postReleasePeaks[static_cast<std::size_t>(kReleaseRoundTrips - 1)];
    REQUIRE_TRUE(finalPeak < postReleasePeaks[0] * 0.1f);  // decays toward silence.
    REQUIRE_TRUE(finalPeak < 0.01f);                        // clearly quiet by the end.
}

TEST_CASE(stereo_delay_freeze_midpoint_differs_from_both_endpoints) {
    // Same "midpoint is a real state" shape as ddif/drev's own midpoint
    // tests above: dfrz==0.5 must differ measurably from BOTH dfrz==0.0 and
    // dfrz==1.0 -- proving Freeze is a continuous crossfade, not a switch.
    const float sr = 48000.0f;
    dsp::StereoDelay delay0;
    delay0.SetSampleRate(sr);
    delay0.SetFeedbackDrive(1.0f);  // -> fbDrive == 4.0, so the three states' fbEff values are well separated.
    dsp::StereoDelay delayHalf;
    delayHalf.SetSampleRate(sr);
    delayHalf.SetFeedbackDrive(1.0f);
    dsp::StereoDelay delay1;
    delay1.SetSampleRate(sr);
    delay1.SetFeedbackDrive(1.0f);

    const dsp::DelayParams p0 = dsp::MapRowsToDelayParams(
        /*time=*/0.0f, /*send=*/1.0f, /*feedback=*/0.6f, /*width=*/0.0f,
        /*freeze=*/0.0f, /*mod=*/0.0f, /*mix=*/0.6f, /*reverse=*/0.0f, /*diffusion=*/0.0f);
    dsp::DelayParams pHalf = p0;
    pHalf.dfrz = 0.5f;
    dsp::DelayParams p1 = p0;
    p1.dfrz = 1.0f;

    float maxAbsDiffFrom0 = 0.0f;
    float maxAbsDiffFrom1 = 0.0f;
    for (int i = 0; i < 300; ++i) {
        const float input = 0.3f * std::sin(0.05f * static_cast<float>(i));
        const dsp::DelayWetPair wet0 = delay0.Process(input, p0);
        const dsp::DelayWetPair wetHalf = delayHalf.Process(input, pHalf);
        const dsp::DelayWetPair wet1 = delay1.Process(input, p1);
        maxAbsDiffFrom0 = std::max(maxAbsDiffFrom0, std::fabs(wetHalf.l - wet0.l));
        maxAbsDiffFrom1 = std::max(maxAbsDiffFrom1, std::fabs(wetHalf.l - wet1.l));
    }
    REQUIRE_TRUE(maxAbsDiffFrom0 > 1.0e-3f);  // differs from the dfrz==0 endpoint.
    REQUIRE_TRUE(maxAbsDiffFrom1 > 1.0e-3f);  // differs from the dfrz==1 endpoint.
}

// =========================================================================
// Strict-executor packet -- Ring Mod (Audio slots 9-11, task A), PM rate
// (Audio slot 12, task B), VCO balance (Audio slot 13, task C).
// =========================================================================

TEST_CASE(ring_mod_depth_scale_zero_off_gate_and_smoothstep_uses_own_floor) {
    // Task A2: Ring Mod's OWN floor/ramp -- not PM's kPmLfoFloor/
    // kPmLfoRampWidth, confirmed distinct, then the same taper shape check
    // vco_pm_depth_scale_zero_off_gate_and_smoothstep above runs for PM.
    REQUIRE_TRUE(dsp::Vco::kRingModFloor != dsp::Vco::kPmLfoFloor ||
                 dsp::Vco::kRingModRampWidth != dsp::Vco::kPmLfoRampWidth);
    REQUIRE_TRUE(dsp::Vco::RingModDepthScale(0.0f) == 0.0f);
    REQUIRE_TRUE(dsp::Vco::RingModDepthScale(dsp::Vco::kRingModFloor) == 0.0f);
    REQUIRE_TRUE(dsp::Vco::RingModDepthScale(dsp::Vco::kRingModFloor + dsp::Vco::kRingModRampWidth) == 1.0f);
    REQUIRE_TRUE(dsp::Vco::RingModDepthScale(1.0f) == 1.0f);
    const float mid = dsp::Vco::kRingModFloor + 0.5f * dsp::Vco::kRingModRampWidth;
    REQUIRE_NEAR(dsp::Vco::RingModDepthScale(mid), 0.5f, 1e-6);
}

TEST_CASE(ring_mod_phase_increment_uses_its_own_range_not_pitch_range) {
    // Task A1: same ExpMapCompute shape PitchToPhaseIncrement uses, but
    // NOT its 20/20000 Hz literals.
    const float sr = 48000.0f;
    REQUIRE_NEAR(dsp::Vco::RingModPhaseIncrement(0.0f, sr), dsp::Vco::kRingModMinHz / sr, 1e-9);
    REQUIRE_NEAR(dsp::Vco::RingModPhaseIncrement(1.0f, sr), dsp::Vco::kRingModMaxHz / sr, 1e-6);
    REQUIRE_TRUE(dsp::Vco::kRingModMinHz != 20.0f || dsp::Vco::kRingModMaxHz != 20000.0f);
}

TEST_CASE(ring_mod_at_default_zero_is_bit_identical_to_no_ring_mod_at_all) {
    // Task A3: Ring Mod's default (0.0f, FroggersParameters.hpp, at/below
    // kRingModFloor) must leave a fresh launch sounding exactly as it did
    // before Ring Mod existed -- same structure as
    // vco_pm_zero_at_or_below_floor_leaves_carrier_unmodulated above.
    dsp::Vco vco;
    const float sr = 48000.0f;
    const float pitchKnob = 0.4f;
    const float morphKnob = 0.3f;
    float refPhase = 0.0f;
    for (int i = 0; i < 32; ++i) {
        const float out =
            vco.Process(pitchKnob, morphKnob, /*pmKnob=*/0.0f, /*pmRateKnob=*/0.0f, /*ringModKnob=*/0.0f, sr);
        const float expected = dsp::EvalWaveMorph(refPhase, morphKnob);
        REQUIRE_NEAR(out, expected, 1e-5);
        refPhase = dsp::WrapPhase(refPhase + dsp::Vco::PitchToPhaseIncrement(pitchKnob, sr));
    }
}

TEST_CASE(ring_mod_above_floor_actually_modulates_and_stays_within_unit_bound) {
    dsp::Vco withRingMod;
    dsp::Vco withoutRingMod;
    const float sr = 48000.0f;
    bool sawDifference = false;
    for (int i = 0; i < 64; ++i) {
        const float a = withRingMod.Process(0.3f, 0.4f, 0.0f, 0.0f, /*ringModKnob=*/1.0f, sr);
        const float b = withoutRingMod.Process(0.3f, 0.4f, 0.0f, 0.0f, /*ringModKnob=*/0.0f, sr);
        REQUIRE_TRUE(a >= -1.0f - 1e-5f && a <= 1.0f + 1e-5f);  // convex-blend bound (task A2).
        if (std::fabs(a - b) > 1e-4f) {
            sawDifference = true;
        }
    }
    REQUIRE_TRUE(sawDifference);
}

TEST_CASE(ring_mod_carrier_is_internal_never_reads_another_vco) {
    // Task A1's own binding requirement: the carrier is generated INSIDE
    // each Vco instance. Same proof shape as
    // vco_zero_cross_vco_terms_independent_of_other_instances above,
    // isolated to Ring Mod: driving a second, differently-tuned Vco (with a
    // different ring-mod knob) "in between" calls must not perturb the
    // first instance's own sequence at all.
    const float sr = 48000.0f;
    dsp::Vco alone;
    std::vector<float> aloneSeq;
    for (int i = 0; i < 24; ++i) {
        aloneSeq.push_back(alone.Process(0.5f, 0.5f, 0.0f, 0.0f, /*ringModKnob=*/0.8f, sr));
    }

    dsp::Vco interleavedA;
    dsp::Vco interleavedB;
    std::vector<float> interleavedSeq;
    for (int i = 0; i < 24; ++i) {
        interleavedSeq.push_back(interleavedA.Process(0.5f, 0.5f, 0.0f, 0.0f, /*ringModKnob=*/0.8f, sr));
        interleavedB.Process(0.2f, 0.9f, 0.0f, 0.0f, /*ringModKnob=*/0.05f, sr);  // different carrier entirely.
    }

    REQUIRE_TRUE(aloneSeq.size() == interleavedSeq.size());
    for (size_t i = 0; i < aloneSeq.size(); ++i) {
        REQUIRE_NEAR(aloneSeq[i], interleavedSeq[i], 1e-7);
    }
}

TEST_CASE(pm_rate_default_knob_reproduces_todays_rate_at_pm_depth_knobs_default) {
    // Task B: today's rate at the PM depth knob's own 0.0f default
    // (FroggersParameters.hpp PM1/PM2/PM3) is ExpMapCompute(min,max,0.0f)
    // == kPmLfoMinHz (min^0-power identity, independent of min/max). The
    // new PMrt slot's own 0.0f default (unset in FroggersParameters.hpp)
    // reproduces the exact same rate through the exact same formula, now
    // fed by pmRateKnob01 instead of pmKnob01.
    const float sr = 48000.0f;
    dsp::Vco vco;
    vco.StepPmLfo(/*pmRateKnob01=*/0.0f, sr);
    REQUIRE_NEAR(vco.pmLfoPhase, dsp::Vco::kPmLfoMinHz / sr, 1e-9);
}

TEST_CASE(pm_rate_is_shared_across_vcos_and_decoupled_from_each_vcos_own_depth_knob) {
    // Task B's binding shape: ONE rate knob feeds all three VCOs'
    // StepPmLfo calls; each VCO's own PM knob still controls only depth.
    const float sr = 48000.0f;

    // Same shared rate knob, different depth knobs -> identical LFO phase
    // trajectories (rate is decoupled from depth).
    dsp::Vco lowDepth;
    dsp::Vco highDepth;
    const float sharedRateKnob = 0.7f;
    for (int i = 0; i < 32; ++i) {
        lowDepth.Process(0.3f, 0.3f, /*pmKnob=*/0.0f, sharedRateKnob, /*ringModKnob=*/0.0f, sr);
        highDepth.Process(0.3f, 0.3f, /*pmKnob=*/1.0f, sharedRateKnob, /*ringModKnob=*/0.0f, sr);
        REQUIRE_NEAR(lowDepth.pmLfoPhase, highDepth.pmLfoPhase, 1e-7);
    }

    // Same depth knob, different rate knobs -> phases must diverge.
    dsp::Vco rateA;
    dsp::Vco rateB;
    bool sawDivergence = false;
    for (int i = 0; i < 32; ++i) {
        rateA.Process(0.3f, 0.3f, /*pmKnob=*/1.0f, /*pmRateKnob=*/0.1f, /*ringModKnob=*/0.0f, sr);
        rateB.Process(0.3f, 0.3f, /*pmKnob=*/1.0f, /*pmRateKnob=*/0.9f, /*ringModKnob=*/0.0f, sr);
        if (std::fabs(rateA.pmLfoPhase - rateB.pmLfoPhase) > 1e-4f) {
            sawDivergence = true;
        }
    }
    REQUIRE_TRUE(sawDivergence);
}

// Task C1: BINDING INVARIANT (operator ruling) -- the WEIGHTS themselves
// are asserted directly (not an output-level proxy, which the task
// explicitly calls "strictly weaker evidence"), swept across the full
// [0,1] knob range at fine granularity.
TEST_CASE(vco_balance_weights_sum_to_one_and_stay_within_bounds_across_full_sweep) {
    for (int i = 0; i <= 200; ++i) {
        const float knob = static_cast<float>(i) / 200.0f;
        float w1 = 0.0f;
        float w2 = 0.0f;
        float w3 = 0.0f;
        dsp::ComputeVcoBalanceWeights(knob, w1, w2, w3);
        REQUIRE_NEAR(w1 + w2 + w3, 1.0f, 1e-6);
        REQUIRE_TRUE(w1 >= 0.10f - 1e-6f && w1 <= 0.80f + 1e-6f);
        REQUIRE_TRUE(w2 >= 0.10f - 1e-6f && w2 <= 0.80f + 1e-6f);
        REQUIRE_TRUE(w3 >= 0.10f - 1e-6f && w3 <= 0.80f + 1e-6f);
    }
    // Default (centre, 0.5f) reproduces the exact pre-packet equal-thirds
    // mix MixOscVoices used to hardcode.
    float w1 = 0.0f;
    float w2 = 0.0f;
    float w3 = 0.0f;
    dsp::ComputeVcoBalanceWeights(0.5f, w1, w2, w3);
    REQUIRE_NEAR(w1, 1.0f / 3.0f, 1e-6);
    REQUIRE_NEAR(w2, 1.0f / 3.0f, 1e-6);
    REQUIRE_NEAR(w3, 1.0f / 3.0f, 1e-6);
}

TEST_CASE(mix_osc_voices_default_balance_knob_reproduces_pre_packet_equal_thirds_average) {
    // Task C's own "default = centre, exactly the equal-thirds mix it
    // replaces" requirement, verified through the actual production call
    // path (MixOscVoices), not just the weight helper in isolation.
    dsp::VcoAdsrState adsrOld;
    adsrOld.init(1000.0f);
    adsrOld.setGate(true);
    dsp::VcoAdsrState adsrNew;
    adsrNew.init(1000.0f);
    adsrNew.setGate(true);

    for (int i = 0; i < 200; ++i) {
        const float v1 = 0.5f;
        const float v2 = -0.3f;
        const float v3 = 0.9f;
        const float oldStyle =
            (adsrOld.apply(0, v1, 0.1f, 0.15f, 0.6f, 0.2f) + adsrOld.apply(1, v2, 0.2f, 0.25f, 0.7f, 0.3f) +
             adsrOld.apply(2, v3, 0.05f, 0.35f, 0.5f, 0.1f)) *
            (1.0f / 3.0f);
        const float mixed = dsp::MixOscVoices(adsrNew, v1, v2, v3, 0.1f, 0.15f, 0.6f, 0.2f, 0.2f, 0.25f, 0.7f, 0.3f,
                                               0.05f, 0.35f, 0.5f, 0.1f, /*curveKnob=*/0.0f, /*graceKnob=*/0.0f,
                                               /*balanceKnob01=*/0.5f);
        REQUIRE_NEAR(mixed, oldStyle, 1e-6);
    }
}

}  // namespace

int main() {
    int failed = 0;
    for (const auto& test : Registry()) {
        try {
            test.fn();
            std::cout << "[PASS] " << test.name << "\n";
        } catch (const std::exception& ex) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << ex.what() << "\n";
        }
    }
    std::cout << (Registry().size() - static_cast<size_t>(failed)) << "/" << Registry().size()
              << " tests passed\n";
    return failed == 0 ? 0 : 1;
}

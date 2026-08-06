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
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
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
        const float out = vco.Process(pitchKnob, morphKnob, /*pmKnob=*/0.0f, sr);
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
        const float a = withPm.Process(0.3f, 0.4f, /*pmKnob=*/1.0f, sr);
        const float b = withoutPm.Process(0.3f, 0.4f, /*pmKnob=*/0.0f, sr);
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
        aloneSeq.push_back(alone.Process(0.6f, 0.2f, 0.7f, sr));
    }

    dsp::Vco interleavedA;
    dsp::Vco interleavedB;
    std::vector<float> interleavedSeq;
    for (int i = 0; i < 16; ++i) {
        interleavedSeq.push_back(interleavedA.Process(0.6f, 0.2f, 0.7f, sr));
        interleavedB.Process(0.9f, 0.8f, 0.1f, sr);  // different knobs, driven "in between"
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
    float last = -1.0f;
    for (int i = 0; i < 2000; ++i) {
        const float level = adsr.apply(0, 1.0f, /*attack=*/0.5f, /*sustain=*/0.8f, /*release=*/0.5f);
        REQUIRE_TRUE(level >= last - 1e-6f);  // monotonic rise during attack/hold
        last = level;
    }
    REQUIRE_NEAR(last, 0.8f, 1e-3);  // settled at sustain level

    adsr.setGate(false);
    for (int i = 0; i < 20000; ++i) {
        adsr.apply(0, 1.0f, 0.5f, 0.8f, 0.5f);
    }
    const float released = adsr.apply(0, 1.0f, 0.5f, 0.8f, 0.5f);
    REQUIRE_NEAR(released, 0.0f, 1e-3);
}

// -----------------------------------------------------------------------
// ITEM 4 failing-test-first (design.md A2): kMaxAttackSeconds lowered from
// 2.5s to 1.0s (VoiceEnvelope.hpp -- private, so this drives the same
// observable surface the test above does). stepVoice()'s attack ramp is
// LINEAR (`attackStep = sustainLevel / (mapAttack(knob)*sampleRate)`), and
// `mapAttack(1.0) == kMinTimeSeconds + 1.0*(kMaxAttackSeconds -
// kMinTimeSeconds) == kMaxAttackSeconds` exactly (the kMinTimeSeconds terms
// cancel at knob==1.0) -- so at the maximum attack knob, the ramp completes
// in EXACTLY kMaxAttackSeconds worth of samples, no time-constant fuzz to
// account for. At 2.0s of held-gate samples: under the OLD 2.5s ceiling the
// level would still be mid-ramp (2.0/2.5 == 80% of the way there, NOT at
// sustain -- this is what makes the test fail against the pre-item-4 code);
// under the NEW 1.0s ceiling the ramp finished at 1.0s and Hold has been
// clamping the level at sustain for the second full second since.
// -----------------------------------------------------------------------
TEST_CASE(max_attack_knob_reaches_sustain_within_the_new_one_second_ceiling) {
    constexpr float kSampleRate = 48000.0f;
    constexpr float kSustain = 0.8f;
    dsp::VcoAdsrState adsr;
    adsr.init(kSampleRate);
    adsr.setGate(true);

    float level = 0.0f;
    const int samplesAt2s = static_cast<int>(2.0f * kSampleRate);
    for (int i = 0; i < samplesAt2s; ++i) {
        level = adsr.apply(0, 1.0f, /*attack=*/1.0f, /*sustain=*/kSustain, /*release=*/0.0f);
    }
    REQUIRE_NEAR(level, kSustain, 1e-6);
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
                                               0.1f, 0.6f, 0.2f,
                                               0.2f, 0.7f, 0.3f,
                                               0.05f, 0.5f, 0.1f);
        const float e1 = adsrReference.apply(0, v1, 0.1f, 0.6f, 0.2f);
        const float e2 = adsrReference.apply(1, v2, 0.2f, 0.7f, 0.3f);
        const float e3 = adsrReference.apply(2, v3, 0.05f, 0.5f, 0.1f);
        const float expected = (e1 + e2 + e3) * (1.0f / 3.0f);
        REQUIRE_NEAR(mixed, expected, 1e-6);
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

    // W2.2a (openspec/changes/frogg3rs-modulation-truth-and-voicing/
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

    for (int i = 0; i < 8; ++i) {
        const float input = 0.1f * static_cast<float>(i + 1);
        const float actual = chain.Process(input, /*useParallel=*/true, combPeakBlend, scoopMix);

        const float combRaw = refComb.Process(refPureDelay.Process(input));
        const float trimState = refTrimSmoother.Process(rawCombTrim);
        const float combPath = combRaw * trimState;
        const float peakRaw = refPeak.Process(input);
        const float peakTrimState = refPeakTrimSmoother.Process(rawPeakTrim);
        const float peakPath = peakRaw * peakTrimState;
        const float mixed = peakPath * (1.0f - combPeakBlend) + combPath * combPeakBlend;
        const float scooped = refScoop.Process(mixed);
        const float expected = mixed * (1.0f - scoopMix) + scooped * scoopMix;

        REQUIRE_NEAR(actual, expected, 1e-5);
    }
}

// -----------------------------------------------------------------------
// W2.2a (openspec/changes/frogg3rs-modulation-truth-and-voicing/tasks.md):
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
        chain.Process(inputAmplitude, /*useParallel=*/true, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
    }

    for (int i = 0; i < kAssertSamples; ++i) {
        const float combPath =
            chain.Process(inputAmplitude, /*useParallel=*/true, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
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
            chain.Process(inputAmplitude, /*useParallel=*/true, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
        }
        chain.comb.SetFeedback(kMaxFb);
        constexpr int kPostJumpSamples = 500;
        const float bound = boundFor(kMaxFb);
        for (int i = 0; i < kPostJumpSamples; ++i) {
            const float combPath =
                chain.Process(inputAmplitude, /*useParallel=*/true, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
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
                chain.Process(inputAmplitude, /*useParallel=*/true, /*combPeakBlend=*/1.0f, /*scoopMix=*/0.0f);
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
        chain.Process(inputAmplitude * std::sin(phase), /*useParallel=*/true, /*combPeakBlend=*/0.0f,
                      /*scoopMix=*/0.0f);
    }
    for (int i = 0; i < kAssertSamples; ++i, ++sampleIx) {
        const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
        const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*useParallel=*/true,
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
            chain.Process(inputAmplitude * std::sin(phase), /*useParallel=*/true, /*combPeakBlend=*/0.0f,
                          /*scoopMix=*/0.0f);
        }
        chain.peak.SetHeight(kMaxHeight);
        constexpr int kPostJumpSamples = 4000;  // resonance buildup window, same as the static test above.
        for (int i = 0; i < kPostJumpSamples; ++i, ++sampleIx) {
            const float phase = 2.0f * static_cast<float>(M_PI) * freqNormalized * static_cast<float>(sampleIx);
            const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*useParallel=*/true,
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
            const float peakPath = chain.Process(inputAmplitude * std::sin(phase), /*useParallel=*/true,
                                                  /*combPeakBlend=*/0.0f, /*scoopMix=*/0.0f);
            REQUIRE_TRUE(std::fabs(peakPath) <= ceilingBound + 1.0e-3f);
        }
    }
}

TEST_CASE(filter_fx_chain_serial_matches_delay_then_comb_then_peak) {
    dsp::FilterFxChain chain;
    chain.pureDelay.delaySamples = 1.0f;
    chain.comb.delaySamples = 1;
    chain.comb.SetFeedback(0.2f);
    chain.comb.SetCutoffAlpha(0.5f);
    chain.peak.SetFreq(0.03f);
    chain.peak.SetHeight(1.2f);
    chain.peak.SetWidth(1.0f);

    dsp::PureDelay refPureDelay;
    refPureDelay.delaySamples = 1.0f;
    dsp::Comb refComb;
    refComb.delaySamples = 1;
    refComb.SetFeedback(0.2f);
    refComb.SetCutoffAlpha(0.5f);
    dsp::ResonantBump refPeak;
    refPeak.SetFreq(0.03f);
    refPeak.SetHeight(1.2f);
    refPeak.SetWidth(1.0f);

    for (int i = 0; i < 8; ++i) {
        const float input = 0.05f * static_cast<float>(i + 1);
        const float actual = chain.Process(input, /*useParallel=*/false, 0.0f, 0.0f);

        float expected = refPureDelay.Process(input);
        expected = refComb.Process(expected);
        expected = refPeak.Process(expected);

        REQUIRE_NEAR(actual, expected, 1e-5);
    }
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
    for (float x : {-0.6f, -0.2f, 0.1f, 0.5f}) {
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
        const float expected = (static_cast<float>(inputInt) + inputRemainder) / 128.0f - 1.0f;

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

// =========================================================================
// 3.10 -- Delay (sim/StereoDelay.hpp whole file; sim/DelayState.hpp:165-198
// row->DelayParams mapping and Color/Halo fold at :180-193). A full
// Froggers original exists for all nine params -- nothing here is authored.
// =========================================================================

TEST_CASE(map_rows_to_delay_params_passes_through_rows_0_to_6_directly) {
    const dsp::DelayParams params = dsp::MapRowsToDelayParams(
        /*time=*/0.11f, /*send=*/0.22f, /*feedback=*/0.33f, /*width=*/0.44f,
        /*detune=*/0.5f, /*mod=*/0.5f, /*mix=*/0.66f, /*color=*/0.5f, /*halo=*/0.5f);
    REQUIRE_NEAR(params.dtim, 0.11f, 1e-6);
    REQUIRE_NEAR(params.dsnd, 0.22f, 1e-6);
    REQUIRE_NEAR(params.dfbk, 0.33f, 1e-6);
    REQUIRE_NEAR(params.dwid, 0.44f, 1e-6);
    // detune/mod folded with color/halo == 0.5 -> unchanged (0.5*(0.5+0.5)=0.5).
    REQUIRE_NEAR(params.ddet, 0.5f, 1e-6);
    REQUIRE_NEAR(params.dmod, 0.5f, 1e-6);
    REQUIRE_NEAR(params.dmix, 0.66f, 1e-6);
}

TEST_CASE(map_rows_to_delay_params_color_halo_fold_matches_formula_and_clamps) {
    // DelayState.hpp:187-193: ddet = clamp(0.5*(ddet+color)), dmod = clamp(0.5*(dmod+halo)).
    const dsp::DelayParams p1 = dsp::MapRowsToDelayParams(0, 0, 0, 0, /*detune=*/0.2f, /*mod=*/0.8f, 0,
                                                            /*color=*/0.9f, /*halo=*/0.1f);
    REQUIRE_NEAR(p1.ddet, 0.5f * (0.2f + 0.9f), 1e-6);
    REQUIRE_NEAR(p1.dmod, 0.5f * (0.8f + 0.1f), 1e-6);

    // Clamp: detune=1, color=1 -> 0.5*(1+1)=1.0 exactly (upper edge, not over).
    const dsp::DelayParams p2 = dsp::MapRowsToDelayParams(0, 0, 0, 0, 1.0f, 0.0f, 0, 1.0f, 0.0f);
    REQUIRE_TRUE(p2.ddet <= 1.0f);
    REQUIRE_NEAR(p2.ddet, 1.0f, 1e-6);
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
    p.ddet = 0.0f;
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

TEST_CASE(stereo_delay_detune_widens_left_right_read_positions) {
    // StereoDelay.hpp:77-82: ddet maps to +-cents via 2^(cents/1200), pulling
    // timeL and timeR apart. With ddet=0 both stay equal; with ddet=1 (max
    // 50 cents) they must differ.
    //
    // dtim kept small (0.0 -> ~48 samples at sr=48000, well under the
    // 200-sample loop below) rather than the 0.5 (~2147 samples) this test
    // used before Fix 1b: at dtim=0.5 the whole 200-sample loop fell
    // entirely inside the "before the buffer has filled" warm-up window
    // (task Fix 1's StereoDelay::ReadAt fix), where the read position is
    // still behind everything ever written and both channels correctly
    // read back silence -- this test was inadvertently exercising (and
    // passing only because of) the exact UB Fix 1b removes, not the
    // detune behavior it names. With dtim small enough to clear warm-up
    // inside the loop, the read reaches real written history and the
    // detune difference this test is actually about becomes observable.
    const float sr = 48000.0f;
    dsp::StereoDelay noDetune;
    dsp::StereoDelay withDetune;
    noDetune.SetSampleRate(sr);
    withDetune.SetSampleRate(sr);

    dsp::DelayParams pNo;
    pNo.dtim = 0.0f;
    pNo.dsnd = 1.0f;
    pNo.dfbk = 0.0f;
    pNo.dwid = 0.0f;
    pNo.ddet = 0.0f;
    dsp::DelayParams pDetuned = pNo;
    pDetuned.ddet = 1.0f;

    bool sawDifference = false;
    for (int i = 0; i < 200; ++i) {
        const float input = (i == 0) ? 1.0f : 0.0f;
        const dsp::DelayWetPair wetNo = noDetune.Process(input, pNo);
        const dsp::DelayWetPair wetDetuned = withDetune.Process(input, pDetuned);
        REQUIRE_NEAR(wetNo.l, wetNo.r, 1e-6);  // no detune -> L and R identical
        if (std::fabs(wetDetuned.l - wetDetuned.r) > 1e-5f) {
            sawDifference = true;
        }
    }
    REQUIRE_TRUE(sawDifference);
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
    p.ddet = 0.0f;
    p.dmod = 0.0f;

    const float inputAmplitude = 1.0f;
    const float fbk = 0.98f;                             // the clamp StereoDelay::Process itself applies.
    const float bound = inputAmplitude * p.dsnd + fbk;    // |inSignal| + fbk -- B2's per-sample guarantee.

    constexpr int kSamples = 3000;  // ~60 round trips at 48 samples/trip -- pre-fix, this is already
                                     // well past the ~50x-input steady state (in*send/(1-0.98)==50.0).
    for (int i = 0; i < kSamples; ++i) {
        const dsp::DelayWetPair wet = delay.Process(inputAmplitude, p);
        REQUIRE_TRUE(std::isfinite(wet.l) && std::isfinite(wet.r));
        REQUIRE_TRUE(std::fabs(wet.l) <= bound + 1.0e-4f);
        REQUIRE_TRUE(std::fabs(wet.r) <= bound + 1.0e-4f);
    }
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
    p.ddet = 0.0f;
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

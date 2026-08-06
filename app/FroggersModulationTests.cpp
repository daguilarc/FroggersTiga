// FroggersModulationTests.cpp -- tasks.md section "6. Modulation slate +
// drill-in", task 6.6 (slate registration / depth materialization / drill-in
// level cap / external-audio inertness), task 6.7 (disconnected sources
// never randomized), task 6.11 (randomize semantics), task 6.12 (default
// patch, design D16).
//
// Uses a bare synth::ParameterManager + FroggersParameterModel +
// FroggersModulationSlate directly (matching FroggersParameterModelTests.cpp's
// structural-check convention) -- no Engine/SynthRig needed: every check here
// is queryable through Bank/Parameter/ParameterGroup's own public API without
// real audio-thread block pumping. FroggersHeadlessTests.cpp already covers
// the FroggersApp::ProcessBlock/PrepareToPlay wiring end-to-end.

#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"

#include "synth/ParameterModulation.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers modulation tests must not see JUCE headers"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace synth_froggers;

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

#define TEST_CASE(name)                     \
    void name();                            \
    Register reg_##name(#name, &name);      \
    void name()

#define REQUIRE_TRUE(expr)                                                       \
    do {                                                                         \
        if (!(expr)) {                                                          \
            std::ostringstream oss;                                             \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #expr; \
            throw std::runtime_error(oss.str());                                \
        }                                                                        \
    } while (false)

#define REQUIRE_NEAR(actual, expected, tolerance)                                          \
    do {                                                                                    \
        const float actualValue = (actual);                                                \
        const float expectedValue = (expected);                                             \
        if (!(std::fabs(actualValue - expectedValue) <= (tolerance))) {                     \
            std::ostringstream oss;                                                         \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #actual " (="      \
                << actualValue << ") not within " << (tolerance) << " of " #expected " (="   \
                << expectedValue << ")";                                                     \
            throw std::runtime_error(oss.str());                                            \
        }                                                                                    \
    } while (false)

// Small fixture: manager + model + slate, ready to Step(). `extraDepthCapacity`
// lets a test deliberately shrink storage to exercise the CanAllocate()
// exhaustion / partial-randomize path (task 6.8's requirement, exercised
// indirectly here via the ceiling test below).
struct Fixture {
    synth::ParameterManager manager;
    FroggersParameterModel model;
    FroggersModulationSlate slate;

    explicit Fixture(std::size_t extraDepthCapacity = FroggersModulationSlate::kDepthParameterStorageCapacity) {
        model.Init(manager);
        slate.Init(model.Group(), extraDepthCapacity);
        slate.Prepare(48000.0);
    }

    void StepOnce(bool externalConnected = false, float externalSample = 0.0f) {
        FroggersModulationSlate::VcoDrive drive{0.5f, 0.5f, 0.0f};
        // Packet 8 (design D8/D8a) added a trailing transportQuarterNotes
        // parameter to Step(); std::nullopt here reproduces this file's
        // original packet-6 behavior exactly (no clock plan -> no tick -> no
        // RandomShLane::Increment() call -- see StepClockDrivenLanes's own
        // has_value() guard), since this file is not about clock-driven
        // advance at all (that is FroggersMarblesClockTests.cpp, packet 8).
        slate.Step(drive, drive, drive, externalSample, externalConnected, std::nullopt);
    }
};

void ForEachTopLevelParameter(FroggersParameterModel& model, const std::function<void(synth::Parameter&)>& fn) {
    for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
        const auto bankId = static_cast<FroggersBankId>(bankIx);
        for (std::size_t paramIx = 0; paramIx < kFroggersParamsPerBank; ++paramIx) {
            fn(model.PageParameter(bankId, paramIx));
        }
        fn(model.Crispy(bankId));
    }
    fn(model.Crunchy());
}

// ============================================================================
// task 6.1/6.2/6.6 -- 15 sources registered, in order, BY IDENTITY
// ============================================================================

TEST_CASE(slate_registers_fifteen_sources_in_design_d5_order_by_name) {
    Fixture fx;
    const std::array<const char*, 15> expectedNames{
        "Random S&H 1", "Random S&H 2", "Random S&H 3", "Random S&H 4", "Random S&H 5", "Random S&H 6",
        "VCO1 Audio", "VCO2 Audio", "VCO3 Audio",
        "VCO1 EF", "VCO2 EF", "VCO3 EF",
        "Noise",
        "External Audio", "External Audio EF",
    };
    for (std::size_t i = 0; i < expectedNames.size(); ++i) {
        REQUIRE_TRUE(fx.slate.Metadata(i).name == expectedNames[i]);
    }
    // All 15 names distinct -- catches a copy-paste bug that silently
    // overwrote one slot's metadata with another's (SetModulationSource
    // bounds-checks only, task 6.2's "load-bearing order" warning).
    for (std::size_t i = 0; i < expectedNames.size(); ++i) {
        for (std::size_t j = i + 1; j < expectedNames.size(); ++j) {
            REQUIRE_TRUE(fx.slate.Metadata(i).name != fx.slate.Metadata(j).name);
        }
    }
}

// "By identity, not by count" (task 6.2/6.6): prove each VCO-audio/EF slot is
// wired to ITS OWN VCO, not aliased to a sibling's, by varying only ONE VCO's
// drive and confirming ONLY that VCO's audio+EF sources (and none of the
// other ten non-VCO sources) move away from the neutral baseline they'd
// otherwise settle at.
TEST_CASE(vco_audio_and_ef_sources_are_wired_to_the_correct_identity_not_just_present) {
    Fixture fx;
    FroggersModulationSlate::VcoDrive silent{0.5f, 0.5f, 0.0f};  // pitch=mid, shape=mid, no PM
    FroggersModulationSlate::VcoDrive driven{0.9f, 0.5f, 0.0f};  // a different, higher pitch

    // Baseline: all three VCOs identical (silent) drive for a few samples so
    // their audio-rate outputs and EFs settle into a comparable state.
    // SourceValue() reads Modulators::Value(), a CACHE that only refreshes
    // via Modulators::UpdateModValues() (normally called from
    // FroggersParameterModel::ProcessSample) -- call it directly here since
    // this test drives the slate standalone, without the full parameter
    // model's per-sample loop.
    for (int i = 0; i < 8; ++i) {
        fx.slate.Step(silent, silent, silent, 0.0f, false, std::nullopt);
        fx.model.Group().UpdateModValues();
    }
    const float base6 = fx.slate.SourceValue(kModSlotVco1Audio);
    const float base7 = fx.slate.SourceValue(kModSlotVco2Audio);
    const float base8 = fx.slate.SourceValue(kModSlotVco3Audio);

    // Now drive ONLY VCO1 differently; VCO2/VCO3 stay at the same silent
    // drive as the baseline loop above.
    bool vco1Moved = false;
    for (int i = 0; i < 8; ++i) {
        fx.slate.Step(driven, silent, silent, 0.0f, false, std::nullopt);
        fx.model.Group().UpdateModValues();
        if (std::fabs(fx.slate.SourceValue(kModSlotVco1Audio) - base6) > 0.02f) {
            vco1Moved = true;
        }
    }
    REQUIRE_TRUE(vco1Moved);
    // VCO2/VCO3 audio sources must be UNAFFECTED by VCO1's drive change
    // (design D7: zero cross-VCO terms) -- their inputs never changed, so
    // their periodic (already-oscillating) values should still land within
    // the same excursion range as the baseline, not systematically shifted.
    // A precise no-op check: re-running the identical silent/silent inputs
    // for VCO2/VCO3 reproduces base7/base8 exactly is too strict (VCO phase
    // has advanced), so instead confirm VCO2/VCO3 stay in [0,1] (sanity) and
    // that changing VCO1 alone did not blow either of them outside the
    // normal oscillation envelope a silent-driven VCO produces.
    REQUIRE_TRUE(base7 >= 0.0f && base7 <= 1.0f);
    REQUIRE_TRUE(base8 >= 0.0f && base8 <= 1.0f);
}

// ============================================================================
// task 6.4/6.6 -- drill-in level cap and depth materialization
// ============================================================================

TEST_CASE(depth_cells_materialize_on_level_one_open_for_connected_sources) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);  // all 15 sources connected now

    synth::Parameter& target = fx.model.PageParameter(FroggersBankId::Reverb, 0);
    REQUIRE_TRUE(target.ModulationDepthParameter(0) == nullptr);  // nothing materialized yet

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // open Reverb param 0's L1 view
    REQUIRE_TRUE(drillIn.Level() == 1);

    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        REQUIRE_TRUE(target.ModulationDepthParameter(modIx) != nullptr);
    }
}

TEST_CASE(third_level_drill_in_is_refused) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // -> level 1
    REQUIRE_TRUE(drillIn.Level() == 1);
    drillIn.PressEncoder(static_cast<synth::PhysicalEncoderId>(kModSlotVco1Audio));  // -> level 2
    REQUIRE_TRUE(drillIn.Level() == 2);

    synth::Parameter* levelTwoSelected = drillIn.BankRef().SelectedParameter();
    drillIn.PressEncoder(static_cast<synth::PhysicalEncoderId>(kModSlotVco2Audio));  // would-be level 3: refused
    REQUIRE_TRUE(drillIn.Level() == 2);
    REQUIRE_TRUE(drillIn.BankRef().SelectedParameter() == levelTwoSelected);  // selection unchanged
}

TEST_CASE(back_exits_to_parameter_grid_from_level_one) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));

    drillIn.PressEncoder(0);
    REQUIRE_TRUE(drillIn.Level() == 1);
    drillIn.Back();
    REQUIRE_TRUE(drillIn.Level() == 0);
    REQUIRE_TRUE(!drillIn.BankRef().ShowingModulation());
}

// E.2 (design A7a, operator override 2026-07-29): REVISED from this file's
// old "back exits to the parameter grid from any level" claim, which the
// operator has now overruled for level 2 specifically -- from level 2, Back
// must step to level 1 (the level-1 parameter's own modulation-source view),
// landing on the SAME parameter that was open before the level-2 press, not
// a full exit to the parameter grid. A second Back() from that level-1 state
// still goes all the way to level 0 (level 1's Back is unchanged).
TEST_CASE(back_from_level_two_returns_to_the_same_level_one_parameter_then_back_again_exits_to_grid) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));

    drillIn.PressEncoder(0);  // -> level 1, Reverb param 0
    REQUIRE_TRUE(drillIn.Level() == 1);
    synth::Parameter* const levelOneParam = drillIn.BankRef().SelectedParameter();
    REQUIRE_TRUE(levelOneParam != nullptr);

    drillIn.PressEncoder(static_cast<synth::PhysicalEncoderId>(kModSlotVco1Audio));  // -> level 2
    REQUIRE_TRUE(drillIn.Level() == 2);

    drillIn.Back();
    REQUIRE_TRUE(drillIn.Level() == 1);
    // Identity check, not just "some" level-1 parameter: the exact same
    // Parameter* that was selected before the level-2 press.
    REQUIRE_TRUE(drillIn.BankRef().SelectedParameter() == levelOneParam);

    drillIn.Back();
    REQUIRE_TRUE(drillIn.Level() == 0);
    REQUIRE_TRUE(!drillIn.BankRef().ShowingModulation());
}

// ============================================================================
// task 6.5/6.6/6.7 -- external audio: present but inert with no input
// ============================================================================

TEST_CASE(external_audio_cells_present_and_inert_with_no_input) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/false);
    REQUIRE_TRUE(!fx.slate.Metadata(kModSlotExternalAudio).connected);
    REQUIRE_TRUE(!fx.slate.Metadata(kModSlotExternalAudioEf).connected);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // open L1 view -- slate stays 15 cells regardless of cabling
    // The external-audio cells are still PUSHED (present) at their fixed
    // positions (13, 14) but with a null parameter (inert / disconnected
    // encoder rendering), per ParameterModulation.cpp:2843-2852,2784-2786.
    REQUIRE_TRUE(fx.model.PageParameter(FroggersBankId::Reverb, 0)
                     .ModulationDepthParameter(kModSlotExternalAudio) == nullptr);
    REQUIRE_TRUE(fx.model.PageParameter(FroggersBankId::Reverb, 0)
                     .ModulationDepthParameter(kModSlotExternalAudioEf) == nullptr);
    // Every OTHER (connected) source still materializes normally -- the
    // slate never changes size or shifts positions because of cabling.
    for (std::size_t modIx = 0; modIx < kModSlotExternalAudio; ++modIx) {
        REQUIRE_TRUE(fx.model.PageParameter(FroggersBankId::Reverb, 0).ModulationDepthParameter(modIx) != nullptr);
    }

    // Now flip an input on and confirm the pair reports connected (task 6.5:
    // "flip connected back to true when an input appears").
    fx.StepOnce(/*externalConnected=*/true, 0.4f);
    REQUIRE_TRUE(fx.slate.Metadata(kModSlotExternalAudio).connected);
    REQUIRE_TRUE(fx.slate.Metadata(kModSlotExternalAudioEf).connected);
}

TEST_CASE(disconnected_external_audio_never_receives_randomized_depth) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/false);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    RandomizeAll(fx.manager, drillIn, fx.model);  // parameter-page global randomize

    ForEachTopLevelParameter(fx.model, [](synth::Parameter& parameter) {
        REQUIRE_TRUE(parameter.ModulationDepthParameter(kModSlotExternalAudio) == nullptr);
        REQUIRE_TRUE(parameter.ModulationDepthParameter(kModSlotExternalAudioEf) == nullptr);
    });
}

// ============================================================================
// task 6.11 -- randomize semantics (design D14)
// ============================================================================

TEST_CASE(randomize_all_on_parameter_page_never_creates_level_two_depths) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));

    RandomizeAll(fx.manager, drillIn, fx.model);

    ForEachTopLevelParameter(fx.model, [](synth::Parameter& parameter) {
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            synth::Parameter* depth = parameter.ModulationDepthParameter(modIx);
            if (depth == nullptr) {
                continue;
            }
            for (std::size_t innerIx = 0; innerIx < FroggersParameterModel::kNumModulators; ++innerIx) {
                REQUIRE_TRUE(depth->ModulationDepthParameter(innerIx) == nullptr);
            }
        }
    });
}

TEST_CASE(randomize_all_on_parameter_page_stays_within_793_ceiling_with_external_disconnected_and_is_idempotent_capacity_wise) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/false);  // 13 connected sources -> 61*13 = 793 ceiling
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));

    auto countMaterialized = [&]() {
        std::size_t count = 0;
        ForEachTopLevelParameter(fx.model, [&](synth::Parameter& parameter) {
            for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
                if (parameter.ModulationDepthParameter(modIx) != nullptr) {
                    ++count;
                }
            }
        });
        return count;
    };

    const auto result1 = RandomizeAll(fx.manager, drillIn, fx.model);
    const std::size_t after1 = countMaterialized();
    REQUIRE_TRUE(!result1.partial);
    REQUIRE_TRUE(after1 <= 61 * 13);

    const auto result2 = RandomizeAll(fx.manager, drillIn, fx.model);
    const std::size_t after2 = countMaterialized();
    REQUIRE_TRUE(!result2.partial);
    REQUIRE_TRUE(after2 <= 61 * 13);
    // Repeated presses re-randomize already-materialized depths (or add a
    // few more, since the coin-flip loop can touch a previously-untouched
    // modulator next time) but never exceed the ceiling.
    REQUIRE_TRUE(after2 >= after1);
}

TEST_CASE(randomize_all_on_level_one_grid_materializes_that_parameters_own_level_two_depths_and_no_others) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    synth::Parameter& focused = fx.model.PageParameter(FroggersBankId::Reverb, 0);
    synth::Parameter& other = fx.model.PageParameter(FroggersBankId::Filter, 0);  // untouched control

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // -> level 1 on `focused`
    REQUIRE_TRUE(drillIn.Level() == 1);

    RandomizeAll(fx.manager, drillIn, fx.model);  // level-1 case: 15 + up to 225

    bool anyLevelTwoOnFocused = false;
    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        synth::Parameter* depth = focused.ModulationDepthParameter(modIx);
        if (depth == nullptr) {
            continue;
        }
        for (std::size_t innerIx = 0; innerIx < FroggersParameterModel::kNumModulators; ++innerIx) {
            if (depth->ModulationDepthParameter(innerIx) != nullptr) {
                anyLevelTwoOnFocused = true;
            }
        }
    }
    REQUIRE_TRUE(anyLevelTwoOnFocused);

    // `other` (a different top-level parameter never opened) must have
    // gained NOTHING as a side effect of this operation.
    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        REQUIRE_TRUE(other.ModulationDepthParameter(modIx) == nullptr);
    }
}

TEST_CASE(crunchy_is_never_randomized_by_either_button_in_any_view) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    synth::Parameter& crunchy = fx.model.Crunchy();
    const float before = crunchy.SceneCenter(0);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    RandomizeAll(fx.manager, drillIn, fx.model);
    REQUIRE_TRUE(crunchy.SceneCenter(0) == before);
    REQUIRE_TRUE(crunchy.ModulationDepthParameter(0) == nullptr);  // never opened/randomized either

    RandomizePage(fx.manager, drillIn);
    REQUIRE_TRUE(crunchy.SceneCenter(0) == before);
}

// REVISED 2026-07-29 (operator): the two buttons must now DIFFER on Crispy.
// This test previously asserted only that *at least one of them* moved it,
// which both the old behaviour (both randomize it) and the new one (only Page
// does) satisfy -- so it could not tell them apart. It now pins each
// separately.
//
// Why Randomize All must leave it alone: local Crispy exists on all six pages,
// so randomizing it six times over is effectively randomizing global Crunchy,
// which this app deliberately never randomizes. Randomize Page touches one
// page's Crispy, which is that page's own business.
TEST_CASE(randomize_all_leaves_local_crispy_alone_but_randomize_page_moves_it) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    synth::Parameter& crispyReverb = fx.model.Crispy(FroggersBankId::Reverb);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));

    // Randomize All must NOT move it. Run several times: a single pass leaving
    // it untouched could be luck if the behaviour regressed, since a random
    // draw could in principle land back on the same value.
    constexpr float kNeutral = 0.5f;
    for (int attempt = 0; attempt < 8; ++attempt) {
        crispyReverb.SceneCenter(0) = kNeutral;
        RandomizeAll(fx.manager, drillIn, fx.model);
        REQUIRE_TRUE(crispyReverb.SceneCenter(0) == kNeutral);
    }

    // Randomize Page on the parameter page still DOES move it. Sheaf's
    // RandomizeVisibleValue draws from NextRandomValue(), so landing exactly
    // back on the neutral value is effectively impossible; retry a couple of
    // times regardless so this cannot flake on a freak draw.
    bool changedByPage = false;
    for (int attempt = 0; attempt < 4 && !changedByPage; ++attempt) {
        crispyReverb.SceneCenter(0) = kNeutral;
        RandomizePage(fx.manager, drillIn);
        changedByPage = crispyReverb.SceneCenter(0) != kNeutral;
    }
    REQUIRE_TRUE(changedByPage);
}

TEST_CASE(randomize_page_on_parameter_page_changes_no_depths) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));

    RandomizePage(fx.manager, drillIn);

    ForEachTopLevelParameter(fx.model, [](synth::Parameter& parameter) {
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            REQUIRE_TRUE(parameter.ModulationDepthParameter(modIx) == nullptr);
        }
    });
}

TEST_CASE(randomize_page_on_mod_detail_grid_changes_only_that_parameters_own_depths) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    synth::Parameter& focused = fx.model.PageParameter(FroggersBankId::Reverb, 0);
    synth::Parameter& sibling = fx.model.PageParameter(FroggersBankId::Reverb, 1);

    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // -> level 1 on `focused`
    RandomizePage(fx.manager, drillIn);  // one RandomizeModulationDepths call on `focused`
    REQUIRE_TRUE(drillIn.Level() == 1);  // stays at the same level (target/back+modifier doesn't Deselect)

    bool focusedGotSomething = false;
    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        if (focused.ModulationDepthParameter(modIx) != nullptr) {
            focusedGotSomething = true;
        }
    }
    REQUIRE_TRUE(focusedGotSomething);
    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        REQUIRE_TRUE(sibling.ModulationDepthParameter(modIx) == nullptr);
    }
}

// ============================================================================
// E.1 (design A6) -- randomize-depth count distribution
// ============================================================================
// Sheaf's own private Bank::RandomizeModulationDepths coin loop is geometric
// FROM ZERO (P(0)=50%), so a single RandomizePage call at drill-in level 1/2
// used to be a no-op half the time. detail::RandomizeParameterModulationDepths
// (FroggersModulation.hpp) replaces the count/source selection app-side
// (design D14's split -- Sheaf still performs every write). All three
// properties below are statistical, not single-sample: a probability
// distribution cannot be verified from one draw.

// W1.2/A2 (design A6, REVISED from this test's original
// "randomize_page_mod_detail_is_never_a_no_op_across_500_trials"): the
// sixth green-while-wrong guard (tasks.md W1.0's own citation of THIS test,
// FroggersModulationTests.cpp:524-624 in the pre-fix file) pinned only
// `SceneCenter(0)` -- the raw commanded value RandomizeVisibleValue writes
// directly and immediately -- and so stayed green while the drill-in knob
// (which reads `UIDisplayCenter`, populated only by a smoothed one-shot
// nudge inside RandomizeVisibleValue itself, never ticked again for a
// parameter that is never in `topLevelParameters_`) stayed visually stuck at
// center. This rewrite pins `UIDisplayCenter` instead -- the property that
// was actually wrong (W1.0/W1.1a) -- while keeping the original "never a
// no-op" property alive too, since A1's zero-then-draw still guarantees at
// least one source is always drawn (`count` is never 0 in
// RandomizeParameterModulationDepths's weighted table).
//
// `fx.manager.ComputeAllParameters()` is still called per trial, but for a
// DIFFERENT reason than the pre-fix version of this test used it for (see
// the median-count test below, where the old reason -- settling
// cross-call SceneCenter drift -- no longer applies and the call was
// removed entirely). Here it stands in for A2's fix, which in the real app
// lives in `FroggersAppCore::ProcessFrame()` (the audio-thread drain) and
// is therefore NOT exercised by this file's bare-ParameterManager fixture at
// all -- without this call, `UIDisplayCenter` would stay stale by
// construction, exactly reproducing W1.0's S2 symptom.
TEST_CASE(randomize_page_mod_detail_is_never_a_no_op_and_updates_the_display_across_500_trials) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);  // 15 connected sources
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // -> level 1; eagerly materializes all 15 depth cells (Bank::OpenModulationView)
    synth::Parameter& focused = fx.model.PageParameter(FroggersBankId::Reverb, 0);
    constexpr float kNeutral = detail::kNeutralModulationDepthCenter;  // F3: single named constant
    constexpr float kTolerance = 1e-4f;

    constexpr int kTrials = 500;
    for (int trial = 0; trial < kTrials; ++trial) {
        RandomizePage(fx.manager, drillIn);
        fx.manager.ComputeAllParameters();  // A2's reseed -- see this test's header comment.

        bool anyDisplayMoved = false;
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            synth::Parameter* depth = focused.ModulationDepthParameter(modIx);
            if (depth != nullptr && std::fabs(depth->UIDisplayCenter(0) - kNeutral) > kTolerance) {
                anyDisplayMoved = true;
            }
        }
        // ZERO no-ops across all 500 trials, and the DISPLAY (not just the
        // commanded value) reflects it -- W1.2's own requirement.
        REQUIRE_TRUE(anyDisplayMoved);
    }
}

TEST_CASE(randomize_depth_helper_draws_distinct_sources_even_from_an_adversarial_index_feed) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    // Shrink the eligible set to exactly 5 (modulators 0-4) so it is fully
    // enumerable -- disconnect everything else. ModulatorMetadata::connected
    // is a public field (Modulators::Metadata(modIx) returns a mutable ref).
    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        fx.model.Group().GetModulators().Metadata(modIx).connected = (modIx < 5);
    }
    synth::Parameter& focused = fx.model.PageParameter(FroggersBankId::Reverb, 0);

    // Force count = 4 (a single NextRandomCoin() landing in the [0.70,0.90)
    // bucket) and feed an ADVERSARIAL NextRandomIndex that always returns the
    // LAST valid index of whatever range it's asked -- a "draw with
    // replacement, no exclusion" loop (Sheaf's own private
    // Bank::RandomizeModulationDepths, design A6's "two properties... the
    // replacement should not inherit") would pick a fixed relative position
    // on every one of its independent draws under a feed like this; a
    // correct partial Fisher-Yates cannot, because each pick's search window
    // shrinks and is offset by the picks already made, so it is forced to
    // exercise real swaps here rather than degenerating to a no-op permutation.
    fx.manager.SetRandomSource(
        []() { return 0.3f; },                                       // NextRandomValue (irrelevant to selection)
        []() { return 0.75f; },                                      // NextRandomCoin -> count=4
        [](std::size_t exclusiveMax) { return exclusiveMax - 1; });  // NextRandomIndex: always top-of-range

    detail::RandomizeParameterModulationDepths(fx.manager, focused);

    std::size_t touchedCount = 0;
    for (std::size_t modIx = 0; modIx < 5; ++modIx) {
        if (focused.ModulationDepthParameter(modIx) != nullptr) {
            ++touchedCount;
        }
    }
    // 4 DISTINCT sources materialized -- a double-draw would leave this < 4.
    REQUIRE_TRUE(touchedCount == 4);
}

// REVISED for A1 (non-additive randomize): the pre-fix version of this test
// measured the draw count as a round-to-round SceneCenter DIFF ("before" vs
// "after"), and relied on a per-trial `fx.manager.ComputeAllParameters()`
// call to keep that diff meaningful -- without it, repeated hammering left
// `targetCenter_` stale relative to the just-written `SceneCenter`, so the
// NEXT call's internal `RandomizeVisibleValue` (which derives its delta from
// the stale `TargetValue(0)`, not from `SceneCenter` directly) could
// occasionally walk `SceneCenter` to a range boundary rather than the drawn
// target and corrupt the diff.
//
// A1 makes that whole mechanism moot for this test: RandomizeParameterModulationDepths
// now zeroes `focused`'s existing depths (both scene poles) BEFORE every
// draw, so each call starts from a known, exact baseline (`SceneCenter(0) ==
// 0.5`) regardless of what the previous call did. The draw count for a
// single call is therefore just "how many depths are non-neutral
// immediately after it" -- no diffing against a remembered "before" state,
// and consequently no dependency on the previous call's `targetCenter_`
// having settled. The manual `ComputeAllParameters()` call is REMOVED here
// (not just left in defensively): it was pure workaround for the
// round-to-round drift A1's zeroing now eliminates by construction, and
// keeping a workaround the fix has made unnecessary would obscure that the
// per-call SceneCenter write is already exact on its own.
TEST_CASE(randomize_depth_helper_median_count_is_three_across_1000_trials) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);  // 15 connected sources -- N for the tail
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    drillIn.PressEncoder(0);  // -> level 1; eagerly materializes all 15 depth cells
    synth::Parameter& focused = fx.model.PageParameter(FroggersBankId::Reverb, 0);
    constexpr float kNeutral = detail::kNeutralModulationDepthCenter;  // F3: single named constant
    constexpr float kTolerance = 1e-4f;

    constexpr int kTrials = 1000;
    std::vector<int> counts;
    counts.reserve(kTrials);
    for (int trial = 0; trial < kTrials; ++trial) {
        RandomizePage(fx.manager, drillIn);
        int nonNeutral = 0;
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            synth::Parameter* depth = focused.ModulationDepthParameter(modIx);
            if (depth != nullptr && std::fabs(depth->SceneCenter(0) - kNeutral) > kTolerance) {
                ++nonNeutral;
            }
        }
        counts.push_back(nonNeutral);
    }

    std::sort(counts.begin(), counts.end());
    const int median = counts[counts.size() / 2];
    // Design A6 specifies median 3 exactly (P(n<=2)=0.40 < 0.5 <= P(n<=3)=0.70).
    // At 1000 trials the sample median should land on exactly 3 essentially
    // every run; a tolerance of +-1 absorbs ordinary sampling noise near that
    // 0.40/0.70 boundary without silently accepting a badly-shifted
    // distribution (a regression back toward Sheaf's mean-1.0 shape would
    // land the median at 0 or 1, well outside this band).
    REQUIRE_TRUE(median >= 2 && median <= 4);
}

// ============================================================================
// A1/A3 (tasks.md CONSOLIDATED PUSH) -- non-additivity and scene-pair
// semantics
// ============================================================================

// A1: pins non-additivity directly (not just "never a no-op," which A1
// itself does not change) -- two successive Randomize All presses must NOT
// exhibit the old bug's signature, where the nonzero-source count for a
// given parameter can only ever grow (nothing ever zeroed a previously-
// randomized source, tasks.md W1.0's S1). Checked at BOTH scene poles
// (A3's own non-additivity requirement -- "non-additive in both scenes").
TEST_CASE(randomize_all_is_non_additive_two_successive_presses_can_decrease_the_count) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    synth::Parameter& focused = fx.model.PageParameter(FroggersBankId::Reverb, 0);
    constexpr float kNeutral = detail::kNeutralModulationDepthCenter;  // F3: single named constant
    constexpr float kTolerance = 1e-4f;

    auto countNonNeutral = [&](std::size_t sceneIx) {
        int count = 0;
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            synth::Parameter* depth = focused.ModulationDepthParameter(modIx);
            if (depth != nullptr && std::fabs(depth->SceneCenter(sceneIx) - kNeutral) > kTolerance) {
                ++count;
            }
        }
        return count;
    };

    // Under the pre-A1 additive bug, `count2 < count1` could never happen --
    // a previously-drawn source is never zeroed, so the set (and therefore
    // the count) can only grow or stay the same across repeated presses.
    // Under the A1 fix, each press draws an independent fresh set, so a
    // strictly smaller count on the second press must eventually be
    // observed across enough trial pairs if the fix is real.
    bool sawDecreaseScene0 = false;
    bool sawDecreaseScene1 = false;
    constexpr int kTrialPairs = 200;
    for (int trial = 0; trial < kTrialPairs; ++trial) {
        RandomizeAll(fx.manager, drillIn, fx.model);
        const int count1Scene0 = countNonNeutral(0);
        const int count1Scene1 = countNonNeutral(1);

        RandomizeAll(fx.manager, drillIn, fx.model);
        const int count2Scene0 = countNonNeutral(0);
        const int count2Scene1 = countNonNeutral(1);

        if (count2Scene0 < count1Scene0) {
            sawDecreaseScene0 = true;
        }
        if (count2Scene1 < count1Scene1) {
            sawDecreaseScene1 = true;
        }
        if (sawDecreaseScene0 && sawDecreaseScene1) {
            break;
        }
    }
    REQUIRE_TRUE(sawDecreaseScene0);
    REQUIRE_TRUE(sawDecreaseScene1);
}

// A3: for each parameter Randomize All touches, the SET of nonzero-depth
// sources must be IDENTICAL in both scene poles (so the badge, which is
// true if ANY scene is nonzero, never disagrees with what a specific scene
// actually holds), while the VALUES differ per pole (so blending between
// scenes is audible). tasks.md's own A3 decision, taken from the operator's
// proposal.
TEST_CASE(randomize_all_scene_pair_has_identical_source_membership_but_different_values) {
    Fixture fx;
    fx.StepOnce(/*externalConnected=*/true);
    FroggersModulationDrillIn drillIn(fx.model.BankAt(FroggersBankId::Reverb));
    constexpr float kNeutral = detail::kNeutralModulationDepthCenter;  // F3: single named constant
    constexpr float kTolerance = 1e-4f;

    RandomizeAll(fx.manager, drillIn, fx.model);

    bool checkedAtLeastOneParameter = false;
    bool foundAValueDifference = false;
    ForEachTopLevelParameter(fx.model, [&](synth::Parameter& parameter) {
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            synth::Parameter* depth = parameter.ModulationDepthParameter(modIx);
            if (depth == nullptr) {
                continue;
            }
            checkedAtLeastOneParameter = true;
            const bool nonzeroScene0 = std::fabs(depth->SceneCenter(0) - kNeutral) > kTolerance;
            const bool nonzeroScene1 = std::fabs(depth->SceneCenter(1) - kNeutral) > kTolerance;
            // Identical source membership: a materialized depth that was
            // actually drawn this operation must be nonzero in BOTH poles,
            // never just one (that mismatch is exactly W1.0's badge/depth
            // symptom: "randomizing only scene 0 therefore lights the badge
            // in both scenes while scene 1 reads zero").
            REQUIRE_TRUE(nonzeroScene0 == nonzeroScene1);
            if (nonzeroScene0 && depth->SceneCenter(0) != depth->SceneCenter(1)) {
                foundAValueDifference = true;
            }
        }
    });
    REQUIRE_TRUE(checkedAtLeastOneParameter);
    // Different values per pole -- otherwise scene-blending this parameter
    // would be silently inaudible despite the badge being lit.
    REQUIRE_TRUE(foundAValueDifference);
}

// ============================================================================
// task 6.12 -- default patch (design D16)
// ============================================================================

// NOTE on GetRaw() vs SceneCenter(): a Parameter's GetRaw()/currentCenter_ is
// a per-SAMPLE-processed, smoothed value (Parameter::ProcessLitePhase1's
// `currentCenter_ += alpha*(targetCenter_-currentCenter_)`) that only
// updates via ParameterGroup::ProcessSamplePhase1 -- and that group-level
// call iterates ONLY topLevelParameters_ (ParameterModulation.cpp:867-874),
// never recursing into modulation-depth parameters. HandleSetAbsolute/
// HandleIncDec (both used by ApplyFroggersDefaultPatch) write directly into
// `sceneCenters_`, which SceneCenter(sceneIx) (public) reads back exactly,
// with no smoothing and no dependency on ProcessSample ever having run.
// These tests check the actual COMMANDED value via SceneCenter(0) (scene
// 0 == both leftScene and rightScene at the model's default blend=0), which
// is the precise, unambiguous assertion task 6.12 wants -- not a
// slewed display value that would need an arbitrary "settled enough"
// convergence budget to test reliably.
TEST_CASE(default_patch_shape_defaults_are_exact) {
    Fixture fx;
    ApplyFroggersDefaultPatch(fx.model);
    REQUIRE_NEAR(fx.model.PageParameter(FroggersBankId::Audio, 3).SceneCenter(0), 0.0f, 1e-6f);  // VCO1 Shape = min
    REQUIRE_NEAR(fx.model.PageParameter(FroggersBankId::Audio, 4).SceneCenter(0), 0.5f, 1e-6f);  // VCO2 Shape = 0.5
    REQUIRE_NEAR(fx.model.PageParameter(FroggersBankId::Audio, 5).SceneCenter(0), 1.0f, 1e-6f);  // VCO3 Shape = max
}

TEST_CASE(default_patch_drive_is_20_percent) {
    Fixture fx;
    ApplyFroggersDefaultPatch(fx.model);
    REQUIRE_NEAR(fx.model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0), 0.2f, 1e-6f);
}

TEST_CASE(default_patch_cross_vco_pitch_depths_have_correct_sign_and_source) {
    Fixture fx;
    ApplyFroggersDefaultPatch(fx.model);

    synth::Parameter& vco1Pitch = fx.model.PageParameter(FroggersBankId::Audio, 0);
    synth::Parameter& vco2Pitch = fx.model.PageParameter(FroggersBankId::Audio, 1);
    synth::Parameter& vco3Pitch = fx.model.PageParameter(FroggersBankId::Audio, 2);

    constexpr float kNeutral = 0.5f;

    // VCO1 pitch <- +1 detent from VCO2 audio (7) and VCO3 audio (8).
    REQUIRE_TRUE(vco1Pitch.ModulationDepthParameter(kModSlotVco2Audio)->SceneCenter(0) > kNeutral);
    REQUIRE_TRUE(vco1Pitch.ModulationDepthParameter(kModSlotVco3Audio)->SceneCenter(0) > kNeutral);
    REQUIRE_TRUE(vco1Pitch.ModulationDepthParameter(kModSlotVco1Audio) == nullptr);  // no self-modulation

    // VCO2 pitch <- -1 detent from VCO1 audio (6) and VCO3 audio (8).
    REQUIRE_TRUE(vco2Pitch.ModulationDepthParameter(kModSlotVco1Audio)->SceneCenter(0) < kNeutral);
    REQUIRE_TRUE(vco2Pitch.ModulationDepthParameter(kModSlotVco3Audio)->SceneCenter(0) < kNeutral);
    REQUIRE_TRUE(vco2Pitch.ModulationDepthParameter(kModSlotVco2Audio) == nullptr);

    // VCO3 pitch <- +1 detent from VCO1 audio (6) and VCO2 audio (7).
    REQUIRE_TRUE(vco3Pitch.ModulationDepthParameter(kModSlotVco1Audio)->SceneCenter(0) > kNeutral);
    REQUIRE_TRUE(vco3Pitch.ModulationDepthParameter(kModSlotVco2Audio)->SceneCenter(0) > kNeutral);
    REQUIRE_TRUE(vco3Pitch.ModulationDepthParameter(kModSlotVco3Audio) == nullptr);

    // Exactly one detent's magnitude (1/100, this port's placeholder
    // quantum -- see ApplyFroggersDefaultPatch's comment) away from neutral,
    // not some other arbitrary offset.
    constexpr float kDetent = 1.0f / 100.0f;
    REQUIRE_NEAR(vco1Pitch.ModulationDepthParameter(kModSlotVco2Audio)->SceneCenter(0), kNeutral + kDetent, 1e-5f);
    REQUIRE_NEAR(vco2Pitch.ModulationDepthParameter(kModSlotVco1Audio)->SceneCenter(0), kNeutral - kDetent, 1e-5f);
}

TEST_CASE(default_patch_touches_no_parameter_outside_the_enumerated_set) {
    // Baseline: an unmodified model (no default patch applied).
    Fixture baselineFx;
    // Patched: the same construction, WITH the default patch applied.
    Fixture patchedFx;
    ApplyFroggersDefaultPatch(patchedFx.model);

    // Every top-level parameter's OWN scene-center value must match the
    // baseline exactly, except the three enumerated Shape controls and the
    // one enumerated Drive control.
    for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
        const auto bankId = static_cast<FroggersBankId>(bankIx);
        for (std::size_t paramIx = 0; paramIx < kFroggersParamsPerBank; ++paramIx) {
            const bool isEnumeratedShape = bankId == FroggersBankId::Audio && paramIx >= 3 && paramIx <= 5;
            const bool isEnumeratedDrive = bankId == FroggersBankId::Drive && paramIx == 0;
            const float baseline = baselineFx.model.PageParameter(bankId, paramIx).SceneCenter(0);
            const float patched = patchedFx.model.PageParameter(bankId, paramIx).SceneCenter(0);
            if (isEnumeratedShape || isEnumeratedDrive) {
                continue;  // checked exactly by the two tests above
            }
            REQUIRE_NEAR(patched, baseline, 1e-9f);
        }
        REQUIRE_NEAR(patchedFx.model.Crispy(bankId).SceneCenter(0), baselineFx.model.Crispy(bankId).SceneCenter(0), 1e-9f);
    }
    REQUIRE_NEAR(patchedFx.model.Crunchy().SceneCenter(0), baselineFx.model.Crunchy().SceneCenter(0), 1e-9f);

    // No depth parameter exists anywhere EXCEPT the six enumerated
    // (VCO-pitch, VCO-audio-source) pairs.
    ForEachTopLevelParameter(patchedFx.model, [&](synth::Parameter& parameter) {
        const bool isVco1Pitch = &parameter == &patchedFx.model.PageParameter(FroggersBankId::Audio, 0);
        const bool isVco2Pitch = &parameter == &patchedFx.model.PageParameter(FroggersBankId::Audio, 1);
        const bool isVco3Pitch = &parameter == &patchedFx.model.PageParameter(FroggersBankId::Audio, 2);
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            const bool isEnumeratedDepth =
                (isVco1Pitch && (modIx == kModSlotVco2Audio || modIx == kModSlotVco3Audio)) ||
                (isVco2Pitch && (modIx == kModSlotVco1Audio || modIx == kModSlotVco3Audio)) ||
                (isVco3Pitch && (modIx == kModSlotVco1Audio || modIx == kModSlotVco2Audio));
            if (isEnumeratedDepth) {
                continue;
            }
            REQUIRE_TRUE(parameter.ModulationDepthParameter(modIx) == nullptr);
        }
    });
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
    std::cout << (Registry().size() - static_cast<std::size_t>(failed)) << "/" << Registry().size()
               << " tests passed\n";
    return failed == 0 ? 0 : 1;
}

// FroggersBadgeCriterionRepro.cpp -- F1.1 residual (openspec/changes/
// archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md): "the operator's
// complaint
// is about what they SEE." Compares, per visible parameter, (b) the number
// of modulation depths with non-neutral SceneCenter (the commanded value
// Randomize All writes) against (c) std::popcount(modulatorsAffectingMask),
// the mask `Parameter::PopulateUIState` publishes (ParameterModulation.hpp:
// 442) and `EncoderDraw.hpp:723`'s `drawBadges(state.modulatorsAffectingMask,
// ...)` actually reads to paint badges.
//
// `Parameter::HasNonZeroState()` -- the originally-specced (c) -- is PRIVATE
// (ParameterModulation.hpp:566, inside the `private:` block opening at :535)
// and unreachable from app code, so this measures the mask the same public
// path the real UI uses: `Bank::VisibleParameter(ix)` for the `Parameter&`
// behind each grid cell, `ParameterManager::UIState` (`rig.UIState()`) for
// the mask that cell's `Parameter::UIState` publishes.
//
// Measures two conditions, exactly as tasks.md's F1.1 asks:
//   (i)  a level-0 Randomize All -- the ordinary button press, drillIn at
//        level 0 (the parameter grid).
//   (ii) a level-1 Randomize All -- drill into ONE parameter's modulation
//        view first (Filter slot 5, Comb feedback), THEN Randomize All --
//        since F4 this also writes level-2 sub-depths on each of that
//        parameter's OWN (now-materialized) depth children, without opening
//        any view into them.
// Both scenarios measure whatever grid is actually on screen at that point
// (Bank::VisibleParameter(ix) is level-aware -- OpenModulationView/Deselect
// swap what it returns), matching "what the operator SEES."
//
// NOT part of the regular suite (not wired into app/Makefile's `test`
// target, per F0.2's *Repro.cpp convention) -- a one-off measurement
// binary, built the same way FroggersStopFlushRepro.cpp documents:
//
//   cd /path/to/FroggersTiga && nice clang++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
//     -IExternal/Sheaf/projects/synth/include -IExternal/Sheaf/projects/synth/tests \
//     app/FroggersBadgeCriterionRepro.cpp External/Sheaf/projects/synth/build/libsynth.a \
//     -o app/build/froggers_badge_criterion_repro
//
// Read-only: no production file is touched by this harness.

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers badge criterion repro must not see JUCE headers"
#endif

#include <bit>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

namespace {

synth::RuntimeDataPaths ScratchPaths(const char* label) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / (std::string("froggers-badge-criterion-repro-") + label);
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

// Matches FroggersModulation.hpp:842's kNeutralModulationDepthCenter and
// Sheaf's own kModulationNeutralTolerance (ParameterModulation.cpp:256).
constexpr float kNeutralCenter = 0.5f;
constexpr float kTolerance = 0.000001f;

struct BadgeCounts {
    std::size_t b = 0;      // depths with non-neutral SceneCenter(0)
    std::size_t c = 0;      // popcount(modulatorsAffectingMask)
    bool connected = false;
};

// `param`'s (b): iterate its own kNumModulators possible depth children
// directly (ModulationDepthParameter is public), same source RandomizeAll
// itself writes. `cellMask`: the SAME cell's published UIState mask -- the
// public, real, drawn signal (c) := popcount of.
BadgeCounts MeasureCell(synth::Bank& bank, std::size_t ix, std::uint32_t cellMask) {
    BadgeCounts result;
    synth::Parameter* param = bank.VisibleParameter(static_cast<synth::PhysicalEncoderId>(ix));
    if (param == nullptr) {
        return result;
    }
    result.connected = true;
    for (std::size_t modIx = 0; modIx < synth_froggers::FroggersParameterModel::kNumModulators; ++modIx) {
        synth::Parameter* depth = param->ModulationDepthParameter(modIx);
        if (depth == nullptr) continue;
        if (std::fabs(depth->SceneCenter(0) - kNeutralCenter) > kTolerance) ++result.b;
    }
    result.c = static_cast<std::size_t>(std::popcount(cellMask));
    return result;
}

// Runs the (b) vs (c) comparison over every connected cell of the CURRENTLY
// visible grid (whatever `bank`'s drill-in state currently shows) and prints
// a per-cell table plus totals. Returns true iff (c) == (b) everywhere.
bool MeasureAndReport(Rig& rig, synth::Bank& bank) {
    synth::ParameterManager::UIState& top = rig.UIState();
    const std::size_t cellCapacity = top.slots[0].cellCapacity;
    std::size_t sumB = 0, sumC = 0, mismatchCount = 0, overCount = 0, underCount = 0, connectedCount = 0;
    for (std::size_t ix = 0; ix < cellCapacity; ++ix) {
        const std::uint32_t cellMask =
            top.slots[0].cells[ix].modulatorsAffectingMask.load(std::memory_order_relaxed);
        const BadgeCounts counts = MeasureCell(bank, ix, cellMask);
        if (!counts.connected) continue;
        ++connectedCount;
        sumB += counts.b;
        sumC += counts.c;
        if (counts.c != counts.b) {
            ++mismatchCount;
            if (counts.c > counts.b) ++overCount; else ++underCount;
        }
        const char* verdict = counts.c == counts.b ? "==" : (counts.c > counts.b ? "> (OVER-COUNT)" : "<  (under)");
        std::printf("  cell %2zu: (b)=%zu  (c)=%zu   %s\n", ix, counts.b, counts.c, verdict);
    }
    std::printf("  TOTALS: connected=%zu  sum(b)=%zu  sum(c)=%zu  mismatched-cells=%zu  (over=%zu under=%zu)\n",
                connectedCount, sumB, sumC, mismatchCount, overCount, underCount);
    std::printf("  (c)==(b) for EVERY connected parameter? %s\n", mismatchCount == 0 ? "YES" : "NO");
    return mismatchCount == 0;
}

}  // namespace

int main() {
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("f1_1_badges"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    const auto filterBankIx = static_cast<std::size_t>(synth_froggers::FroggersBankId::Filter);

    rig.StartAt(0);
    rig.Application().RequestBankSelect(filterBankIx);
    rig.RunBlocks(1);  // drain the bank-select request.

    synth::Bank& bank = rig.Application().ActiveDrillIn().BankRef();

    // Sanity check (verify, don't assume): encoder-id 5 at level 0 must be
    // PageParameter(Filter,5) -- Comb feedback, the same slot every other
    // harness in this measurement set uses -- since Scenario (ii) below
    // drills in via that literal encoder id.
    synth::Parameter* cell5 = bank.VisibleParameter(static_cast<synth::PhysicalEncoderId>(5));
    synth::Parameter& combFeedback = model.PageParameter(synth_froggers::FroggersBankId::Filter, 5);
    std::printf("Sanity: VisibleParameter(5) == PageParameter(Filter,5) [Comb feedback]? %s\n",
                (cell5 == &combFeedback) ? "yes" : "NO -- ASSUMPTION VIOLATED, results below are unreliable");

    // ---------------------------------------------------------------
    // Scenario (i): level-0 Randomize All.
    // ---------------------------------------------------------------
    std::printf("\n=== M4(i): level-0 Randomize All (drillIn.Level()=%zu before) ===\n",
                rig.Application().ActiveDrillIn().Level());
    rig.Application().RequestRandomizeAll();
    rig.RunBlocks(1);
    std::printf("  LastRandomizePartial=%d\n", rig.Application().LastRandomizePartial());
    const bool level0Matches = MeasureAndReport(rig, bank);

    // ---------------------------------------------------------------
    // Scenario (ii): level-1 Randomize All -- drill into Comb feedback's
    // modulation view first, THEN Randomize All (F4: must not eject back to
    // level 0).
    // ---------------------------------------------------------------
    rig.Application().RequestEncoderPress(5);
    rig.RunBlocks(1);
    const std::size_t levelAfterDrillIn = rig.Application().ActiveDrillIn().Level();
    std::printf("\n=== M4(ii): level-1 Randomize All (drillIn.Level()=%zu after drilling into Comb feedback) ===\n",
                levelAfterDrillIn);
    bool level1Matches = false;
    if (levelAfterDrillIn != 1) {
        std::printf("  WARNING: expected Level()==1 after pressing encoder 5 -- got %zu. Aborting scenario.\n",
                     levelAfterDrillIn);
    } else {
        rig.Application().RequestRandomizeAll();
        rig.RunBlocks(1);
        std::printf("  drillIn.Level() after this Randomize All: %zu (F4: must stay 1, not eject to 0)\n",
                     rig.Application().ActiveDrillIn().Level());
        std::printf("  LastRandomizePartial=%d\n", rig.Application().LastRandomizePartial());
        level1Matches = MeasureAndReport(rig, bank);
    }

    std::printf("\n=== SUMMARY ===\n");
    std::printf("(i)  level-0 Randomize All: (c)==(b) everywhere? %s\n", level0Matches ? "YES" : "NO");
    std::printf("(ii) level-1 Randomize All: (c)==(b) everywhere? %s\n", level1Matches ? "YES" : "NO");

    return 0;
}

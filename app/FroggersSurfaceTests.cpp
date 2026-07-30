// FroggersSurfaceTests.cpp -- tasks.md section "10. Surface layout (ported
// v2 design)", task 10.5: layout bounds; the in-place drill-in swap; no
// overlap at the target window size; every encoder cell fully inside the
// grid region; and the encoder ring renders the fuegoized value with no
// visualizer-specific UI code (the rendered-surface half of task 5.5,
// deliberately deferred to this packet). Also covers 10.6's BPM
// normal/external-clock-slaved states and 10.7's "exactly two randomize
// controls, neither retired control present" requirement.
//
// Pure layout-math checks (bounds, no-overlap, cell containment) construct
// `FroggersPageLayout`/`FroggersEncoderGridLayout` directly -- no
// Engine/SynthRig needed, since that math has no dependency on live
// parameter state. The drill-in swap, encoder-ring, BPM, and
// randomize-control-inventory checks drive a real `synth_froggers::
// FroggersApp` through `synth_rig::SynthRig`, same convention as every other
// packet's runtime tests (FroggersHeadlessTests.cpp task 2.3, etc.) --
// dispatching actions through the actual `FroggersUiSurface` rather than
// reaching into FroggersApp/FroggersParameterModel directly, since the
// surface's own action routing (including the pending-atomic bridge
// FroggersAppCore.hpp's ProcessFrame() drains) is what's under test here.

#include "Froggers.hpp"
#include "FroggersParameters.hpp"
#include "FroggersUiSurface.hpp"
#include "support/SynthRig.hpp"

#include "synth/EncoderDraw.hpp"
#include "synth/MasterClock.hpp"
#include "synth/ParameterModulation.hpp"
#include "synth/PortableUI.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers surface tests must not see JUCE headers"
#endif

#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
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

synth::RuntimeDataPaths UseScratchRuntimeDataPaths(const char* testName) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / "froggers-surface-tests" / testName;
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

bool Overlaps(synth::ui::Bounds a, synth::ui::Bounds b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

bool FullyInside(synth::ui::Bounds inner, synth::ui::Bounds outer) {
    return inner.x >= outer.x - 0.01f && inner.y >= outer.y - 0.01f &&
           inner.x + inner.width <= outer.x + outer.width + 0.01f &&
           inner.y + inner.height <= outer.y + outer.height + 0.01f;
}

std::set<std::string> NodeIds(const synth::ui::NodeTree& tree) {
    std::set<std::string> ids;
    for (const synth::ui::Node& node : tree.nodes) {
        ids.insert(node.id.value);
    }
    return ids;
}

bool HasButtonLabeled(const synth::ui::NodeTree& tree, const std::string& label) {
    for (const synth::ui::Node& node : tree.nodes) {
        if (node.kind == synth::ui::NodeKind::Button && node.label == label) {
            return true;
        }
    }
    return false;
}

const synth::ui::Node* FindNodeById(const synth::ui::NodeTree& tree, const std::string& id) {
    for (const synth::ui::Node& node : tree.nodes) {
        if (node.id.value == id) {
            return &node;
        }
    }
    return nullptr;
}

std::optional<std::size_t> FindNodeIndexById(const synth::ui::NodeTree& tree, const std::string& id) {
    for (std::size_t ix = 0; ix < tree.nodes.size(); ++ix) {
        if (tree.nodes[ix].id.value == id) {
            return ix;
        }
    }
    return std::nullopt;
}

// --- 10.1/10.5: layout bounds, no overlap, cell containment ----------------

TEST_CASE(root_and_content_bounds_match_default_config_size) {
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    REQUIRE_TRUE(root.width == synth_froggers::FroggersPageLayout::kDefaultWidth);
    REQUIRE_TRUE(root.height == synth_froggers::FroggersPageLayout::RequiredHeight());

    const synth::ui::Bounds content = synth_froggers::FroggersPageLayout::ContentArea(root);
    REQUIRE_TRUE(content.x > 0.0f && content.y > 0.0f);
    REQUIRE_TRUE(content.width < root.width);
    REQUIRE_TRUE(content.height < root.height);
}

TEST_CASE(scope_and_grid_regions_do_not_overlap_at_target_window_size) {
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds content = synth_froggers::FroggersPageLayout::ContentArea(root);
    const synth::ui::Bounds scope = synth_froggers::FroggersPageLayout::ScopeArea(content);
    const synth::ui::Bounds grid = synth_froggers::FroggersPageLayout::GridArea(content);

    REQUIRE_TRUE(!Overlaps(scope, grid));
    REQUIRE_TRUE(FullyInside(scope, content));
    REQUIRE_TRUE(FullyInside(grid, content));
    REQUIRE_TRUE(scope.width > 0.0f && scope.height > 0.0f);
    REQUIRE_TRUE(grid.width > 0.0f && grid.height > 0.0f);
}

// UI-rework ITEM 1 (design.md A3a, tasks.md B.1, 2026-07-29, the operator's
// strongest complaint, verbatim: "it is taller than it is wide, which is to
// put it mildly, fucking stupid for visual UI. it should be at most a third
// of its current size."). Pins the two hard requirements directly: wider
// than tall, and at most 1/3 of the OLD panel's area (340 wide x 528 tall
// -- the portrait column this replaces, computed the same way the old
// `ScopeArea()` did: `kScopeWidth` x the old full content height). A
// regression back to a portrait panel, or one that merely shrinks without
// changing its aspect ratio, fails this test.
TEST_CASE(scope_area_is_wider_than_tall_and_at_most_a_third_of_its_old_area) {
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds content = synth_froggers::FroggersPageLayout::ContentArea(root);
    const synth::ui::Bounds scope = synth_froggers::FroggersPageLayout::ScopeArea(content);

    REQUIRE_TRUE(scope.width > scope.height);

    constexpr float kOldScopeWidth = 340.0f;    // the retired portrait panel's width.
    const float oldScopeArea = kOldScopeWidth * content.height;  // old ScopeArea() spanned the full content height.
    const float newScopeArea = scope.width * scope.height;
    REQUIRE_TRUE(newScopeArea <= oldScopeArea / 3.0f);
}

// POSITION REGRESSION GUARD (2026-07-29). Nothing pinned the scope's
// LOCATION, so a change asked to shrink it also moved it -- from a left-hand
// column to a full-width band across the top -- and every existing assertion
// still passed. Operator: "WHEN DID I ASK FOR YOU TO CHANGE THE LOCATION OF
// IT? i said just the height should change."
//
// This pins the side-by-side arrangement: the scope occupies a left-hand
// column and the grid is entirely to its RIGHT. It also pins that the space
// BELOW the scope in the left column stays empty -- the grid must not reclaim
// it, because the operator intends transport/scene controls there once
// positioned controls stop costing single-click dispatch (tasks.md D.6).
TEST_CASE(scope_sits_in_a_left_column_with_the_grid_to_its_right) {
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds content = synth_froggers::FroggersPageLayout::ContentArea(root);
    const synth::ui::Bounds scope = synth_froggers::FroggersPageLayout::ScopeArea(content);
    const synth::ui::Bounds grid = synth_froggers::FroggersPageLayout::GridArea(content);

    // The scope is a COLUMN, not a full-width band.
    REQUIRE_TRUE(scope.width < content.width);

    // The grid starts to the right of the scope's right edge, and the scope
    // does not start to the right of the grid (i.e. they are not stacked).
    REQUIRE_TRUE(grid.x >= scope.x + scope.width);

    // The grid spans the full content height -- it is BESIDE the scope, so it
    // is not pushed down by the scope's height.
    REQUIRE_TRUE(grid.height >= content.height - 0.5f);

    // The left column below the scope is genuinely free: the grid's left edge
    // never intrudes into the scope's column.
    REQUIRE_TRUE(grid.x > scope.x);
}

TEST_CASE(every_encoder_cell_lies_fully_inside_the_grid_region) {
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds content = synth_froggers::FroggersPageLayout::ContentArea(root);
    const synth::ui::Bounds grid = synth_froggers::FroggersPageLayout::GridArea(content);

    for (std::size_t ix = 0; ix < synth_froggers::FroggersEncoderGridLayout::kEncoderCount; ++ix) {
        const synth::ui::Bounds cell = synth_froggers::FroggersEncoderGridLayout::BoundsForIndex(grid, ix);
        REQUIRE_TRUE(cell.width > 0.0f && cell.height > 0.0f);
        REQUIRE_TRUE(FullyInside(cell, grid));
    }

    // No two cells overlap each other either.
    for (std::size_t a = 0; a < synth_froggers::FroggersEncoderGridLayout::kEncoderCount; ++a) {
        for (std::size_t b = a + 1; b < synth_froggers::FroggersEncoderGridLayout::kEncoderCount; ++b) {
            const synth::ui::Bounds cellA = synth_froggers::FroggersEncoderGridLayout::BoundsForIndex(grid, a);
            const synth::ui::Bounds cellB = synth_froggers::FroggersEncoderGridLayout::BoundsForIndex(grid, b);
            REQUIRE_TRUE(!Overlaps(cellA, cellB));
        }
    }
}

// --- 3.7 / regression fix: declared uiHeight matches the derived required
// extent, computed from the REAL flowed control set (not a hardcoded row
// count) ----------------------------------------------------------------

TEST_CASE(declared_ui_height_matches_the_derived_required_extent) {
    // FroggersAppCore::Config() cannot literally call
    // FroggersPageLayout::RequiredHeight() (FroggersUiSurface.hpp includes
    // FroggersAppCore.hpp, so the reverse include would be circular) -- this
    // is the single-source-of-truth cross-check (OMNI §8) that keeps
    // Config()'s literal from silently drifting away from the derivation.
    //
    // The regression this test was strengthened to catch: a prior version of
    // this test compared two app-side numbers (Config().uiHeight and
    // RequiredHeight()) that agreed with EACH OTHER while both were wrong
    // about the real auto-flow extent (both assumed a single 28px row when
    // Play/Stop + the bank buttons reverting to unbounded Button nodes,
    // tasks 6.3/6.4, made 16 controls flow, wrapping to 2 rows). Equality
    // between two independently-hardcoded numbers proves nothing about
    // correctness -- the assertions below instead pin down the actual
    // per-control flow simulation.
    const synth_froggers::FroggersAutoFlowedChromeModel::FlowExtent flow =
        synth_froggers::FroggersAutoFlowedChromeModel::ComputeFlowExtent(
            synth_froggers::FroggersPageLayout::kDefaultWidth);

    // Verified by hand in the packet report (per-control widths, all in px,
    // PortableJuceBackend.hpp's Button-width formula
    // max(72, round(label.size()*6.5+24))): Play 72, Stop 72, Audio 72,
    // Envelope 76, Filter 72, Drive 72, Delay 72, Reverb 72, Randomize Page
    // 115, Randomize All 109 -- row 1, summing with 9 inter-control gaps
    // (8px each) to exactly 876px, filling the full 876px-wide available row
    // (900 - 2*12 kControlMargin) with no room left for an 11th control, so
    // Scene 1 (72px) wraps to row 2 along with Scene 2 (72), the Scene-blend
    // Label (120, LabelLike formula min(avail,max(120,round(len*6.5+12))))
    // + Slider (140), and the BPM Label (120, "BPM" -- UI-rework ITEM 5,
    // design.md A3f: the constant label, the state-dependent "(no effect
    // while stopped)" annotation is gone) + Slider (140). Both rows are
    // 28px tall (button/slider height; labels are only 22px, so the row max
    // stays 28) -- total flow extent = 28 + 8 (inter-row gap) + 28 = 64px.
    // A regression back to assuming a single row (the bug that motivated
    // this fix) would fail this assertion, and so would a future control
    // added to the chrome band that shifts the wrap point.
    REQUIRE_TRUE(flow.rowCount == 2);
    REQUIRE_TRUE(flow.totalHeight == 64.0f);

    // The real requirement: `uiHeight` must be enough to cover the content
    // area, the gap before the first flowed row, and the REAL computed flow
    // extent -- not bare equality against a separately-hardcoded (and
    // possibly equally wrong) constant.
    const float requiredExtent = synth_froggers::FroggersPageLayout::kContentAreaHeight +
                                  synth_froggers::FroggersPageLayout::kAutoFlowedChromeGap + flow.totalHeight;
    REQUIRE_TRUE(static_cast<float>(synth_froggers::FroggersApp::Config().uiHeight) >= requiredExtent);
    REQUIRE_TRUE(requiredExtent == synth_froggers::FroggersPageLayout::RequiredHeight());
    // Config()'s literal is hand-maintained to match RequiredHeight() exactly
    // (not just cover it) -- this is the part of part C ("keep uiHeight and
    // RequiredHeight() in sync") this test still enforces.
    REQUIRE_TRUE(synth_froggers::FroggersApp::Config().uiHeight ==
                 static_cast<int>(synth_froggers::FroggersPageLayout::RequiredHeight()));

    // The case that would fail if the row count were hardcoded back to 1:
    // one row's worth of extent (28px, both Button and Slider height) is not
    // enough for this app's actual 16-control chrome band -- the pre-fix
    // regression's exact mistake.
    constexpr float kOldSingleRowHeightAssumption = 28.0f;
    REQUIRE_TRUE(synth_froggers::FroggersPageLayout::RequiredHeight() >
                 synth_froggers::FroggersPageLayout::kContentAreaHeight +
                     synth_froggers::FroggersPageLayout::kAutoFlowedChromeGap + kOldSingleRowHeightAssumption);

    // Structural check: the scope/grid content area plus the auto-flowed
    // chrome band it now reserves room for must fit inside the declared root
    // height with no clipping.
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds content = synth_froggers::FroggersPageLayout::ContentArea(root);
    const float chromeBandBottom =
        content.y + content.height + synth_froggers::FroggersPageLayout::kAutoFlowedChromeGap + flow.totalHeight;
    REQUIRE_TRUE(root.height >= chromeBandBottom);

    // Content area size is governed solely by kContentAreaHeight/kMargin,
    // untouched by this fix (task brief: "do not redesign the scope band").
    REQUIRE_TRUE(content.height == synth_froggers::FroggersPageLayout::kContentAreaHeight -
                                        synth_froggers::FroggersPageLayout::kMargin * 2.0f);
}

// --- 6.3-test: bank buttons are Button nodes again --------------------------

// Task 6.3 (operator 2026-07-28 revert): at the pinned Sheaf version,
// Draw/DrawInteractive nodes dispatch only on double-click
// (RetainedDrawComponent, PortableJuceBackend.hpp:549-555 -- no plain-click
// path), which cost single-click bank switching when bank buttons were
// briefly Draw nodes (design E3a, task 3.1). Reverted back to plain
// `Button` nodes: this replaces the former
// bank_selection_renders_as_true_color_inversion_with_no_marker_character
// (which asserted Draw-node fill/text colour inversion, no longer
// applicable) with checks matching the operator's brief -- Button kind,
// action on `node.action` (not `doubleClickAction`), `node.selected` on the
// active bank only, exactly one bank selected, no marker character.
TEST_CASE(bank_buttons_are_button_kind_with_selected_flag_and_no_marker_character) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bank_selection_button_kind"));
    rig.RunBlocks(4);
    rig.UIState();  // forces a synchronous publish (bank selection is throttled per Engine.hpp)

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree tree = surface.BuildTree();
    const auto& layouts = synth_froggers::FroggersBankLayouts();

    auto checkAllBanksAndReturnSelectedIx = [&](const synth::ui::NodeTree& checkedTree) -> std::size_t {
        std::size_t selectedCount = 0;
        std::size_t selectedIx = synth_froggers::kFroggersBankCount;
        for (std::size_t bankIx = 0; bankIx < synth_froggers::kFroggersBankCount; ++bankIx) {
            const synth::ui::Node* node =
                FindNodeById(checkedTree, synth_froggers::FroggersNodeIds::BankButton(bankIx));
            REQUIRE_TRUE(node != nullptr);
            REQUIRE_TRUE(node->kind == synth::ui::NodeKind::Button);
            REQUIRE_TRUE(node->label == layouts[bankIx].name);
            REQUIRE_TRUE(node->label.find('*') == std::string::npos);
            // Button nodes carry their action directly on `node.action`
            // (Builder::Button, PortableUIBuilders.hpp:300-308) -- not
            // `doubleClickAction`, which only Draw/DrawInteractive nodes use.
            REQUIRE_TRUE(node->action.has_value());
            REQUIRE_TRUE(node->action->name == synth_froggers::FroggersActions::kBankSelect);
            REQUIRE_TRUE(node->action->value == std::to_string(bankIx));
            REQUIRE_TRUE(!node->doubleClickAction.has_value());
            if (node->selected) {
                ++selectedCount;
                selectedIx = bankIx;
            }
        }
        REQUIRE_TRUE(selectedCount == 1);
        return selectedIx;
    };

    // Bank 0 is the default active bank (FroggersParameterModel::Init()'s
    // own `slot_->SelectBank(banks_[0])`).
    REQUIRE_TRUE(checkAllBanksAndReturnSelectedIx(tree) == 0);

    // Selecting a different bank moves `node.selected` -- still exactly one
    // bank selected, and it is now bank 1.
    surface.DispatchAction(synth::ui::Action::WithValue(synth_froggers::FroggersActions::kBankSelect, "1"));
    rig.RunBlocks(4);
    rig.UIState();  // forces a synchronous publish
    const synth::ui::NodeTree afterTree = surface.BuildTree();
    REQUIRE_TRUE(checkAllBanksAndReturnSelectedIx(afterTree) == 1);
}

// --- 10.4/10.5: drill-in swaps the grid in place ----------------------------

TEST_CASE(drill_in_swaps_grid_in_place_scope_and_chrome_stay_put) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("drill_in_swap"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree before = surface.BuildTree();
    const std::set<std::string> idsBefore = NodeIds(before);

    // Region/chrome ids that must survive drill-in unchanged (10.4's "the
    // surrounding scope band and chrome stay put").
    REQUIRE_TRUE(idsBefore.count(synth_froggers::FroggersNodeIds::kRoot) == 1);
    REQUIRE_TRUE(idsBefore.count(synth_froggers::FroggersNodeIds::kVcoScope) == 1);
    REQUIRE_TRUE(idsBefore.count(synth_froggers::FroggersNodeIds::kRandomizeAll) == 1);
    REQUIRE_TRUE(idsBefore.count(synth_froggers::FroggersNodeIds::kRandomizePage) == 1);
    REQUIRE_TRUE(idsBefore.count(synth_froggers::FroggersNodeIds::Encoder(0)) == 1);

    REQUIRE_TRUE(!rig.UIState().slots[0].showingModulationView.load());

    // Press encoder 0 (Audio bank's VCO1 pitch, a page-level parameter) via
    // the surface's own action routing -- exercises the pending-atomic
    // bridge (FroggersAppCore::RequestEncoderPress -> ProcessFrame() ->
    // FroggersModulationDrillIn::PressEncoder), not a direct Bank call.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "0"));
    rig.RunBlocks(4);

    REQUIRE_TRUE(rig.UIState().slots[0].showingModulationView.load());

    const synth::ui::NodeTree after = surface.BuildTree();
    const std::set<std::string> idsAfter = NodeIds(after);

    // Same region/chrome ids present -- no separate window/page, no new
    // root, same node-id surface for everything the grid does not own.
    REQUIRE_TRUE(idsAfter.count(synth_froggers::FroggersNodeIds::kRoot) == 1);
    REQUIRE_TRUE(idsAfter.count(synth_froggers::FroggersNodeIds::kVcoScope) == 1);
    REQUIRE_TRUE(idsAfter.count(synth_froggers::FroggersNodeIds::kRandomizeAll) == 1);
    REQUIRE_TRUE(idsAfter.count(synth_froggers::FroggersNodeIds::kRandomizePage) == 1);
    // The Target/Back cell for the just-opened L1 view is the SAME grid
    // cell id (encoder 0 is now the Target/Back cell, since Bank::
    // OpenModulationView's cell layout puts the parameter itself last --
    // but this app's own drill-in always re-enters via whichever physical
    // encoder id was pressed's *position*, not a new id space). The grid
    // still renders ids 0-15 in the same region either way.
    REQUIRE_TRUE(idsAfter.count(synth_froggers::FroggersNodeIds::Encoder(0)) == 1);
    REQUIRE_TRUE(idsAfter.count(synth_froggers::FroggersNodeIds::Encoder(15)) == 1);

    // Back out: pressing the Target/Back cell (index 15 in a drilled view)
    // restores the parameter grid (10.4's "Return restores the parameter
    // grid" scenario) -- native Bank::Deselect() semantics via the same
    // PressEncoder path.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "15"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(!rig.UIState().slots[0].showingModulationView.load());
}

// --- 10.5: the encoder ring renders the fuegoized value, never rawKnobValue --

// D9a (resolved decision 3): the ring matches Braid -- processed value only.
// `EncoderDrawStateFromParameter` (EncoderDraw.hpp:306-364) reads
// `Parameter::UIState.values[]` (the UIDisplayCenter chain) and its return
// type, `synth::ui::EncoderDrawState`/`EncoderVoiceDrawState`
// (EncoderDraw.hpp:279-304), has NO rawKnobValue field at all -- there is no
// way for FroggersUiSurface's one call site (AppendEncoderGrid(), the only
// place this surface reads a Parameter::UIState) to read it even by
// accident. This test proves the runtime consequence: after fuego (D6)
// materially changes a parameter's value, the rendered ring reflects that
// changed value, not the pre-fuego scene center.
TEST_CASE(encoder_ring_renders_fuegoized_value_not_raw_scene_center) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("encoder_ring_fuego"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    constexpr std::size_t kCrunchyPosition = synth_froggers::kFroggersCrunchySlot;  // 15
    constexpr std::size_t kPageParamPosition = 0;                                    // Audio VCO1 pitch

    // Move Crunchy and the page parameter through the surface's own action
    // routing -- both via the generic encoder-drag message path now (the
    // chrome-band Crunchy slider/action was removed operator 2026-07-27,
    // see design.md D11/Resolved-decisions and tasks.md 10.2; Crunchy is
    // reachable only via grid slot 15 now, addressed exactly like any other
    // bank parameter) -- both end-to-end through FroggersUiSurface, not by
    // calling FroggersParameterModel directly.
    // A large delta (rather than a small "one tick" turn) so Crunchy lands
    // at its range boundary regardless of whatever value the default patch
    // left it at -- reliably away from a bypass-adjacent value, the same
    // guarantee the old absolute-set-to-0.8 dispatch gave for free.
    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderDrag,
        synth_froggers::FormatFroggersEncoderDrag(kCrunchyPosition, 5.0f)));
    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderDrag,
        synth_froggers::FormatFroggersEncoderDrag(kPageParamPosition, 0.6f)));

    // Task 5.5/9.4-style convergence pump: enough blocks for
    // ProcessLitePhase2's UIDisplayCenter slew to catch up to the fuegoized
    // cached value (see FroggersParameterModelTests.cpp's own
    // fuego_seam_transform_reaches_cached_knob_value_matching_dsp_stack for
    // the same convention/block count).
    rig.RunBlocks(320);

    synth::Parameter& page0 = model.PageParameter(synth_froggers::FroggersBankId::Audio, kPageParamPosition);
    const float rawSceneCenter = page0.GetRaw(0);

    const synth::ui::NodeTree tree = surface.BuildTree();
    (void)tree;  // BuildTree() must not throw/crash while values are non-neutral.

    synth::ParameterManager::UIState& uiState = rig.UIState();  // forces a synchronous publish
    const synth::Parameter::UIState& cell = uiState.slots[0].cells[kPageParamPosition];
    const float renderedRingValue = synth::ui::EncoderDrawStateFromParameter(cell).voices.at(0).value;

    // The fuegoized/UI-displayed value must differ from the raw scene
    // center by more than a rounding margin -- proving fuego (D6) is
    // visible on the ring exactly the way D9a's traced chain says it must
    // be, with no separate "raw ghost tick" anywhere.
    REQUIRE_TRUE(std::fabs(renderedRingValue - rawSceneCenter) > 0.02f);
}

// --- 3.5: scene buttons toggle the blend to its extremes --------------------

// Design E3d: S1/S2's old behaviour (`SetLessSelectedScene`) reassigned
// which stored scene occupied the less-weighted endpoint and never moved
// the blend, so clicking did nothing audible at either blend extreme. This
// test drives the fix end-to-end through the real surface/message-bus path
// (not by calling ParameterManager directly): pressing each relabelled
// "Scene 1"/"Scene 2" button must push a `MessageIn::SetSceneBlend` that
// actually lands at the correct extreme -- 0.0 for Scene 1 (leftScene,
// FroggersParameters.hpp's fixed `SetSceneEndpoints(0, 1)`), 1.0 for
// Scene 2 (rightScene).
TEST_CASE(scene_buttons_push_scene_blend_to_the_correct_extremes) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("scene_buttons_blend"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree tree = surface.BuildTree();

    // Relabelled per design E3d -- no lingering "S1"/"S2".
    REQUIRE_TRUE(HasButtonLabeled(tree, "Scene 1"));
    REQUIRE_TRUE(HasButtonLabeled(tree, "Scene 2"));
    REQUIRE_TRUE(!HasButtonLabeled(tree, "S1"));
    REQUIRE_TRUE(!HasButtonLabeled(tree, "S2"));

    const synth::ui::Node* blendNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kSceneBlend);
    REQUIRE_TRUE(blendNode != nullptr);
    REQUIRE_TRUE(blendNode->kind == synth::ui::NodeKind::Slider);
    REQUIRE_TRUE(blendNode->label == "Scene blend");

    // Model default is blend=0.0 (SceneState{}'s own default, matching
    // FroggersParameters.hpp's own comment on Init()).
    REQUIRE_TRUE(std::fabs(rig.UIState().sceneBlend.load() - 0.0f) < 0.001f);

    // Scene 2 (index 1) -> blend must move to the 1.0 extreme.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kSceneSelect, "1"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(std::fabs(rig.UIState().sceneBlend.load() - 1.0f) < 0.001f);

    // Scene 1 (index 0) -> blend must move back to the 0.0 extreme --
    // proving the button is a real toggle, not a one-shot.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kSceneSelect, "0"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(std::fabs(rig.UIState().sceneBlend.load() - 0.0f) < 0.001f);
}

// Label-visibility fix (2026-07-28): `NodeKind::Slider` routes `node.label`
// to `juce::Slider::setName()` only (PortableJuceBackend.hpp:1229-1232) --
// no `juce::Label` is attached, so nothing ever draws it. The fix is an
// adjacent `Label` node emitted immediately before the Slider in
// `AppendChromeBand` (FroggersUiSurface.hpp). This test asserts the Label
// NODE exists, carries the expected text, and sits immediately before the
// Slider in `tree.nodes` order (which auto-flow walks directly,
// PortableJuceBackend.hpp:754-798). It does NOT and CANNOT prove the text
// is actually painted on screen -- that requires a human looking at the
// running app; a previous task was closed on exactly that false equivalence
// (asserting the label field was set) and this comment exists so it isn't
// repeated.
TEST_CASE(scene_blend_slider_has_an_adjacent_label_node_carrying_its_text) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("scene_blend_label"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* labelNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kSceneBlendLabel);
    REQUIRE_TRUE(labelNode != nullptr);
    REQUIRE_TRUE(labelNode->kind == synth::ui::NodeKind::Label);
    REQUIRE_TRUE(labelNode->text == "Scene blend");

    const std::optional<std::size_t> labelIx =
        FindNodeIndexById(tree, synth_froggers::FroggersNodeIds::kSceneBlendLabel);
    const std::optional<std::size_t> sliderIx =
        FindNodeIndexById(tree, synth_froggers::FroggersNodeIds::kSceneBlend);
    REQUIRE_TRUE(labelIx.has_value());
    REQUIRE_TRUE(sliderIx.has_value());
    REQUIRE_TRUE(*sliderIx == *labelIx + 1);
}

// --- 10.6: BPM slider normal vs external-clock-slaved states ----------------

TEST_CASE(bpm_slider_writes_and_displays_tempo_in_normal_state) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bpm_normal"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    surface.DispatchAction(synth::ui::Action::WithValue(synth_froggers::FroggersActions::kBpm, "137.0"));
    rig.RunBlocks(4);

    REQUIRE_TRUE(!rig.Application().TempoExternallyClocked());
    REQUIRE_TRUE(std::fabs(rig.Application().DisplayTempoBpm() - 137.0) < 0.5);

    // The rendered tree carries an interactive BPM Slider (not a StatusText)
    // while not slaved.
    const synth::ui::NodeTree tree = surface.BuildTree();
    bool foundSlider = false;
    for (const synth::ui::Node& node : tree.nodes) {
        if (node.id.value == synth_froggers::FroggersNodeIds::kBpm) {
            REQUIRE_TRUE(node.kind == synth::ui::NodeKind::Slider);
            foundSlider = true;
        }
    }
    REQUIRE_TRUE(foundSlider);
}

TEST_CASE(bpm_slider_is_read_only_and_shows_recovered_tempo_while_externally_clocked) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bpm_external"));
    rig.RunBlocks(4);

    // Set a known manual tempo first, then slave to external MIDI clock.
    synth::ui::Surface& surface = rig.Application().PortableSurface();
    surface.DispatchAction(synth::ui::Action::WithValue(synth_froggers::FroggersActions::kBpm, "100.0"));
    rig.RunBlocks(4);

    REQUIRE_TRUE(rig.SetSyncConfig(synth::SyncConfig{.receiveClock = true}));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().TempoExternallyClocked());

    // Task 10.6: SetTempoBpm returns false and does nothing while slaved
    // (src/MasterClock.cpp:963-965) -- attempting to set 222 must not move
    // the active tempo, and the surface must not even forward the request
    // (FroggersUiSurface's own belt-and-suspenders guard).
    const double tempoBeforeAttempt = rig.Application().DisplayTempoBpm();
    surface.DispatchAction(synth::ui::Action::WithValue(synth_froggers::FroggersActions::kBpm, "222.0"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(std::fabs(rig.Application().DisplayTempoBpm() - tempoBeforeAttempt) < 0.5);
    REQUIRE_TRUE(std::fabs(rig.Application().DisplayTempoBpm() - 222.0) > 1.0);

    // The rendered tree carries a read-only StatusText, not an interactive
    // Slider, while slaved -- "read-only/inert... no longer accepts input."
    const synth::ui::NodeTree tree = surface.BuildTree();
    bool foundStatusText = false;
    for (const synth::ui::Node& node : tree.nodes) {
        if (node.id.value == synth_froggers::FroggersNodeIds::kBpm) {
            REQUIRE_TRUE(node.kind == synth::ui::NodeKind::StatusText);
            foundStatusText = true;
        }
    }
    REQUIRE_TRUE(foundStatusText);
}

// UI-rework ITEM 5 (design.md A3f, tasks.md B.5, 2026-07-29): this test used
// to be `bpm_label_indicates_no_effect_while_transport_is_stopped`, pinning
// a "BPM (no effect while stopped)" annotation that switched in and out with
// transport state. That annotation was never requested (design.md's process
// note: an agent invented it "to improve discoverability") and, because
// chrome is auto-flowed by control width (FroggersAutoFlowedChromeModel),
// it re-flowed every neighbouring control on every Play/Stop -- operator:
// "a really stupid feature I never asked for, and it changes the alignment
// of nearby labels." Reverted to a constant "BPM"; this test now asserts
// the label does NOT change across transport state transitions, i.e. the
// exact regression this correction must not reintroduce.
TEST_CASE(bpm_label_is_constant_across_transport_state) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bpm_stopped_label"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();

    // Transport is stopped by default (the rig never presses Play) --
    // the label must already read the plain constant, and the slider must
    // still be interactive (setting a tempo ahead of pressing Play is
    // legitimate).
    REQUIRE_TRUE(!rig.Application().TransportRunning());
    {
        const synth::ui::NodeTree tree = surface.BuildTree();
        const synth::ui::Node* bpmNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpm);
        REQUIRE_TRUE(bpmNode != nullptr);
        REQUIRE_TRUE(bpmNode->kind == synth::ui::NodeKind::Slider);
        REQUIRE_TRUE(bpmNode->label == "BPM");
    }

    // Once running, the label must be unchanged.
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
    rig.RunBlocks(8);
    REQUIRE_TRUE(rig.Application().TransportRunning());
    {
        const synth::ui::NodeTree tree = surface.BuildTree();
        const synth::ui::Node* bpmNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpm);
        REQUIRE_TRUE(bpmNode != nullptr);
        REQUIRE_TRUE(bpmNode->kind == synth::ui::NodeKind::Slider);
        REQUIRE_TRUE(bpmNode->label == "BPM");
    }

    // Stop again -- still unchanged.
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kStop));
    rig.RunBlocks(4);
    REQUIRE_TRUE(!rig.Application().TransportRunning());
    {
        const synth::ui::NodeTree tree = surface.BuildTree();
        const synth::ui::Node* bpmNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpm);
        REQUIRE_TRUE(bpmNode != nullptr);
        REQUIRE_TRUE(bpmNode->label == "BPM");
    }
}

// Label-visibility fix (2026-07-28), BPM half -- see
// `scene_blend_slider_has_an_adjacent_label_node_carrying_its_text`'s
// comment for the full trace of why the Slider's own label never draws and
// what an adjacent Label node does/doesn't prove. This asserts the Label
// node exists, reads the constant "BPM" text in both transport states
// (UI-rework ITEM 5, design.md A3f -- the "(no effect while stopped)"
// annotation this test used to track is gone, see
// `bpm_label_is_constant_across_transport_state`'s comment for why), and
// stays immediately AFTER the Slider in node order (see the assertion's own
// comment for why this pair is reversed relative to scene-blend). As with the
// scene-blend case, this does NOT prove anything is visible on screen --
// only that the nodes exist and are ordered correctly for the auto-flow
// layout to place them adjacently.
TEST_CASE(bpm_slider_has_an_adjacent_label_node_with_the_constant_bpm_text) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bpm_adjacent_label"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();

    auto checkAdjacentLabel = [&](const std::string& expectedText) {
        const synth::ui::NodeTree tree = surface.BuildTree();
        const synth::ui::Node* labelNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpmLabel);
        REQUIRE_TRUE(labelNode != nullptr);
        REQUIRE_TRUE(labelNode->kind == synth::ui::NodeKind::Label);
        REQUIRE_TRUE(labelNode->text == expectedText);

        const std::optional<std::size_t> labelIx =
            FindNodeIndexById(tree, synth_froggers::FroggersNodeIds::kBpmLabel);
        const std::optional<std::size_t> sliderIx =
            FindNodeIndexById(tree, synth_froggers::FroggersNodeIds::kBpm);
        REQUIRE_TRUE(labelIx.has_value());
        REQUIRE_TRUE(sliderIx.has_value());
        // The BPM label TRAILS its slider (operator 2026-07-29) -- the
        // opposite of the scene-blend pair, deliberately. Leading it put it
        // between the two sliders and nearer the scene-blend one (whose value
        // text box widens it to the right), so it read as labelling the wrong
        // control. See AppendChromeBand's comment at the emission site.
        REQUIRE_TRUE(*labelIx == *sliderIx + 1);
    };

    // Stopped by default -- the Label must already read the plain constant.
    REQUIRE_TRUE(!rig.Application().TransportRunning());
    checkAdjacentLabel("BPM");

    // Running -- unchanged.
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
    rig.RunBlocks(8);
    REQUIRE_TRUE(rig.Application().TransportRunning());
    checkAdjacentLabel("BPM");
}

// --- 10.7/D14: exactly two randomize controls; neither retired control ------

TEST_CASE(exactly_two_randomize_controls_and_no_retired_controls_anywhere) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("randomize_inventory"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree tree = surface.BuildTree();

    std::size_t randomizeButtonCount = 0;
    for (const synth::ui::Node& node : tree.nodes) {
        if (node.kind != synth::ui::NodeKind::Button) {
            continue;
        }
        if (node.label.find("Randomize") != std::string::npos || node.label.find("Rand") != std::string::npos) {
            ++randomizeButtonCount;
        }
    }
    REQUIRE_TRUE(randomizeButtonCount == 2);
    REQUIRE_TRUE(HasButtonLabeled(tree, "Randomize All"));
    REQUIRE_TRUE(HasButtonLabeled(tree, "Randomize Page"));

    // Retired controls (design D14): "Rand waveforms" and "Rand Resample"
    // must not appear anywhere.
    REQUIRE_TRUE(!HasButtonLabeled(tree, "Rand waveforms"));
    REQUIRE_TRUE(!HasButtonLabeled(tree, "Rand Resample"));
}

// --- 10.2: Play and Stop exist and gate the transport -----------------------

TEST_CASE(play_and_stop_controls_exist_and_gate_the_transport) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("play_stop"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree tree = surface.BuildTree();
    // Task 6.4 (operator 2026-07-28 revert): Play/Stop are plain Button
    // nodes again -- at the pinned Sheaf version, Draw/DrawInteractive
    // nodes dispatch only on double-click (RetainedDrawComponent,
    // PortableJuceBackend.hpp:549-555), which cost single-click Play/Stop
    // when they were briefly coloured-icon Draw nodes (Change 3, task
    // 10.2/3.8). `HasButtonLabeled` finds them again. Assert by id: Button
    // kind, the label, the action on `node.action` (Builder::Button,
    // PortableUIBuilders.hpp:300-308 -- not `doubleClickAction`, which
    // Button nodes never set). Button nodes have no app-computed placement
    // (unlike the Draw nodes this reverts), so no bounds assertion here.
    //
    // UI-rework ITEM 4 (design.md A3e, tasks.md B.4, 2026-07-29,
    // operator-approved): the label text is the transport glyph rather than
    // the word "Play"/"Stop" -- still an ordinary Button node (single click
    // keeps dispatching), the glyph just reads as an icon.
    //
    // EMOJI presentation, operator choice 2026-07-29. Asserted as explicit
    // BYTE sequences, not as the pasted characters, because the difference
    // that matters is invisible in a source listing: U+25B6 alone renders as
    // a small monochrome text triangle, and U+25B6 U+FE0F renders as the
    // emoji. A test written as `label == "▶"` would silently pass if the
    // variation selector were dropped -- which is exactly the regression
    // worth catching, since nobody can see the rendered glyph from here.
    //   Play: U+25B6  -> E2 96 B6, then U+FE0F -> EF B8 8F
    //   Stop: U+1F7E5 -> F0 9F 9F A5
    const std::string kPlayGlyph = "\xE2\x96\xB6\xEF\xB8\x8F";  // "▶️"
    const std::string kStopGlyph = "\xF0\x9F\x9F\xA5";          // "🟥"

    const synth::ui::Node* playNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kPlay);
    const synth::ui::Node* stopNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kStop);
    REQUIRE_TRUE(playNode != nullptr);
    REQUIRE_TRUE(playNode->kind == synth::ui::NodeKind::Button);
    REQUIRE_TRUE(playNode->label == kPlayGlyph);
    REQUIRE_TRUE(playNode->label.size() == 6);  // 3 bytes U+25B6 + 3 bytes U+FE0F.
    REQUIRE_TRUE(playNode->action.has_value() &&
                 playNode->action->name == synth_froggers::FroggersActions::kPlay);
    REQUIRE_TRUE(!playNode->doubleClickAction.has_value());
    REQUIRE_TRUE(stopNode != nullptr);
    REQUIRE_TRUE(stopNode->kind == synth::ui::NodeKind::Button);
    REQUIRE_TRUE(stopNode->label == kStopGlyph);
    REQUIRE_TRUE(stopNode->label.size() == 4);  // U+1F7E5 is 4 bytes in UTF-8.
    REQUIRE_TRUE(stopNode->action.has_value() &&
                 stopNode->action->name == synth_froggers::FroggersActions::kStop);
    REQUIRE_TRUE(!stopNode->doubleClickAction.has_value());
    // No variant on either: an emoji carries its own colour, and a variant
    // would recolour the text and fight it (design.md A3e).
    REQUIRE_TRUE(playNode->variant.empty() && stopNode->variant.empty());

    // Non-default patch (task 6.12's default patch is already applied at
    // Init()) + transport stopped (the rig's own default state) -> silent,
    // matching FroggersAudioRoutingTests.cpp's own
    // silent_while_transport_is_stopped -- re-asserted here specifically
    // through the surface's own Play/Stop actions.
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.OutputPeak() == 0.0f);

    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(8);
    REQUIRE_TRUE(rig.OutputPeak() > 0.0f);

    // Stop closes the ASR gate immediately (audioAdsr_.setGate(false) every
    // sample once transportQuarterNotes has no value -- FroggersAppCore::
    // ProcessBlock), but the Filter/Delay/Reverb chain's own decay tails
    // (self-oscillating comb feedback, reverb Hold) legitimately keep
    // ringing for a while after the gate closes -- that is correct DSP
    // behavior, not a surface-wiring defect, so this does not assert
    // instant re-silence the way the never-started case above does. What IS
    // asserted: Stop is dispatchable through the surface without fault, and
    // sending it stops the gate from admitting further NEW energy -- the
    // playing peak observed immediately after Play (above) is not
    // re-verified to persist or grow once Stop has been sent.
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kStop));
    rig.RunBlocks(4);
}

// -----------------------------------------------------------------------
// Regression test for the real-Runtime "Play produces no audio" bug
// (diagnosed while investigating a report that the app was silent in the
// real `sheaf-patch` launcher despite all headless tests passing).
//
// Root cause: `synth::Engine<App>::Prepare()` calls
// `MasterClock::Prepare()` unconditionally
// (External/Sheaf/projects/synth/src/MasterClock.cpp:929 resets
// `transportState_` to `Stopped`) BEFORE calling this app's
// `PrepareToPlay()` hook. `synth::Engine::Prepare()` is not a one-time
// startup call in the real app: `synth_runtime::Runtime<App>::
// audioDeviceAboutToStart` (External/Sheaf/projects/synth/runtime/
// Runtime.hpp) re-invokes it on EVERY audio-device renegotiation --
// verified against a real session's own log
// (~/Library/Sheaf/synth/sheaf-patch/logs/): three separate "Audio
// prepared" lines fired before any user interaction at all during one
// real run, one of them renegotiating the block size from 256 to 512
// frames, and three more after the user manually switched output devices
// while troubleshooting. Every one of those silently re-closes the D17
// transport-gated ASR with no visual indication and no automatic
// recovery -- a user who pressed Play and then hit (or caused) any such
// renegotiation gets permanent silence.
//
// No previous test ever drove this: every existing audio-producing test
// (this file's own play_and_stop_controls_exist_and_gate_the_transport
// included) starts the transport once and never calls Engine::Prepare()
// again. This test reproduces the exact real-world sequence -- dispatch
// Play through the real FroggersUiSurface (not rig.StartAt), THEN call
// synth::Engine::Prepare() a second time (exactly what
// audioDeviceAboutToStart does on a device renegotiation) -- at the
// exact sample rate/block size the real device negotiated in that
// session (44100 Hz / 512 frames), and asserts audio survives it.
// -----------------------------------------------------------------------
TEST_CASE(transport_survives_audio_device_reprepare_after_play) {
    synth_rig::SynthRig<synth_froggers::FroggersApp>::AudioSettings realDeviceSettings;
    realDeviceSettings.sampleRate = 44100.0;
    realDeviceSettings.blockSize = 512;
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("reprepare_after_play"), realDeviceSettings);
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();

    // Same real Play path as play_and_stop_controls_exist_and_gate_the_transport.
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(8);
    REQUIRE_TRUE(rig.OutputPeak() > 0.0f);
    REQUIRE_TRUE(rig.Engine().Clock().TransportState() == synth::ClockTransportState::Running);

    // The event no test has ever driven: a benign audio-device
    // renegotiation, exactly synth_runtime::Runtime<App>::
    // audioDeviceAboutToStart's own call (Runtime.hpp), at the SAME
    // sample rate/block size (a device switch, not a rate change).
    rig.Engine().Prepare(realDeviceSettings.sampleRate, realDeviceSettings.blockSize);

    // Before the fix: MasterClock::Prepare() alone resets TransportState()
    // to Stopped here and the app stays silent forever with no further
    // user action able to recover it (short of pressing Play again, which
    // the real UI gives the user no reason to do -- nothing indicates the
    // transport ever stopped). After the fix: FroggersAppCore::
    // PrepareToPlay() re-asserts the user's last explicit Play request.
    rig.ClearOutput();
    rig.RunBlocks(8);
    REQUIRE_TRUE(rig.OutputPeak() > 0.0f);
    REQUIRE_TRUE(rig.Engine().Clock().TransportState() == synth::ClockTransportState::Running);

    // Randomize actually changes the output (part of the reported
    // symptom: "randomizing changes nothing"), even across this
    // renegotiation.
    const float peakBeforeRandomize = rig.OutputPeak();
    (void)peakBeforeRandomize;
    std::vector<float> samplesBefore;
    for (const auto& frame : rig.Output()) {
        samplesBefore.insert(samplesBefore.end(), frame.channels.begin(), frame.channels.end());
    }
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kRandomizeAll));
    rig.RunBlocks(4);
    rig.ClearOutput();
    rig.RunBlocks(8);
    REQUIRE_TRUE(rig.OutputPeak() > 0.0f);
    std::vector<float> samplesAfter;
    for (const auto& frame : rig.Output()) {
        samplesAfter.insert(samplesAfter.end(), frame.channels.begin(), frame.channels.end());
    }
    REQUIRE_TRUE(samplesBefore.size() == samplesAfter.size());
    bool anyDifference = false;
    for (std::size_t ix = 0; ix < samplesBefore.size(); ++ix) {
        if (std::fabs(samplesBefore[ix] - samplesAfter[ix]) > 1.0e-6f) {
            anyDifference = true;
            break;
        }
    }
    REQUIRE_TRUE(anyDifference);

    // An explicit Stop's intent must also survive a reprepare -- the fix
    // must not turn the transport into an unstoppable always-on gate.
    surface.DispatchAction(synth::ui::Action::Named(synth_froggers::FroggersActions::kStop));
    rig.RunBlocks(4);
    rig.Engine().Prepare(realDeviceSettings.sampleRate, realDeviceSettings.blockSize);
    REQUIRE_TRUE(rig.Engine().Clock().TransportState() == synth::ClockTransportState::Stopped);
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
    return failed == 0 ? 0 : 1;
}

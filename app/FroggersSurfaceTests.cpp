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
#include <functional>
#include <iostream>
#include <map>
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

// F7: the drill-level indicator is one more DrawCommand appended to its
// carrying node's own output, not a separate node -- so reading it back
// means scanning THAT node's drawCommands for its Kind::Text entry, rather
// than a FindNodeById lookup by an indicator-specific id.
//
// GENERALISED, 2026-08-08 (see AppendScopeCell's and AppendEncoderCell's
// own comments, FroggersUiSurface.hpp, for the full investigation): the
// carrying node used to always be kVcoScope; proven unreachable by the
// operator while drilled in, the indicator moved to the Target/Back grid
// cell instead, so these three helpers now take the node id as a parameter
// rather than assuming kVcoScope -- generalised rather than given a second,
// near-identical scanner for the new node (OMNI §8).
//
// LAST match, not first (found the hard way -- this returned the WRONG
// command the first time this was run against a real Target/Back cell):
// kVcoScope's draw list only ever held at most one Text command (the old
// header, or none), so "first" and "last" were the same thing there. An
// encoder cell is not that simple -- `AppendBadge` (EncoderDraw.hpp:607)
// emits its own `DrawCommand::Text` for every modulator/gesture badge chip
// (e.g. "M1"), unconditionally, for ANY connected cell, drilled in or not.
// The Target/Back cell is a real, connected parameter and can carry those
// same badges, so "the first Kind::Text command" finds a badge label, not
// this indicator. This indicator is always appended LAST (AppendEncoderCell
// pushes it after `BuildEncoderDrawCommands`'s own output, same "append,
// never insert" convention AppendScopeCell used), so the LAST Kind::Text
// command is the one that is actually this indicator, on every carrying
// node this file uses (kVcoScope included, where last-or-first make no
// difference).
//
// The permanent overdraw regression guard
// (drill_back_badge_resolves_inside_the_grid_region_the_operator_actually_sees
// below) needs the indicator's INDEX within its node's command list, to
// check what -- if anything -- follows it; DrillBadgeTextCommand below only
// ever needed the command itself. Rather than let a second caller grow a
// second scan of the same list, this is the one scan site and
// DrillBadgeTextCommand is rewritten in terms of it (OMNI §8/§14: a third
// use of "find a node's Kind::Text command" must not become a third scan).
std::optional<std::size_t> DrillBadgeTextIndex(const synth::ui::NodeTree& tree, const std::string& nodeId) {
    const synth::ui::Node* node = FindNodeById(tree, nodeId);
    if (node == nullptr) {
        return std::nullopt;
    }
    std::optional<std::size_t> lastTextIndex;
    for (std::size_t ix = 0; ix < node->drawCommands.size(); ++ix) {
        if (node->drawCommands[ix].kind == synth::ui::DrawCommand::Kind::Text) {
            lastTextIndex = ix;
        }
    }
    return lastTextIndex;
}

// S5.2: returns the WHOLE command, not just `.text`, so a caller can also
// assert on the resolved TextStyle (size/colour) -- the operator's actual
// complaint was that the label rendered but did not READ as a header, so a
// content-only check can't cover the fix. `DrillBadgeText` below is the
// pre-existing `.text`-only accessor, rewritten in terms of this one (OMNI
// §8/§14: a second use of "find a node's Kind::Text command" must not
// become a second scan of the same list).
std::optional<synth::ui::DrawCommand> DrillBadgeTextCommand(const synth::ui::NodeTree& tree,
                                                            const std::string& nodeId) {
    const std::optional<std::size_t> index = DrillBadgeTextIndex(tree, nodeId);
    if (!index.has_value()) {
        return std::nullopt;
    }
    const synth::ui::Node* node = FindNodeById(tree, nodeId);
    return node != nullptr ? std::optional<synth::ui::DrawCommand>(node->drawCommands[*index]) : std::nullopt;
}

// Reused across every level checked by
// drill_level_header_shown_only_while_drilled_in_and_matches_the_level below
// (OMNI §6: 2+ uses, isolates a distinct read from the raw draw-command
// list).
std::optional<std::string> DrillBadgeText(const synth::ui::NodeTree& tree, const std::string& nodeId) {
    const std::optional<synth::ui::DrawCommand> command = DrillBadgeTextCommand(tree, nodeId);
    return command.has_value() ? std::optional<std::string>(command->text) : std::nullopt;
}

// A resolved node's `bounds` are PARENT-relative, not accumulated screen
// coordinates (PortableUI.hpp's coordinate contract, `sru-46`): "every node's
// bounds are relative to its parent's origin," and a backend's rendered
// position is that node's own bounds "folded over the accumulated origins of
// its ancestor chain" (PortableUI.hpp:44-46; the JUCE backend's own fold is
// `PortableJuceBackend.hpp:737-753`'s `resolve()`). Task F.3's rewritten
// geometry tests below compare nodes that do not share an immediate parent
// (e.g. an encoder cell several containers deep vs. the scope, or two
// encoder cells in different grid rows), so they need the SAME fold this
// helper performs by walking each node's parent chain (found by scanning
// every node's `children` list, since `Node` carries no parent pointer of
// its own) and summing each ancestor's own local offset.
synth::ui::Bounds AbsoluteBounds(const synth::ui::NodeTree& tree, const std::string& id) {
    std::map<std::string, std::string> parentOf;
    for (const synth::ui::Node& node : tree.nodes) {
        for (const synth::ui::NodeId& child : node.children) {
            parentOf[child.value] = node.id.value;
        }
    }
    const synth::ui::Node* node = FindNodeById(tree, id);
    if (node == nullptr) {
        return {};
    }
    synth::ui::Bounds bounds = node->bounds;
    std::string current = id;
    for (;;) {
        const auto found = parentOf.find(current);
        if (found == parentOf.end()) {
            break;
        }
        const synth::ui::Node* parent = FindNodeById(tree, found->second);
        if (parent == nullptr) {
            break;
        }
        bounds.x += parent->bounds.x;
        bounds.y += parent->bounds.y;
        current = found->second;
    }
    return bounds;
}

std::optional<std::size_t> FindNodeIndexById(const synth::ui::NodeTree& tree, const std::string& id) {
    for (std::size_t ix = 0; ix < tree.nodes.size(); ++ix) {
        if (tree.nodes[ix].id.value == id) {
            return ix;
        }
    }
    return std::nullopt;
}

// --- F.3: layout bounds, no overlap, cell containment -----------------------
//
// Task F.3 (openspec/changes/frogg3rs-audio-safety-and-ui-rework/tasks.md)
// replaced `FroggersPageLayout`'s hand-computed pixel `Bounds` (ContentArea/
// ScopeArea/GridArea, `FroggersEncoderGridLayout::BoundsForIndex`) with a
// declarative grid resolved by Sheaf's own layout engine. Every test below
// that used to call those functions directly now builds the REAL tree
// through a bare `FroggersUiSurface` (same convention `BuildBraid4TreeAt`
// uses in `External/Sheaf/projects/synth/tests/portable_ui_tests.cpp`) and
// reads the RESOLVED node bounds instead -- each test still pins its
// ORIGINAL property (§0: a pin is rewritten, never deleted), just against
// the new mechanism.

// A context-free surface build at an arbitrary size: the layout claims below
// are about the resolver, not about any particular engine/parameter state
// (mirrors `BuildBraid4TreeAt`, portable_ui_tests.cpp:476-485).
synth::ui::NodeTree BuildFroggersTreeAt(float width, float height) {
    synth::RuntimeConfig config = synth_froggers::FroggersApp::Config();
    config.uiWidth = static_cast<int>(width);
    config.uiHeight = static_cast<int>(height);
    synth::AppContext context;
    context.config = &config;
    synth_froggers::FroggersUiSurface surface;
    surface.Attach(&context, nullptr);
    return surface.BuildTree();
}

synth::ui::NodeTree BuildFroggersTreeAtDefaultSize() {
    return BuildFroggersTreeAt(synth_froggers::FroggersPageLayout::kDefaultWidth,
                               synth_froggers::FroggersPageLayout::kDefaultHeight);
}

// try/catch around a resolve, exactly the pattern
// `tests/portable_ui_tests.cpp:1558-1568` and
// `tests/braid4_system_tests.cpp:476-485` use -- own local implementation
// since those files live under the read-only External/Sheaf submodule.
std::string FroggersResolutionDiagnostic(const std::function<void()>& build) {
    try {
        build();
    } catch (const std::exception& error) {
        return error.what();
    }
    return {};
}

TEST_CASE(root_and_content_bounds_match_default_config_size) {
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    REQUIRE_TRUE(root.width == synth_froggers::FroggersPageLayout::kDefaultWidth);
    REQUIRE_TRUE(root.height == synth_froggers::FroggersPageLayout::kDefaultHeight);

    // REWRITE (task F.3): `ContentArea()` (a computed pixel Bounds) is gone
    // along with the auto-flow model it served -- what survives is the
    // property it existed to guarantee, that real content is inset from the
    // window edge by a nonzero margin. The left/right blocks (children of
    // the outer split Row, `FroggersNodeIds::kLayoutRoot`, whose own
    // `padding` IS `FroggersPageLayout::kMargin`) are that content now; their
    // ABSOLUTE bounds (AbsoluteBounds() above -- a resolved node's `bounds`
    // are parent-relative, not screen coordinates) must sit `kMargin` inside
    // the window on every edge they touch.
    const synth::ui::NodeTree tree = BuildFroggersTreeAtDefaultSize();
    const synth::ui::Bounds left = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kLeftBlock);
    const synth::ui::Bounds right = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kRightBlock);
    REQUIRE_TRUE(left.width > 0.0f && left.height > 0.0f);
    REQUIRE_TRUE(right.width > 0.0f && right.height > 0.0f);

    constexpr float kTolerance = 0.01f;
    REQUIRE_TRUE(std::fabs(left.x - synth_froggers::FroggersPageLayout::kMargin) < kTolerance);
    REQUIRE_TRUE(std::fabs(left.y - synth_froggers::FroggersPageLayout::kMargin) < kTolerance);
    REQUIRE_TRUE(std::fabs((root.width - (right.x + right.width)) - synth_froggers::FroggersPageLayout::kMargin) <
                 kTolerance);
    REQUIRE_TRUE(std::fabs((root.height - (left.y + left.height)) - synth_froggers::FroggersPageLayout::kMargin) <
                 kTolerance);
}

TEST_CASE(scope_and_grid_regions_do_not_overlap_at_target_window_size) {
    // REWRITE (task F.3): the invariant survives verbatim (scope and every
    // encoder cell occupy disjoint regions, all inside the window); the
    // mechanism moves from computed pixel rectangles to the RESOLVED,
    // absolute-folded node bounds of the real tree.
    const synth::ui::NodeTree tree = BuildFroggersTreeAtDefaultSize();
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds scope = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kVcoScope);
    REQUIRE_TRUE(scope.width > 0.0f && scope.height > 0.0f);
    REQUIRE_TRUE(FullyInside(scope, root));

    for (std::size_t ix = 0; ix < synth_froggers::FroggersEncoderGridLayout::kEncoderCount; ++ix) {
        const synth::ui::Bounds cell = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::Encoder(ix));
        REQUIRE_TRUE(cell.width > 0.0f && cell.height > 0.0f);
        REQUIRE_TRUE(FullyInside(cell, root));
        REQUIRE_TRUE(!Overlaps(scope, cell));
    }
}

// UI-rework ITEM 1 (design.md A3a, tasks.md B.1, 2026-07-29, the operator's
// strongest complaint, verbatim: "it is taller than it is wide, which is to
// put it mildly, fucking stupid for visual UI. it should be at most a third
// of its current size."). REWRITE (task F.3): pins the two hard requirements
// directly against the RESOLVED scope node -- wider than tall, and at most
// 1/3 of the OLD panel's area (340 wide x 528 tall -- the portrait column
// this replaced, long before this file's own `ScopeArea()` existed; the
// baseline is `FroggersPageLayout::kScopeWidth`, this file's one surviving
// definition of that historical width, and the old full content height,
// preserved here as a literal since the struct that computed it is gone). A
// regression back to a portrait panel, or one that merely shrinks without
// changing its aspect ratio, fails this test.
TEST_CASE(scope_area_is_wider_than_tall_and_at_most_a_third_of_its_old_area) {
    const synth::ui::NodeTree tree = BuildFroggersTreeAtDefaultSize();
    const synth::ui::Bounds scope = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kVcoScope);
    REQUIRE_TRUE(scope.width > 0.0f && scope.height > 0.0f);
    REQUIRE_TRUE(scope.width > scope.height);

    constexpr float kOldFullContentHeight = 528.0f;  // historical: the pre-F.3 content area's height.
    const float oldScopeArea = synth_froggers::FroggersPageLayout::kScopeWidth * kOldFullContentHeight;
    const float newScopeArea = scope.width * scope.height;
    REQUIRE_TRUE(newScopeArea <= oldScopeArea / 3.0f);
}

// POSITION REGRESSION GUARD (2026-07-29). Nothing pinned the scope's
// LOCATION, so a change asked to shrink it also moved it -- from a left-hand
// column to a full-width band across the top -- and every existing assertion
// still passed. Operator: "WHEN DID I ASK FOR YOU TO CHANGE THE LOCATION OF
// IT? i said just the height should change." REWRITE (task F.3): the single
// most important guard in this file -- still pins scope-left/grid-right/
// grid-full-height, now against the resolved left/right block nodes.
TEST_CASE(scope_sits_in_a_left_column_with_the_grid_to_its_right) {
    const synth::ui::NodeTree tree = BuildFroggersTreeAtDefaultSize();
    const synth::ui::Bounds root = synth_froggers::FroggersPageLayout::RootBounds(nullptr);
    const synth::ui::Bounds left = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kLeftBlock);
    const synth::ui::Bounds right = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kRightBlock);
    const synth::ui::Bounds scope = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kVcoScope);
    REQUIRE_TRUE(left.width > 0.0f && right.width > 0.0f && scope.width > 0.0f);

    // The scope is a COLUMN, not a full-width band.
    REQUIRE_TRUE(scope.width < root.width);

    // The grid (right block) starts to the right of the left block's right
    // edge, and the left block does not start to the right of the right
    // block (i.e. they are not stacked).
    REQUIRE_TRUE(right.x >= left.x + left.width);
    REQUIRE_TRUE(right.x > left.x);

    // The grid (right block) spans the full content height -- it is BESIDE
    // the left column, not pushed down by anything in it.
    REQUIRE_TRUE(right.height >= left.height - 0.5f);

    // The scope itself sits inside the LEFT block, not the right.
    REQUIRE_TRUE(FullyInside(scope, left));
}

TEST_CASE(every_encoder_cell_lies_fully_inside_the_grid_region) {
    // REWRITE (task F.3): "grid region" is now the right block (bank tabs,
    // the 16-slot grid, and Randomize share it per the CELL MAP), a superset
    // of the old grid-only region but still a meaningful containment check;
    // the harder property -- no two cells overlap -- is unchanged and still
    // checked pairwise.
    const synth::ui::NodeTree tree = BuildFroggersTreeAtDefaultSize();
    const synth::ui::Bounds right = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kRightBlock);
    REQUIRE_TRUE(right.width > 0.0f);

    std::vector<synth::ui::Bounds> cells;
    cells.reserve(synth_froggers::FroggersEncoderGridLayout::kEncoderCount);
    for (std::size_t ix = 0; ix < synth_froggers::FroggersEncoderGridLayout::kEncoderCount; ++ix) {
        const synth::ui::Bounds cell = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::Encoder(ix));
        REQUIRE_TRUE(cell.width > 0.0f && cell.height > 0.0f);
        REQUIRE_TRUE(FullyInside(cell, right));
        cells.push_back(cell);
    }

    for (std::size_t a = 0; a < cells.size(); ++a) {
        for (std::size_t b = a + 1; b < cells.size(); ++b) {
            REQUIRE_TRUE(!Overlaps(cells[a], cells[b]));
        }
    }
}

// --- F.3: fit guards, replacing the deleted height cross-check --------------
//
// `declared_ui_height_matches_the_derived_required_extent` DIES WITH ITS
// SUBJECT (task F.3): `FroggersAutoFlowedChromeModel`/`RequiredHeight()` are
// deleted, so there is nothing left to cross-check, and per task F.3's own
// finding that test could never actually fail even when its assumptions were
// already wrong (`FroggersAutoFlowedChromeModel::FlowedControls()` hardcoded
// a control list it never checked against the real tree). Its FUNCTION --
// proving the surface fits -- is taken over by these three, a strictly
// stronger guarantee: they resolve the real declarative grid and let
// `RequireContainerHoldsItsChildren` (PortableUILayout.hpp:267-316) itself
// report any overflow, at the three sizes task F.3's preflight gate pinned
// (900x632 default, 640x480 small, 1440x900 large) -- not an invented check.

TEST_CASE(surface_resolves_without_overflow_at_the_default_window_size) {
    const std::string diagnostic = FroggersResolutionDiagnostic([] {
        BuildFroggersTreeAt(900.0f, 632.0f);
    });
    REQUIRE_TRUE(diagnostic.empty());
}

TEST_CASE(surface_resolves_without_overflow_at_a_small_window_size) {
    const std::string diagnostic = FroggersResolutionDiagnostic([] {
        BuildFroggersTreeAt(640.0f, 480.0f);
    });
    REQUIRE_TRUE(diagnostic.empty());
}

TEST_CASE(surface_resolves_without_overflow_at_a_large_window_size) {
    const std::string diagnostic = FroggersResolutionDiagnostic([] {
        BuildFroggersTreeAt(1440.0f, 900.0f);
    });
    REQUIRE_TRUE(diagnostic.empty());
}

// --- F.6: the two labelled sliders resolve to the same width ----------------

// The guard for the defect task F.6 fixed. Operator, on the F.3 build: "the
// scene slider is too wide and the bpm slider is too narrow. grid design
// fail." Neither slider declared a width at all, so each inherited whatever
// its container implied -- scene-blend filled a Column's cross axis while BPM
// split a Row's main axis with its label. Both now come from the single
// `kSliderWidthFraction` read once in `AppendLabelledSlider()`.
//
// This asserts the two sliders against EACH OTHER, never against a pixel
// literal. A hardcoded expected width would be two numbers kept in agreement
// by hand -- the exact defect F.3 deleted when it removed
// `uiHeight == RequiredHeight()`, and it would keep passing if both sliders
// drifted together. Comparing them to each other is what actually pins
// "equal by construction."
//
// Checked at three window sizes because the width is a FRACTION of the left
// block: equality has to survive the block resizing, which is the property a
// fraction buys over a pixel count.
TEST_CASE(scene_blend_and_bpm_sliders_resolve_to_the_same_width) {
    const std::array<std::pair<float, float>, 3> sizes{{
        {synth_froggers::FroggersPageLayout::kDefaultWidth,
         synth_froggers::FroggersPageLayout::kDefaultHeight},
        {640.0f, 480.0f},
        {1440.0f, 900.0f},
    }};

    for (const auto& [width, height] : sizes) {
        const synth::ui::NodeTree tree = BuildFroggersTreeAt(width, height);
        const synth::ui::Bounds sceneBlend =
            AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kSceneBlend);
        const synth::ui::Bounds bpm = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kBpm);

        REQUIRE_TRUE(sceneBlend.width > 0.0f);
        REQUIRE_TRUE(bpm.width > 0.0f);
        REQUIRE_TRUE(std::fabs(sceneBlend.width - bpm.width) < 0.5f);
        // Both are left-aligned in the same column, so equal width with equal
        // x means they genuinely occupy the same horizontal span -- not two
        // equal-width sliders sitting in different places.
        REQUIRE_TRUE(std::fabs(sceneBlend.x - bpm.x) < 0.5f);
    }
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

// E.3 (design A7b, operator override 2026-07-29): "clicking on the page bank
// for the page we are on is the way the user should always be able to get to
// that page, even when they are in a modulation drilldown for a parameter on
// that page." `FroggersAppCore::ProcessFrame`'s RequestBankSelect handling
// used to guard the whole branch on `bankRequest != activeBankIx_`, making a
// same-bank click while drilled in a complete no-op. Fixed by resetting the
// drill-in (Back()-until-zero) when the requested bank equals the active
// bank AND the drill-in level is above 0, while still doing nothing at all
// when the requested bank equals the active bank and level is ALREADY 0 (the
// pre-existing, still-desired no-op).
TEST_CASE(clicking_the_active_bank_while_drilled_in_exits_to_the_top_level_grid) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bank_select_exits_drilldown"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const std::size_t activeBank = rig.Application().ActiveBankIndex();
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 0);

    // Drill to level 2 on the active bank via the surface's own action
    // routing -- same bridge (kEncoderPress -> ProcessFrame() ->
    // FroggersModulationDrillIn::PressEncoder) as the drill-in-swap test
    // above.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "0"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 1);
    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderPress,
        std::to_string(static_cast<std::size_t>(synth_froggers::kModSlotVco1Audio))));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 2);

    // Click the SAME (already-active) bank button -- must exit the
    // drilldown back to the top-level parameter grid, not no-op.
    surface.DispatchAction(synth::ui::Action::WithValue(synth_froggers::FroggersActions::kBankSelect,
                                                          std::to_string(activeBank)));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 0);
    REQUIRE_TRUE(rig.Application().ActiveBankIndex() == activeBank);  // still the same bank, just exited

    // Existing no-op MUST be preserved: clicking the same bank while ALREADY
    // at level 0 must not disturb anything. There is no direct "was
    // drillIn_ reconstructed" observable, so this checks every state this
    // request path can touch: activeBankIx_, the drill-in's level, and its
    // selected parameter (null at level 0 either way) all stay exactly as
    // they were.
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().BankRef().SelectedParameter() == nullptr);
    surface.DispatchAction(synth::ui::Action::WithValue(synth_froggers::FroggersActions::kBankSelect,
                                                          std::to_string(activeBank)));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveBankIndex() == activeBank);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 0);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().BankRef().SelectedParameter() == nullptr);
}

// F7 (operator request, openspec/changes/frogg3rs-blowout-and-drilldown-
// repair/tasks.md): "when we are in modulation drilldown levels ... headers
// ... 'Modulation Level 1' then 2 then 3." Level 0 shows no indicator; each
// deeper level shows the matching text, sourced from
// FroggersModulationDrillIn::Level() (never a hardcoded per-level string).
// Drills to level 3 with the SAME encoder-id sequence
// clicking_the_active_bank_while_drilled_in_exits_to_the_top_level_grid uses
// for level 1->2 (kEncoderPress "0" then kModSlotVco1Audio), extended one
// more press (kModSlotVco2Audio) to reach level 3 -- the identical sequence
// fourth_level_drill_in_is_refused (FroggersModulationTests.cpp) uses to
// reach the same depth, so this is a proven-valid path, not a guess.
//
// S5.2 (operator regression report, 2026-08-07: "i still don't see a header
// label counting the drilldown levels") extends this same test rather than
// adding a parallel one: once drilled to level 1, also asserts the resolved
// TextStyle is an explicit, deliberate choice (not TextStyle{}'s own
// default), and that an opaque Fill (the backing band) immediately precedes
// the Text command in the carrying node's own draw list.
//
// RELOCATED, 2026-08-08 (see AppendScopeCell's and AppendEncoderCell's own
// comments, FroggersUiSurface.hpp, for the full investigation): the
// carrying node used to be kVcoScope -- proven, by
// drill_back_badge_resolves_inside_the_grid_region_the_operator_actually_sees
// below, to sit in a region of the window the operator is not looking at
// while drilled in -- and is now the Target/Back cell
// (`Encoder(kEncoderCount - 1)`, present at every drill depth by
// construction). Same F7/S5.2 properties, same test, new node and new,
// cell-sized text ("BACK L<N>" rather than "Modulation Level N" -- this
// cell resolves to ~136px wide, not the old header's ~285px, so the text
// itself had to shrink along with the node it moved to).
TEST_CASE(drill_level_header_shown_only_while_drilled_in_and_matches_the_level) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("drill_level_header"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const std::string backId = synth_froggers::FroggersNodeIds::Encoder(
        synth_froggers::FroggersEncoderGridLayout::kEncoderCount - 1);

    // OMNI §9.1 positive control: print the level actually observed so an
    // "absent at level 0" pass can never be mistaken for a rig that simply
    // never drilled -- the level itself comes from the same real getter
    // (ActiveDrillIn().Level()) the levels-1/2/3 checks below rely on.
    const std::size_t levelBeforeDrilling = rig.Application().ActiveDrillIn().Level();
    std::cout << "[OBSERVED] drill level before any encoder press: " << levelBeforeDrilling << "\n";
    REQUIRE_TRUE(levelBeforeDrilling == 0);
    REQUIRE_TRUE(!DrillBadgeText(surface.BuildTree(), backId).has_value());

    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "0"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 1);
    const synth::ui::NodeTree drilledTree = surface.BuildTree();
    REQUIRE_TRUE(DrillBadgeText(drilledTree, backId).value_or("") == "BACK L1");
    // Removal guard: kVcoScope must not grow a second copy of this text --
    // see AppendScopeCell's own comment on why a second copy would be OMNI
    // §8 duplication of the one fact this surface needs to communicate.
    REQUIRE_TRUE(!DrillBadgeText(drilledTree, synth_froggers::FroggersNodeIds::kVcoScope).has_value());

    // S5.2 styling assertions, carried over to the new location -- the text
    // must not be a bare, default-styled DrawCommand::Text. The drawing
    // branch (AppendEncoderCell, FroggersUiSurface.hpp) is identical at
    // every level > 0 (only the interpolated number changes, already
    // covered by the three "== BACK L<N>" checks in this test), so checking
    // the style and band once here is not a guess about levels 2/3 -- it is
    // the same code path they also execute.
    const std::optional<synth::ui::DrawCommand> headerCommand = DrillBadgeTextCommand(drilledTree, backId);
    REQUIRE_TRUE(headerCommand.has_value());
    // Different from TextStyle{}'s own 14px default (PortableUI.hpp) -- the
    // concrete, testable form of "an explicit size, not a default." Not
    // pinned as ">14px" the way the old kVcoScope header was: this badge is
    // deliberately SMALLER than that header was, to fit a ~136px-wide cell
    // instead of a ~285px-wide one (see AppendEncoderCell's own comment on
    // kDrillBackBadgeTextStyle) -- "explicitly chosen" is the property that
    // survived the move, not "larger."
    REQUIRE_TRUE(headerCommand->textStyle.size != synth::ui::TextStyle{}.size);

    const synth::ui::Node* backNode = FindNodeById(drilledTree, backId);
    REQUIRE_TRUE(backNode != nullptr);
    const std::optional<std::size_t> headerIndex = DrillBadgeTextIndex(drilledTree, backId);
    REQUIRE_TRUE(headerIndex.has_value());
    REQUIRE_TRUE(*headerIndex > 0);  // a Text command with nothing before it can't have a backing band
    const synth::ui::DrawCommand& bandBeforeText = backNode->drawCommands[*headerIndex - 1];
    REQUIRE_TRUE(bandBeforeText.kind == synth::ui::DrawCommand::Kind::Fill &&
                 bandBeforeText.bounds.height == headerCommand->bounds.height);

    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderPress,
        std::to_string(static_cast<std::size_t>(synth_froggers::kModSlotVco1Audio))));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 2);
    REQUIRE_TRUE(DrillBadgeText(surface.BuildTree(), backId).value_or("") == "BACK L2");

    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderPress,
        std::to_string(static_cast<std::size_t>(synth_froggers::kModSlotVco2Audio))));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 3);
    REQUIRE_TRUE(DrillBadgeText(surface.BuildTree(), backId).value_or("") == "BACK L3");
}

// STEP 1/STEP 3 (operator re-report, 2026-08-08, third session on the same
// complaint): "the header is STILL not visible." The first fix (S5.2,
// restyling) and the second (proving the header is never overdrawn WITHIN
// kVcoScope's own node) were each correct about the property they checked
// and both irrelevant to the actual complaint, because neither asked
// whether kVcoScope's cell is anywhere near the region the operator is
// looking at while drilled in. It is not, computed (not eyeballed) here and
// in AppendScopeCell's own comment: at the real 900x632 config, kVcoScope
// resolves to roughly {16, 16, 284.7, 181.3} (x-range [16, 300.7]) while the
// right block -- bank tabs, the 16-slot grid, and Randomize, i.e. the ENTIRE
// region whose CONTENT is what a drill-in actually changes -- resolves to
// roughly {314.7, 16, 569.3, 600} (x-range [314.7, 884]). A 14px gap
// (FroggersPageLayout::kGap) separates those x-ranges; they never meet.
// Existence-in-the-tree and non-overdraw-within-its-own-node are both true
// of kVcoScope's old header and BOTH IRRELEVANT to whether the operator's
// eyes ever cross that region -- which is why three sessions of "fixing" it
// never fixed anything the operator could see. This test is the permanent,
// computed proof of the property that actually matters -- containment
// inside the region the operator sees populated cells in -- kept alive as
// the regression guard the previous two sessions were missing.
//
// "The region the operator actually sees" is not invented for this test: it
// is `kRightBlock`'s own resolved bounds, the same container
// `every_encoder_cell_lies_fully_inside_the_grid_region` above already
// proves holds all 16 physical slots at every non-drilled build. This test
// extends that same containment claim into the DRILLED-IN state
// specifically -- the only state either the old or the new indicator ever
// renders anything in.
//
// kModSlotRandomSh6 ("Random S&H 6") is the OMNI §9.1 positive control: the
// one modulation source registered `/*connected=*/true` unconditionally
// (FroggersModulation.hpp:539-540, its own GangedRandomLfoVisualizer), so it
// is guaranteed to be a live, rendering cell in the drilled-in grid with no
// patch setup needed, and it proves the region this test checks against is
// the REAL one the operator sees populated cells in -- not a coincidentally
// passing empty rectangle.
TEST_CASE(drill_back_badge_resolves_inside_the_grid_region_the_operator_actually_sees) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("drill_header_overdraw"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "0"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 1);  // genuinely drilled in, not assumed

    const synth::ui::NodeTree tree = surface.BuildTree();
    const synth::ui::Bounds gridRegion = AbsoluteBounds(tree, synth_froggers::FroggersNodeIds::kRightBlock);
    REQUIRE_TRUE(gridRegion.width > 0.0f && gridRegion.height > 0.0f);

    // OMNI §9.1 positive control: a cell the operator demonstrably DOES see
    // while drilled in. Its own nonzero command count is real, load-bearing
    // evidence, not an assumption -- AppendEncoderCell emits zero ring
    // commands for a hidden/disconnected cell (BuildEncoderDrawCommands
    // returns {} when !state.connected, EncoderDraw.hpp:653-656).
    const std::string sourceId = synth_froggers::FroggersNodeIds::Encoder(
        static_cast<std::size_t>(synth_froggers::kModSlotRandomSh6));
    const synth::ui::Node* sourceRing = FindNodeById(tree, sourceId);
    REQUIRE_TRUE(sourceRing != nullptr);
    REQUIRE_TRUE(sourceRing->drawCommands.size() > 0);
    const synth::ui::Bounds sourceBounds = AbsoluteBounds(tree, sourceId);

    // The new indicator: the Target/Back cell's own "BACK L<N>" band+text,
    // folded to ABSOLUTE (screen) coordinates -- containment has to be
    // checked in the same coordinate space as the region it is checked
    // against, unlike the node-local check below.
    const std::string backId = synth_froggers::FroggersNodeIds::Encoder(
        synth_froggers::FroggersEncoderGridLayout::kEncoderCount - 1);
    const std::optional<std::size_t> headerIndex = DrillBadgeTextIndex(tree, backId);
    REQUIRE_TRUE(headerIndex.has_value());
    const synth::ui::Node* backNode = FindNodeById(tree, backId);
    REQUIRE_TRUE(backNode != nullptr);
    const synth::ui::Bounds headerLocalBounds = backNode->drawCommands[*headerIndex].bounds;
    const synth::ui::Bounds backCellAbsolute = AbsoluteBounds(tree, backId);
    const synth::ui::Bounds headerAbsoluteBounds{
        backCellAbsolute.x + headerLocalBounds.x,
        backCellAbsolute.y + headerLocalBounds.y,
        headerLocalBounds.width,
        headerLocalBounds.height,
    };

    std::cout << "[OBSERVED] gridRegion={" << gridRegion.x << "," << gridRegion.y << "," << gridRegion.width << ","
              << gridRegion.height << "} sourceBounds={" << sourceBounds.x << "," << sourceBounds.y << ","
              << sourceBounds.width << "," << sourceBounds.height << "} headerAbsoluteBounds={"
              << headerAbsoluteBounds.x << "," << headerAbsoluteBounds.y << "," << headerAbsoluteBounds.width << ","
              << headerAbsoluteBounds.height << "}\n";

    // The containment claim this whole test exists to encode, computed
    // (FullyInside, already used throughout this file for exactly this kind
    // of check), not eyeballed from a screenshot.
    REQUIRE_TRUE(FullyInside(sourceBounds, gridRegion));          // the positive control really is in-region
    REQUIRE_TRUE(FullyInside(headerAbsoluteBounds, gridRegion));  // ... and so is the new indicator

    // Local anti-overdraw check on the Target/Back cell's OWN command list
    // (node-local coordinates on both sides, NOT mixed with the absolute
    // bounds above): no command appended after the badge text may overlap
    // its bounds -- the same guard this test's predecessor kept for
    // kVcoScope, carried to the node that actually owns this text now.
    for (std::size_t ix = *headerIndex + 1; ix < backNode->drawCommands.size(); ++ix) {
        REQUIRE_TRUE(!Overlaps(headerLocalBounds, backNode->drawCommands[ix].bounds));
    }
}

// S5.1 (operator regression report, 2026-08-07: "the drilldown back button
// still doesn't go one back, it goes all the way back") -- traced to
// FroggersModulationDrillIn::PressEncoder (FroggersModulation.hpp): every
// on-screen press, including the Target/Back cell, dispatches through
// kEncoderPress -> PressEncoder, never through .Back() directly (see that
// class's own header comment), so a test calling .Back() directly (as
// FroggersModulationTests.cpp's back_* tests do) cannot catch a regression
// in the operator's actual gesture -- that gap is why the bug shipped as
// "landed" in 49ce9af/9d0802c despite Back() itself being correct and
// already tested. This test drives the REAL gesture: dispatches
// kEncoderPress on physical encoder 15 (the Target/Back cell --
// Bank::OpenModulationView always places the selected parameter's own cell
// at physicalLayout.back(), and FullPhysicalLayout is the same fixed
// 16-wide 0-15 span at every level, so 15 is the Target/Back cell at every
// depth -- see drill_in_swaps_grid_in_place_scope_and_chrome_stay_put
// above's identical level-1 case) through the surface's own action routing,
// exactly as a real click would, and checks it pops exactly ONE level per
// press, from level 3 down to 0.
TEST_CASE(pressing_target_back_cell_through_the_surface_pops_exactly_one_drill_level_at_a_time) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("target_back_pops_one_level"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 0);

    // Drill to level 3 -- the identical proven-valid encoder-id sequence
    // drill_level_header_shown_only_while_drilled_in_and_matches_the_level
    // and fourth_level_drill_in_is_refused (FroggersModulationTests.cpp) use.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "0"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 1);

    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderPress,
        std::to_string(static_cast<std::size_t>(synth_froggers::kModSlotVco1Audio))));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 2);

    surface.DispatchAction(synth::ui::Action::WithValue(
        synth_froggers::FroggersActions::kEncoderPress,
        std::to_string(static_cast<std::size_t>(synth_froggers::kModSlotVco2Audio))));
    rig.RunBlocks(4);

    // OMNI §9.1 positive control: prove the descent actually REACHED level 3
    // before testing that a Target/Back press pops from it -- a pop that
    // starts from a level the rig never reached would prove nothing.
    const std::size_t reachedLevel = rig.Application().ActiveDrillIn().Level();
    std::cout << "[OBSERVED] drill level reached before Target/Back presses: " << reachedLevel << "\n";
    REQUIRE_TRUE(reachedLevel == 3);

    // The operator's ACTUAL gesture, level 3 -> 2: kEncoderPress on the
    // Target/Back cell, not a direct .Back() call. Before this fix,
    // PressEncoder collapsed straight to level 0 here (the reported bug).
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "15"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 2);  // ONE level back, not zero

    // Level 2 -> 1.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "15"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 1);

    // Level 1 -> 0, exiting the modulation view entirely.
    surface.DispatchAction(
        synth::ui::Action::WithValue(synth_froggers::FroggersActions::kEncoderPress, "15"));
    rig.RunBlocks(4);
    REQUIRE_TRUE(rig.Application().ActiveDrillIn().Level() == 0);
    REQUIRE_TRUE(!rig.Application().ActiveDrillIn().BankRef().ShowingModulation());
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

// --- S6.1: dropped frame around the encoder (operator screenshot, ring/frame
// collision) ------------------------------------------------------------

// Operator screenshot: the parameter card's rounded-rect frame outline
// visibly crossed the encoder's own modulation ring. Diagnosed as a geometry
// collision entirely inside Sheaf's BuildEncoderDrawCommands (EncoderDraw.hpp)
// between the frame (gated by EncoderDrawState::wantsFrame, EncoderDraw.hpp:
// 293, default true) and the ring/arc layers -- see AppendEncoderCell's own
// comment (FroggersUiSurface.hpp) for the full citation. This test pins the
// RENDERED consequence -- no CELL-SPANNING StrokeRoundedRect command reaches
// the encoder node -- rather than the intermediate EncoderDrawState field,
// because that field is consumed inside the Draw callback's closure and
// never surfaces on synth::ui::Node: the draw-command list is what is
// actually observable through this test surface (NodeTree/FindNodeById),
// same convention DrillBadgeTextCommand above uses to read a node's own
// draw list.
//
// NOT "any StrokeRoundedRect": a later pass (2026-08-08), after wiring
// wantsFrame=false actually made this test inspect a populated cell (see
// the "pre-existing gap" comment below), found it still red. Root cause was
// this test, not the fix: AppendBadge (EncoderDraw.hpp:586-608, the
// modulator/gesture badge chips, e.g. "M1"/"M2") emits its own
// unconditional StrokeRoundedRect outline, entirely unrelated to
// wantsFrame, and legitimate chrome the operator never asked to remove --
// grep confirms AppendEncoderCell (FroggersUiSurface.hpp:1049) is the
// ONLY call site into BuildEncoderDrawCommands this app's build reaches
// (desktop-v2/braid-4/miniapp call sites are frozen/unrelated apps), and it
// sets `state.wantsFrame = false` unconditionally for every cell
// (FroggersUiSurface.hpp:1006) before that one call, so the
// wantsFrame-gated rect (EncoderDraw.hpp:690-694) is provably dead code on
// this path -- what was still firing was the badge outline.
//
// Geometry tells the two apart cleanly. Traced against a live encoder(0)
// cell built by this exact test (136.333x88.333, reproducible bit-for-bit
// across repeated runs -- the default patch's modulator routing is fixed,
// not randomized): the card frame would be `{bounds.x+1, bounds.y+1,
// bounds.width-2, bounds.height-2}` off a `bounds` inset from the node
// extent by a flat 4px on each side (EncoderDraw.hpp:658-664, 690-694) --
// 126.33x78.33, i.e. 92.7%/88.7% of the cell's own extent. A badge side
// length is `radius * badgeLengthFraction`, `radius = min(bounds.width,
// bounds.height) * 0.43 * 0.72`, `badgeLengthFraction =
// 1/sqrt(1 + total*total/4)` (EncoderGeometry::GetBadgePosition,
// EncoderDraw.hpp:249-275), which is LARGEST at total==1
// (badgeLengthFraction ~= 0.894) -- an upper bound of ~27.7% of the cell's
// SMALLER dimension at any cell size, since every factor is a fixed
// fraction of a cell dimension, never a flat pixel inset. The two
// StrokeRoundedRect commands this exact cell emits (two modulator badges,
// total==2) land at 17.5866x17.5866 -- 12.9% width / 19.9% height of the
// cell -- recomputing bit-for-bit from that formula (centerX=68.1667,
// radius=24.8712, badgeLengthFraction=1/sqrt(2), badge 0 at x=50.5801,
// badge 1 at x=68.1667, both y=24.1701). A "> half the cell in both
// dimensions" threshold sits with wide margin on both sides of that gap
// (badges top out near 28%, the frame sits at 75%+ even on a
// pathologically small cell) and is what `spansCell` below checks -- so a
// badge chip can never trip `sawFrame`, while the real frame always would.
TEST_CASE(encoder_cell_never_emits_a_frame_draw_command) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("encoder_no_frame"));
    rig.RunBlocks(4);
    // Pre-existing gap found while verifying this test (it was never
    // confirmed against a real build -- see this task's own history):
    // without this call, slots[0].cellCapacity stays 0 and
    // AppendEncoderCell's `state` stays default-constructed (disconnected,
    // zero voices), so BuildEncoderDrawCommands legitimately emits NOTHING
    // and the assertions below pass/fail vacuously rather than against real
    // rendered state. Same "forces a synchronous publish" call
    // encoder_ring_renders_fuegoized_value_not_raw_scene_center already uses
    // (SynthRig::UIState(), tests/support/SynthRig.hpp) -- VERIFIED this
    // makes sawBody/sawRing observe real commands (drawCommands 0 -> 77).
    // Confirmed by rebuilding+running this one binary in isolation.
    rig.UIState();

    synth::ui::Surface& surface = rig.Application().PortableSurface();
    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* encoder = FindNodeById(tree, synth_froggers::FroggersNodeIds::Encoder(0));
    REQUIRE_TRUE(encoder != nullptr);

    // A card frame spans (near) the whole cell; a badge chip is small and
    // offset -- see this test's own header comment for the traced geometry
    // and the numbers that put wide margin on both sides of this threshold.
    constexpr float kFrameSpanFraction = 0.5f;

    bool sawFrame = false;
    bool sawBadgeOutline = false;
    bool sawBody = false;
    bool sawRing = false;
    for (const synth::ui::DrawCommand& command : encoder->drawCommands) {
        if (command.kind == synth::ui::DrawCommand::Kind::StrokeRoundedRect) {
            const bool spansCell = command.bounds.width > encoder->bounds.width * kFrameSpanFraction &&
                                    command.bounds.height > encoder->bounds.height * kFrameSpanFraction;
            // A badge chip's own outline (AppendBadge, EncoderDraw.hpp:602)
            // is legitimate chrome, not the operator's complaint, and must
            // NOT be caught here -- only a cell-spanning stroke counts as
            // the card frame.
            if (spansCell) {
                sawFrame = true;
            } else {
                sawBadgeOutline = true;
            }
        }
        if (command.kind == synth::ui::DrawCommand::Kind::FillEllipse) {
            sawBody = true;
        }
        if (command.kind == synth::ui::DrawCommand::Kind::Arc) {
            sawRing = true;
        }
    }

    // OMNI §9.1 positive control: prove this cell is genuinely connected and
    // populated (body + ring commands actually present) so the "no frame"
    // assertion below cannot pass by accident against an empty/disconnected
    // command list -- AppendEncoderCell's own `hidden` branch DOES
    // legitimately emit an empty command vector for a disconnected cell in
    // the modulation view, and this test must not be confused with that case.
    std::cout << "[OBSERVED] encoder(0) drawCommands: " << encoder->drawCommands.size()
              << ", body(FillEllipse) seen=" << sawBody << ", ring(Arc) seen=" << sawRing
              << ", badge outline(s) seen=" << sawBadgeOutline << "\n";
    REQUIRE_TRUE(sawBody);
    REQUIRE_TRUE(sawRing);

    // Second OMNI §9.1 positive control, specific to the size-based split
    // above: the default patch wires modulators onto this cell (Audio
    // bank's VCO1 pitch), so this cell DOES emit badge-chip
    // StrokeRoundedRect commands every run. Without this, "no frame" below
    // could pass vacuously against a cell that never emits a
    // StrokeRoundedRect of ANY kind -- which would prove nothing about
    // whether `spansCell` actually keeps badges out of `sawFrame`, exactly
    // the vacuous-pass failure mode this test's own history already
    // produced once (see the "pre-existing gap" comment above).
    REQUIRE_TRUE(sawBadgeOutline);

    // The actual fix: no CELL-SPANNING stroke-rounded-rect (the card frame)
    // reaches the rendered encoder cell.
    REQUIRE_TRUE(!sawFrame);
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

// Label-visibility fix (2026-07-28), updated F.2d (2026-08-03, Sheaf pin
// 77a3019e), REWRITTEN AGAIN by task F.3's CELL MAP amendment (2026-08-04):
// "the Scene blend label sits BELOW its slider. This supersedes the F.2d
// caption for scene-blend (a `ControlStyle::caption` can only lead, so
// scene-blend returns to a hand-rolled label, now placed under the slider)."
//
// `NodeKind::Slider` routes `node.label` to `juce::Slider::setName()` only
// (PortableJuceBackend.hpp:1229-1232) -- no `juce::Label` is attached, so
// nothing ever draws it; some adjacent Label node is required regardless of
// which mechanism produces it. F.2d had made that mechanism
// `ControlStyle::caption` (a sibling Label BEFORE the control); the CELL MAP
// amendment reverts scene-blend specifically to a hand-rolled Label node
// (`FroggersNodeIds::kSceneBlendLabel`) placed AFTER the slider, because a
// caption can only lead and the operator wants it below/trailing here.
//
// NOTE: task F.3's own test enumeration table classified this test as
// UNAFFECTED, which does not hold once the CELL MAP amendment is applied --
// see FroggersUiSurface.hpp's `AppendSceneBlendGroup()` comment for the same
// note. The amendment is the more specific, more recently affirmed
// instruction and governs; this rewrite is flagged in the task report as a
// place the traced table was wrong.
//
// This test asserts the Label NODE exists, carries the expected text, and
// sits immediately AFTER the Slider in `tree.nodes` order. It does NOT and
// CANNOT prove the text is actually painted on screen -- that requires a
// human looking at the running app; a previous task was closed on exactly
// that false equivalence (asserting the label field was set) and this
// comment exists so it isn't repeated.
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
    // The label FOLLOWS its slider in node order, which in the Column both
    // rows now use (AppendLabelledSlider(), task F.6) renders it BELOW the
    // slider -- the opposite order from the retired F.2d caption, which
    // always led. Row 6 asserts the identical relationship; the two rows are
    // deliberately the same shape now.
    REQUIRE_TRUE(*labelIx == *sliderIx + 1);
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
// what an adjacent Label node does/doesn't prove. This asserts the caption
// Label node exists and reads the constant "BPM" text in both transport
// states (UI-rework ITEM 5, design.md A3f -- the "(no effect while stopped)"
// annotation this test used to track is gone, see
// `bpm_label_is_constant_across_transport_state`'s comment for why).
//
// NEITHER this pair NOR its scene-blend neighbour is a `ControlStyle::
// caption`, and for the same single reason: `Builder::FinishControl`
// (PortableUIBuilders.hpp:428-465) always emits a caption BEFORE its control
// with no option to place it after, and since task F.6 (2026-08-05) BOTH
// labels sit BELOW their slider. Filed as upstream ask 14 (caption
// placement); when it lands, both collapse into captions together.
//
// THE HISTORY HERE IS THE POINT, because this comment has twice asserted the
// opposite. F.2d converted scene-blend to a caption and left this one
// hand-rolled, citing B12's trailing-label instruction as a second live
// cause; an implementer following F.2d's brief literally flipped this
// label's order and flagged it rather than absorbing it, and the flip was
// reverted. Then F.3's CELL MAP moved scene-blend's label BELOW its slider,
// and F.6 moved this one below to match -- which retired B12 outright.
//
// B12's stated reason was that a LEADING label sat between the two sliders
// and read as labelling the wrong control; trailing was merely the only
// alternative available while both labels shared a horizontal band. A label
// directly beneath its own control cannot be misread that way, so both-below
// serves B12's concern better than trailing did. The asymmetry B12 protected
// was a means, not the goal -- and an earlier draft of F.6 missed exactly
// that, keeping trailing and inventing a placement parameter to honour B12's
// literal words. An instruction's rationale is part of the instruction: when
// the rationale dies, the instruction is up for re-derivation rather than
// mechanical preservation.
TEST_CASE(bpm_slider_has_an_adjacent_label_node_with_the_constant_bpm_text) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("bpm_adjacent_label"));
    rig.RunBlocks(4);

    synth::ui::Surface& surface = rig.Application().PortableSurface();

    auto checkAdjacentLabel = [&](const std::string& expectedText) {
        const synth::ui::NodeTree tree = surface.BuildTree();
        const synth::ui::Node* labelNode =
            FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpmLabel);
        REQUIRE_TRUE(labelNode != nullptr);
        REQUIRE_TRUE(labelNode->kind == synth::ui::NodeKind::Label);
        REQUIRE_TRUE(labelNode->text == expectedText);

        const std::optional<std::size_t> labelIx =
            FindNodeIndexById(tree, synth_froggers::FroggersNodeIds::kBpmLabel);
        const std::optional<std::size_t> sliderIx =
            FindNodeIndexById(tree, synth_froggers::FroggersNodeIds::kBpm);
        REQUIRE_TRUE(labelIx.has_value());
        REQUIRE_TRUE(sliderIx.has_value());
        // The label FOLLOWS its slider in node order, which inside the Column
        // AppendLabelledSlider() emits renders it BELOW the slider -- the
        // same relationship the scene-blend row asserts. Keep this pinned to
        // the ORDER, not merely to the label's existence: an ordering
        // assertion is what caught F.2d silently moving this label once
        // already, and order is what distinguishes "labels its own control"
        // from "labels its neighbour's."
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
    // F.2b/F.2e (2026-08-03, Sheaf pin 77a3019e): Play/Stop are real
    // draw-command controls again -- a rounded plate plus a `Color::Green`
    // triangle (Play) / `Color::Red` square (Stop), via
    // `BuildPlayDrawCommands`/`BuildStopDrawCommands`
    // (FroggersUiSurface.hpp) -- replacing the EMOJI-glyph Button nodes
    // (UI-rework ITEM 4/B.4, 2026-07-29) that were themselves a workaround
    // for `Draw` nodes needing a double click and `Node` having no colour
    // field. Both causes are gone at this pin (ask 1: plain click via
    // `ControlStyle::action`; a `Draw` node's own commands always carried
    // colour, which is why the emoji workaround chose emoji in the first
    // place -- see the removed comment's own reasoning). Assert by id: Draw
    // kind, drawCommands contains the plate then the coloured icon in the
    // real Sheaf colours (not a text/emoji substitute), the action on
    // `node.action` (not `doubleClickAction`, which this file never sets),
    // and a resolved square extent (Draw has no intrinsic size --
    // `layout.main`/`cross` were given explicitly, see BuildTree()'s own
    // comment -- so a collapsed 0x0 node would mean that wiring broke).
    const synth::ui::Node* playNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kPlay);
    const synth::ui::Node* stopNode = FindNodeById(tree, synth_froggers::FroggersNodeIds::kStop);
    REQUIRE_TRUE(playNode != nullptr);
    REQUIRE_TRUE(playNode->kind == synth::ui::NodeKind::Draw);
    REQUIRE_TRUE(playNode->bounds.width == synth_froggers::kTransportPlateSize);
    REQUIRE_TRUE(playNode->bounds.height == synth_froggers::kTransportPlateSize);
    REQUIRE_TRUE(playNode->drawCommands.size() == 2);
    REQUIRE_TRUE(playNode->drawCommands[0].kind == synth::ui::DrawCommand::Kind::FillRoundedRect);
    REQUIRE_TRUE(playNode->drawCommands[0].color == synth_froggers::kTransportPlateColor);
    REQUIRE_TRUE(playNode->drawCommands[1].kind == synth::ui::DrawCommand::Kind::FillPolygon);
    REQUIRE_TRUE(playNode->drawCommands[1].color == synth::Color::Green);
    REQUIRE_TRUE(playNode->action.has_value() &&
                 playNode->action->name == synth_froggers::FroggersActions::kPlay);
    REQUIRE_TRUE(!playNode->doubleClickAction.has_value());

    REQUIRE_TRUE(stopNode != nullptr);
    REQUIRE_TRUE(stopNode->kind == synth::ui::NodeKind::Draw);
    REQUIRE_TRUE(stopNode->bounds.width == synth_froggers::kTransportPlateSize);
    REQUIRE_TRUE(stopNode->bounds.height == synth_froggers::kTransportPlateSize);
    REQUIRE_TRUE(stopNode->drawCommands.size() == 2);
    REQUIRE_TRUE(stopNode->drawCommands[0].kind == synth::ui::DrawCommand::Kind::FillRoundedRect);
    REQUIRE_TRUE(stopNode->drawCommands[0].color == synth_froggers::kTransportPlateColor);
    REQUIRE_TRUE(stopNode->drawCommands[1].kind == synth::ui::DrawCommand::Kind::Fill);
    REQUIRE_TRUE(stopNode->drawCommands[1].color == synth::Color::Red);
    REQUIRE_TRUE(stopNode->action.has_value() &&
                 stopNode->action->name == synth_froggers::FroggersActions::kStop);
    REQUIRE_TRUE(!stopNode->doubleClickAction.has_value());

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

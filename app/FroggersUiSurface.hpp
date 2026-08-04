#pragma once

// synth_froggers::FroggersUiSurface -- packet 10 of the froggers-sheaf-app
// change (openspec/changes/froggers-sheaf-app/tasks.md, section "10. Surface
// layout (ported v2 design)", tasks 10.1-10.7; design D9/D9a/D9b/D11/D14/D17;
// spec `specs/froggers-app-surface-layout/spec.md`).
//
// Follows Braid 4's builder composition exactly (apps/braid-4/Braid4UI.hpp:
// 31-96; layout math apps/braid-4/Braid4UiModel.hpp:70-88,90-206, controls
// :386-413): a portable `synth::ui::Surface` built fresh every BuildTree()
// call from a Builder, reading published `ParameterManager::UIState`/
// `BankSlot::UIState` snapshots for display and routing every operator
// interaction through `synth::ui::Action`. No JUCE, no v2 JUCE component
// reused (design D11) -- only the v2 *layout* (top band of scopes + chrome,
// a bank selector, a 16-slot grid that swaps in place on drill-in) is
// ported.
//
// Bounds note: `synth::ui::Builder`'s Button/Slider/Toggle/ComboBox/
// TextField/StatusText node kinds take NO explicit `Bounds` (see
// PortableUI.hpp's `Node`/Builder's method signatures) -- only `Draw`/
// `DrawInteractive`/`Visualizer` nodes do. Braid4UI.hpp's own
// `AppendBraid4Controls` confirms this: it calls `builder.Button(...)`/
// `builder.Slider(...)` with no bounds at all, leaving control placement to
// whatever host renders the node tree. This surface follows the identical
// division of labor: `FroggersPageLayout`/`FroggersEncoderGridLayout` below
// compute real `Bounds` for the two regions that are actually
// Draw-positioned -- the VCO scope panel and the 16-slot encoder grid (task
// 10.5's "no overlap"/"every encoder cell fully inside the grid region"
// tests target exactly these two, plus -- as of F.2b, 2026-08-03, Sheaf pin
// 77a3019e -- Play/Stop, restored as `Draw` nodes). Play/Stop and the
// bank-select buttons were briefly coloured-icon/hand-drawn Draw nodes
// (operator 2026-07-27/2026-07-28) to get custom icons and true
// colour-inversion selection, but were reverted back to plain `Button`
// nodes on 2026-07-28 (tasks 6.3/6.4): at Sheaf pin 1940ddcb,
// Draw/DrawInteractive nodes dispatched only on double-click
// (`RetainedDrawComponent`, PortableJuceBackend.hpp:549-555 -- no
// plain-click path), which cost single-click page-switching and transport
// control. Ask 1 landed plain click for `Draw` nodes at pin 77a3019e, so
// F.2a/F.2b (2026-08-03) moved the encoder grid's drill-in press and
// Play/Stop back onto `Draw` nodes using `ControlStyle::action` -- see
// AppendEncoderGrid()'s and BuildTree()'s own comments. Randomize All/Page,
// scene, BPM, and the bank buttons remain plain Button/Slider/StatusText
// nodes with no app-computed placement, exactly like Braid4's own
// scene/bank buttons and scene-blend slider. The transport-strip/
// bank-header-strip vertical RESERVATIONS that used to sit above the
// scope/grid regions for the Draw-node versions of these controls have been
// removed as dead weight (regression fix, see `FroggersAutoFlowedChromeModel`/
// `FroggersPageLayout` below): they no longer draw anything, and (traced
// below) removing them does not move where the runtime's auto-flow chrome
// band starts -- it only lets the scope/grid regions use the space those
// strips used to blank out.
//
// Crunchy was removed from this chrome band entirely (operator 2026-07-27:
// "why is there a fucking slider for crunchy... i never asked for that. It
// duplicates bank slot 15"). Crunchy is reachable only via the encoder
// grid's slot 15 now, addressed exactly like any other bank parameter --
// see design.md D11/Resolved-decisions and tasks.md 10.2 for the recorded
// trade-off (Crunchy is unreachable while a modulation view is open, since
// slot 15 is then Target/Back).
//
// Threading note: see FroggersAppCore.hpp's own header comment for the full
// reasoning. Encoder DRAG, scene select/blend, and transport Start/Stop are
// pushed straight onto `context_->uiBus` (`Bank::HandleTick`/
// `HandleSetAbsolute` never touch drill-in state, so the generic message-bus
// path Braid4 itself uses is safe here too); encoder PRESS, Randomize All/
// Page, and the BPM slider instead call `FroggersAppCore::Request*` (a
// pending-atomic bridge the audio thread drains in `ProcessFrame()`),
// because those three mutate audio-thread-owned state (the app's own
// drill-in cap or MasterClock) with no existing generic `MessageIn` shape
// safe for this app's sparse (11-of-16) bank layout.

#include "FroggersAppCore.hpp"

#include "synth/EncoderDraw.hpp"
#include "synth/MasterClock.hpp"
#include "synth/ParameterModulation.hpp"
#include "synth/PortableUI.hpp"
#include "synth/PortableUIBuilders.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace synth_froggers {

namespace FroggersNodeIds {

inline constexpr const char* kRoot = "froggers.root";
inline constexpr const char* kPlay = "froggers.transport.play";
inline constexpr const char* kStop = "froggers.transport.stop";
inline constexpr const char* kRandomizeAll = "froggers.randomize.all";
inline constexpr const char* kRandomizePage = "froggers.randomize.page";
inline constexpr const char* kSceneBlend = "froggers.scene.blend";
inline constexpr const char* kBpm = "froggers.bpm";
// Survives F.2d's caption conversion on purpose. The scene-blend label's
// hand-rolled node went away because its ONLY cause -- upstream never drawing
// slider captions -- is dead at pin 77a3019e. This one had a SECOND cause,
// B12 (tasks.md, 2026-07-29): the BPM label must TRAIL its slider, because
// leading it puts it between the two sliders and reads as labelling the
// scene-blend one. `ControlStyle::caption` always emits the caption before
// its control (`Builder::FinishControl`, PortableUIBuilders.hpp:428-465) and
// offers no trailing option, so B12's cause is still live and this node stays.
// Deleting it would have silently reversed an explicit operator instruction.
// Tracked upstream as ask 14 (caption placement); when that lands, this
// collapses into `ControlStyle::caption` like the scene-blend one already has.
inline constexpr const char* kBpmLabel = "froggers.bpm.label";
inline constexpr const char* kVcoScope = "froggers.scope.vco";

inline std::string BankButton(std::size_t bankIx) {
    return "froggers.bank." + std::to_string(bankIx);
}

inline std::string SceneButton(std::size_t sceneIx) {
    return "froggers.scene." + std::to_string(sceneIx);
}

inline std::string Encoder(std::size_t ix) {
    return "froggers.encoder." + std::to_string(ix);
}

}  // namespace FroggersNodeIds

namespace FroggersActions {

inline constexpr const char* kPlay = "froggers.transport.play";
inline constexpr const char* kStop = "froggers.transport.stop";
inline constexpr const char* kRandomizeAll = "froggers.randomize.all";
inline constexpr const char* kRandomizePage = "froggers.randomize.page";
inline constexpr const char* kBankSelect = "froggers.bank.select";
inline constexpr const char* kSceneSelect = "froggers.scene.select";
inline constexpr const char* kSceneBlend = "froggers.scene.blend";
inline constexpr const char* kBpm = "froggers.bpm";
inline constexpr const char* kEncoderPress = "froggers.encoder.press";
inline constexpr const char* kEncoderDrag = "froggers.encoder.drag";

}  // namespace FroggersActions

// Regression fix (operator 2026-07-28, task 3.7 follow-up): a just-landed
// change reverted Play/Stop and the six bank buttons from bounds-carrying
// `Draw` nodes back to unbounded `Button` nodes (tasks 6.3/6.4, see this
// file's header comment). That made SIXTEEN controls flow via the runtime's
// own auto-layout instead of one, but `FroggersPageLayout::RequiredHeight()`
// (below) still reserved room for only a single 28px row -- silently
// under-reserving `config.uiHeight` (FroggersAppCore.hpp) and clipping the
// bottom of the chrome band.
//
// This struct is a small, EXPLICIT app-side replica of just enough of
// `PortableJuceBackend.hpp`'s control-sizing/greedy-wrap rules to count how
// many rows this app's own fixed 16-control chrome band will occupy and how
// tall that block is. The app cannot query the backend directly for this
// (this file is portable code with no JUCE dependency -- see this file's
// header comment and `FroggersSurfaceTests.cpp`'s own
// `#ifdef JUCE_MAJOR_VERSION #error` guard), and Sheaf exposes no "measured
// auto-flow extent" accessor. Every constant/formula below cites the exact
// `PortableJuceBackend.hpp` line range it mirrors, so a future toolkit
// change to those rules is a findable trail rather than a silent drift, and
// this is kept deliberately minimal -- enough to size THIS app's control
// set, not a general layout engine.
struct FroggersAutoFlowedChromeModel {
    // PortableJuceBackend.hpp:339-347 (private consts of
    // PortableJuceMainComponent -- the same class whose LayoutControls()/
    // DefaultSizeForNode() this mirrors).
    static constexpr float kControlGap = 8.0f;
    static constexpr float kControlMargin = 12.0f;
    static constexpr float kDefaultButtonWidth = 72.0f;
    static constexpr float kDefaultButtonHeight = 28.0f;
    static constexpr float kDefaultSliderWidth = 140.0f;
    static constexpr float kDefaultSliderHeight = 28.0f;
    static constexpr float kDefaultLabelHeight = 22.0f;

    // This app never flows a Toggle/ComboBox/TextField, so DefaultSizeForNode's
    // branches for those kinds (PortableJuceBackend.hpp:597-600,613-617) have
    // no replica here -- `LabelLike` below covers both `Label` and
    // `StatusText` (identical case in the switch, :601-607).
    enum class Kind { Button, LabelLike, Slider };

    struct ControlSpec {
        Kind kind;
        std::string label;  // Button label, or Label/StatusText text.
    };

    // PortableJuceBackend.hpp:608-612 (`NodeKind::Button` case of
    // `DefaultSizeForNode`): width = max(kDefaultButtonWidth,
    // round(label.size()*6.5 + 24)); height = kDefaultButtonHeight.
    static float ButtonWidth(const std::string& label) {
        const float raw = std::round(static_cast<float>(label.size()) * 6.5f + 24.0f);
        return std::max(kDefaultButtonWidth, raw);
    }

    // PortableJuceBackend.hpp:601-607 (`NodeKind::Label`/`NodeKind::StatusText`
    // case): width = min(availableWidth, max(120, round(text.size()*6.5 +
    // 12))); height = kDefaultLabelHeight. The original reads
    // `node.text.empty() ? node.label : node.text` -- this app's Label/
    // StatusText nodes (AppendChromeBand() below) only ever set `label` via
    // `Builder::Label`/`Builder::StatusText`, never `text`, so this replica
    // only needs the label branch.
    static float LabelLikeWidth(const std::string& text, float availableWidth) {
        const float raw = std::round(static_cast<float>(text.size()) * 6.5f + 12.0f);
        return std::min(availableWidth, std::max(120.0f, raw));
    }

    static float ControlWidth(const ControlSpec& spec, float availableWidth) {
        switch (spec.kind) {
            case Kind::Button:
                return ButtonWidth(spec.label);
            case Kind::LabelLike:
                return LabelLikeWidth(spec.label, availableWidth);
            case Kind::Slider:
                return kDefaultSliderWidth;  // PortableJuceBackend.hpp:595-596.
        }
        return kDefaultButtonWidth;
    }

    static float ControlHeight(const ControlSpec& spec) {
        switch (spec.kind) {
            case Kind::Button:
                return kDefaultButtonHeight;
            case Kind::LabelLike:
                return kDefaultLabelHeight;
            case Kind::Slider:
                return kDefaultSliderHeight;
        }
        return kDefaultButtonHeight;
    }

    // This app's exact flowed-chrome control set, IN BUILD ORDER -- flow
    // order follows `m_tree.nodes` push order (PortableJuceBackend.hpp:754,
    // 786-797's `FlowCursor` walks the tree in order), which in turn matches
    // `synth::ui::Builder`'s push_back-on-call-order (PortableUIBuilders.hpp:
    // 277-278, 407-416) -- so this list must match BuildTree()/
    // AppendBankHeader()/AppendChromeBand()'s own build call order exactly:
    // Play, Stop, the 6 bank buttons (read from `FroggersBankLayouts()`, the
    // single source of truth AppendBankHeader() itself reads -- not
    // retyped), Randomize Page, Randomize All, Scene 1, Scene 2, Scene-blend
    // label+slider, BPM label+slider. 16 controls total.
    //
    // UI-rework ITEM 5 (design.md A3f, tasks.md B.5, 2026-07-29): the BPM
    // label used to read "BPM (no effect while stopped)" pre-Play
    // (AppendChromeBand() below) -- this list used to carry that longer
    // string deliberately, as the WIDEST state, since reserving for the
    // stopped-state label was the worst case. That annotation is gone (a
    // constant "BPM" now, in both transport states -- see
    // AppendChromeBand()'s own comment), so this list carries the one
    // string that is now the only state: "BPM".
    // Task 6.14: this control set is width-independent (only ControlWidth,
    // called separately per spec, takes availableWidth) and never varies
    // across calls, but ComputeFlowExtent() calls it once per FlowedControls
    // spec in a loop, and ComputeFlowExtent is itself called from
    // ContentArea(), which BuildTree() calls once per UI frame at 30 Hz --
    // so a plain by-value return here heap-allocates this vector and its 16
    // std::strings 30 times a second to recompute something that cannot
    // change. A function-local `static const`, returned by reference, builds
    // the vector exactly once (first call, thread-safe under C++11's
    // "magic statics") and every subsequent call is a pointer return.
    static const std::vector<ControlSpec>& FlowedControls() {
        static const std::vector<ControlSpec> controls = [] {
            std::vector<ControlSpec> built;
            // UI-rework ITEM 4 (design.md A3e, tasks.md B.4, 2026-07-29):
            // labels are the glyphs "▶"/"■" now (AppendChromeBand() below),
            // not the words "Play"/"Stop" -- both are short enough that
            // ButtonWidth's max(72, ...) floor dominates either way (no
            // numeric change to this model's output), but this list models
            // the REAL flowed control set, so it carries the real label.
            built.push_back({Kind::Button, "▶"});
            built.push_back({Kind::Button, "■"});
            for (const FroggersBankLayout& layout : FroggersBankLayouts()) {
                built.push_back({Kind::Button, layout.name});
            }
            built.push_back({Kind::Button, "Randomize Page"});
            built.push_back({Kind::Button, "Randomize All"});
            built.push_back({Kind::Button, "Scene 1"});
            built.push_back({Kind::Button, "Scene 2"});
            built.push_back({Kind::LabelLike, "Scene blend"});
            built.push_back({Kind::Slider, "Scene blend"});
            built.push_back({Kind::LabelLike, "BPM"});
            built.push_back({Kind::Slider, "BPM"});
            return built;
        }();
        return controls;
    }

    struct FlowExtent {
        int rowCount = 0;
        float totalHeight = 0.0f;
    };

    // Replica of the greedy wrap in `LayoutControls()`
    // (PortableJuceBackend.hpp:740-798): a control starts a new row when
    // `cursor.x + width > cursor.right` AND the current row is non-empty
    // (:787), advancing `cursor.y` by `rowHeight + kControlGap` on wrap
    // (:791). `cursor.x`/`cursor.right`/`availableWidth` derive from the
    // flow root's own width minus `kControlMargin` on each side (:780-783).
    // This app's chrome band has exactly one flow root
    // (`froggers.root`/`FroggersNodeIds::kRoot`), so no per-root grouping is
    // needed here, unlike the general `cursors` map in the original.
    static FlowExtent ComputeFlowExtent(float rootWidth) {
        const float availableWidth = std::max(0.0f, rootWidth - kControlMargin * 2.0f);
        const float right = kControlMargin + availableWidth;  // rootBounds.getRight() - kControlMargin (root.x == 0).
        float cursorX = kControlMargin;
        float rowHeight = 0.0f;
        FlowExtent extent;
        for (const ControlSpec& spec : FlowedControls()) {
            const float width = ControlWidth(spec, availableWidth);
            const float height = ControlHeight(spec);
            if (cursorX + width > right && rowHeight > 0.0f) {
                extent.totalHeight += rowHeight + kControlGap;
                cursorX = kControlMargin;
                rowHeight = 0.0f;
            }
            if (rowHeight == 0.0f) {
                ++extent.rowCount;
            }
            cursorX += width + kControlGap;
            rowHeight = std::max(rowHeight, height);
        }
        extent.totalHeight += rowHeight;
        return extent;
    }
};

// Task 10.1 (design D11): layout math for the two Draw-positioned regions
// (see this file's header comment on why buttons/sliders have none). Named
// and shaped after `Braid4PageLayout` (apps/braid-4/Braid4UiModel.hpp:
// 90-206) but reduced to what this app actually needs: one scope panel (not
// Braid4's dual VCO+LFO scope stacks -- FroggersAppCore exposes exactly one
// combined, 3-layer VCO ScopeVisualizer, built in packet 7; see this
// packet's own report for the "dual" wording discrepancy) stacked above a
// single 16-slot grid (UI-rework ITEM 1, 2026-07-29, retired the original
// side-by-side "scope column beside grid column" split -- see
// `ScopeArea()`/`GridArea()`'s own comments below for why).
struct FroggersPageLayout {
    static constexpr float kDefaultWidth = 900.0f;
    static constexpr float kMargin = 16.0f;
    static constexpr float kGap = 14.0f;

    // UI-rework ITEM 1 (design.md A3a, tasks.md B.1, 2026-07-29, the
    // operator's strongest complaint): this used to be `kScopeWidth`
    // (340.0f), the width of a portrait scope column that spanned the
    // FULL content height (528px at the default 900x632 window --
    // `kContentAreaHeight` below minus `kMargin*2`) -- 340 * 528 =
    // 179520 px^2, far taller than wide. Operator, verbatim: "it is taller
    // than it is wide, which is to put it mildly, fucking stupid for visual
    // UI. it should be at most a third of its current size." Requirement:
    // wider than tall, at most 1/3 of that area, and the reclaimed space
    // goes to the encoder grid (not left blank).
    //
    // Fix: ScopeArea()/GridArea() below switch from a SIDE-BY-SIDE split
    // (scope column left, grid column right, both spanning the full content
    // height) to a STACKED one (a short, full-width landscape scope band at
    // the top of the content area; the grid fills the entire rectangle
    // below it). That is what makes "reclaimed space goes to the encoder
    // grid, not left blank" literal: GridArea() inherits exactly the
    // height/width the scope band does not use, with no leftover strip --
    // same idea FroggersPageLayout's own transport-strip-removal note above
    // already used for the 2026-07-28 fix.
    //
    // `kScopeHeight` is sized against the DEFAULT content width (868 =
    // 900 - 2*kMargin): 868 * 64 = 55552 px^2 = 55552/179520 = ~31.0% of
    // the old area -- under the 1/3 (33.3%) ceiling with headroom, not
    // pinned to the exact boundary, so a few-px content-width change cannot
    // push it over. 868 > 64 (wider than tall) with a wide margin.
    static constexpr float kScopeHeight = 64.0f;
    // Restored 2026-07-29 with the scope's position (see ScopeArea below).
    // Unchanged from the original layout: only the HEIGHT was ever meant to
    // change. 340 x 64 is 5.3x wider than tall and ~12% of the original
    // 340 x 528 column's area.
    static constexpr float kScopeWidth = 340.0f;

    // `kContentAreaHeight` is the height the scope/grid content area needs
    // (560 -- untouched by this fix, per the task brief's "do not redesign
    // the scope band"): ScopeArea/GridArea below just divide up whatever
    // height content is given, with no minimum of their own. It was chosen
    // once for a comfortable scope/grid size.
    //
    // `RequiredHeight()` adds, below that, room for the chrome band the
    // runtime auto-flows below this content area (Play, Stop, the 6 bank
    // buttons, Randomize Page, Randomize All, Scene 1, Scene 2, Scene
    // blend, BPM -- BuildTree()/AppendBankHeader()/AppendChromeBand()
    // below): `RuntimeMainComponent::IntrinsicBounds()`
    // (External/Sheaf/projects/synth/include/synth/RuntimeMainComponent.hpp:
    // 204-210) gives `uiHeight` ZERO vertical slack for it on its own, so
    // whatever this struct does not explicitly reserve clips below the
    // window (design E3g's original trace).
    //
    // Regression fix (2026-07-28): 16 controls now flow here (Play/Stop and
    // the bank buttons were reverted from bounds-carrying `Draw` nodes back
    // to unbounded `Button` nodes, tasks 6.3/6.4), not the 6 a prior
    // revision of this comment assumed -- `FroggersAutoFlowedChromeModel`
    // (above) computes the REAL flowed row count/height for this app's
    // actual control set instead of a single hardcoded row height, so a
    // future control added to the chrome band changes this number instead
    // of silently under-reserving again.
    static constexpr float kContentAreaHeight = 560.0f;
    // The gap between the lowest Draw-positioned node (the scope/grid
    // content area's bottom edge -- see ContentArea()'s own trace below)
    // and the first auto-flowed control row (PortableJuceBackend.hpp:781,
    // `cursor.y = maxDrawBottom + kControlGap`) -- numerically the same
    // `kControlGap` `FroggersAutoFlowedChromeModel` replicates, kept as its
    // own named constant here since it plays a distinct structural role
    // (the ONE inter-region gap, vs. the N-1 inter-row gaps folded into
    // `FlowExtent::totalHeight`).
    static constexpr float kAutoFlowedChromeGap = 8.0f;

    // Callable derivation (not `constexpr`: `FroggersAutoFlowedChromeModel::
    // ComputeFlowExtent` loops over `std::string`-labelled controls, so this
    // is evaluated at runtime, not compile time, unlike the old bare-sum
    // constant it replaces). `FroggersAppCore::Config()` cannot call this
    // directly (FroggersUiSurface.hpp includes FroggersAppCore.hpp -- the
    // reverse include would be circular), so Config() carries a literal
    // matching this value, cross-checked by FroggersSurfaceTests.cpp's
    // `declared_ui_height_matches_the_derived_required_extent`.
    static float RequiredHeight() {
        return kContentAreaHeight + kAutoFlowedChromeGap +
               FroggersAutoFlowedChromeModel::ComputeFlowExtent(kDefaultWidth).totalHeight;
    }

    static synth::ui::Bounds RootBounds(const synth::AppContext* context) {
        const float width = context != nullptr && context->config != nullptr
                                 ? static_cast<float>(context->config->uiWidth)
                                 : kDefaultWidth;
        const float height = context != nullptr && context->config != nullptr
                                  ? static_cast<float>(context->config->uiHeight)
                                  : RequiredHeight();
        return {0.0f, 0.0f, width, height};
    }

    // The content area's height reserves the auto-flowed chrome band's
    // space (gap + the REAL computed flow extent for `rootBounds.width`,
    // not a hardcoded single-row guess) IN ADDITION to the usual top/bottom
    // `kMargin` -- so a taller `rootBounds.height` does NOT simply hand the
    // extra room to the scope/grid area (which would happily consume it and
    // leave the chrome band clipped again).
    static synth::ui::Bounds ContentArea(synth::ui::Bounds rootBounds) {
        const float chromeExtent = FroggersAutoFlowedChromeModel::ComputeFlowExtent(rootBounds.width).totalHeight;
        return {
            kMargin,
            kMargin,
            std::max(0.0f, rootBounds.width - kMargin * 2.0f),
            std::max(0.0f, rootBounds.height - kMargin * 2.0f - kAutoFlowedChromeGap - chromeExtent),
        };
    }

    // Regression fix (2026-07-28), part B: this struct used to reserve two
    // EXTRA blank vertical strips above the scope/grid content --
    // `kTransportHeight`(40)+kGap and `kBankHeaderHeight`(28)+kGap,
    // left over from when Play/Stop and the bank buttons were briefly
    // bounds-carrying `Draw` nodes (tasks 10.2/3.1) needing dedicated
    // strips. Tasks 6.3/6.4 reverted both back to unbounded `Button` nodes
    // (this file's header comment) and removed their per-button bounds
    // helpers (`TransportArea()`/`BankHeaderArea()`) as dead code, but LEFT
    // the strip-height reservations in place, so the reserved space simply
    // went blank instead of being removed.
    //
    // Traced (not assumed) that removing them is safe: `ScopeArea`/
    // `GridArea` below always spanned the FULL height of whatever `content`
    // (or `belowTransport`/`gridColumn`) they were given, and the removed
    // helpers preserved the BOTTOM edge while only pushing the TOP edge (and
    // shrinking the height by the same amount) -- `y' = y + offset`,
    // `height' = height - offset`, so `y' + height' == y + height` always.
    // The runtime's own auto-flow start point,
    // `maxDrawBottom = max(Draw-node bottoms) + kControlGap`
    // (PortableJuceBackend.hpp:768-781), reads exactly that bottom edge --
    // which these two strips left UNCHANGED. So removing them moves nothing
    // about where the auto-flowed chrome band starts (`RequiredHeight()`
    // above is unaffected); it only lets `ScopeArea`/`GridArea` actually use
    // the space that used to render blank. Chosen over shrinking the window
    // instead (the task brief's other listed option) because it requires no
    // new constant to redistribute the freed space and it fixes an existing
    // asymmetry for free: before this fix, `ScopeArea` height (content minus
    // only the transport strip) and the grid's actual cell height (content
    // minus BOTH strips) already differed by `kBankHeaderHeight + kGap`
    // (42px) -- after removing both, they are equal again. Freed: 54px
    // (`kTransportHeight` 40 + `kGap` 14) to the scope area; 96px total (54
    // + `kBankHeaderHeight` 28 + `kGap` 14) to the grid area, since the grid
    // previously lost both strips and the scope only the first.

    // Top of content: the VCO scope panel (packets 7-9's visualizers get
    // placed here -- task 10.2), UI-rework ITEM 1 (see this struct's own
    // comment above): a full-width landscape band, `kScopeHeight` tall,
    // wider than tall by construction. `std::min(kScopeHeight,
    // content.height)` is defensive only (mirrors the old code's own
    // defensiveness against a content area smaller than the panel it was
    // asked to hold) -- at the app's one configured window size this is
    // always `kScopeHeight` itself, never the clamp.
    // POSITION REGRESSION FIXED 2026-07-29. An earlier revision of this item
    // shrank the scope AND moved it -- from a left-hand column to a
    // full-width band across the top of the content area. The operator asked
    // for neither: "WHEN DID I ASK FOR YOU TO CHANGE THE LOCATION OF IT? i
    // said just the height should change." Only the HEIGHT was ever in scope.
    //
    // So: back to the original SIDE-BY-SIDE split -- scope in a left-hand
    // column `kScopeWidth` wide, encoder grid in the column to its right --
    // with the height fix kept. The panel is `kScopeWidth` x `kScopeHeight`
    // = 340 x 64 = 21760 px^2 against the original full-height column's
    // 340 x 528 = 179520 px^2, i.e. ~12% of the old area (requirement was at
    // most a third) and 5.3x wider than tall (requirement was wider than
    // tall).
    //
    // The remainder of the left column, below this panel, is deliberately
    // LEFT EMPTY: the operator intends transport/scene controls there
    // ("stop / start ; scene 1 / scene 2 ; scene blend" in two columns) but
    // has deferred it, because positioning controls requires Draw nodes and
    // Draw nodes are double-click-only at Sheaf pin 1940ddcb. Do not fill
    // this space and do not reclaim its width for the grid -- see tasks.md
    // D.6.
    static synth::ui::Bounds ScopeArea(synth::ui::Bounds content) {
        return {
            content.x,
            content.y,
            std::min(kScopeWidth, content.width),
            std::min(kScopeHeight, content.height),
        };
    }

    // The 16-slot grid (task 10.3), in the column to the RIGHT of the scope's
    // left-hand column -- restored alongside ScopeArea's position fix above.
    // It spans the full content HEIGHT (it is beside the scope, not beneath
    // it), so it is unaffected by the scope panel's height and does not
    // inherit the empty space the shrunk scope left below itself in the left
    // column. `kGap` separates the two columns horizontally, as it did
    // originally.
    static synth::ui::Bounds GridArea(synth::ui::Bounds content) {
        const float x = content.x + std::min(kScopeWidth, content.width) + kGap;
        return {
            x,
            content.y,
            std::max(0.0f, content.x + content.width - x),
            content.height,
        };
    }
};

// Task 10.3 (design D5a/D11): the 16-slot grid, slots 0-15 laid out 4x4 --
// same division-of-area math as `Braid4EncoderGridLayout`
// (apps/braid-4/Braid4UiModel.hpp:70-88), sized to
// `kFroggersSlotsPerBank` (FroggersParameters.hpp) rather than Braid4's own
// `kEncoderCount`.
struct FroggersEncoderGridLayout {
    static constexpr std::size_t kColumns = 4;
    static constexpr std::size_t kRows = 4;
    static constexpr std::size_t kEncoderCount = kColumns * kRows;
    static constexpr float kGap = 8.0f;

    static synth::ui::Bounds BoundsForIndex(synth::ui::Bounds area, std::size_t index) {
        const std::size_t row = index / kColumns;
        const std::size_t column = index % kColumns;
        const float cellWidth = (area.width - kGap * static_cast<float>(kColumns - 1)) /
                                 static_cast<float>(kColumns);
        const float cellHeight = (area.height - kGap * static_cast<float>(kRows - 1)) /
                                  static_cast<float>(kRows);
        return {
            area.x + static_cast<float>(column) * (cellWidth + kGap),
            area.y + static_cast<float>(row) * (cellHeight + kGap),
            cellWidth,
            cellHeight,
        };
    }
};

static_assert(FroggersEncoderGridLayout::kEncoderCount == kFroggersSlotsPerBank,
              "the grid must render exactly the 16 physical encoder slots FroggersParameterModel wires up");

// Change 3 (operator 2026-07-27): Play/Stop as coloured icons -- "Play =
// green triangle on white. Stop = red square on white." Built from exactly
// Sheaf's existing portable primitives (verified present and painted in
// both the JUCE and browser backends): `DrawCommand::FillRoundedRect` for
// the plate, `DrawCommand::FillPolygon` for the Play triangle,
// `DrawCommand::Fill(Bounds, Color)` for the Stop square. Commands are
// authored in the SAME absolute coordinate space as `bounds` (matching
// `synth::ui::BuildEncoderDrawCommands`'s own convention, EncoderDraw.hpp)
// so the backend's local-vs-absolute heuristic (`DrawCommandsLookLocal`)
// resolves them as absolute, not node-local.
//
// Task 3.8 (operator 2026-07-28, "look like shit from a butt" -- geometry,
// not concept): the icon is inset to a fixed FRACTION of the plate rather
// than a fixed pixel amount so it scales with the square and lands at
// ~55-60% of the plate with even padding on all sides, and the plate uses
// Sheaf's own chrome "primary" button colour (`ButtonColourForNode`'s
// variant=="primary" branch, PortableJuceBackend.hpp:1130-1148, RGB
// 57/106/127) instead of stark white so it sits in the dark instrument face
// instead of glaring out of it. F.2b (2026-08-03, Sheaf pin 77a3019e, ask 1
// landed): these two builders are back in use as the Play/Stop `Draw` nodes
// in BuildTree() below, in the in-flow factory form
// (`Builder::Draw(id, DrawFactory, ControlStyle)`,
// PortableUIBuilders.hpp:316-327) -- their signature
// `vector<DrawCommand>(Bounds)` already matches `DrawFactory` exactly, so
// they are passed as the factory directly. `kTransportPlateSize` gives the
// node an explicit square extent: `Draw` has no case in `metrics::
// IntrinsicFor` (PortableUIMetrics.hpp:36-53, `default: {0,0,0,0}`), so an
// in-flow Draw node with no explicit `layout.main`/`layout.cross` would
// resolve to zero size.
inline constexpr synth::Color kTransportPlateColor = synth::Color::Rgb(57, 106, 127);
inline constexpr float kTransportIconFraction = 0.575f;  // ~55-60% of the plate
inline constexpr float kTransportPlateSize = 28.0f;      // matches the old Button height

inline std::vector<synth::ui::DrawCommand> BuildPlayDrawCommands(synth::ui::Bounds bounds) {
    constexpr float kCornerRadius = 4.0f;
    std::vector<synth::ui::DrawCommand> commands;
    commands.push_back(synth::ui::DrawCommand::FillRoundedRect(bounds, kCornerRadius, kTransportPlateColor));
    const float insetX = bounds.width * (1.0f - kTransportIconFraction) * 0.5f;
    const float insetY = bounds.height * (1.0f - kTransportIconFraction) * 0.5f;
    const float left = bounds.x + insetX;
    const float right = bounds.x + bounds.width - insetX;
    const float top = bounds.y + insetY;
    const float bottom = bounds.y + bounds.height - insetY;
    commands.push_back(synth::ui::DrawCommand::FillPolygon(
        {
            synth::ui::Point{left, top},
            synth::ui::Point{left, bottom},
            synth::ui::Point{right, (top + bottom) * 0.5f},
        },
        synth::Color::Green));
    return commands;
}

inline std::vector<synth::ui::DrawCommand> BuildStopDrawCommands(synth::ui::Bounds bounds) {
    constexpr float kCornerRadius = 4.0f;
    std::vector<synth::ui::DrawCommand> commands;
    commands.push_back(synth::ui::DrawCommand::FillRoundedRect(bounds, kCornerRadius, kTransportPlateColor));
    const float insetX = bounds.width * (1.0f - kTransportIconFraction) * 0.5f;
    const float insetY = bounds.height * (1.0f - kTransportIconFraction) * 0.5f;
    const synth::ui::Bounds square{
        bounds.x + insetX,
        bounds.y + insetY,
        std::max(0.0f, bounds.width - insetX * 2.0f),
        std::max(0.0f, bounds.height - insetY * 2.0f),
    };
    commands.push_back(synth::ui::DrawCommand::Fill(square, synth::Color::Red));
    return commands;
}

// Task 3.1 (operator 2026-07-28, design E3a) added `BankButtonBounds` here
// (per-button placement for bank-select buttons rendered as Draw nodes) and
// `kBankChromeBackground` (a hand-ported copy of Sheaf's own unselected
// Button chrome, needed only because Draw nodes paint their own explicit
// colours). Task 6.3 (operator 2026-07-28) reverted bank buttons back to
// plain `Button` nodes (see AppendBankHeader()'s own note) -- Button nodes
// have no app-computed bounds and get their selected/unselected chrome for
// free from Sheaf's own `ButtonColourForNode`
// (PortableJuceBackend.hpp:1130-1149), so both became unreachable and were
// removed.

// Small parse helpers (own implementation, following Braid4UiModel.hpp's
// ParseSize/ParseFloat *pattern* -- design D11 ports the design, not the
// implementation, and Braid4UiModel.hpp itself lives under the read-only
// External/Sheaf submodule).
inline std::size_t FroggersParseSize(const std::string& value, std::size_t fallback) {
    if (value.empty()) {
        return fallback;
    }
    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(value, &consumed, 10);
        return consumed == value.size() ? static_cast<std::size_t>(parsed) : fallback;
    } catch (...) {
        return fallback;
    }
}

inline float FroggersParseFloat(const std::string& value, float fallback) {
    if (value.empty()) {
        return fallback;
    }
    try {
        std::size_t consumed = 0;
        const float parsed = std::stof(value, &consumed);
        return consumed == value.size() ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}

// Encoder drag actions carry both the grid position and the delta, encoded
// as "position:delta" (no separate slotIx field -- unlike Braid4, this app
// has exactly one BankSlot, always slot 0).
inline std::string FormatFroggersEncoderDrag(std::size_t position, float delta) {
    return std::to_string(position) + ":" + std::to_string(delta);
}

inline bool ParseFroggersEncoderDrag(const std::string& value, std::size_t& position, float& delta) {
    const std::size_t separator = value.find(':');
    if (separator == std::string::npos) {
        return false;
    }
    const std::size_t parsedPosition =
        FroggersParseSize(value.substr(0, separator), std::numeric_limits<std::size_t>::max());
    const float parsedDelta =
        FroggersParseFloat(value.substr(separator + 1), std::numeric_limits<float>::quiet_NaN());
    if (parsedPosition == std::numeric_limits<std::size_t>::max() || !std::isfinite(parsedDelta)) {
        return false;
    }
    position = parsedPosition;
    delta = parsedDelta;
    return true;
}

inline std::string FormatFroggersBpm(double bpm) {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << bpm;
    return oss.str();
}

class FroggersUiSurface final : public synth::ui::Surface {
public:
    void Attach(synth::AppContext* context, FroggersAppCore* app) {
        context_ = context;
        app_ = app;
    }

    synth::ui::NodeTree BuildTree() override {
        const synth::ui::Bounds root = FroggersPageLayout::RootBounds(context_);
        const synth::ui::Bounds content = FroggersPageLayout::ContentArea(root);

        synth::ui::Builder builder;
        builder.Root(FroggersNodeIds::kRoot, root);
        // Task 3.4 (operator 2026-07-28): the on-canvas "Frogg3rs Synth"
        // title label is removed -- `config.appName`
        // (FroggersAppCore.hpp:135) and `FroggersManifest().displayName`
        // (FroggersRegistration.hpp:22) already cover launcher/window-title
        // naming and are untouched by this change (design E3f). No
        // replacement node is appended here: the title was a generic,
        // unbounded `Label` node auto-flowed by host chrome (see this
        // file's header comment on Node kinds), so removing it simply frees
        // that row for whatever the host flows next -- nothing else in this
        // surface computes a Bounds that depended on its presence. The
        // freed space is left for a future logo (design E3f, deferred
        // pending upstream `DrawCommand::Image`).

        // F.2b/F.2e (2026-08-03, Sheaf pin 77a3019e): Play/Stop are real
        // draw-command controls again -- a rounded plate plus a
        // `Color::Green` triangle (Play) / `Color::Red` square (Stop), via
        // `BuildPlayDrawCommands`/`BuildStopDrawCommands` above -- restoring
        // Task 3.8's colour-icon design (E3a/Change 3) that Task 6.4
        // reverted on 2026-07-28 only because Draw/DrawInteractive nodes
        // dispatched exclusively on double-click at pin 1940ddcb, which cost
        // single-click transport control. That cause is gone: ask 1 landed
        // (`Draw` nodes now dispatch plain click from `ControlStyle::
        // action`, confirmed at `RetainedDrawComponent::mouseUp`,
        // PortableJuceBackend.hpp:592-603), so the workaround this file
        // carried in its place -- Button nodes with EMOJI glyphs ("▶️"/"🟥")
        // as the label TEXT, chosen 2026-07-29 (UI-rework ITEM 4/B.4)
        // specifically because a `Node` has no colour field and an emoji
        // carries its own -- is retired along with it. `Node` still has no
        // colour field for Button/Label text, but a `Draw` node's own
        // commands do, so the real green triangle / red square replace the
        // emoji substitute.
        synth::ui::ControlStyle playStyle{};
        playStyle.action = synth::ui::Action::Named(FroggersActions::kPlay);
        playStyle.layout.main = synth::ui::Extent::Px(kTransportPlateSize);
        playStyle.layout.cross = synth::ui::Extent::Px(kTransportPlateSize);
        builder.Draw(FroggersNodeIds::kPlay, BuildPlayDrawCommands, playStyle);

        synth::ui::ControlStyle stopStyle{};
        stopStyle.action = synth::ui::Action::Named(FroggersActions::kStop);
        stopStyle.layout.main = synth::ui::Extent::Px(kTransportPlateSize);
        stopStyle.layout.cross = synth::ui::Extent::Px(kTransportPlateSize);
        builder.Draw(FroggersNodeIds::kStop, BuildStopDrawCommands, stopStyle);

        // Task 10.2: the packet 7-9 VCO scope panel, finally placed. Exactly
        // one combined 3-layer ScopeVisualizer exists (see this file's
        // header comment) -- placed in the scope area, left of the grid.
        // No longer offset below a transport strip -- see
        // FroggersPageLayout's removal note.
        const synth::ui::Bounds scopeArea = FroggersPageLayout::ScopeArea(content);
        if (app_ != nullptr) {
            synth::ui::Visualizer& vcoScope = app_->VcoScopeVisualizer();
            vcoScope.SetBounds(scopeArea);
            builder.Visualizer(FroggersNodeIds::kVcoScope, &vcoScope, synth::ui::ControlStyle{});
        }

        // Task 10.3/10.7/3.1/6.3: bank header -- direct-select bank buttons
        // plus Randomize Page (restoring the per-page position desktop-v2
        // removed, design D14). Task 6.3 (operator 2026-07-28) reverted the
        // bank buttons back to unbounded Button nodes (see
        // AppendBankHeader()'s own note), so no header-area bounds are
        // computed or passed here any more.
        const synth::ui::Bounds gridArea = FroggersPageLayout::GridArea(content);
        AppendBankHeader(builder);

        // Task 10.3/10.4/10.5: the 16-slot grid, in place -- reads the
        // SAME `context_->uiState->slots[0]` snapshot whether it currently
        // holds the parameter grid or a drilled-in modulation-detail grid
        // (Bank::OpenModulationView/Deselect swap `visible_`'s contents;
        // this surface has no branch of its own for "which grid" -- it just
        // renders whatever BankSlot::PopulateUIState published, exactly like
        // Braid4UI.hpp's own encoder loop). No longer offset below a
        // bank-header strip -- see FroggersPageLayout's removal note.
        AppendEncoderGrid(builder, gridArea);

        // Task 10.2/10.6/10.7 (Crunchy removed operator 2026-07-27): global
        // chrome -- Randomize All (only -- Randomize Page lives in the bank
        // header above, never here), scenes, and the BPM slider beside the
        // scene slider.
        AppendChromeBand(builder);

        // F.2a/F.2c (2026-08-03, Sheaf pin 77a3019e): nothing in this tree
        // needs post-Build() patching any more. The encoder grid's drill-in
        // press and drag actions are set at build-once time via
        // `ControlStyle::action`/`pointerDragAction` in AppendEncoderGrid()
        // (`WireDrawNodeActions()`/`SetNodeAction()`, removed); Play/Stop and
        // the bank buttons already carried their `Action` directly
        // (Task 6.3/6.4); and `node.selected` for the active bank is set via
        // `ControlStyle::selected` in AppendBankHeader() (`MarkSelectedBank()`,
        // removed).
        return builder.Build(root);
    }

    void SetActionHandler(ActionHandler handler) override {
        outerHandler_ = std::move(handler);
    }

    void DispatchAction(const synth::ui::Action& action) override {
        HandleAction(action);
        if (outerHandler_) {
            outerHandler_(action);
        }
    }

private:
    void AppendBankHeader(synth::ui::Builder& builder) const {
        // Bank labels are derived from FroggersBankLayouts() (single source
        // of truth for bank identity/order -- app/FroggersParameters.hpp)
        // rather than kept as a separate parallel array here, so adding or
        // reordering a bank requires editing only that one place (OMNI §8).
        //
        // Task 3.1 (operator 2026-07-28, design E3a) rendered these as
        // Draw nodes with hand-authored FillRoundedRect+Text commands so
        // selection could invert BOTH background and text colour (Sheaf's
        // `Node::selected` only inverts a Button's background via
        // `ButtonColourForNode`, PortableJuceBackend.hpp:1130-1149;
        // `TextColourForNode`, :1109-1127, has no `selected` branch at all
        // -- confirmed still absent at the pinned version). Task 6.3
        // (operator 2026-07-28) reverts that: at the pinned Sheaf version,
        // Draw/DrawInteractive nodes dispatch only on double-click
        // (RetainedDrawComponent, PortableJuceBackend.hpp:549-555, no
        // plain-click path), which cost the operator single-click bank
        // switching. Function over cosmetics -- back to plain `Button`
        // nodes with the action supplied directly, accepting BACKGROUND-ONLY
        // selection inversion. F.2c (2026-08-03, Sheaf pin 77a3019e):
        // `ControlStyle::selected` now exists, so `node.selected` is set at
        // build-once time via the style argument instead of a post-Build()
        // patch (`MarkSelectedBank()`, removed). Still no marker character
        // appended to the label either way.
        const auto& layouts = FroggersBankLayouts();
        for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
            synth::ui::ControlStyle bankStyle{};
            bankStyle.selected = BankSelected(bankIx);
            builder.Button(FroggersNodeIds::BankButton(bankIx), layouts[bankIx].name,
                           synth::ui::Action::WithValue(FroggersActions::kBankSelect, std::to_string(bankIx)),
                           bankStyle);
        }
        // Task 10.7 (design D11/D14): Randomize Page restores the
        // per-page/bank-header position desktop-v2 removed
        // (desktop-v2/Source/ui/SubmodulePagePanel.cpp:11-13). Exactly one
        // randomize control lives here; Randomize All never does (task
        // 10.2's own explicit constraint).
        builder.Button(FroggersNodeIds::kRandomizePage, "Randomize Page",
                       synth::ui::Action::Named(FroggersActions::kRandomizePage), synth::ui::ControlStyle{});
    }

    void AppendEncoderGrid(synth::ui::Builder& builder, synth::ui::Bounds gridArea) const {
        const bool showingModulationView =
            context_ != nullptr && context_->uiState != nullptr && context_->uiState->slotCapacity > 0 &&
            context_->uiState->slots[0].showingModulationView.load(std::memory_order_relaxed);

        for (std::size_t ix = 0; ix < FroggersEncoderGridLayout::kEncoderCount; ++ix) {
            synth::ui::EncoderDrawState state{};
            synth::ui::Visualizer* visualizer = nullptr;
            if (context_ != nullptr && context_->uiState != nullptr && context_->uiState->slotCapacity > 0) {
                const synth::BankSlot::UIState& slotState = context_->uiState->slots[0];
                if (ix < slotState.cellCapacity) {
                    // Design D9a/task 10.5: EncoderDrawStateFromParameter
                    // reads only `Parameter::UIState.values[]` (the
                    // post-fuego, post-modulation published display
                    // center, see EncoderDraw.hpp:334) -- never
                    // `.rawKnobValue`. This is the ONLY place this whole
                    // surface reads a `Parameter::UIState`, so it is also
                    // the one place to verify that guarantee.
                    state = synth::ui::EncoderDrawStateFromParameter(slotState.cells[ix]);
                    visualizer = slotState.cells[ix].visualizer.load(std::memory_order_relaxed);
                }
            }
            if (showingModulationView && !state.connected) {
                // Matches apps/braid-4/Braid4UI.hpp:65-68: hide disconnected
                // depth cells entirely once drilled in (design D9b's "cell
                // still pushed with parameter=nullptr" renders as this
                // default, disconnected EncoderDrawState).
                continue;
            }
            state.hasVisualizerUnderlay = visualizer != nullptr && visualizer->Visible();

            const synth::ui::Bounds cellBounds = FroggersEncoderGridLayout::BoundsForIndex(gridArea, ix);
            const std::string encoderId = FroggersNodeIds::Encoder(ix);
            if (visualizer != nullptr && visualizer->Visible()) {
                // Design D9b/D10: bump/comb transfer-function underlays and
                // modulation-source underlays render here automatically,
                // with no visualizer-specific code above this generic
                // "does this cell have a visualizer" branch.
                visualizer->SetBounds(cellBounds);
                builder.Visualizer(encoderId + ".visualizer", visualizer, synth::ui::ControlStyle{});
            }
            // Change 2 REVERTED (operator 2026-07-27): encoder press used to
            // be a DOUBLE click, because plain-click dispatch for Draw nodes
            // did not exist in upstream Sheaf at pin 1940ddcb (a fork would
            // have made the gitlink unresolvable from any other checkout).
            // F.2a (2026-08-03, Sheaf pin 77a3019e, ask 1 landed): `Draw`
            // nodes now dispatch plain click from `ControlStyle::action`, so
            // the drill-in press moves there and the drag stays on the
            // separate `ControlStyle::pointerDragAction` field -- no
            // conflict, no post-Build() patch needed
            // (`WireDrawNodeActions()`/`SetNodeAction()`, removed).
            synth::ui::ControlStyle encoderStyle{};
            encoderStyle.action = synth::ui::Action::WithValue(FroggersActions::kEncoderPress, std::to_string(ix));
            encoderStyle.pointerDragAction =
                synth::ui::Action::WithValue(FroggersActions::kEncoderDrag, FormatFroggersEncoderDrag(ix, 0.0f));
            builder.Draw(encoderId, cellBounds, synth::ui::BuildEncoderDrawCommands(state, cellBounds),
                        encoderStyle);
        }
    }

    void AppendChromeBand(synth::ui::Builder& builder) const {
        // Crunchy chrome-band slider REMOVED (operator 2026-07-27: "why is
        // there a fucking slider for crunchy between the randomize
        // buttons, i never asked for that. It duplicates bank slot 15.").
        // Crunchy is reachable only via the encoder grid's slot 15 now,
        // exactly like any other bank parameter -- see this file's header
        // comment and design.md D11/Resolved-decisions for the recorded
        // trade-off (Crunchy is unreachable while a modulation view is
        // open, since slot 15 is then Target/Back).

        // Task 10.2/10.7: Randomize All -- the ONLY randomize control in the
        // chrome band (Randomize Page lives in the bank header, above).
        builder.Button(FroggersNodeIds::kRandomizeAll, "Randomize All",
                       synth::ui::Action::Named(FroggersActions::kRandomizeAll), synth::ui::ControlStyle{});

        // Task 3.5 (operator 2026-07-28, design E3d): "Scene 1"/"Scene 2" --
        // relabelled from the old "S1"/"S2" -- are now a TOGGLE between the
        // scene-blend extremes, not a re-assignment of which stored scene
        // occupies the less-weighted endpoint. Traced: the old behaviour
        // dispatched `kSceneSelect` -> `ParameterManager::
        // SetLessSelectedScene` (External/Sheaf/projects/synth/src/
        // ParameterModulation.cpp:3336-3341), which reassigns
        // scene_.leftScene/rightScene and never moves scene_.blend -- at
        // either blend extreme, clicking did nothing audible (the
        // operator's "inconsistent" report). This DELIBERATELY diverges
        // from Braid 4's own convention (apps/braid-4/Braid4UiModel.hpp:
        // 402-404 dispatches the identical `kSceneSelect`/
        // `SetLessSelectedScene` pair) -- see FroggersParameters.hpp's
        // kNumScenes comment for the matching note. Handled below in
        // HandleAction(), which now pushes `MessageIn::SetSceneBlend`
        // straight over `context_->uiBus` -- the same push pattern this
        // file already uses for kSceneBlend/kPlay/kStop.
        for (std::size_t sceneIx = 0; sceneIx < 2; ++sceneIx) {
            builder.Button(FroggersNodeIds::SceneButton(sceneIx), "Scene " + std::to_string(sceneIx + 1),
                           synth::ui::Action::WithValue(FroggersActions::kSceneSelect, std::to_string(sceneIx)),
                           synth::ui::ControlStyle{});
        }
        const float sceneBlend =
            context_ != nullptr && context_->uiState != nullptr
                ? context_->uiState->sceneBlend.load(std::memory_order_relaxed)
                : 0.0f;
        // Task 3.5: relabelled "Scene Blend" -> "Scene blend" (design E3d).
        // The spec also asks that no raw floating-point blend value be
        // shown; note this file authors no such readout of its own (no
        // StatusText/Label anywhere formats `sceneBlend` as text) -- the
        // only numeric display of a Slider's value is JUCE's own built-in
        // text box (`PortableJuceBackend.hpp:1228`,
        // `setTextBoxStyle(juce::Slider::TextBoxBelow, ...)`), which is
        // unconditional for EVERY Slider node in EVERY Sheaf app (Braid 4's
        // own scene-blend slider, apps/braid-4/Braid4UiModel.hpp:406-412,
        // has the identical box) and cannot be suppressed per-node from app
        // code -- Sheaf exposes no field for it and this app must not
        // modify External/Sheaf. Reported, not fixed.
        // Task (2026-07-28 label-visibility fix): `NodeKind::Slider` routes
        // `node.label` to `juce::Slider::setName()` only (PortableJuceBackend.hpp:
        // 1229-1232) -- no `juce::Label` is attached, so the slider's own
        // label argument never draws. That used to be worked around with a
        // hand-rolled adjacent `Label` node (`FroggersNodeIds::kSceneBlendLabel`)
        // built immediately before the Slider so it landed in the same
        // flowed row. F.2d (2026-08-03, Sheaf pin 77a3019e): slider captions
        // now draw upstream via `ControlStyle::caption`
        // (PortableUIBuilders.hpp:20-33/424-465 -- emitted as a sibling
        // `Label` "<controlId>.caption" wrapped with the control in an
        // implicit Row, always BEFORE the control), so the hand-rolled node
        // id/Label call are gone. The Slider's own label argument is kept --
        // redundant for display, but it still feeds `juce::Slider::
        // setName()`, the accessible name JUCE/screen readers see.
        synth::ui::ControlStyle sceneBlendStyle{};
        sceneBlendStyle.caption = "Scene blend";
        builder.Slider(FroggersNodeIds::kSceneBlend, "Scene blend", sceneBlend, 0.0f, 1.0f, 0.001f,
                       synth::ui::Action::Named(FroggersActions::kSceneBlend), sceneBlendStyle);

        // Task 10.6 (design cited MasterClock.hpp:318/:321,
        // MasterClock.cpp:963-965/:1182): the BPM slider sits beside the
        // scene slider (just added above) and goes read-only/inert while
        // slaved to external MIDI clock -- rendered as a non-interactive
        // StatusText in that state (Builder has no "disabled slider" -- see
        // this file's header comment on Node kinds), an interactive Slider
        // otherwise. Both states display TempoBpm() (normal: the manually
        // set tempo; slaved: the recovered external tempo).
        const double tempoBpm = app_ != nullptr ? app_->DisplayTempoBpm() : synth::MasterClock::kDefaultTempoBpm;
        const bool externallyClocked = app_ != nullptr && app_->TempoExternallyClocked();
        if (externallyClocked) {
            builder.StatusText(FroggersNodeIds::kBpm, "BPM " + FormatFroggersBpm(tempoBpm) + " (external clock)",
                               synth::ui::ControlStyle{});
        } else {
            // Task 3.6 (design E3e): the label conflict is settled -- the
            // control genuinely IS labelled "BPM" (this line, verified by
            // running the app; see FroggersSurfaceTests.cpp for the
            // regression test). BPM is correctly wired (RequestTempoBpm ->
            // MasterClock::SetTempoBpm -> pendingQuarterNotesPerSample_ ->
            // TransportQuarterNotesAt -> gates audioAdsr_) and genuinely
            // drives the D17 ASR gate rate -- no wiring change here.
            //
            // UI-rework ITEM 5 (design.md A3f, tasks.md B.5, 2026-07-29):
            // this used to switch to "BPM (no effect while stopped)" while
            // the transport was stopped. That annotation was never
            // requested -- an agent invented it "to improve discoverability"
            // (design.md's process note) -- and, because chrome is
            // auto-flowed by control width (FroggersAutoFlowedChromeModel
            // above), a longer label re-flowed every neighbouring control
            // each time the transport started or stopped. Operator:
            // "a really stupid feature I never asked for, and it changes the
            // alignment of nearby labels." Reverted to a constant "BPM";
            // the state-dependent branch is gone, not merely disabled --
            // per the standing rule (design.md A3f, tasks.md §0) not to add
            // user-visible behaviour the operator did not request, this is
            // not to be reintroduced without asking first.
            constexpr const char* kBpmLabel = "BPM";
            // NOT converted to `ControlStyle::caption`, unlike the
            // scene-blend slider just above. That conversion was F.2d's whole
            // point, and it applies wherever the hand-rolled adjacent `Label`
            // existed ONLY because upstream never drew slider captions -- a
            // cause that is dead at pin 77a3019e. Here a second cause is
            // still live: B12 (tasks.md, 2026-07-29) requires this label to
            // TRAIL its slider, since leading it puts it between the two
            // sliders and reads as labelling the scene-blend one. The
            // operator's words were "the two labels are now deliberately
            // asymmetric -- do not 'fix' that."
            //
            // `ControlStyle::caption` cannot express that: `FinishControl`
            // (PortableUIBuilders.hpp:428-465) always emits the caption Label
            // BEFORE its control, wrapped with it in an implicit Row. So the
            // adjacent `Label` node stays here and the asymmetry is
            // preserved. Filed as upstream ask 14; when caption placement
            // lands this collapses to a caption like its neighbour.
            builder.Slider(FroggersNodeIds::kBpm, kBpmLabel, static_cast<float>(tempoBpm), 30.0f, 300.0f, 1.0f,
                           synth::ui::Action::Named(FroggersActions::kBpm), synth::ui::ControlStyle{});
            builder.Label(FroggersNodeIds::kBpmLabel, kBpmLabel, synth::ui::ControlStyle{});
        }
    }

    bool BankSelected(std::size_t bankIx) const {
        if (context_ == nullptr || context_->uiState == nullptr || bankIx >= context_->uiState->bankCapacity) {
            return bankIx == 0;
        }
        return context_->uiState->banks[bankIx].selected.load(std::memory_order_relaxed);
    }

    void HandleAction(const synth::ui::Action& action) {
        if (app_ == nullptr) {
            return;
        }

        // Generic, safe over the existing uiBus (see this file's header
        // comment): transport, scene select/blend, encoder drag.
        if (action.name == FroggersActions::kPlay) {
            PushMessage(synth::MessageIn::Start(NowMicros()));
            // D17 robustness fix (see FroggersAppCore::PrepareToPlay's own
            // comment): record the intent behind this push so a later
            // audio-device renegotiation (which resets
            // MasterClock::TransportState() to Stopped out from under the
            // app, with no message of its own) can re-assert Start rather
            // than leave the app silently and permanently silent.
            app_->SetDesiredTransportRunning(true);
            return;
        }
        if (action.name == FroggersActions::kStop) {
            PushMessage(synth::MessageIn::Stop(NowMicros()));
            app_->SetDesiredTransportRunning(false);
            return;
        }
        if (action.name == FroggersActions::kSceneSelect) {
            // Task 3.5 (design E3d): Scene 1/Scene 2 now toggle the blend to
            // its extremes rather than reassigning a stored-scene endpoint
            // (see AppendChromeBand's comment for the full trace). The
            // mapping below is verified, not assumed: FroggersParameters.hpp
            // wires `manager.SetSceneEndpoints(0, 1)` once at Init() (fixed
            // for this app's lifetime -- nothing here or in ParameterManager
            // ever calls SetSceneEndpoints again after this change removes
            // SetLessSelectedScene's own call to it), and
            // ParameterGroup::ApplySceneDistribution's blend arithmetic
            // (External/Sheaf/projects/synth/src/ParameterModulation.cpp:2172)
            // is `SceneCenter(leftScene) * (1-blend) + SceneCenter(rightScene)
            // * blend` -- blend 0.0 is pure leftScene (scene index 0), blend
            // 1.0 is pure rightScene (scene index 1). So scene index 0
            // ("Scene 1") -> blend 0.0, scene index 1 ("Scene 2") -> blend
            // 1.0, matching the button's own ordinal exactly.
            const std::size_t sceneIx = FroggersParseSize(action.value, 0);
            const float blend = sceneIx == 0 ? 0.0f : 1.0f;
            PushMessage(synth::MessageIn::SetSceneBlend(NowMicros(), blend));
            return;
        }
        if (action.name == FroggersActions::kSceneBlend) {
            PushMessage(synth::MessageIn::SetSceneBlend(NowMicros(), FroggersParseFloat(action.value, 0.0f)));
            return;
        }
        if (action.name == FroggersActions::kEncoderDrag) {
            std::size_t position = 0;
            float delta = 0.0f;
            if (!ParseFroggersEncoderDrag(action.value, position, delta) || std::fabs(delta) < 0.0001f) {
                return;
            }
            PushMessage(synth::MessageIn::ParamIncDec(NowMicros(), /*slotIx=*/0, position, delta));
            return;
        }

        // App-request bridge (see FroggersAppCore.hpp's header comment):
        // encoder press (drill-in cap), Randomize All/Page, BPM. (Crunchy
        // removed operator 2026-07-27 -- see AppendChromeBand's comment.)
        if (action.name == FroggersActions::kEncoderPress) {
            app_->RequestEncoderPress(FroggersParseSize(action.value, 0));
            return;
        }
        if (action.name == FroggersActions::kBankSelect) {
            app_->RequestBankSelect(FroggersParseSize(action.value, 0));
            return;
        }
        if (action.name == FroggersActions::kRandomizeAll) {
            app_->RequestRandomizeAll();
            return;
        }
        if (action.name == FroggersActions::kRandomizePage) {
            app_->RequestRandomizePage();
            return;
        }
        if (action.name == FroggersActions::kBpm) {
            // Task 10.6: belt-and-suspenders -- the slider itself renders as
            // a non-interactive StatusText while slaved (BuildTree(),
            // above), so this action should not even be reachable then; the
            // guard here means the audio-thread's own no-op
            // (MasterClock::SetTempoBpm's `syncConfig_.receiveClock` check)
            // is not the ONLY thing preventing a stray request from an
            // out-of-date rendered tree.
            if (!app_->TempoExternallyClocked()) {
                app_->RequestTempoBpm(static_cast<double>(FroggersParseFloat(action.value, 120.0f)));
            }
            return;
        }
    }

    void PushMessage(const synth::MessageIn& message) {
        if (context_ != nullptr && context_->uiBus != nullptr) {
            context_->uiBus->Push(message);
        }
    }

    std::uint64_t NowMicros() const {
        if (context_ != nullptr && context_->now) {
            return context_->now();
        }
        return fallbackTimestamp_++;
    }

    synth::AppContext* context_ = nullptr;
    FroggersAppCore* app_ = nullptr;
    ActionHandler outerHandler_;
    mutable std::uint64_t fallbackTimestamp_ = 1;
};

}  // namespace synth_froggers

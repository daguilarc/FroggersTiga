#pragma once

// synth_froggers::FroggersUiSurface -- the app's surface layout
// (spec `specs/froggers-app-surface-layout/spec.md`), built on Sheaf's
// portable layout engine (`PortableUILayout.hpp`/`PortableUIMetrics.hpp`)
// rather than hand-rolled pixel arithmetic.
//
// Topology: sheaf supplies the class idioms but this app owns its own
// topology. This surface is declared as ONE grid -- encoders and chrome
// alike are grid citizens with cell positions, not a scope/grid region plus
// a separately auto-flowed chrome band. `FroggersCellMap` below is the
// topology, expressed as data (a 6-row-by-6-column table); the mechanism --
// how a cell becomes real geometry -- is the Sheaf idiom
// Braid 4's own app uses for its grids (`apps/braid-4/Braid4UiModel.hpp`'s
// `EmitBraid4CellGrid`/`Braid4CellLayout`): a `Column` of `Row`s, weighted
// `Extent::Weight(n)` cells (n>1 expresses a span), and Draw nodes whose
// commands are built from the RESOLVED bounds a `DrawFactory` receives, not
// from pixel math this file computes ahead of time. `EmitBraid4CellGrid`
// itself lives in an app-specific Sheaf header this app must not include
// (cross-app coupling into another app's file) -- this file writes its own
// equivalent, same convention `FroggersParseSize`/`FroggersParseFloat` below
// already follow for Braid4UiModel.hpp's parse-helper pattern.
//
// `StandardAppLayout` (`PortableUIStandardLayout.hpp`) is NOT used: it is
// Braid4's OWN topology (an empty second-visualizer slot does not collapse,
// `PortableUIStandardLayout.hpp:89-99`), not a neutral scaffold, and this
// app's topology is its own, not Braid4's.
//
// Window size: the surface still resolves against
// `context->config->uiWidth/uiHeight` (a fixed, compiled-in size) rather
// than a live window extent -- making the layout track the ACTUAL window
// requires an upstream shell change (`RuntimeMainComponent::BuildTree()`
// composes the sidebar from `App::Config().uiWidth`, not a live extent, so a
// resizable surface here would desync from it). Everything internal to this
// surface is nonetheless fully declarative, so adopting a live extent later
// is a change to `FroggersPageLayout::RootBounds()`'s source, not a
// redesign.
//
// Bounds note (still true): `synth::ui::Builder`'s Button/Slider/Toggle/
// ComboBox/TextField/StatusText node kinds take no explicit `Bounds` --
// placement comes entirely from each node's own `LayoutOptions` (`main`/
// `cross` extents), resolved by the engine against whatever region its
// container was given. Every region this surface used to compute a pixel
// `Bounds` for by hand -- the scope panel, the encoder grid, Play/Stop's
// plates -- is now an in-flow cell with a declared `LayoutOptions` instead;
// the sole exception is the transport plates' own fixed `Extent::Px(28)`
// size (see `kTransportPlateSize`), which was already a
// `LayoutOptions`-expressed size, not an `explicitBounds` out-of-flow
// declaration.
//
// Crunchy has no dedicated chrome slider: it duplicates bank slot 15, so it
// is reachable only via the encoder grid's slot 15, addressed exactly like
// any other bank parameter (Crunchy is unreachable while a modulation view
// is open, since slot 15 is then Target/Back). Crunchy (slot 15) is GLOBAL
// -- one shared `Parameter` aliased into all six banks
// (`FroggersParameters.hpp:323-328, 428-437`) carrying its own
// fixed Yellow rather than the bank colour, and excluded from drill-in/
// randomize dispatch (`FroggersModulation.hpp:113-119`). That colour already
// flows through `Parameter::UIState.color` into
// `EncoderDrawStateFromParameter` with no special-casing needed here -- this
// file's one encoder-cell code path renders slot 14 (Crispy, per-bank
// colour) and slot 15 (Crunchy, fixed Yellow) identically; the colour
// difference is data, not branching.
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
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace synth_froggers {

namespace FroggersNodeIds {

inline constexpr const char* kRoot = "froggers.root";
// The single outer split Row (left block | right block, per FroggersCellMap
// below) and the two blocks themselves.
inline constexpr const char* kLayoutRoot = "froggers.layout.root";
inline constexpr const char* kLeftBlock = "froggers.layout.left";
inline constexpr const char* kRightBlock = "froggers.layout.right";

inline constexpr const char* kPlay = "froggers.transport.play";
inline constexpr const char* kStop = "froggers.transport.stop";
// The Freeze transport BUTTON's own node id -- distinct from the Freeze
// ENCODER (Delay bank slot 4, FroggersParameters.hpp), which has no id here
// because encoder cells are addressed by grid index
// (FroggersNodeIds::Encoder), not by name.
inline constexpr const char* kFreeze = "froggers.transport.freeze";
// The Record transport BUTTON's own node id -- fourth child of the
// transport row, beside Play/Stop/Freeze.
inline constexpr const char* kRecord = "froggers.transport.record";
// The "FREEZE" text label that sits beside kFreeze -- emitted ONLY when this
// surface is attached in plugin-host mode
// (FroggersUiSurface::SetPluginHostMode(true), see that method's own
// comment); never emitted in the default/standalone/browser mode this file
// otherwise renders. Naming convention: sibling label id under the SAME
// dotted namespace as the control it labels
// (froggers.transport.freeze -> froggers.transport.freeze.label), mirroring
// kBpmLabel's own "<control>.label" suffix off kBpm above.
inline constexpr const char* kFreezeLabel = "froggers.transport.freeze.label";
// Row 3 of the left block (FroggersCellMap): Play | Stop | Freeze | Record.
inline constexpr const char* kTransportRow = "froggers.layout.left.transport";
// Row 4 of the left block: Scene 1 | Scene 2.
inline constexpr const char* kScenesRow = "froggers.layout.left.scenes";

inline constexpr const char* kRandomizeAll = "froggers.randomize.all";
inline constexpr const char* kRandomizePage = "froggers.randomize.page";
inline constexpr const char* kResetAll = "froggers.reset.all";
inline constexpr const char* kResetPage = "froggers.reset.page";
// Row 6 of the right block: Randomize Page | Randomize All (moved out of the
// bank-header group by the CELL MAP -- see AppendRandomizeRow()'s comment).
inline constexpr const char* kRandomizeRow = "froggers.layout.right.randomize";
// T5.1: Row 7, directly below Randomize, same two-halves weighting.
inline constexpr const char* kResetRow = "froggers.layout.right.reset";
// Row 1 of the right block: the six bank-select tabs.
inline constexpr const char* kBankTabsRow = "froggers.layout.right.banks";
// STEP 1 (operator, 2026-08-09, fourth session on the same complaint): a
// dedicated header ROW, inserted between the bank tabs row and the first
// encoder row -- see AppendModulationHeaderRow()'s own comment for the full
// placement investigation and FroggersCellMap::RightKind::Header for its
// place in the topology table.
inline constexpr const char* kModulationHeader = "froggers.layout.right.header";
// Children of kModulationHeader at level 0 (the arrow pair) and level > 0
// (the title, now a distinct child rather than the row's own leaf content --
// see AppendModulationHeaderRow()'s own comment for the full child-structure
// switch).
inline constexpr const char* kBankPrevArrow = "froggers.bank.prev";
inline constexpr const char* kBankNextArrow = "froggers.bank.next";
inline constexpr const char* kModulationHeaderTitle = "froggers.layout.right.header.title";

inline constexpr const char* kSceneBlend = "froggers.scene.blend";
// Row 5 of the left block: the Scene-blend slider with its label BELOW it
// (the CELL MAP's one amendment to the operator-approved table, superseding
// F.2d's `ControlStyle::caption` conversion for this control -- see
// AppendSceneBlendGroup()'s own comment).
inline constexpr const char* kSceneBlendGroup = "froggers.scene.blend.group";
inline constexpr const char* kSceneBlendLabel = "froggers.scene.blend.label";

inline constexpr const char* kBpm = "froggers.bpm";
// Row 6 of the left block: the BPM slider with its label BELOW it, the same
// shape as row 5 -- task F.6 (2026-08-05) superseded B12's trailing label
// once scene-blend's moved below removed the ambiguity B12 existed to
// prevent. See AppendLabelledSlider()'s own comment for the full reasoning.
inline constexpr const char* kBpmGroup = "froggers.bpm.group";
// A hand-rolled Label node rather than `ControlStyle::caption`, exactly like
// scene-blend's (kSceneBlendLabel above). Both sit BELOW their slider, and
// `Builder::FinishControl` (PortableUIBuilders.hpp:428-465) always emits a
// caption BEFORE its control with no option to place it after -- so neither
// can be a caption while the labels sit below. Tracked upstream as ask 14
// (caption placement); when that lands, BOTH collapse into captions
// together.
//
// History worth keeping, because this comment previously said the opposite:
// F.2d converted scene-blend to a caption and left this one hand-rolled,
// citing B12's trailing-label instruction as a second live cause. F.3's CELL
// MAP then moved scene-blend's label below (retiring its caption and
// restoring its hand-rolled node), and F.6 moved this one below to match --
// which retired B12, since B12's stated reason was that a LEADING label read
// as labelling the wrong control, and a label beneath its own control cannot.
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

// One Row per 4-wide slice of the 16-slot encoder grid (rows 2-5 of the
// right block, FroggersCellMap): row 0 = slots 0-3, row 1 = slots 4-7,
// etc.
inline std::string EncoderRow(std::size_t row) {
    return "froggers.layout.right.row." + std::to_string(row);
}

}  // namespace FroggersNodeIds

namespace FroggersActions {

inline constexpr const char* kPlay = "froggers.transport.play";
inline constexpr const char* kStop = "froggers.transport.stop";
inline constexpr const char* kFreeze = "froggers.transport.freeze";
inline constexpr const char* kRecord = "froggers.transport.record";
inline constexpr const char* kRandomizeAll = "froggers.randomize.all";
inline constexpr const char* kRandomizePage = "froggers.randomize.page";
inline constexpr const char* kResetAll = "froggers.reset.all";
inline constexpr const char* kResetPage = "froggers.reset.page";
inline constexpr const char* kBankSelect = "froggers.bank.select";
// The secondary arrow-pair navigation beside direct bank selection above --
// routed through the same single selection authority (HandleAction, not this
// file's own concern here).
inline constexpr const char* kBankPrevious = "froggers.bank.previous";
inline constexpr const char* kBankNext = "froggers.bank.next";
inline constexpr const char* kSceneSelect = "froggers.scene.select";
inline constexpr const char* kSceneBlend = "froggers.scene.blend";
inline constexpr const char* kBpm = "froggers.bpm";
inline constexpr const char* kEncoderPress = "froggers.encoder.press";
inline constexpr const char* kEncoderDrag = "froggers.encoder.drag";

}  // namespace FroggersActions

// The surface's own extent and design tokens. Everything that used to
// compute a pixel `Bounds` for the scope/grid regions by hand
// (`ContentArea`/`RequiredHeight`/`ScopeArea`/`GridArea`/
// `FroggersAutoFlowedChromeModel`, all now removed) is gone -- that
// arithmetic DIES, but the tokens and the historical ratio it enforced
// SURVIVE AS DATA below, either still consumed (kMargin/kGap, as the outer
// grid's own padding/gap) or preserved as the documented baseline
// FroggersSurfaceTests.cpp's ratio guard checks the RESOLVED layout against
// (kScopeWidth/kScopeHeight -- nothing here computes pixels from them any
// more, but they remain this file's one definition of "the original scope
// proportions" rather than a duplicate literal in the test).
struct FroggersPageLayout {
    static constexpr float kDefaultWidth = 900.0f;
    // Replaces the old `RequiredHeight()`-derived fallback: a plain literal,
    // matching `FroggersAppCore::Config().uiHeight` (see that file's own
    // comment -- demoted from a derived cross-check to an initial window
    // size, task F.3's "config.uiHeight is NOT deleted" precision).
    //
    // So the encoder ring does not shrink, and the missing space is added
    // below each encoder rather than taken from it: 632.0f -> 712.0f, +80px,
    // exactly `FroggersEncoderGridLayout::kLabelBandHeight` (20px) times the
    // 4 encoder rows -- see that struct's own comment for the
    // exact-by-construction row-height arithmetic this pays for (bank
    // tabs/randomize/reset stay pixel-identical; each encoder row alone
    // grows by kLabelBandHeight). `FroggersAppCore::Config()`'s own
    // `config.uiHeight` is the SAME number, hand-synced (see that file's
    // own comment on why it cannot be derived FROM this one) -- kept in
    // sync here, not verified by any cross-check test (task F.3's own
    // finding: such a test can pass even when both sides are already
    // wrong, see this file's own report for a fresh instance of exactly
    // that failure mode in the row-height comment this change corrects).
    static constexpr float kDefaultHeight = 712.0f;

    // The outer split Row's own padding (inset from the window edge) and the
    // gap between the left/right blocks and between each block's own stacked
    // rows.
    static constexpr float kMargin = 16.0f;
    static constexpr float kGap = 14.0f;

    // The scope's proportions come from a specific historical complaint:
    // "it is taller than it is wide... it should be at most a third of its
    // current size." The
    // scope's cell is now weight-resolved against whatever window the
    // surface builds against, not sized from these pixels directly -- but
    // FroggersSurfaceTests.cpp's ratio guard still checks the resolved cell
    // against this exact historical baseline (340 wide portrait column x the
    // old full content height), so these stay this file's one definition
    // site for that baseline rather than a second copy in the test.
    static constexpr float kScopeWidth = 340.0f;
    static constexpr float kScopeHeight = 64.0f;

    // The ONE declared width for every labelled slider in the left block
    // (task F.6, operator 2026-08-05). Before F.6 neither slider declared a
    // width at all: Scene blend emitted a Column (horizontal is the CROSS
    // axis, so its slider filled the block) while BPM emitted a Row
    // (horizontal is the MAIN axis, so its slider split the width with its
    // label). The operator's report -- "the scene slider is too wide and the
    // bpm slider is too narrow. grid design fail" -- was two symptoms of that
    // one cause: width was a side effect of label placement rather than a
    // declared property.
    //
    // Deliberately a FRACTION, not a pixel count. `Extent::Fraction(f)`
    // resolves as `contentExtent * f` (ResolveCrossExtent,
    // PortableUILayout.hpp:318-347, Fraction case :333-335, with ClampExtent
    // applying Min/Max at :346), so this tracks the left block's real
    // resolved width and keeps working if that ever changes. A pixel width
    // would have to be re-tuned for each.
    //
    // Sliders are equal to each other BY CONSTRUCTION because this is the
    // single definition site, read once in AppendLabelledSlider(). Do not add
    // a second per-control width: two values kept in agreement by hand is the
    // defect F.3 deleted when it removed `uiHeight == RequiredHeight()`.
    static constexpr float kSliderWidthFraction = 0.8f;

    static synth::ui::Bounds RootBounds(const synth::AppContext* context) {
        const float width = context != nullptr && context->config != nullptr
                                 ? static_cast<float>(context->config->uiWidth)
                                 : kDefaultWidth;
        const float height = context != nullptr && context->config != nullptr
                                  ? static_cast<float>(context->config->uiHeight)
                                  : kDefaultHeight;
        return {0.0f, 0.0f, width, height};
    }
};

// The 16-slot grid topology, slots 0-15 laid out 4x4 -- `kColumns`/`kRows`/
// `kEncoderCount` are the slot topology (static_assert-tied to
// `kFroggersSlotsPerBank`); `BoundsForIndex`'s old pixel division is gone
// (cells are now in-flow grid cells the layout engine sizes, see
// AppendEncoderRow() below), but the row/column mapping it embodied (`ix /
// kColumns`, `ix % kColumns`) survives as the loop shape AppendEncoderGrid()
// below walks.
struct FroggersEncoderGridLayout {
    static constexpr std::size_t kColumns = 4;
    static constexpr std::size_t kRows = 4;
    static constexpr std::size_t kEncoderCount = kColumns * kRows;
    // The gap between encoder cells within a row and between encoder rows --
    // its own distinct structural role vs. `FroggersPageLayout::kGap`, which
    // separates the left/right blocks and each block's own top-level rows.
    static constexpr float kGap = 8.0f;

    // The measured value (`BuildFroggersTreeAtDefaultSize()`'s resolved
    // Encoder(0) bounds, cross-checked against `AllocateExtents`' documented
    // formula -- kRightBlock content height 600px at the pre-existing
    // 900x632 window, minus the modulation header's fixed 26px and 7
    // inter-row gaps of 14 (98), leaves 476, split 7 ways across BankTabs +
    // the 4 EncoderRows + Randomize + Reset, all Weight(1.0) at the time of
    // measurement: 476/7 = 68.0 exactly).
    //
    // VERIFIED (not eyeballed): measured directly against a built tree and
    // matches the analytic formula above bit-for-bit, so the ring math below
    // is built on a confirmed figure rather than an assumed one.
    static constexpr float kUnchangedRowHeight = 68.0f;

    // The label band this task adds strictly BELOW the ring (operator:
    // "space you should have ADDED below each encoder"), ~20px per the
    // task brief.
    static constexpr float kLabelBandHeight = 20.0f;

    // Each encoder row's own resolved height after this task: the
    // ring-bearing portion (unchanged) plus the new label band. Deliberately
    // the SUM of the two constants above, never a separately hand-typed
    // literal -- AppendEncoderRow passes this as an `Extent::Weight` value
    // (not a later multiply) specifically so `AllocateExtents`
    // (PortableUILayout.hpp:165-241)'s `remaining * weight / totalWeight`
    // resolves EXACTLY (bit-for-bit, no float rounding) at the default
    // window: `kDefaultHeight` above is chosen so that `remaining` there
    // exactly equals the total weight (BankTabs/Randomize/Reset at
    // kUnchangedRowHeight=68 each + 4 EncoderRows at kGrownRowHeight=88
    // each = 556), which makes every `remaining * weight / totalWeight`
    // division reduce to the exact input weight (a property of IEEE754
    // correctly-rounded division over exactly-representable integer
    // operands, not an approximation) -- see AppendEncoderCell's own
    // comment for how this feeds the ring's byte-identical sub-extent.
    static constexpr float kGrownRowHeight = kUnchangedRowHeight + kLabelBandHeight;
};

static_assert(FroggersEncoderGridLayout::kEncoderCount == kFroggersSlotsPerBank,
              "the grid must render exactly the 16 physical encoder slots FroggersParameterModel wires up");

// The operator-approved topology, kept as PURE DATA -- no builder calls, no
// layout math -- separate from the emission code that interprets it
// (AppendLeftBlock()/AppendRightBlock() below). This is what a future mobile
// or VST topology would replace with a DIFFERENT table consumed by analogous
// emission code, without forking this surface: this stays the one definition
// site for "what goes where".
//
// The left and right columns are two INDEPENDENT stacked Columns (siblings
// under the outer split Row, AppendLeftBlock()/AppendRightBlock()), not one
// shared grid: each has its own row count and its own gap total, so a given
// row index does not land at the same y-coordinate in both (the left
// column's 5 rows split 6 weight-units across 4 gaps; the right column's,
// after STEP 1, splits 6 weight-units plus one fixed-height row across 6
// gaps). The two tables below are listed side by side for readability, not
// because their rows align pixel-for-pixel.
//
//   LEFT (L1-L2, 5 rows, weights sum to 6):
//   1 | Scope (weight 2, spans rows 1-2's worth of height)
//   2 | Play | Stop
//   3 | Scene 1 | Scene 2
//   4 | Scene blend (label below)
//   5 | BPM (label below)
//   Rows 4 and 5 are the same shape on purpose (task F.6): both labels
//   below, both sliders one declared width. The CELL MAP originally had row
//   5's label trailing, per B12; F.6 superseded that -- see
//   AppendLabelledSlider().
//
//   RIGHT (E1-E4, 7 rows -- STEP 1 added row 2, all others renumbered down
//   by one from the pre-STEP-1 table):
//   1 | Bank tabs x6 (span E1-E4)
//   2 | Modulation header (span E1-E4, fixed height, empty at drill level 0)
//   3 | slot 0 | slot 1 | slot 2 | slot 3
//   4 | slot 4 | slot 5 | slot 6 | slot 7
//   5 | slot 8 | slot 9 | slot 10 | slot 11
//   6 | slot 12 | slot 13 | slot 14 CRIS | slot 15 CRNC
//   7 | Randomize page (span 2) | Randomize all (span 2)
struct FroggersCellMap {
    enum class LeftKind { Scope, Transport, Scenes, SceneBlend, Bpm };
    enum class RightKind { BankTabs, Header, EncoderRow, Randomize, Reset };

    struct LeftRow {
        LeftKind kind;
        // Vertical share of the left column's 6 row-units (a span, exactly
        // like a horizontal `Extent::Weight` span within a row -- the Scope
        // row is 2 units tall, matching rows 1-2 of the table above).
        float rowWeight;
    };
    struct RightRow {
        RightKind kind;
        // Meaningful only for RightKind::EncoderRow: the first of the 4
        // consecutive encoder slot indices this row renders.
        std::size_t firstEncoderIndex;
    };

    static constexpr std::array<LeftRow, 5> kLeftRows = {{
        {LeftKind::Scope, 2.0f},
        {LeftKind::Transport, 1.0f},
        {LeftKind::Scenes, 1.0f},
        {LeftKind::SceneBlend, 1.0f},
        {LeftKind::Bpm, 1.0f},
    }};

    // T5.1: extent 7 -> 8 for the Reset row. This is a FIXED-extent
    // std::array, so appending a row means changing the count too, not just
    // adding an initializer.
    static constexpr std::array<RightRow, 8> kRightRows = {{
        {RightKind::BankTabs, 0},
        {RightKind::Header, 0},
        {RightKind::EncoderRow, 0},
        {RightKind::EncoderRow, 4},
        {RightKind::EncoderRow, 8},
        {RightKind::EncoderRow, 12},
        {RightKind::Randomize, 0},
        {RightKind::Reset, 0},
    }};

    // The outer split Row's weights (L1+L2 = 2 units, E1-E4 = 4 units,
    // matching the table's 6-column width exactly).
    static constexpr float kLeftBlockWeight = 2.0f;
    static constexpr float kRightBlockWeight = 4.0f;
};

// Change 3 (operator 2026-07-27): Play/Stop as coloured icons -- "Play =
// green triangle on white. Stop = red square on white." Built from exactly
// Sheaf's existing portable primitives (verified present and painted in
// both the JUCE and browser backends): `DrawCommand::FillRoundedRect` for
// the plate, `DrawCommand::FillPolygon` for the Play triangle,
// `DrawCommand::Fill(Bounds, Color)` for the Stop square. Commands are
// authored against the node-LOCAL (0,0,width,height) box (PortableUI.hpp's
// coordinate contract), which is unaffected by whether that box came from
// hand-computed pixel math or, as of task F.3, the layout engine resolving
// this node's `Extent::Px(kTransportPlateSize)` declaration -- the DrawFactory
// signature `vector<DrawCommand>(Bounds)` receives the SAME 28x28 box either
// way, so this inset-fraction arithmetic needed no change for F.3 and is
// unchanged from before it.
//
// The icon is inset to a fixed FRACTION of the plate rather than a fixed
// pixel amount so it scales with the square and lands at ~55-60% of the
// plate with even padding on all sides, and the plate uses a muted chrome
// colour (RGB 57/106/127) instead of stark white so it sits in the dark
// instrument face instead of glaring out of it.
inline constexpr synth::Color kTransportPlateColor = synth::Color::Rgb(57, 106, 127);
// Record's own glyph/armed-plate colour -- a dark red, distinct from Stop's
// plain synth::Color::Red square sharing this same row.
inline constexpr synth::Color kRecordColor = synth::Color::Rgb(139, 0, 0);
inline constexpr float kTransportIconFraction = 0.575f;  // ~55-60% of the plate
inline constexpr float kTransportPlateSize = 28.0f;      // matches the old Button height

// Six builders below (Play/Stop/Freeze/Record/BankPrevArrow/BankNextArrow)
// each opened with the identical rounded-rect plate plus inset-box
// arithmetic. Factored to one shared helper -- the plate `DrawCommand` and
// the inset `Bounds` every caller derives its own glyph geometry from.
// Byte-identical output is preserved: the inset `Bounds` computed here is
// LITERALLY the same expression BuildStopDrawCommands/BuildRecordDrawCommands
// already built inline (`square`/`circleBounds` below), and the four
// polygon-glyph builders derive left/top directly from `inset.x`/`inset.y`
// (again the same expression, `bounds.x + insetX` / `bounds.y + insetY`) --
// only right/bottom move from `bounds.x + bounds.width - insetX` to
// `inset.x + inset.width`, algebraically identical and, per this file's own
// test suite (FroggersSurfaceTests.cpp's `PointsClose`/`BoundsClose`, 0.01-0.02f
// tolerance, never bit-exact), not something any test distinguishes.
struct PlateAndInsetBox {
    synth::ui::DrawCommand plate;
    synth::ui::Bounds inset;
};

inline PlateAndInsetBox BuildPlateAndInsetBox(synth::ui::Bounds bounds, synth::Color plateColor) {
    constexpr float kCornerRadius = 4.0f;
    const float insetX = bounds.width * (1.0f - kTransportIconFraction) * 0.5f;
    const float insetY = bounds.height * (1.0f - kTransportIconFraction) * 0.5f;
    return PlateAndInsetBox{
        synth::ui::DrawCommand::FillRoundedRect(bounds, kCornerRadius, plateColor),
        synth::ui::Bounds{
            bounds.x + insetX,
            bounds.y + insetY,
            std::max(0.0f, bounds.width - insetX * 2.0f),
            std::max(0.0f, bounds.height - insetY * 2.0f),
        },
    };
}

inline std::vector<synth::ui::DrawCommand> BuildPlayDrawCommands(synth::ui::Bounds bounds) {
    std::vector<synth::ui::DrawCommand> commands;
    const PlateAndInsetBox plate = BuildPlateAndInsetBox(bounds, kTransportPlateColor);
    commands.push_back(plate.plate);
    const float left = plate.inset.x;
    const float right = plate.inset.x + plate.inset.width;
    const float top = plate.inset.y;
    const float bottom = plate.inset.y + plate.inset.height;
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
    std::vector<synth::ui::DrawCommand> commands;
    const PlateAndInsetBox plate = BuildPlateAndInsetBox(bounds, kTransportPlateColor);
    commands.push_back(plate.plate);
    commands.push_back(synth::ui::DrawCommand::Fill(plate.inset, synth::Color::Red));
    return commands;
}

// Freeze, third transport plate beside Play/Stop -- same plate-plus-glyph idiom as
// BuildPlayDrawCommands/BuildStopDrawCommands above (rounded-rect plate,
// inset glyph at kTransportIconFraction), but with a `latched` parameter the
// other two do not take. A diamond glyph, visually distinct from Play's
// triangle and Stop's square.
//
// WHY A Draw NODE AND NOT A Button: `StateColourFor` renders
// `ControlStyle::selected` as `brighter(0.14f)` on
// the background and `TextColourForNode` (PortableJuceBackend.hpp:1036-1042)
// branches on `enabled` only -- text colour never changes on selection, so a
// genuine colour INVERSION is not available from the library's own
// selected-state handling. A Draw node emits its own commands, so the
// inversion below is free and needs no upstream change.
//
// When `latched` is true the plate and glyph colours SWAP outright (a real
// exchange, not a brightness bump) -- this is the whole reason Freeze is a
// Draw node rather than a Button relying on `selected`.
inline std::vector<synth::ui::DrawCommand> BuildFreezeDrawCommands(synth::ui::Bounds bounds, bool latched) {
    const synth::Color plateColor = latched ? synth::Color::Cyan : kTransportPlateColor;
    const synth::Color glyphColor = latched ? kTransportPlateColor : synth::Color::Cyan;
    std::vector<synth::ui::DrawCommand> commands;
    const PlateAndInsetBox plate = BuildPlateAndInsetBox(bounds, plateColor);
    commands.push_back(plate.plate);
    const float left = plate.inset.x;
    const float right = plate.inset.x + plate.inset.width;
    const float top = plate.inset.y;
    const float bottom = plate.inset.y + plate.inset.height;
    const float midX = (left + right) * 0.5f;
    const float midY = (top + bottom) * 0.5f;
    commands.push_back(synth::ui::DrawCommand::FillPolygon(
        {
            synth::ui::Point{midX, top},
            synth::ui::Point{right, midY},
            synth::ui::Point{midX, bottom},
            synth::ui::Point{left, midY},
        },
        glyphColor));
    return commands;
}

// Record, fourth transport plate beside Play/Stop/Freeze -- same
// plate-plus-glyph idiom as BuildFreezeDrawCommands
// above (rounded-rect plate, inset glyph at kTransportIconFraction, a
// genuine colour EXCHANGE while armed, not a brightness tweak -- see that
// function's own comment for why a Draw node is what makes the exchange
// free). A filled circle glyph (DrawCommand::FillEllipse over a square inset
// box, same inset box BuildStopDrawCommands already computes for its
// square), visually distinct from Play's triangle, Stop's square, and
// Freeze's diamond. Dark red normally; armed swaps plate and glyph outright,
// same as Freeze's own cyan/plate-colour exchange.
inline std::vector<synth::ui::DrawCommand> BuildRecordDrawCommands(synth::ui::Bounds bounds, bool armed) {
    const synth::Color plateColor = armed ? kRecordColor : kTransportPlateColor;
    const synth::Color glyphColor = armed ? kTransportPlateColor : kRecordColor;
    std::vector<synth::ui::DrawCommand> commands;
    const PlateAndInsetBox plate = BuildPlateAndInsetBox(bounds, plateColor);
    commands.push_back(plate.plate);
    commands.push_back(synth::ui::DrawCommand::FillEllipse(plate.inset, glyphColor));
    return commands;
}

// The bank-carousel back/forward arrow pair, AppendModulationHeaderRow's
// level-0 children. Same
// plate-plus-glyph idiom as the four builders above (rounded-rect plate,
// glyph inset at `kTransportIconFraction`) -- but plain triangles with no
// state to invert (no latched/armed toggle, unlike Freeze/Record), so no
// second colour-swapped variant is needed. The plate colour is
// `kTransportPlateColor`, the SAME neutral chrome colour every transport
// plate above uses; the glyph is `synth::Color::White`, matching the header
// band's own title text colour (`kModulationHeaderTextStyle`,
// AppendModulationHeaderRow's own comment) rather than any one transport
// plate's glyph colour -- Play/Stop/Freeze/Record's glyph colours
// (Green/Red/Cyan/dark-red) each encode that control's own state semantics,
// which this plain nav pair does not carry.
//
// The forward glyph (BuildBankNextArrowDrawCommands) is the exact same
// apex-right triangle shape as BuildPlayDrawCommands' own glyph above,
// reused verbatim; the back glyph (BuildBankPrevArrowDrawCommands) mirrors
// it (apex left, base right).
inline std::vector<synth::ui::DrawCommand> BuildBankPrevArrowDrawCommands(synth::ui::Bounds bounds) {
    std::vector<synth::ui::DrawCommand> commands;
    const PlateAndInsetBox plate = BuildPlateAndInsetBox(bounds, kTransportPlateColor);
    commands.push_back(plate.plate);
    const float left = plate.inset.x;
    const float right = plate.inset.x + plate.inset.width;
    const float top = plate.inset.y;
    const float bottom = plate.inset.y + plate.inset.height;
    commands.push_back(synth::ui::DrawCommand::FillPolygon(
        {
            synth::ui::Point{right, top},
            synth::ui::Point{right, bottom},
            synth::ui::Point{left, (top + bottom) * 0.5f},
        },
        synth::Color::White));
    return commands;
}

inline std::vector<synth::ui::DrawCommand> BuildBankNextArrowDrawCommands(synth::ui::Bounds bounds) {
    std::vector<synth::ui::DrawCommand> commands;
    const PlateAndInsetBox plate = BuildPlateAndInsetBox(bounds, kTransportPlateColor);
    commands.push_back(plate.plate);
    const float left = plate.inset.x;
    const float right = plate.inset.x + plate.inset.width;
    const float top = plate.inset.y;
    const float bottom = plate.inset.y + plate.inset.height;
    commands.push_back(synth::ui::DrawCommand::FillPolygon(
        {
            synth::ui::Point{left, top},
            synth::ui::Point{left, bottom},
            synth::ui::Point{right, (top + bottom) * 0.5f},
        },
        synth::Color::White));
    return commands;
}

// Small parse helpers (own implementation, following Braid4UiModel.hpp's
// ParseSize/ParseFloat *pattern* -- this ports the pattern, not the
// implementation: Braid4UiModel.hpp itself lives under the read-only
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

// The label actually RENDERED in each encoder cell, kept deliberately
// SEPARATE from `FroggersParamSpec::name`/`shortName`
// (FroggersParameters.hpp): the approved rendering here wins for RENDERING
// ONLY -- the seven marked shortenings below (e.g. "Comb feedback" -> "Comb
// FB") must not rename the REGISTERED parameter, whose `name`/`shortName`
// stay load-bearing for ParameterManager's own name space and other
// consumers. Indexed [bankIx][slot] for slots 0-13, in
// `FroggersBankLayouts()`'s own bank order (FroggersBankId::Audio=0 ...
// Reverb=5); the Envelope row (index 1) uses its canonical short forms
// verbatim (short names are acceptable there), every other row is the
// readable long name except the seven marked shortenings, called out per
// row below. Slots 14/15 (Crispy/Crunchy) are handled by
// `FroggersApprovedGlobalLabel` below, not this table -- they are not
// per-bank `FroggersParamSpec` entries (Crispy is six separate per-bank
// Parameter objects that all render the same word; Crunchy is one shared
// Parameter across all six banks).
inline const std::array<std::array<const char*, kFroggersParamsPerBank>, kFroggersBankCount>&
FroggersApprovedLabels() {
    static const std::array<std::array<const char*, kFroggersParamsPerBank>, kFroggersBankCount> labels{{
        {{"VCO1", "VCO2", "VCO3", "Shape 1", "Shape 2", "Shape 3", "Ph.mod 1", "Ph.mod 2", "Ph.mod 3",
          "Ringmod 1", "Ringmod 2", "Ringmod 3", "PM rate", "VCO balance"}},
        // Envelope -- canonical short forms, not a truncation: the short
        // form IS the name here.
        {{"A1", "D1", "S1", "R1", "A2", "D2", "S2", "R2", "A3", "D3", "S3", "R3", "Curve", "Grace"}},
        // Filter -- slot 5 shortened ("Comb feedback" -> "Comb FB").
        {{"Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb FB", "Comb LP",
          "Comb/Peak", "Scoop", "Topology", "Scoop freq", "Scoop width", "Comb drive", "Scoop depth"}},
        // Drive -- slot 9 shortened ("Anti-alias brightness" -> "Anti-alias"),
        // slot 13 shortened ("Waveshaper offset" -> "Bias").
        {{"Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Blend", "Phase",
          "Anti-alias", "Link", "Fold", "Tone", "Bias"}},
        // Delay -- slot 7 ("Reverse blend" -> "Reverse"), slot 9
        // ("Feedback drive" -> "FB drive"), slot 10 ("Feedback tone" ->
        // "FB tone"), slot 12 ("Width balance" -> "Width bal").
        {{"Delay time", "Send", "Feedback", "Stereo width", "Freeze", "Mod depth", "Wet mix",
          "Reverse", "Diffusion", "FB drive", "FB tone", "Mod rate", "Width bal", "Crush"}},
        // Reverb -- no shortenings.
        {{"Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion",
          "Mod depth", "Hold", "Mod rate", "Tank drive", "Grit", "Tilt", "Tuned"}},
    }};
    return labels;
}

// labels.md's "Global" row: one rendered word regardless of which bank's
// local Crispy this is (all six read the same word; Crunchy is the one
// shared Parameter). Deliberately independent of Crunchy's registered
// `shortName` ("Crnchy", FroggersParameters.hpp) -- labels.md wins for
// RENDERING, per this table's own header comment.
inline const char* FroggersApprovedGlobalLabel(std::size_t slot) {
    return slot == kFroggersCrispySlot ? "Crispy" : "Crunchy";
}

// The SINGLE-ROW native idiom, replacing an earlier fixed 10-column x 2-row
// plate design (see AppendEncoderCell's own comment) -- sized to hold the
// longest approved label. Verified (not assumed) by FroggersSurfaceTests.cpp's
// own
// `every_approved_label_fits_the_single_row_grid`: the longest of all 86
// entries in `FroggersApprovedLabels()`/`FroggersApprovedGlobalLabel` is
// "Stereo width" (Delay slot 3 and Reverb slot 5), 12 characters including
// the space.
inline constexpr int kApprovedLabelGridColumns = 12;

// Builds the single-row 14-segment label block for one encoder cell's label
// band -- ONE function, called from AppendEncoderCell's own Draw lambda AND
// directly from FroggersSurfaceTests.cpp's verbatim/non-intersection guards,
// so those tests compare against the exact commands production emits, not a
// second hand-written copy of this centering/padding arithmetic. `label` is
// uppercased here (the
// display is uppercase-only, same convention the retired
// SplitFourteenSegmentLines used) and centered by left-padding with spaces
// to `columns` -- `BuildFourteenSegmentCommands` (EncoderDraw.hpp) already
// left-aligns and pads the TRAILING side with spaces on its own, so
// centering only needs the leading pad computed here.
inline std::vector<synth::ui::DrawCommand> BuildEncoderLabelRowCommands(std::string_view label,
                                                                        synth::ui::Bounds rowBounds,
                                                                        synth::Color onColor,
                                                                        synth::Color offColor, int columns) {
    std::string upper;
    upper.reserve(label.size());
    for (char c : label) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    const int len = std::min(columns, static_cast<int>(upper.size()));
    const int leadingSpaces = (columns - len) / 2;
    const std::string padded = std::string(static_cast<std::size_t>(leadingSpaces), ' ') + upper.substr(0, static_cast<std::size_t>(len));
    return synth::ui::BuildFourteenSegmentCommands(padded, rowBounds, onColor, offColor, columns);
}

class FroggersUiSurface final : public synth::ui::Surface {
public:
    void Attach(synth::AppContext* context, FroggersAppCore* app) {
        context_ = context;
        app_ = app;
    }

    // A runtime host-capability flag, defaulted false -- every existing host
    // (desktop, browser) never calls this, so their rendered tree is
    // byte-identical to before this flag existed. The plugin host (the VST
    // editor) calls SetPluginHostMode(true) before/at attach time so
    // AppendTransportRow() below thins Play | Stop | Freeze | Record down to
    // Freeze | "FREEZE" label -- the plugin spec's binding requirement
    // (Play/Stop/Record are meaningless when the DAW is transport authority;
    // only Freeze survives as a plugin-reachable control).
    //
    // Deliberately a plain runtime setter on the surface instance, NOT a
    // new FroggersCellMap::LeftKind row-table entry (contrast the CELL
    // MAP's own convention, kLeftRows above, ~line 442): kLeftRows is ONE
    // compile-time static array shared by every host that constructs this
    // class -- baking a per-host variant into that table would make the
    // STANDALONE host's row shape depend on a flag it never touches, when
    // what actually varies here is which CHILDREN one already-declared row
    // (Transport) emits, not which rows exist at all. This follows the same
    // "read live state fresh every rebuild" idiom AppendTransportRow
    // already uses for app_->FreezeLatched() (T5.2, this class rebuilds its
    // tree every frame) -- pluginHostMode_ is just another per-rebuild-read
    // runtime flag, owned by the surface instance rather than by app_
    // (it is a HOST fact, not an application-state fact, so it does not
    // belong on FroggersAppCore either).
    void SetPluginHostMode(bool pluginHostMode) { pluginHostMode_ = pluginHostMode; }
    bool PluginHostMode() const { return pluginHostMode_; }

    synth::ui::NodeTree BuildTree() override {
        const synth::ui::Bounds root = FroggersPageLayout::RootBounds(context_);

        synth::ui::Builder builder;
        builder.Root(FroggersNodeIds::kRoot, root);
        // The on-canvas "Frogg3rs Synth" title label is removed --
        // `config.appName` (FroggersAppCore.hpp:183) and
        // `FroggersManifest().displayName` (FroggersRegistration.hpp:24)
        // already cover launcher/window-title naming. The freed space is
        // left for a future logo (design E3f,
        // deferred pending upstream `DrawCommand::Image`).

        // Task F.3: ONE outer split Row -- left block (Weight(2): scope,
        // transport, scenes, scene-blend, BPM) beside right block
        // (Weight(4): bank tabs, the 16-slot encoder grid, randomize) --
        // matching the CELL MAP's 2-of-6 vs 4-of-6 column split. Outer
        // padding/gap are this file's own design tokens
        // (FroggersPageLayout::kMargin/kGap), not upstream defaults.
        synth::ui::LayoutOptions outerLayout;
        outerLayout.main = synth::ui::Extent::Weight(1.0f);
        outerLayout.cross = synth::ui::Extent::Weight(1.0f);
        outerLayout.padding = FroggersPageLayout::kMargin;
        outerLayout.gap = FroggersPageLayout::kGap;
        builder.Row(FroggersNodeIds::kLayoutRoot, outerLayout, [this](synth::ui::Builder& b) {
            AppendLeftBlock(b);
            AppendRightBlock(b);
        });

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
    // Display-only offset for the scene-blend slider (operator 2026-08-17):
    // the Scene 1/Scene 2 buttons read 1/2 (design E3d, AppendScenesRow()
    // above, DO NOT CHANGE), while Sheaf's own `SceneState.blend` -- clamped
    // 0..1 inside Sheaf -- must stay 0..1. Rather than relabel the buttons to
    // 0/1, the operator chose to make the slider PRESENT 1.0-2.0 so it agrees
    // with the buttons; the underlying message still carries 0..1. Both call
    // sites (AppendSceneBlendGroup()'s `b.Slider(...)` below and
    // HandleAction()'s `kSceneBlend` branch) must apply the SAME offset, so
    // it is this one named constant rather than two bare `1.0f`s that would
    // have to be kept in agreement by hand.
    static constexpr float kSceneBlendDisplayOffset = 1.0f;

    // -- Left block (FroggersCellMap, columns L1-L2) ------------------

    void AppendLeftBlock(synth::ui::Builder& builder) const {
        synth::ui::LayoutOptions blockLayout;
        blockLayout.main = synth::ui::Extent::Weight(FroggersCellMap::kLeftBlockWeight);
        blockLayout.cross = synth::ui::Extent::Weight(1.0f);
        blockLayout.padding = 0.0f;
        blockLayout.gap = FroggersPageLayout::kGap;
        builder.Column(FroggersNodeIds::kLeftBlock, blockLayout, [this](synth::ui::Builder& b) {
            for (const FroggersCellMap::LeftRow& row : FroggersCellMap::kLeftRows) {
                AppendLeftRow(b, row);
            }
        });
    }

    void AppendLeftRow(synth::ui::Builder& builder, const FroggersCellMap::LeftRow& row) const {
        switch (row.kind) {
            case FroggersCellMap::LeftKind::Scope:
                AppendScopeCell(builder, row.rowWeight);
                return;
            case FroggersCellMap::LeftKind::Transport:
                AppendTransportRow(builder, row.rowWeight);
                return;
            case FroggersCellMap::LeftKind::Scenes:
                AppendScenesRow(builder, row.rowWeight);
                return;
            case FroggersCellMap::LeftKind::SceneBlend:
                AppendSceneBlendGroup(builder, row.rowWeight);
                return;
            case FroggersCellMap::LeftKind::Bpm:
                AppendBpmGroup(builder, row.rowWeight);
                return;
        }
    }

    // The VCO scope panel. Its bounds are not known until the layout
    // resolves (it is now an in-flow, weight-sized cell, not a
    // hand-computed pixel rectangle), so the DrawFactory form is used
    // exactly like Braid4UI.hpp's own encoder-visualizer-underlay pattern:
    // the factory receives the RESOLVED extent and sets it on the
    // visualizer at that point.
    //
    // RELOCATED, 2026-08-08 (three sessions on the same operator report:
    // "i still don't see a header label counting the drilldown levels").
    // A header was first appended directly to THIS node's own draw
    // commands, then restyled (larger text, opaque band) when that proved
    // still invisible; a further check confirmed the header is never
    // overdrawn WITHIN this node either.
    // Both fixes were correct about the property they checked and
    // irrelevant to the actual complaint, because neither asked where this
    // node's cell IS relative to what the operator is looking at while
    // drilled in.
    //
    // Computed (FroggersSurfaceTests.cpp's
    // modulation_header_sits_below_bank_row_and_above_parameter_cells,
    // and the pre-existing scope_and_grid_regions_do_not_overlap_at_target_
    // window_size / scope_sits_in_a_left_column_with_the_grid_to_its_right),
    // not eyeballed: at the real 900x632 config this cell resolves to
    // roughly {16, 16, 284.7, 181.3} -- inside the CELL MAP's LEFT block
    // (columns L1-L2). The 16-slot grid the operator watches while drilled
    // in -- the only thing that changes content when drill-in changes --
    // lives in the RIGHT block (columns E1-E4), which resolves to roughly
    // {314.7, 16, 569.3, 600}. A 14px gap (FroggersPageLayout::kGap)
    // separates x-range [16, 300.7] from x-range [314.7, 884]; they never
    // meet. A header painted correctly, on top, and never overdrawn INSIDE
    // this node was never going to be seen by an operator looking at that
    // one -- existence and non-overdraw are not visibility.
    //
    // The indicator lived on the Target/Back cell next (AppendEncoderCell),
    // physically inside the region above and present at every drill depth
    // by construction -- but the operator rejected THAT placement too
    // (2026-08-09, fourth session): "i don't know why you thought i wanted
    // the header to be 'Back' and by the back button, instead of a HEADER
    // above all the modulation parameters, below the bank button row??
    // ... nothing needs to be labeled 'back' there." A badge reading "BACK
    // L<N>" on the one cell whose JOB is to go back a level conflated two
    // separate facts (the current drill depth, and "this cell exits one
    // level") into one label, and answered a question ("how do I get back
    // out") the operator never asked while asking a different one ("how
    // deep am I") to go unanswered in its own right.
    //
    // STEP 1 (2026-08-09): the indicator is now a dedicated header ROW
    // (FroggersNodeIds::kModulationHeader, AppendModulationHeaderRow()
    // below) spanning the right block's full width, between the bank tabs
    // row and the first row of parameter cells -- not attached to any
    // button or cell, exactly the operator's own description. This node
    // (kVcoScope) and the Target/Back encoder cell both keep NO copy of
    // this text: three renderings of the same one fact (the current drill
    // level) would be duplication, and the two that would remain
    // here/there are the ones already proven either unreachable (kVcoScope)
    // or actively confusing (Target/Back).
    void AppendScopeCell(synth::ui::Builder& builder, float rowWeight) const {
        synth::ui::LayoutOptions layout;
        layout.main = synth::ui::Extent::Weight(rowWeight);
        layout.cross = synth::ui::Extent::Weight(1.0f);
        // The cell is always emitted (matching Braid4UI.hpp's own scope-cell
        // idiom, `EmitScopeCell`): only the DATA depends on `app_`, not
        // whether the node exists, so a bare-context resolve (no app
        // attached -- FroggersSurfaceTests.cpp's layout-only tests) still
        // sees a real `kVcoScope` cell with a real resolved extent, just
        // empty draw commands.
        FroggersAppCore* app = app_;
        builder.Draw(FroggersNodeIds::kVcoScope, layout,
                     [app](synth::ui::Bounds extent) -> std::vector<synth::ui::DrawCommand> {
                         if (app == nullptr) {
                             return {};
                         }
                         synth::ui::Visualizer& vcoScope = app->VcoScopeVisualizer();
                         vcoScope.SetBounds(extent);
                         return vcoScope.Draw();
                     });
    }

    // Row 3: Play | Stop, restored as `Draw` nodes (F.2b, ask 1 landed --
    // plain click on `Draw` nodes). `kTransportPlateSize` is an unchanged,
    // fixed `Extent::Px` size on both axes (survives task F.3's deletion
    // table verbatim), so the plates stay their original 28x28 square
    // regardless of the row's resolved width.
    void AppendTransportRow(synth::ui::Builder& builder, float rowWeight) const {
        synth::ui::LayoutOptions rowLayout;
        rowLayout.main = synth::ui::Extent::Weight(rowWeight);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersPageLayout::kGap;
        // T5.2: the Freeze plate's draw factory needs to read `app_`'s
        // CURRENT latch state on every rebuild (the tree rebuilds every
        // frame, this file's header comment) -- captured into a local first,
        // same idiom AppendScopeCell above uses for kVcoScope, so the
        // now-const-context row-builder lambda below can capture it too.
        FroggersAppCore* app = app_;
        // Captured by value into the row-builder lambda, same reason `app`
        // is -- read fresh every rebuild, but this one never
        // actually changes mid-session (a host does not switch modes after
        // attaching), so it is really just "the value this render pass
        // uses," not a live cross-thread read like app_->FreezeLatched().
        const bool pluginHostMode = pluginHostMode_;
        builder.Row(FroggersNodeIds::kTransportRow, rowLayout, [app, pluginHostMode](synth::ui::Builder& b) {
            // In plugin mode, Play, Stop, and Record are not rendered; the
            // Freeze button stays and gains a "FREEZE" text label beside it
            // in the freed row space. pluginHostMode_ defaults false, so
            // this branch is never taken by the standalone/desktop/browser
            // hosts -- their Play/Stop/Freeze/Record sequence below is
            // byte-identical to before this flag existed.
            if (!pluginHostMode) {
                synth::ui::ControlStyle playStyle{};
                playStyle.action = synth::ui::Action::Named(FroggersActions::kPlay);
                playStyle.layout.main = synth::ui::Extent::Px(kTransportPlateSize);
                playStyle.layout.cross = synth::ui::Extent::Px(kTransportPlateSize);
                b.Draw(FroggersNodeIds::kPlay, BuildPlayDrawCommands, playStyle);

                synth::ui::ControlStyle stopStyle{};
                stopStyle.action = synth::ui::Action::Named(FroggersActions::kStop);
                stopStyle.layout.main = synth::ui::Extent::Px(kTransportPlateSize);
                stopStyle.layout.cross = synth::ui::Extent::Px(kTransportPlateSize);
                b.Draw(FroggersNodeIds::kStop, BuildStopDrawCommands, stopStyle);
            }

            // Freeze, same 28px plate idiom as Play/Stop -- present in BOTH
            // modes (the Freeze button stays even when Play/Stop/Record are
            // hidden). A capturing
            // lambda wrapping BuildFreezeDrawCommands (it takes a `latched`
            // bool the DrawFactory signature -- Bounds only -- has no room
            // for), same style the encoder-cell `b.Draw(encoderId, [state,
            // ...](Bounds){...}, ...)` call (AppendEncoderCell) already uses
            // for its own per-frame captured state. Reads
            // `app->FreezeLatched()` fresh on every call rather than a value
            // cached at click time, so a click updates the drawn colours on
            // the very next rebuild.
            synth::ui::ControlStyle freezeStyle{};
            freezeStyle.action = synth::ui::Action::Named(FroggersActions::kFreeze);
            freezeStyle.layout.main = synth::ui::Extent::Px(kTransportPlateSize);
            freezeStyle.layout.cross = synth::ui::Extent::Px(kTransportPlateSize);
            b.Draw(
                FroggersNodeIds::kFreeze,
                [app](synth::ui::Bounds bounds) {
                    return BuildFreezeDrawCommands(bounds, app != nullptr && app->FreezeLatched());
                },
                freezeStyle);

            if (pluginHostMode) {
                // The "FREEZE" text label, beside the Freeze plate, in the
                // row space Play/Stop/Record no longer occupy -- same
                // hand-rolled Label idiom kBpmLabel/
                // kSceneBlendLabel use (AppendBpmControl() above,
                // `builder.Label(id, text, ControlStyle{})`), just placed
                // beside its control (a Row's in-flow next child) instead
                // of below it (those two sit in a Column).
                b.Label(FroggersNodeIds::kFreezeLabel, "FREEZE", synth::ui::ControlStyle{});
            } else {
                // Record, fourth child, same 28px plate idiom and same
                // "read live state fresh every rebuild" lambda shape as
                // Freeze just above -- app->RecordArmed() rather than
                // app->FreezeLatched(). Not rendered in plugin-host mode.
                synth::ui::ControlStyle recordStyle{};
                recordStyle.action = synth::ui::Action::Named(FroggersActions::kRecord);
                recordStyle.layout.main = synth::ui::Extent::Px(kTransportPlateSize);
                recordStyle.layout.cross = synth::ui::Extent::Px(kTransportPlateSize);
                b.Draw(
                    FroggersNodeIds::kRecord,
                    [app](synth::ui::Bounds bounds) {
                        return BuildRecordDrawCommands(bounds, app != nullptr && app->RecordArmed());
                    },
                    recordStyle);
            }
        });
    }

    // Row 4: Scene 1 | Scene 2, each taking half the row (Weight(1), an
    // intrinsic cross size so the button does not stretch to the row's full
    // resolved height).
    //
    // "Scene 1"/"Scene 2" are a TOGGLE between the scene-blend extremes, not
    // a re-assignment of which stored scene occupies the less-weighted
    // endpoint -- see HandleAction() below for the full trace.
    void AppendScenesRow(synth::ui::Builder& builder, float rowWeight) const {
        synth::ui::LayoutOptions rowLayout;
        rowLayout.main = synth::ui::Extent::Weight(rowWeight);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersPageLayout::kGap;
        builder.Row(FroggersNodeIds::kScenesRow, rowLayout, [](synth::ui::Builder& b) {
            for (std::size_t sceneIx = 0; sceneIx < 2; ++sceneIx) {
                synth::ui::ControlStyle style{};
                style.layout.main = synth::ui::Extent::Weight(1.0f);
                style.layout.cross = synth::ui::Extent::Intrinsic();
                b.Button(FroggersNodeIds::SceneButton(sceneIx), "Scene " + std::to_string(sceneIx + 1),
                         synth::ui::Action::WithValue(FroggersActions::kSceneSelect, std::to_string(sceneIx)),
                         style);
            }
        });
    }

    // Row 5: the Scene-blend slider with its label BELOW it, superseding the
    // F.2d caption for scene-blend (a `ControlStyle::caption` can only lead,
    // so scene-blend returns to a hand-rolled label, now placed under the
    // slider). Row 6 now has the identical shape -- BPM's label moved below
    // too, superseding B12; see AppendLabelledSlider() above for why that
    // honours B12 rather than overriding it.
    //
    // NOTE ON A STALE TEST-ENUMERATION ENTRY: task F.3's own test
    // enumeration table classified
    // `scene_blend_slider_has_an_adjacent_label_node_carrying_its_text` as
    // UNAFFECTED, which is inconsistent with this amendment (that test
    // asserted the F.2d `<controlId>.caption` node, which this amendment
    // retires for scene-blend). The CELL MAP's amendment is the more
    // specific and more recently affirmed instruction, so it governs; the
    // test was rewritten in FroggersSurfaceTests.cpp to match, and this is
    // called out in the task report as a place the traced table was wrong.
    // ROWS 5 AND 6 SHARE THIS ONE EMITTER.
    //
    // One container kind (`Column`), one declared slider width, called twice.
    // There is deliberately **no placement parameter and no branch on
    // container kind**: both labels sit BELOW their slider. Two emitters is
    // what produced the defect -- they drifted into a Column and a Row, and
    // since neither declared a width, "how wide is this slider" was answered
    // by which container it happened to live in.
    //
    // B12 IS SUPERSEDED, NOT IGNORED (operator 2026-08-05, "the labels should
    // BOTH be below"). B12 (2026-07-29) read: "BPM label moved to trail its
    // slider. LEADING it put it between the two sliders and nearer the
    // scene-blend one, reading as labelling the wrong control. The two labels
    // are now deliberately asymmetric -- do not 'fix' that." Its stated reason
    // is entirely about LEADING being ambiguous; trailing was simply the only
    // alternative while both labels shared a horizontal band. Once the CELL
    // MAP put scene-blend's label BELOW its slider, that ambiguity was gone --
    // a label directly beneath its own control cannot be read as its
    // neighbour's. Both-below therefore serves B12's actual concern better
    // than trailing did, and the asymmetry B12 protected was a means, not the
    // goal. An earlier draft of F.6 kept trailing and invented a
    // `LabelPlacement{Below,Trailing}` parameter to honour B12's literal
    // words; that is a workaround kept alive after its cause died, and the
    // extra branch was the tell. An instruction's rationale is part of the
    // instruction.
    //
    // `emitControl` receives the shared slider style, so the width is read
    // from its single definition site exactly once, here, while each caller
    // keeps its own control logic -- BPM has a read-only StatusText state
    // this row must not flatten away.
    void AppendLabelledSlider(
        synth::ui::Builder& builder,
        float rowWeight,
        const char* groupId,
        const std::function<void(synth::ui::Builder&, const synth::ui::ControlStyle&)>& emitControl) const {
        synth::ui::LayoutOptions groupLayout;
        groupLayout.main = synth::ui::Extent::Weight(rowWeight);
        groupLayout.cross = synth::ui::Extent::Weight(1.0f);
        groupLayout.padding = 0.0f;
        groupLayout.gap = FroggersPageLayout::kGap;

        // In a Column the CROSS axis is the horizontal one, so this is where
        // the shared width lands. See kSliderWidthFraction's own comment for
        // why it is a fraction rather than a pixel count.
        synth::ui::ControlStyle sliderStyle;
        sliderStyle.layout.cross = synth::ui::Extent::Fraction(FroggersPageLayout::kSliderWidthFraction);

        builder.Column(groupId, groupLayout, [&emitControl, &sliderStyle](synth::ui::Builder& b) {
            emitControl(b, sliderStyle);
        });
    }

    void AppendSceneBlendGroup(synth::ui::Builder& builder, float rowWeight) const {
        const float sceneBlend = context_ != nullptr && context_->uiState != nullptr
                                      ? context_->uiState->sceneBlend.load(std::memory_order_relaxed)
                                      : 0.0f;
        AppendLabelledSlider(
            builder, rowWeight, FroggersNodeIds::kSceneBlendGroup,
            [sceneBlend](synth::ui::Builder& b, const synth::ui::ControlStyle& sliderStyle) {
                // Presented value/range are the blend shifted by
                // kSceneBlendDisplayOffset (this class's own display-only
                // constant, see its declaration above) so the slider reads
                // 1.0-2.0 to match the Scene 1/Scene 2 buttons; the message
                // this produces still carries 0..1 -- see HandleAction()'s
                // `kSceneBlend` branch, which subtracts the same offset back
                // out.
                b.Slider(FroggersNodeIds::kSceneBlend, "Scene blend", sceneBlend + kSceneBlendDisplayOffset,
                         kSceneBlendDisplayOffset, 1.0f + kSceneBlendDisplayOffset, 0.001f,
                         synth::ui::Action::Named(FroggersActions::kSceneBlend), sliderStyle);
                // Label-visibility fix (2026-07-28): `NodeKind::Slider` routes
                // `node.label` to `juce::Slider::setName()` only
                // (PortableJuceBackend.hpp:1229-1232) -- no `juce::Label` is
                // attached, so the slider's own label argument never draws;
                // this adjacent Label node is what actually renders the text.
                b.Label(FroggersNodeIds::kSceneBlendLabel, "Scene blend", synth::ui::ControlStyle{});
            });
    }

    // Row 6: the BPM slider with its label BELOW it, exactly like row 5 --
    // see AppendLabelledSlider()'s B12 supersession note. Still a read-only
    // StatusText while slaved to external MIDI clock.
    void AppendBpmGroup(synth::ui::Builder& builder, float rowWeight) const {
        AppendLabelledSlider(builder, rowWeight, FroggersNodeIds::kBpmGroup,
                             [this](synth::ui::Builder& b, const synth::ui::ControlStyle& sliderStyle) {
                                 AppendBpmControl(b, sliderStyle);
                             });
    }

    // Read-only/inert (a StatusText) while slaved to external MIDI clock, an
    // interactive Slider otherwise (see MasterClock.hpp:318/:321,
    // MasterClock.cpp:963-965/:1182). Both states
    // display TempoBpm(). Unchanged in substance from before task F.3 --
    // only its container moved (from the old auto-flowed chrome band into
    // this row's own group, see AppendBpmGroup() above).
    void AppendBpmControl(synth::ui::Builder& builder, const synth::ui::ControlStyle& sliderStyle) const {
        const double tempoBpm = app_ != nullptr ? app_->DisplayTempoBpm() : synth::MasterClock::kDefaultTempoBpm;
        const bool externallyClocked = app_ != nullptr && app_->TempoExternallyClocked();
        if (externallyClocked) {
            // Takes the same declared width as the interactive slider it
            // replaces, so the row does not change shape when the clock is
            // slaved. There is no adjacent Label in this state (the status
            // text names itself) -- unchanged from before task F.6.
            builder.StatusText(FroggersNodeIds::kBpm, "BPM " + FormatFroggersBpm(tempoBpm) + " (external clock)",
                               sliderStyle);
            return;
        }
        // The control genuinely IS labelled "BPM" -- the
        // transport-state-dependent "(no effect while stopped)" annotation
        // was never requested and is not to be reintroduced without asking
        // first.
        constexpr const char* kLabel = "BPM";
        // Still a hand-rolled adjacent Label rather than
        // `ControlStyle::caption`, for the same reason as scene-blend's:
        // `Builder::FinishControl` (PortableUIBuilders.hpp:428-465) always
        // emits a caption BEFORE its control, and both labels now sit BELOW
        // theirs. Upstream ask 14 (caption placement) would let both of these
        // collapse into captions; until it lands, both stay hand-rolled --
        // and they stay hand-rolled TOGETHER, which is the point of task F.6.
        builder.Slider(FroggersNodeIds::kBpm, kLabel, static_cast<float>(tempoBpm), 30.0f, 300.0f, 1.0f,
                       synth::ui::Action::Named(FroggersActions::kBpm), sliderStyle);
        builder.Label(FroggersNodeIds::kBpmLabel, kLabel, synth::ui::ControlStyle{});
    }

    // -- Right block (FroggersCellMap, columns E1-E4) -----------------

    void AppendRightBlock(synth::ui::Builder& builder) const {
        synth::ui::LayoutOptions blockLayout;
        blockLayout.main = synth::ui::Extent::Weight(FroggersCellMap::kRightBlockWeight);
        blockLayout.cross = synth::ui::Extent::Weight(1.0f);
        blockLayout.padding = 0.0f;
        blockLayout.gap = FroggersPageLayout::kGap;
        builder.Column(FroggersNodeIds::kRightBlock, blockLayout, [this](synth::ui::Builder& b) {
            for (const FroggersCellMap::RightRow& row : FroggersCellMap::kRightRows) {
                AppendRightRow(b, row);
            }
        });
    }

    void AppendRightRow(synth::ui::Builder& builder, const FroggersCellMap::RightRow& row) const {
        switch (row.kind) {
            case FroggersCellMap::RightKind::BankTabs:
                AppendBankTabsRow(builder);
                return;
            case FroggersCellMap::RightKind::Header:
                AppendModulationHeaderRow(builder);
                return;
            case FroggersCellMap::RightKind::EncoderRow:
                AppendEncoderRow(builder, row.firstEncoderIndex);
                return;
            case FroggersCellMap::RightKind::Randomize:
                AppendRandomizeRow(builder);
                return;
            case FroggersCellMap::RightKind::Reset:
                AppendResetRow(builder);
                return;
        }
    }

    // Row 1: the six bank-select tabs, LOOPED from `FroggersBankLayouts()`
    // (single source of truth for bank identity/order,
    // app/FroggersParameters.hpp), not a second hand-written list.
    //
    // Plain `Button` nodes with the action supplied directly -- an earlier
    // Draw/DrawInteractive approach dispatched only on double-click, later
    // reverted for single-click bank switching. `node.selected` for the
    // active bank comes from `ControlStyle::selected`.
    void AppendBankTabsRow(synth::ui::Builder& builder) const {
        synth::ui::LayoutOptions rowLayout;
        // T4.2: `kUnchangedRowHeight` (68, not the encoder rows' 88) --
        // paired with AppendEncoderRow's own change above so this row
        // stays pixel-identical to today at the default window while the
        // 4 encoder rows alone grow by kLabelBandHeight. See
        // FroggersEncoderGridLayout's own comment for the exact-division
        // property this relies on.
        rowLayout.main = synth::ui::Extent::Weight(FroggersEncoderGridLayout::kUnchangedRowHeight);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersPageLayout::kGap;
        const auto& layouts = FroggersBankLayouts();
        builder.Row(FroggersNodeIds::kBankTabsRow, rowLayout, [this, &layouts](synth::ui::Builder& b) {
            for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
                synth::ui::ControlStyle style{};
                style.selected = BankSelected(bankIx);
                style.layout.main = synth::ui::Extent::Weight(1.0f);
                style.layout.cross = synth::ui::Extent::Intrinsic();
                b.Button(FroggersNodeIds::BankButton(bankIx), layouts[bankIx].name,
                         synth::ui::Action::WithValue(FroggersActions::kBankSelect, std::to_string(bankIx)), style);
            }
        });
    }

    // Row 2 (STEP 1, operator 2026-08-09, fourth session on the same
    // complaint -- see this file's git history for F7/S5.2/the Target-Back
    // badge, all rejected). Operator, verbatim: "i don't know why you
    // thought i wanted the header to be 'Back' and by the back button,
    // instead of a HEADER above all the modulation parameters, below the
    // bank button row?? ... nothing needs to be labeled 'back' there, that
    // implementation sucks." Unambiguous ask: a header BAR spanning the
    // grid's width, between the bank tabs row and the first row of
    // parameter cells -- not attached to any button or cell.
    //
    // APPROACH: a dedicated row in the right block's CELL MAP
    // (FroggersCellMap::RightKind::Header, kRightRows), a fixed-height Draw
    // node -- the same Extent::Px sizing idiom the transport plates already
    // use (kTransportPlateSize) for a chrome element that must not stretch.
    // A dedicated row is now explicitly ACCEPTABLE (operator's own
    // instruction), superseding the earlier "do not disturb the 6x6 grid"
    // guidance for this one case.
    //
    // WHAT MOVED: no row/column WEIGHT value changed anywhere -- every
    // pre-existing Weight(1.0) right-block row stays Weight(1.0), and
    // kLeftBlockWeight/kRightBlockWeight/kSliderWidthFraction are untouched.
    // Inserting one more FIXED-size sibling simply leaves less remaining
    // space for the six pre-existing weighted rows to divide
    // (AllocateExtents, PortableUILayout.hpp:165-241: `remaining =
    // contentExtent - totalGaps - nonWeighted`, then split by weight) -- an
    // arithmetic CONSEQUENCE of the insertion, not a declared change.
    // ARITHMETIC RESTATED 2026-08-17 (the numbers below were computed for a
    // 7-row/900x632 surface and went stale twice -- once when the Reset row
    // made it 8 rows, again when the label-band task grew the window to
    // 900x712; see this file's own drift note where the encoder cell height
    // is derived). Current, at 900x712: kRightBlock is 680px tall; 8 rows
    // cost 7 gaps of 14 = 98, plus this row's fixed 26px, leaving 556 --
    // which is exactly the total weight (3 unchanged rows at 68 + 4 encoder
    // rows at 88), so every row resolves to its declared height bit-exactly.
    // Nothing moves horizontally,
    // no row is reordered, and the encoder grid's own 4-column-per-row
    // internal structure (ids, order, weights) is completely untouched.
    // Verified in FroggersSurfaceTests.cpp's
    // modulation_header_sits_below_bank_row_and_above_parameter_cells,
    // which computes the resolved bounds rather than asserting this
    // comment's arithmetic.
    //
    // CONTENT: "Modulation Level <N>" (N = FroggersModulationDrillIn::
    // Level() via app_->DrillLevel(), the same source F7 originally read,
    // never a hardcoded per-level string), drawn ONLY while drilled in
    // (level > 0) -- an empty Draw at level 0, per the operator's own spec.
    // The row's own SPACE is always reserved (constant Px height regardless
    // of drill state): the same "always emit the node, sometimes with empty
    // commands, so sibling geometry never jumps" idiom AppendEncoderCell
    // already uses for a hidden slot in the modulation view (see that
    // method's own comment) -- entering/exiting a drilldown never reflows
    // the bank tabs or the parameter grid by even one pixel; only this
    // row's own content changes.
    static constexpr float kModulationHeaderRowHeight = 26.0f;
    static constexpr synth::Color kModulationHeaderBandColor = synth::Color::Rgb(32, 38, 44);
    // Centered, not left-aligned like the retired scope-cell/Target-Back
    // attempts: this is now a genuine full-width title bar, not a label
    // squeezed into a corner of unrelated content, so a centered title
    // reads as a header rather than another chip.
    static constexpr synth::ui::TextStyle kModulationHeaderTextStyle{
        20.0f, synth::Color::Rgb(255, 255, 255), synth::ui::TextAlign::Center};

    // The row's OUTER geometry (id kModulationHeader,
    // Px(kModulationHeaderRowHeight) main / Weight(1) cross) is identical in
    // both drill states -- only the CHILDREN switch. Level 0 emits a
    // centered back/forward arrow pair; level > 0 emits the single
    // full-width title child (kModulationHeaderTitle) carrying the exact
    // fill+text commands this row itself used to draw directly before this
    // change (only the carrying node moved, not the content).
    void AppendModulationHeaderRow(synth::ui::Builder& builder) const {
        synth::ui::LayoutOptions layout;
        layout.main = synth::ui::Extent::Px(kModulationHeaderRowHeight);
        layout.cross = synth::ui::Extent::Weight(1.0f);
        // Container defaults (padding=12/gap=8, PortableUILayout.hpp's
        // kSpacing) would eat most of a 26px-tall band and misplace the
        // arrow pair -- explicit zero padding, same idiom AppendTransportRow
        // already uses for its own fixed-height row (`rowLayout.padding =
        // 0.0f`, above); the row's own `gap` is the pair's documented
        // separation (FroggersPageLayout::kGap).
        layout.padding = 0.0f;
        layout.gap = FroggersPageLayout::kGap;
        const std::size_t drillLevel = app_ != nullptr ? app_->DrillLevel() : 0;
        builder.Row(FroggersNodeIds::kModulationHeader, layout, [drillLevel](synth::ui::Builder& b) {
            if (drillLevel == 0) {
                // [spacer Weight(1)][prev Px][next Px][spacer Weight(1)]:
                // two equal-weight spacers centre the fixed-size pair
                // regardless of the band's resolved width -- the row's own
                // uniform `gap` (set above) applies symmetrically on every
                // side of the pair, so the pair's midpoint lands on the
                // band's midpoint by construction (verified by
                // bank_carousel_arrows_are_centered_in_the_modulation_header_band_at_top_level,
                // FroggersSurfaceTests.cpp). Spacers are empty Draw nodes --
                // the same "always emit the node, sometimes with empty
                // commands" idiom AppendEncoderCell already uses for a
                // hidden grid slot -- with ad hoc suffixed ids, the same
                // convention `encoderId + ".visualizer"` already uses
                // elsewhere in this file rather than new named constants.
                synth::ui::LayoutOptions spacerLayout;
                spacerLayout.main = synth::ui::Extent::Weight(1.0f);
                const auto emptyDraw = [](synth::ui::Bounds) -> std::vector<synth::ui::DrawCommand> {
                    return {};
                };
                b.Draw(std::string(FroggersNodeIds::kModulationHeader) + ".spacer.left", spacerLayout, emptyDraw);

                synth::ui::ControlStyle prevStyle{};
                prevStyle.action = synth::ui::Action::Named(FroggersActions::kBankPrevious);
                prevStyle.layout.main = synth::ui::Extent::Px(kModulationHeaderRowHeight);
                prevStyle.layout.cross = synth::ui::Extent::Px(kModulationHeaderRowHeight);
                b.Draw(FroggersNodeIds::kBankPrevArrow, BuildBankPrevArrowDrawCommands, prevStyle);

                synth::ui::ControlStyle nextStyle{};
                nextStyle.action = synth::ui::Action::Named(FroggersActions::kBankNext);
                nextStyle.layout.main = synth::ui::Extent::Px(kModulationHeaderRowHeight);
                nextStyle.layout.cross = synth::ui::Extent::Px(kModulationHeaderRowHeight);
                b.Draw(FroggersNodeIds::kBankNextArrow, BuildBankNextArrowDrawCommands, nextStyle);

                b.Draw(std::string(FroggersNodeIds::kModulationHeader) + ".spacer.right", spacerLayout, emptyDraw);
                return;
            }

            synth::ui::LayoutOptions titleLayout;
            titleLayout.main = synth::ui::Extent::Weight(1.0f);
            b.Draw(FroggersNodeIds::kModulationHeaderTitle, titleLayout,
                   [drillLevel](synth::ui::Bounds extent) -> std::vector<synth::ui::DrawCommand> {
                       std::vector<synth::ui::DrawCommand> commands;
                       commands.push_back(synth::ui::DrawCommand::Fill(
                           synth::ui::Bounds{0.0f, 0.0f, extent.width, extent.height},
                           kModulationHeaderBandColor));
                       commands.push_back(synth::ui::DrawCommand::Text(
                           synth::ui::Bounds{0.0f, 0.0f, extent.width, extent.height},
                           "Modulation Level " + std::to_string(drillLevel), kModulationHeaderTextStyle));
                       return commands;
                   });
        });
    }

    // Rows 3-6 (renumbered by STEP 1's inserted header row; the 16-slot
    // grid, 4 slots per row) -- LOOPED over
    // `FroggersEncoderGridLayout::kColumns`, the row/col mapping
    // (`firstEncoderIndex / kColumns` below, `ix / kColumns`/`ix % kColumns`
    // in spirit) surviving task F.3's deletion of `BoundsForIndex`'s pixel
    // division.
    void AppendEncoderRow(synth::ui::Builder& builder, std::size_t firstEncoderIndex) const {
        const std::size_t row = firstEncoderIndex / FroggersEncoderGridLayout::kColumns;
        synth::ui::LayoutOptions rowLayout;
        // T4.2: `kGrownRowHeight` (88, not the sibling rows' 68) -- see
        // that constant's own comment for why a weight VALUE equal to the
        // target px height, alongside `AppendBankTabsRow`/
        // `AppendTwoButtonRow`'s matching change below, resolves this
        // row's height EXACTLY at the default window, growing only the
        // encoder rows and leaving bank tabs/Randomize/Reset pixel-
        // identical to today.
        rowLayout.main = synth::ui::Extent::Weight(FroggersEncoderGridLayout::kGrownRowHeight);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersEncoderGridLayout::kGap;
        builder.Row(FroggersNodeIds::EncoderRow(row), rowLayout, [this, firstEncoderIndex](synth::ui::Builder& b) {
            for (std::size_t column = 0; column < FroggersEncoderGridLayout::kColumns; ++column) {
                AppendEncoderCell(b, firstEncoderIndex + column);
            }
        });
    }

    // One encoder cell. Reads the SAME
    // `context_->uiState->slots[0]` snapshot whether it currently holds the
    // parameter grid or a drilled-in modulation-detail grid (Bank::
    // OpenModulationView/Deselect swap `visible_`'s contents; this surface
    // has no branch of its own for "which grid" -- see the drill-in note
    // below) -- and it is the ONLY place this surface reads a
    // `Parameter::UIState`.
    //
    // BRIEF-CHANGING (task F.3 topology trace): this loop must NOT
    // re-derive slot->parameter from `FroggersBankLayouts()`/
    // `PageParameter()`/`Crispy()`/`Crunchy()` -- those are
    // construction-time accessors that do not reflect modulation drill-in
    // substitution. `context_->uiState->slots[0].cells[ix]` already reflects
    // `Bank::VisibleParameter(ix)` (BankSlot::PopulateUIState publishes
    // exactly that), so this file already satisfies that requirement and did
    // not need to change to do so.
    //
    // A disconnected cell in the modulation view still holds its place in
    // the grid (Braid4UI.hpp's own EmitEncoderCell idiom, `hidden` below):
    // in the old fixed-pixel-index layout a `continue`-skip left the cell's
    // designated position blank; in this weight-resolved grid, omitting the
    // node entirely would let its siblings' weights redistribute and shift
    // position, silently RESEQUENCING the remaining cells on every drill-in
    // change. Always emitting the node with empty draw commands (and no
    // action/drag) when hidden keeps the grid geometry stable and is the
    // established Sheaf idiom this surface's own header comment points at
    // (Braid4UI.hpp:154-160).
    // REMOVED 2026-08-09 (operator, fourth session on the same complaint):
    // the drill-level indicator briefly lived here as a "BACK L<N>" badge
    // painted on the Target/Back cell (S5.2, 2026-08-08). Operator: "i
    // don't know why you thought i wanted the header to be 'Back' and by
    // the back button... nothing needs to be labeled 'back' there, that
    // implementation sucks." It is now a real header ROW spanning the
    // grid's width, between the bank tabs row and the first parameter row
    // -- see AppendModulationHeaderRow() and FroggersNodeIds::
    // kModulationHeader. This cell carries no drill-level text of any kind
    // any more (verified: FroggersSurfaceTests.cpp's
    // modulation_header_sits_below_bank_row_and_above_parameter_cells
    // asserts nothing reading "BACK" is drawn anywhere in the tree).

    void AppendEncoderCell(synth::ui::Builder& builder, std::size_t ix) const {
        const bool showingModulationView =
            context_ != nullptr && context_->uiState != nullptr && context_->uiState->slotCapacity > 0 &&
            context_->uiState->slots[0].showingModulationView.load(std::memory_order_relaxed);

        synth::ui::EncoderDrawState state{};
        synth::ui::Visualizer* visualizer = nullptr;
        if (context_ != nullptr && context_->uiState != nullptr && context_->uiState->slotCapacity > 0) {
            const synth::BankSlot::UIState& slotState = context_->uiState->slots[0];
            if (ix < slotState.cellCapacity) {
                // `EncoderDrawStateFromParameter` reads only
                // `Parameter::UIState.values[]` (the post-fuego,
                // post-modulation published display center) -- never
                // `.rawKnobValue`.
                state = synth::ui::EncoderDrawStateFromParameter(slotState.cells[ix]);
                visualizer = slotState.cells[ix].visualizer.load(std::memory_order_relaxed);
            }
        }
        const bool hidden = showingModulationView && !state.connected;
        state.hasVisualizerUnderlay = !hidden && visualizer != nullptr && visualizer->Visible();

        // S6.1 (operator screenshot, 2026-08-07): the parameter card's frame
        // outline visibly crossed the encoder's own modulation ring. Both the
        // frame and every ring/arc layer are emitted by ONE Sheaf function,
        // BuildEncoderDrawCommands (External/Sheaf/projects/synth/include/
        // synth/EncoderDraw.hpp:649-797), from geometry this app does not
        // own: the ring's radius is `baseRadius = min(bounds.width,
        // bounds.height) * 0.43f` (EncoderDraw.hpp:669) while the frame is
        // `bounds` inset by a fixed 1px with a 6px corner radius
        // (EncoderDraw.hpp:690-694) -- two different linear functions of the
        // SAME cell bounds this app supplies, so at this app's actual cell
        // size they collide (operator-measured: ring outer edge 38.057px
        // from centre vs. frame inner edge 36.257px, a 1.80px overlap). The
        // two edges' slopes (0.43 vs. 0.5) mean a SMALLER cell only widens
        // this collision, never closes it, so shrinking the bounds this app
        // passes into BuildEncoderDrawCommands is not a usable fix. The one
        // app-facing lever is `EncoderDrawState::wantsFrame`
        // (EncoderDraw.hpp:293, defaults true) -- the same field Sheaf's own
        // MiniAppUI.hpp sets per-visualizer (apps/miniapp/MiniAppUI.hpp:111);
        // Froggers never set it, so the frame always drew here. Operator
        // decision: drop the box entirely rather than live with the overlap.
        // Set once, unconditionally, for every encoder cell -- this is the
        // surface's one call site into BuildEncoderDrawCommands (below) --
        // not per-bank or per-cell.
        state.wantsFrame = false;

        const std::string encoderId = FroggersNodeIds::Encoder(ix);
        if (!hidden && visualizer != nullptr && visualizer->Visible()) {
            // Design D9b/D10: bump/comb transfer-function underlays and
            // modulation-source underlays render here automatically. The
            // underlay is deferred to the resolved bounds of its SIBLING
            // encoder cell via `overlayOf` (PortableUILayout.hpp:672-683,
            // 743-752) -- the same mechanism Braid4UI.hpp's own
            // EmitEncoderCell uses, needed here because the cell's own
            // bounds are not known until the layout resolves.
            synth::ui::LayoutOptions underlayLayout;
            underlayLayout.overlayOf = encoderId;
            builder.Draw(encoderId + ".visualizer", underlayLayout, [visualizer](synth::ui::Bounds extent) {
                visualizer->SetBounds(extent);
                return visualizer->Draw();
            });
        }

        // Change 2 REVERTED (operator 2026-07-27), F.2a (2026-08-03, pin
        // 77a3019e, ask 1 landed): the drill-in press dispatches from
        // `ControlStyle::action` (plain click) and the drag from the
        // separate `pointerDragAction` field -- no conflict, no post-Build()
        // patch. Neither is set while hidden: a disconnected cell in the
        // modulation view is inert as well as invisible.
        // `Draw` has no case in `metrics::IntrinsicFor`
        // (PortableUIMetrics.hpp:36-53, `default: {0,0,0,0}`) -- an in-flow
        // Draw node needs an explicit `layout.main` or it resolves to zero
        // size (this file's header comment makes the same point about the
        // transport plates). `Weight(1)` makes the cell fill its equal share
        // of the row, exactly `Braid4CellLayout()`'s own square-cell idiom
        // (`cross` stays the library default `Weight(1)` too, filling the
        // row's height).
        synth::ui::ControlStyle cellStyle{};
        cellStyle.layout.main = synth::ui::Extent::Weight(1.0f);
        if (!hidden) {
            cellStyle.action = synth::ui::Action::WithValue(FroggersActions::kEncoderPress, std::to_string(ix));
            cellStyle.pointerDragAction =
                synth::ui::Action::WithValue(FroggersActions::kEncoderDrag, FormatFroggersEncoderDrag(ix, 0.0f));
        }
        // Replacing an earlier fixed 10-column x 2-row plate, which covered
        // ~95% of the ring's lower semicircle: a SINGLE-ROW 14-segment
        // strip, sized to the longest approved label
        // (`kApprovedLabelGridColumns`), living ENTIRELY in the ~20px of
        // cell height added below the ring
        // (`FroggersEncoderGridLayout::kLabelBandHeight`) -- never inside
        // the ring's own sub-extent, so it structurally cannot intersect
        // the ring's drawn arc. `EncoderDrawState` has no field
        // to change Sheaf's own trailing block, but `BuildEncoderDrawCommands`
        // returns its command vector BY VALUE with that block appended LAST
        // (EncoderDraw.hpp:793-796) and `BuildFourteenSegmentCommands` is
        // public/inline -- so this app strips Sheaf's trailing block by its
        // exact, deterministic size and appends its own single-row strip
        // instead, same mechanism as before this task, just one row not two.
        const std::size_t bankIx = CurrentBankIndex();
        builder.Draw(
            encoderId,
            [state, hidden, bankIx, ix, showingModulationView](synth::ui::Bounds extent) {
                if (hidden) {
                    return std::vector<synth::ui::DrawCommand>{};
                }

                // `extent` is THIS cell's full resolved bounds, now
                // `FroggersEncoderGridLayout::kLabelBandHeight` px TALLER
                // than the ring alone needs (that constant is exactly how
                // much `AppendEncoderRow` grew each encoder row by). Handing
                // `BuildEncoderDrawCommands` the FULL extent would grow the
                // ring too (it derives `baseRadius`/`centerY` from
                // `min(width,height)` of whatever extent it is given,
                // EncoderDraw.hpp:664-669) -- so this app instead hands it a
                // SUB-extent, anchored at the cell's own top-left origin
                // (unaffected by growth, since the label band is added
                // BELOW, not above), whose height is `extent.height` minus
                // the label band -- at the default window this resolves to
                // EXACTLY `kUnchangedRowHeight` (68.0px, bit-for-bit: see
                // `FroggersEncoderGridLayout`'s own comment for why the row
                // weights make this an exact division, not an
                // approximation), reproducing today's ring geometry
                // byte-identically. At other window sizes the ring still
                // scales with the window, just `kLabelBandHeight` narrower
                // than the full cell -- the SAME resize behaviour as before
                // this task, offset by a constant.
                const synth::ui::Bounds ringExtent{
                    0.0f,
                    0.0f,
                    extent.width,
                    std::max(0.0f, extent.height - FroggersEncoderGridLayout::kLabelBandHeight),
                };
                std::vector<synth::ui::DrawCommand> commands = synth::ui::BuildEncoderDrawCommands(state, ringExtent);
                if (!state.connected) {
                    // BuildEncoderDrawCommands returns {} immediately for a
                    // disconnected cell (EncoderDraw.hpp:653-656) -- there
                    // is no trailing label block to strip or replace.
                    return commands;
                }

                // Sheaf's trailing block size is exactly
                // kSheafLabelCommandsPerChar (15: 14 AppendCharacter
                // segment polygons -- 4 horizontal/G-bar + 6 vertical + 4
                // diagonal, EncoderDraw.hpp:495-526, every one of which
                // returns a non-empty polygon at any real display size --
                // plus 1 unconditional decimal-point FillEllipse,
                // EncoderDraw.hpp:528-531) times kSheafLabelDefaultChars
                // (4, BuildFourteenSegmentCommands's own numChars default,
                // EncoderDraw.hpp:540) = 60 commands, always, for a
                // connected cell. Guarded (size >= that) so a cell that
                // somehow emitted fewer commands can never underflow --
                // the `!state.connected` branch above already covers the
                // one case (a disconnected cell) that legitimately returns
                // fewer/none.
                constexpr std::size_t kSheafLabelCommandsPerChar = 15;
                constexpr std::size_t kSheafLabelDefaultChars = 4;
                constexpr std::size_t kSheafLabelCommandCount =
                    kSheafLabelCommandsPerChar * kSheafLabelDefaultChars;
                if (commands.size() >= kSheafLabelCommandCount) {
                    commands.resize(commands.size() - kSheafLabelCommandCount);
                }

                // The label band: everything below the ring's own
                // sub-extent (`ringExtent.height`), i.e. exactly the added
                // `kLabelBandHeight` strip -- by construction this can never
                // reach up into `ringExtent`'s own [0, ringExtent.height)
                // span, which is the whole of what `BuildEncoderDrawCommands`
                // was given to draw into (T4.3(b)'s guarantee).
                const float bandTop = ringExtent.height;
                const float bandHeight = std::max(0.0f, extent.height - bandTop);
                const float plateWidth = extent.width * 0.94f;
                const float plateLeft = (extent.width - plateWidth) * 0.5f;
                const synth::ui::Bounds rowBounds{plateLeft, bandTop, plateWidth, bandHeight};

                const synth::Color cellColor = state.baseColor;
                const synth::Color onColor = synth::Brighten(cellColor, 0.45f);
                const synth::Color offColor = synth::Color::Rgb(36, 40, 42);
                // Same plate colour AppendBadge already uses for its own
                // chip background (EncoderDraw.hpp:595) -- one opaque plate
                // behind the row, drawn before it.
                const synth::Color plateColor = synth::Color::Rgb(32, 34, 36);
                commands.push_back(synth::ui::DrawCommand::FillRoundedRect(
                    rowBounds, bandHeight * 0.15f, plateColor));

                // T4.1: labels.md is the authority for what RENDERS here,
                // not `FroggersParamSpec::name`/`shortName`
                // (FroggersApprovedLabels()'s own header comment). Slots
                // 14/15 (Crispy/Crunchy) have no page spec, so they read
                // `FroggersApprovedGlobalLabel` instead. A modulation
                // drill-in view ALSO substitutes a DIFFERENT Parameter into
                // this same physical slot index (Bank::OpenModulationView,
                // ParameterModulation.cpp:2648/2813 -- BankSlot::
                // PopulateUIState's cells[] then reflects that substituted
                // parameter, not the bank's own page layout; this is
                // exactly why this file's own AppendEncoderCell header
                // comment says this loop must NOT re-derive slot->parameter
                // from FroggersBankLayouts()) -- looking the approved list
                // up by (bank,slot) in that case would silently mislabel
                // the substituted control, and that substituted parameter
                // is outside labels.md's 86-entry scope in any case. This
                // one case falls back to the live `state.shortLabel`
                // (EncoderDrawStateFromParameter's own source), unchanged
                // from before this task.
                std::string approvedLabel;
                if (!showingModulationView && bankIx < kFroggersBankCount) {
                    approvedLabel = (ix < kFroggersParamsPerBank) ? FroggersApprovedLabels()[bankIx][ix]
                                                                   : FroggersApprovedGlobalLabel(ix);
                } else {
                    approvedLabel = state.shortLabel;
                }
                std::vector<synth::ui::DrawCommand> rowCommands = BuildEncoderLabelRowCommands(
                    approvedLabel, rowBounds, onColor, offColor, kApprovedLabelGridColumns);
                commands.insert(commands.end(), rowCommands.begin(), rowCommands.end());

                return commands;
            },
            cellStyle);
    }

    // Row 7 (STEP 1 renumbered this from 6 -- see AppendModulationHeaderRow()'s
    // own comment): Randomize Page | Randomize All, each spanning 2 of the 4
    // encoder columns (`Extent::Weight(2)`, matching the encoder rows'
    // per-column `Weight(1)` unit so the two rows visually align).
    //
    // Moved here from the old bank-header group by FroggersCellMap -- row 1
    // is bank tabs only now (AppendBankTabsRow() above); Randomize Page and
    // Randomize All sit together in the last row.
    // Exactly one Randomize All control exists anywhere in this surface.
    // The ONE two-half-width-buttons row builder. AppendResetRow began as a
    // name-for-name copy of AppendRandomizeRow -- byte-identical after
    // substitution, i.e. duplicated code. Both rows now route through this
    // one builder instead, which prevents that duplication from recurring.
    // The operator-facing property ("same size" halves) is now structural:
    // the two rows cannot drift apart.
    void AppendTwoButtonRow(synth::ui::Builder& builder, const char* rowId,
                            const char* leftId, const char* leftLabel, const char* leftAction,
                            const char* rightId, const char* rightLabel, const char* rightAction) const {
        synth::ui::LayoutOptions rowLayout;
        // T4.2: `kUnchangedRowHeight`, same reasoning as AppendBankTabsRow
        // above -- Randomize and Reset (this method's two callers) both
        // stay pixel-identical to today at the default window.
        rowLayout.main = synth::ui::Extent::Weight(FroggersEncoderGridLayout::kUnchangedRowHeight);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersEncoderGridLayout::kGap;
        builder.Row(rowId, rowLayout, [&](synth::ui::Builder& b) {
            synth::ui::ControlStyle leftStyle{};
            leftStyle.layout.main = synth::ui::Extent::Weight(2.0f);
            leftStyle.layout.cross = synth::ui::Extent::Intrinsic();
            b.Button(leftId, leftLabel, synth::ui::Action::Named(leftAction), leftStyle);

            synth::ui::ControlStyle rightStyle{};
            rightStyle.layout.main = synth::ui::Extent::Weight(2.0f);
            rightStyle.layout.cross = synth::ui::Extent::Intrinsic();
            b.Button(rightId, rightLabel, synth::ui::Action::Named(rightAction), rightStyle);
        });
    }

    void AppendRandomizeRow(synth::ui::Builder& builder) const {
        AppendTwoButtonRow(builder, FroggersNodeIds::kRandomizeRow,
                           FroggersNodeIds::kRandomizePage, "Randomize Page", FroggersActions::kRandomizePage,
                           FroggersNodeIds::kRandomizeAll, "Randomize All", FroggersActions::kRandomizeAll);
    }

    // T5.1 (operator: "below them, same size"): Row 7, directly below
    // Randomize. Deliberately the SAME shape as AppendRandomizeRow above --
    // row Weight(1), two Buttons at Weight(2) of four weight-units, i.e. two
    // equal halves, same gap -- because "same size" is defined by that row,
    // not re-derived. Reset is Randomize's exact inverse in scope, so the
    // two rows should also be each other's visual twin.
    void AppendResetRow(synth::ui::Builder& builder) const {
        AppendTwoButtonRow(builder, FroggersNodeIds::kResetRow,
                           FroggersNodeIds::kResetPage, "Reset Page", FroggersActions::kResetPage,
                           FroggersNodeIds::kResetAll, "Reset All", FroggersActions::kResetAll);
    }

    bool BankSelected(std::size_t bankIx) const {
        if (context_ == nullptr || context_->uiState == nullptr || bankIx >= context_->uiState->bankCapacity) {
            return bankIx == 0;
        }
        return context_->uiState->banks[bankIx].selected.load(std::memory_order_relaxed);
    }

    // Which bank (index into FroggersBankLayouts(), same order banks are
    // created in -- FroggersParameters.hpp's Init() loop -- and the same
    // order `uiState->banks[]` is populated in, ParameterModulation.cpp:
    // 3403-3406/3716-3727 push_back/populate in lockstep) is currently
    // selected, for AppendEncoderCell's label-source lookup below. Same
    // default (0) BankSelected() above already uses when uiState isn't
    // ready yet.
    std::size_t CurrentBankIndex() const {
        return context_ == nullptr ? 0 : FroggersVisibleBankIndex(*context_);
    }

    void HandleAction(const synth::ui::Action& action) {
        if (app_ == nullptr) {
            return;
        }

        // Generic, safe over the existing uiBus (see this file's header
        // comment): transport, scene select/blend, encoder drag.
        if (action.name == FroggersActions::kPlay) {
            // T7.7 (operator 2026-08-17): Play disarms the Freeze latch, for
            // the same reason kStop does (T7.2) -- and it is arguably more
            // urgent here. A latched Freeze holds the voice gate OPEN
            // unconditionally (FroggersAppCore's `setGate(gateOpen ||
            // FreezeLatched())`), so starting the transport with the latch
            // still engaged would run the sequencer while every voice was
            // pinned sustaining, and leave the delay frozen at its latch
            // overdrive -- Play would not actually return the instrument to
            // playing. Cleared BEFORE the Start push, the same happens-before
            // ordering kStop's own comment traces.
            app_->SetFreezeLatched(false);
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
            // Disarm the latch BEFORE pushing Stop, not after -- the two
            // reach the audio
            // thread by different paths (freezeLatched_ is a plain
            // release/acquire atomic FreezeLatched() re-reads every sample;
            // MessageIn::Stop travels through MessageInBus's ring buffer,
            // drained once per block by Engine::DrainMessageBus() BEFORE
            // FroggersAppCore::ProcessBlock's per-sample transport-edge
            // check runs). Writing the latch first in THIS thread's program
            // order, before the Push() whose internal fetch_add is the
            // release half of that ring buffer's release/acquire pair,
            // makes the write happen-before the corresponding acquire load
            // in MessageInBus::Pop -- and therefore happen-before every
            // later read on the audio thread this sample, including the
            // FreezeLatched() read TransportTeardownActive() makes on the
            // very edge this Stop causes. So there is no window where the
            // audio thread can observe "stopped AND still latched": by
            // construction, not by timing luck. (Reversing the two calls
            // would still very likely land the latch write in time in
            // practice -- an atomic store completes far faster than a
            // message surviving a full block's round trip -- but "very
            // likely" is not the guarantee TransportTeardownActive()'s
            // silence contract needs, so this order is deliberate, not
            // cosmetic.)
            app_->SetFreezeLatched(false);
            PushMessage(synth::MessageIn::Stop(NowMicros()));
            app_->SetDesiredTransportRunning(false);
            return;
        }
        if (action.name == FroggersActions::kFreeze) {
            // Freeze is self-contained: engaging it stops the transport
            // itself (spec.md's "The Freeze button alone reaches the
            // sustained drone" scenario), so the drone needs no separate
            // Stop press and a later Stop always means stop (see the kStop
            // branch above).
            //
            // ENGAGE (latch false -> true): push MessageIn::Stop and call
            // SetDesiredTransportRunning(false), exactly as the kStop branch
            // above -- including SetDesiredTransportRunning, which is not
            // optional bookkeeping (FroggersAppCore::PrepareToPlay's own D17
            // comment: without it, a later audio-device renegotiation could
            // re-assert Start behind the app's back, silently restarting the
            // transport under a still-latched Freeze). The latch write is
            // ordered before the Stop push here for the same happens-before
            // reason the kStop branch's own comment traces -- here the
            // property it buys is the mirror image: the audio thread must
            // see the latch ALREADY true on the very edge that stops the
            // transport, so TransportTeardownActive() reads false and
            // suppresses teardown (sustains) instead of silencing.
            //
            // RELEASE (latch true -> false): do NOT start the transport --
            // the operator resumes with Play, not by releasing Freeze. No
            // MessageIn is pushed on release; FroggersAppCore's existing
            // "latch released while already stopped" edge
            // (`latchReleasedWhileStopped`, FroggersAppCore.hpp) is what
            // notices the plain atomic flip and runs the teardown that
            // silences the held drone.
            const bool engaging = !app_->FreezeLatched();
            app_->SetFreezeLatched(engaging);
            if (engaging) {
                PushMessage(synth::MessageIn::Stop(NowMicros()));
                app_->SetDesiredTransportRunning(false);
            }
            return;
        }
        if (action.name == FroggersActions::kRecord) {
            // Unlike Freeze's plain latch flip, Record can REFUSE (transport
            // stopped -- FroggersAppCore::
            // ArmRecording's own comment) and produces a result the host
            // cares about (captured audio to export), so both outcomes are
            // surfaced through the host-facing callbacks
            // (SetOnRecordRefused/SetOnRecordingFinished, FroggersAppCore.hpp)
            // rather than a silent state flip -- see this file's own
            // HandleAction() header comment: this is still a direct
            // message-thread call, same as kFreeze above, not the
            // Request*/pending*_ bridge the encoder/randomize/BPM actions
            // below use.
            if (!app_->RecordArmed()) {
                if (!app_->ArmRecording()) {
                    app_->NotifyRecordRefused(app_->RecordRefusalReason());
                }
                return;
            }
            app_->StopRecording();
            if (app_->RecordedFrameCount() > 0) {
                app_->NotifyRecordingFinished();
            }
            return;
        }
        if (action.name == FroggersActions::kSceneSelect) {
            // Scene 1/Scene 2 now toggle the blend to its extremes rather
            // than reassigning a stored-scene endpoint. Verified:
            // FroggersParameters.hpp wires `manager.SetSceneEndpoints(0, 1)`
            // once at Init() (fixed for this app's lifetime), and
            // Parameter::ComputeRawCenter's blend arithmetic
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
            // action.value now carries the DISPLAYED 1.0-2.0 reading (see
            // AppendSceneBlendGroup()'s `b.Slider(...)` and
            // kSceneBlendDisplayOffset's own comment above), so the fallback
            // fed to FroggersParseFloat must be in that same displayed
            // domain -- kSceneBlendDisplayOffset (the bottom of the
            // displayed range, i.e. "no offset applied") rather than the
            // pre-offset 0.0f this used to pass. 0.0f would be a
            // displayed-domain value that only happened to land on the
            // right blend (0.0, Scene 1's extreme) after the subtract-and-
            // clamp below coincidentally rescued it; kSceneBlendDisplayOffset
            // is the value that is actually correct in the domain being
            // parsed, not merely one that clamps to the same place. Subtract
            // the offset back out and clamp to [0,1]: the message must still
            // carry Sheaf's own 0..1 blend, and a slider edge value (or a
            // malformed action) must never push an out-of-range blend.
            const float displayed = FroggersParseFloat(action.value, kSceneBlendDisplayOffset);
            const float blend = std::clamp(displayed - kSceneBlendDisplayOffset, 0.0f, 1.0f);
            PushMessage(synth::MessageIn::SetSceneBlend(NowMicros(), blend));
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
        // removed operator 2026-07-27 -- see this file's header comment.)
        if (action.name == FroggersActions::kEncoderPress) {
            app_->RequestEncoderPress(FroggersParseSize(action.value, 0));
            return;
        }
        if (action.name == FroggersActions::kBankSelect) {
            app_->RequestBankSelect(FroggersParseSize(action.value, 0));
            return;
        }
        // The carousel arrows route through the SAME single selection
        // authority as the bank buttons above (RequestBankSelect -- no
        // second selection state).
        // GATED on DrillLevel() == 0, the same source
        // AppendModulationHeaderRow reads to decide whether to emit the
        // arrow nodes at all: HandleAction matches on action NAME with no
        // node-presence check, so without this gate a synthetic dispatch
        // while drilled would still switch banks and, via the ProcessFrame
        // drain reconstructing drillIn_ on any bank change
        // (FroggersAppCore.hpp:627-641), silently exit the drill -- even
        // though no arrow node exists in the tree to click.
        if (action.name == FroggersActions::kBankPrevious) {
            if (app_->DrillLevel() == 0) {
                const std::size_t bankIx =
                    (CurrentBankIndex() + kFroggersBankCount - 1) % kFroggersBankCount;
                app_->RequestBankSelect(bankIx);
            }
            return;
        }
        if (action.name == FroggersActions::kBankNext) {
            if (app_->DrillLevel() == 0) {
                // No `+ kFroggersBankCount` term here, unlike kBankPrevious
                // above: this is a plain addition of two non-negative
                // std::size_t values, which cannot underflow, so there is no
                // borrow to guard against the way the subtraction above has.
                const std::size_t bankIx = (CurrentBankIndex() + 1) % kFroggersBankCount;
                app_->RequestBankSelect(bankIx);
            }
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
        if (action.name == FroggersActions::kResetAll) {
            app_->RequestResetAll();
            return;
        }
        if (action.name == FroggersActions::kResetPage) {
            app_->RequestResetPage();
            return;
        }
        if (action.name == FroggersActions::kBpm) {
            // Belt-and-suspenders -- the slider itself renders as a
            // non-interactive StatusText while slaved (AppendBpmControl(),
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
    // See SetPluginHostMode()'s own comment. Defaults false, so every
    // existing construction of this class -- default constructed, this flag
    // never touched -- renders exactly as before.
    bool pluginHostMode_ = false;
    mutable std::uint64_t fallbackTimestamp_ = 1;
};

}  // namespace synth_froggers

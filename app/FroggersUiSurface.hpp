#pragma once

// synth_froggers::FroggersUiSurface -- packet 10 of the froggers-sheaf-app
// change (openspec/changes/froggers-sheaf-app/tasks.md, section "10. Surface
// layout (ported v2 design)"; design D9/D9a/D9b/D11/D14/D17;
// spec `specs/froggers-app-surface-layout/spec.md`), re-architected under
// `openspec/changes/frogg3rs-audio-safety-and-ui-rework/tasks.md` task F.3
// (2026-08-04/05) to adopt Sheaf's portable layout engine
// (`PortableUILayout.hpp`/`PortableUIMetrics.hpp`, pin `77a3019e`) instead of
// this file's own hand-rolled pixel arithmetic.
//
// TOPOLOGY DIRECTIVE (operator, 2026-08-03): "sheaf is the guide for classes
// but froggers is the guide for topology." This surface is now declared as
// ONE grid -- encoders and chrome alike are grid citizens with cell
// positions, not a scope/grid region plus a separately auto-flowed chrome
// band. `FroggersCellMap` below is the topology, expressed as data (the
// operator-approved 6-row-by-6-column table, tasks.md F.3 CELL MAP); the
// mechanism -- how a cell becomes real geometry -- is the Sheaf idiom
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
// app's topology is the operator's, not Braid4's.
//
// Window size: OPERATOR DECISION 2026-08-05, route 2a. The surface still
// resolves against `context->config->uiWidth/uiHeight` (a fixed, compiled-in
// size, unchanged from before this task) rather than a live window extent --
// making the layout track the ACTUAL window requires an upstream shell
// change (filed as ask 15: `RuntimeMainComponent::BuildTree()` composes the
// sidebar from `App::Config().uiWidth`, not a live extent, so a resizable
// surface here would desync from it). Everything internal to this surface is
// nonetheless fully declarative, so adopting a live extent later is a change
// to `FroggersPageLayout::RootBounds()`'s source, not a redesign.
//
// Bounds note (still true): `synth::ui::Builder`'s Button/Slider/Toggle/
// ComboBox/TextField/StatusText node kinds take no explicit `Bounds` --
// placement comes entirely from each node's own `LayoutOptions` (`main`/
// `cross` extents), resolved by the engine against whatever region its
// container was given. Every region this surface used to compute a pixel
// `Bounds` for by hand -- the scope panel, the encoder grid, Play/Stop's
// plates -- is now an in-flow cell with a declared `LayoutOptions` instead;
// the sole exception is the transport plates' own fixed `Extent::Px(28)`
// size (unchanged from before this task, see `kTransportPlateSize`), which
// was already a `LayoutOptions`-expressed size, not an `explicitBounds`
// out-of-flow declaration.
//
// Crunchy was removed from a dedicated chrome slider entirely (operator
// 2026-07-27: "why is there a fucking slider for crunchy... i never asked
// for that. It duplicates bank slot 15"). Crunchy is reachable only via the
// encoder grid's slot 15, addressed exactly like any other bank parameter --
// see design.md D11/Resolved-decisions and tasks.md 10.2 for the recorded
// trade-off (Crunchy is unreachable while a modulation view is open, since
// slot 15 is then Target/Back). Crunchy (slot 15) is GLOBAL -- one shared
// `Parameter` aliased into all six banks (`FroggersParameters.hpp:79-80,
// 191,251-256,342-366`) carrying its own fixed Yellow rather than the bank
// colour, and excluded from drill-in/randomize dispatch
// (`FroggersModulation.hpp:120-126`). That colour already flows through
// `Parameter::UIState.color` into `EncoderDrawStateFromParameter` with no
// special-casing needed here -- this file's one encoder-cell code path
// renders slot 14 (Crispy, per-bank colour) and slot 15 (Crunchy, fixed
// Yellow) identically; the colour difference is data, not branching.
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
// The single outer split Row (left block | right block, tasks.md F.3 CELL
// MAP) and the two blocks themselves.
inline constexpr const char* kLayoutRoot = "froggers.layout.root";
inline constexpr const char* kLeftBlock = "froggers.layout.left";
inline constexpr const char* kRightBlock = "froggers.layout.right";

inline constexpr const char* kPlay = "froggers.transport.play";
inline constexpr const char* kStop = "froggers.transport.stop";
// Row 3 of the left block (tasks.md F.3 CELL MAP): Play | Stop.
inline constexpr const char* kTransportRow = "froggers.layout.left.transport";
// Row 4 of the left block: Scene 1 | Scene 2.
inline constexpr const char* kScenesRow = "froggers.layout.left.scenes";

inline constexpr const char* kRandomizeAll = "froggers.randomize.all";
inline constexpr const char* kRandomizePage = "froggers.randomize.page";
// Row 6 of the right block: Randomize Page | Randomize All (moved out of the
// bank-header group by the CELL MAP -- see AppendRandomizeRow()'s comment).
inline constexpr const char* kRandomizeRow = "froggers.layout.right.randomize";
// Row 1 of the right block: the six bank-select tabs.
inline constexpr const char* kBankTabsRow = "froggers.layout.right.banks";

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
// right block, tasks.md F.3 CELL MAP): row 0 = slots 0-3, row 1 = slots 4-7,
// etc.
inline std::string EncoderRow(std::size_t row) {
    return "froggers.layout.right.row." + std::to_string(row);
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

// Task 10.1/F.3: the surface's own extent and design tokens. Everything that
// used to compute a pixel `Bounds` for the scope/grid regions by hand (task
// F.3's deletion table: `ContentArea`/`RequiredHeight`/`ScopeArea`/
// `GridArea`/`FroggersAutoFlowedChromeModel`, all removed) is gone -- that
// arithmetic DIES, but the tokens and the historical ratio it enforced
// SURVIVE AS DATA below, either still consumed (kMargin/kGap, as the outer
// grid's own padding/gap) or preserved as the documented baseline
// FroggersSurfaceTests.cpp's ratio guard checks the RESOLVED layout against
// (kScopeWidth/kScopeHeight -- nothing here computes pixels from them any
// more, but they remain this file's one definition of "the operator's
// original scope proportions" rather than a duplicate literal in the test).
struct FroggersPageLayout {
    static constexpr float kDefaultWidth = 900.0f;
    // Replaces the old `RequiredHeight()`-derived fallback: a plain literal,
    // matching `FroggersAppCore::Config().uiHeight` (see that file's own
    // comment -- demoted from a derived cross-check to an initial window
    // size, task F.3's "config.uiHeight is NOT deleted" precision).
    static constexpr float kDefaultHeight = 632.0f;

    // The outer split Row's own padding (inset from the window edge) and the
    // gap between the left/right blocks and between each block's own stacked
    // rows.
    static constexpr float kMargin = 16.0f;
    static constexpr float kGap = 14.0f;

    // Historical operator-mandated scope proportions (tasks.md B.1,
    // 2026-07-29, the operator's strongest complaint: "it is taller than it
    // is wide... it should be at most a third of its current size"). The
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
    // resolved width and keeps working if that ever changes -- the upstream
    // reflow of ask 15, or the deferred mobile topology (tasks.md section H).
    // A pixel width would have to be re-tuned for each.
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

// Task 10.3 (design D5a/D11): the 16-slot grid topology, slots 0-15 laid out
// 4x4 -- `kColumns`/`kRows`/`kEncoderCount` are the slot topology
// (static_assert-tied to `kFroggersSlotsPerBank`) and SURVIVE task F.3
// unchanged; `BoundsForIndex`'s pixel division DIED with it (cells are now
// in-flow grid cells the layout engine sizes, see AppendEncoderRow() below),
// but the row/column mapping it embodied (`ix / kColumns`, `ix % kColumns`)
// survives as the loop shape AppendEncoderGrid() below walks.
struct FroggersEncoderGridLayout {
    static constexpr std::size_t kColumns = 4;
    static constexpr std::size_t kRows = 4;
    static constexpr std::size_t kEncoderCount = kColumns * kRows;
    // The gap between encoder cells within a row and between encoder rows --
    // its own distinct structural role vs. `FroggersPageLayout::kGap`, which
    // separates the left/right blocks and each block's own top-level rows.
    static constexpr float kGap = 8.0f;
};

static_assert(FroggersEncoderGridLayout::kEncoderCount == kFroggersSlotsPerBank,
              "the grid must render exactly the 16 physical encoder slots FroggersParameterModel wires up");

// The operator-approved 6-column x 6-row topology (tasks.md F.3 CELL MAP,
// 2026-08-04/05), kept as PURE DATA -- no builder calls, no layout math --
// separate from the emission code that interprets it (AppendLeftBlock()/
// AppendRightBlock() below). This is what a future mobile (tasks.md §H) or
// VST (§I) topology would replace with a DIFFERENT table consumed by
// analogous emission code, without forking this surface (§8: one definition
// site for "what goes where").
//
//   Row | L1                          | L2  | E1 E2 E3 E4
//   1   | Scope (spans L1-L2, rows1-2)| <-  | Bank tabs x6 (span E1-E4)
//   2   | (scope)                     | <-  | slot 0 | slot 1 | slot 2 | slot 3
//   3   | Play                        | Stop| slot 4 | slot 5 | slot 6 | slot 7
//   4   | Scene 1                     | Scene 2 | slot 8 | slot 9 | slot 10 | slot 11
//   5   | Scene blend (label below)   | <-  | slot 12 | slot 13 | slot 14 CRIS | slot 15 CRNC
//   6   | BPM (label below)           | <-  | Randomize page (span 2) | Randomize all (span 2)
// Rows 5 and 6 are the same shape on purpose (task F.6): both labels below,
// both sliders one declared width. The CELL MAP originally had row 6's label
// trailing, per B12; F.6 superseded that -- see AppendLabelledSlider().
struct FroggersCellMap {
    enum class LeftKind { Scope, Transport, Scenes, SceneBlend, Bpm };
    enum class RightKind { BankTabs, EncoderRow, Randomize };

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

    static constexpr std::array<RightRow, 6> kRightRows = {{
        {RightKind::BankTabs, 0},
        {RightKind::EncoderRow, 0},
        {RightKind::EncoderRow, 4},
        {RightKind::EncoderRow, 8},
        {RightKind::EncoderRow, 12},
        {RightKind::Randomize, 0},
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
// Task 3.8 (operator 2026-07-28, "look like shit from a butt" -- geometry,
// not concept): the icon is inset to a fixed FRACTION of the plate rather
// than a fixed pixel amount so it scales with the square and lands at
// ~55-60% of the plate with even padding on all sides, and the plate uses
// Sheaf's own chrome "primary" button colour (`ButtonColourForNode`'s
// variant=="primary" branch, PortableJuceBackend.hpp:1130-1148, RGB
// 57/106/127) instead of stark white so it sits in the dark instrument face
// instead of glaring out of it.
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

        synth::ui::Builder builder;
        builder.Root(FroggersNodeIds::kRoot, root);
        // Task 3.4 (operator 2026-07-28): the on-canvas "Frogg3rs Synth"
        // title label is removed -- `config.appName`
        // (FroggersAppCore.hpp:135) and `FroggersManifest().displayName`
        // (FroggersRegistration.hpp:22) already cover launcher/window-title
        // naming. The freed space is left for a future logo (design E3f,
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
    // -- Left block (tasks.md F.3 CELL MAP, columns L1-L2) ------------------

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

    // Task 10.2: the packet 7-9 VCO scope panel. Its bounds are not known
    // until the layout resolves (it is now an in-flow, weight-sized cell,
    // not a hand-computed pixel rectangle), so the DrawFactory form is used
    // exactly like Braid4UI.hpp's own encoder-visualizer-underlay pattern:
    // the factory receives the RESOLVED extent and sets it on the
    // visualizer at that point.
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
                         std::vector<synth::ui::DrawCommand> commands = vcoScope.Draw();
                         // F7 (operator request, openspec/changes/
                         // frogg3rs-blowout-and-drilldown-repair/tasks.md):
                         // "when we are in modulation drilldown levels ...
                         // headers ... 'Modulation Level 1' then 2 then 3."
                         // Level 0 (the parameter grid) shows nothing; levels
                         // 1+ show the label, DERIVED from
                         // FroggersModulationDrillIn::Level() (DrillLevel()'s
                         // own comment) -- never a hardcoded level-name set,
                         // since the cap has already moved once (F5.3, 2->3)
                         // and a hardcoded set would need a second edit next
                         // time it does (OMNI §8).
                         //
                         // Emitted as one more DrawCommand on this EXISTING
                         // node rather than a new grid node or row: the CELL
                         // MAP (tasks.md F.3) is a settled, operator-approved
                         // 6x6 topology with no spare row/column weight to
                         // give a header its own cell, and every other row
                         // stays live during drill-in (bank tabs still
                         // switch banks; Randomize All/Page still work per
                         // F6.2; transport/scene/BPM stay meaningful while
                         // inspecting modulation) -- overlaying any of them
                         // would either shadow a working control or silently
                         // change what drill-in does to it, neither of which
                         // this task asked for. kVcoScope is the one
                         // non-interactive, Draw-only cell in the whole grid
                         // (no action, nothing to shadow), so appending to
                         // its own draw output is the only place a header
                         // can appear without perturbing the grid.
                         const std::size_t drillLevel = app->DrillLevel();
                         if (drillLevel > 0) {
                             commands.push_back(synth::ui::DrawCommand::Text(
                                 synth::ui::Bounds{4.0f, 4.0f, std::max(0.0f, extent.width - 8.0f), 18.0f},
                                 "Modulation Level " + std::to_string(drillLevel), synth::ui::TextStyle{}));
                         }
                         return commands;
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
        builder.Row(FroggersNodeIds::kTransportRow, rowLayout, [](synth::ui::Builder& b) {
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
        });
    }

    // Row 4: Scene 1 | Scene 2, each taking half the row (Weight(1), an
    // intrinsic cross size so the button does not stretch to the row's full
    // resolved height).
    //
    // Task 3.5 (operator 2026-07-28, design E3d): "Scene 1"/"Scene 2" are a
    // TOGGLE between the scene-blend extremes, not a re-assignment of which
    // stored scene occupies the less-weighted endpoint -- see HandleAction()
    // below for the full trace (kept unchanged by task F.3).
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

    // Row 5: the Scene-blend slider with its label BELOW it -- the CELL
    // MAP's one amendment to the operator-approved table (tasks.md F.3,
    // 2026-08-04): "the Scene blend label sits BELOW its slider. This
    // supersedes the F.2d caption for scene-blend (a `ControlStyle::caption`
    // can only lead, so scene-blend returns to a hand-rolled label, now
    // placed under the slider)." Row 6 now has the identical shape -- task
    // F.6 (2026-08-05) moved BPM's label below too, superseding B12; see
    // AppendLabelledSlider() above for why that honours B12 rather than
    // overriding it.
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
    // ROWS 5 AND 6 SHARE THIS ONE EMITTER (task F.6, tasks.md F.6-PLAN).
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
                b.Slider(FroggersNodeIds::kSceneBlend, "Scene blend", sceneBlend, 0.0f, 1.0f, 0.001f,
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

    // Task 10.6 (design cited MasterClock.hpp:318/:321, MasterClock.cpp:
    // 963-965/:1182): read-only/inert (a StatusText) while slaved to
    // external MIDI clock, an interactive Slider otherwise. Both states
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
        // Task 3.6 (design E3e): the control genuinely IS labelled "BPM"
        // (UI-rework ITEM 5, design.md A3f, tasks.md B.5, 2026-07-29 -- the
        // transport-state-dependent "(no effect while stopped)" annotation
        // was never requested and is not to be reintroduced without asking
        // first).
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

    // -- Right block (tasks.md F.3 CELL MAP, columns E1-E4) -----------------

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
            case FroggersCellMap::RightKind::EncoderRow:
                AppendEncoderRow(builder, row.firstEncoderIndex);
                return;
            case FroggersCellMap::RightKind::Randomize:
                AppendRandomizeRow(builder);
                return;
        }
    }

    // Row 1: the six bank-select tabs, LOOPED from `FroggersBankLayouts()`
    // (single source of truth for bank identity/order,
    // app/FroggersParameters.hpp) -- OMNI §8, not a second hand-written list.
    //
    // Task 3.1/6.3 (operator 2026-07-28): plain `Button` nodes with the
    // action supplied directly (Draw/DrawInteractive nodes dispatched only
    // on double-click at the pin then current; reverted for single-click bank
    // switching). F.2c (2026-08-03, pin 77a3019e): `node.selected` for the
    // active bank now comes from `ControlStyle::selected`.
    void AppendBankTabsRow(synth::ui::Builder& builder) const {
        synth::ui::LayoutOptions rowLayout;
        rowLayout.main = synth::ui::Extent::Weight(1.0f);
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

    // Rows 2-5: the 16-slot grid, 4 slots per row -- LOOPED over
    // `FroggersEncoderGridLayout::kColumns`, the row/col mapping
    // (`firstEncoderIndex / kColumns` below, `ix / kColumns`/`ix % kColumns`
    // in spirit) surviving task F.3's deletion of `BoundsForIndex`'s pixel
    // division.
    void AppendEncoderRow(synth::ui::Builder& builder, std::size_t firstEncoderIndex) const {
        const std::size_t row = firstEncoderIndex / FroggersEncoderGridLayout::kColumns;
        synth::ui::LayoutOptions rowLayout;
        rowLayout.main = synth::ui::Extent::Weight(1.0f);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersEncoderGridLayout::kGap;
        builder.Row(FroggersNodeIds::EncoderRow(row), rowLayout, [this, firstEncoderIndex](synth::ui::Builder& b) {
            for (std::size_t column = 0; column < FroggersEncoderGridLayout::kColumns; ++column) {
                AppendEncoderCell(b, firstEncoderIndex + column);
            }
        });
    }

    // Task 10.3/10.4/10.5: one encoder cell. Reads the SAME
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
    void AppendEncoderCell(synth::ui::Builder& builder, std::size_t ix) const {
        const bool showingModulationView =
            context_ != nullptr && context_->uiState != nullptr && context_->uiState->slotCapacity > 0 &&
            context_->uiState->slots[0].showingModulationView.load(std::memory_order_relaxed);

        synth::ui::EncoderDrawState state{};
        synth::ui::Visualizer* visualizer = nullptr;
        if (context_ != nullptr && context_->uiState != nullptr && context_->uiState->slotCapacity > 0) {
            const synth::BankSlot::UIState& slotState = context_->uiState->slots[0];
            if (ix < slotState.cellCapacity) {
                // Design D9a/task 10.5: EncoderDrawStateFromParameter reads
                // only `Parameter::UIState.values[]` (the post-fuego,
                // post-modulation published display center) -- never
                // `.rawKnobValue`.
                state = synth::ui::EncoderDrawStateFromParameter(slotState.cells[ix]);
                visualizer = slotState.cells[ix].visualizer.load(std::memory_order_relaxed);
            }
        }
        const bool hidden = showingModulationView && !state.connected;
        state.hasVisualizerUnderlay = !hidden && visualizer != nullptr && visualizer->Visible();

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
        builder.Draw(
            encoderId,
            [state, hidden](synth::ui::Bounds extent) {
                return hidden ? std::vector<synth::ui::DrawCommand>{} : synth::ui::BuildEncoderDrawCommands(state, extent);
            },
            cellStyle);
    }

    // Row 6: Randomize Page | Randomize All, each spanning 2 of the 4
    // encoder columns (`Extent::Weight(2)`, matching the encoder rows'
    // per-column `Weight(1)` unit so the two rows visually align).
    //
    // Task 10.7 (design D11/D14): moved here from the old bank-header group
    // by the CELL MAP -- row 1 is bank tabs only now (AppendBankTabsRow()
    // above); Randomize Page and Randomize All sit together in row 6.
    // Exactly one Randomize All control exists anywhere in this surface
    // (task 10.2's constraint, unchanged).
    void AppendRandomizeRow(synth::ui::Builder& builder) const {
        synth::ui::LayoutOptions rowLayout;
        rowLayout.main = synth::ui::Extent::Weight(1.0f);
        rowLayout.cross = synth::ui::Extent::Weight(1.0f);
        rowLayout.padding = 0.0f;
        rowLayout.gap = FroggersEncoderGridLayout::kGap;
        builder.Row(FroggersNodeIds::kRandomizeRow, rowLayout, [](synth::ui::Builder& b) {
            synth::ui::ControlStyle pageStyle{};
            pageStyle.layout.main = synth::ui::Extent::Weight(2.0f);
            pageStyle.layout.cross = synth::ui::Extent::Intrinsic();
            b.Button(FroggersNodeIds::kRandomizePage, "Randomize Page",
                     synth::ui::Action::Named(FroggersActions::kRandomizePage), pageStyle);

            synth::ui::ControlStyle allStyle{};
            allStyle.layout.main = synth::ui::Extent::Weight(2.0f);
            allStyle.layout.cross = synth::ui::Extent::Intrinsic();
            b.Button(FroggersNodeIds::kRandomizeAll, "Randomize All",
                     synth::ui::Action::Named(FroggersActions::kRandomizeAll), allStyle);
        });
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
            // its extremes rather than reassigning a stored-scene endpoint.
            // Verified: FroggersParameters.hpp wires
            // `manager.SetSceneEndpoints(0, 1)` once at Init() (fixed for
            // this app's lifetime), and
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
        // removed operator 2026-07-27 -- see this file's header comment.)
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
            // a non-interactive StatusText while slaved (AppendBpmControl(),
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

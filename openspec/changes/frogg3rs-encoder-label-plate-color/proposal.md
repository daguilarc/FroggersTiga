# Proposal — `frogg3rs-encoder-label-plate-color`

**Created 2026-08-30.** The strip behind every encoder label ("VCO1",
"SHAPE 2", …) renders lighter than everything around it. The knobs sit
seamlessly on the app background; the label plates do not.

## The mechanism, traced

The encoder cells are `Draw` nodes. Below each ring the app paints an opaque
rounded plate and then 14-segment glyphs:

| value | where | what it is |
|---|---|---|
| `Rgb(32, 34, 36)` | `app/FroggersUiSurface.hpp:2039` | the label plate |
| `Rgb(36, 40, 42)` | `app/FroggersUiSurface.hpp:2035` | unlit ghost segments |
| `Rgb(18, 20, 22)` | `External/Sheaf/.../juce/PortableJuceBackend.hpp:266` | root background `fillAll` |
| `Rgb(18, 20, 22)` | `PortableJuceBackend.hpp:275` | default root fill when the model carries none |
| `Rgb(18, 20, 22)` | `External/Sheaf/.../include/synth/EncoderDraw.hpp:680` | the encoder cell's own fill (`Rgba(…,150)` over a visualizer underlay) |
| `Rgb(32, 34, 36)` | `EncoderDraw.hpp:595` | Sheaf's badge chip background — ON the knob |

The knob cells match the root exactly — `18,20,22` on `18,20,22` — which is
why everything else looks seamless. The plate's own comment says where its
colour came from: "Same plate colour AppendBadge already uses for its own chip
background (EncoderDraw.hpp:595)". A chip that sits on the dark knob was the
wrong precedent for a band that sits flush on the root background: 14 points
lighter per channel, which is exactly the visible strip.

The browser shows the same strip because the plate colour travels inside the
draw commands the shared C++ emits; both backends paint what the command
carries.

## The plate is load-bearing — the colour is the defect, not the plate

Visualizer underlays attach with `overlayOf = encoderId`
(`app/FroggersUiSurface.hpp:1853-1855`, resolved by
`PortableUILayout.hpp:672-683`), i.e. to the FULL cell bounds — and the cell
is `kLabelBandHeight` taller than the ring. On any cell with a visible
visualizer the trace runs under the label band, so an opaque plate is what
keeps the glyphs legible. Removing it is not an option; recolouring it is the
whole fix.

## Why abstraction, not a matched literal

The background is one concept currently spelled as a literal in three Sheaf
paint sites and about to be needed by a fourth site in this app. The members
CAN collapse into one definition: this app already compiles Sheaf headers
(`BuildEncoderDrawCommands`, `BuildFourteenSegmentCommands` are called from
`FroggersUiSurface.hpp`), so a named constant in Sheaf is reachable by every
site, and no cross-boundary drift check is needed once all of them read it.

Plan: one `inline constexpr Color kSurfaceBackground` in Sheaf's
`synth/Color.hpp` — the header every consumer already reaches (four of five
sites include it directly; `PortableJuceBackend.hpp` reaches it via
`PortableUI.hpp:3`) — referenced by:

- `PortableJuceBackend.hpp:266` and `:275` (via the existing
  `UiToJuceColour()` at `PortableJuceBackend.hpp:23`),
- `EncoderDraw.hpp:680` (both the opaque and the translucent-underlay
  variant, which reuses the same r/g/b with alpha 150),
- the app's plate at `FroggersUiSurface.hpp:2039`,
- the VST editor's root fill at `app/vst/FroggersPluginEditor.cpp:37`
  (`kBackgroundColour(18, 20, 22)`, painted at `:151`) — a second window
  root fill the first enumeration missed; app/vst links Sheaf's synth core,
  so it includes the same header and converts by components.

The ghost segments keep their designed subtlety by derivation, not by a
second literal: off = plate + (4, 6, 6), today `36,40,42` over `32,34,36`,
becoming `22,26,28` over `18,20,22`.

The badge chip (`EncoderDraw.hpp:595`) stays `32,34,36` UNCHANGED — it sits
on the knob, not on the root, and is deliberately a chip.

## Enumeration (found vs to change; corrected by preflight)

- `Rgb(18,20,22)` in C++: 7 real sites — backend `:266`, `:275`,
  `EncoderDraw:680`, the VST editor root fill
  (`app/vst/FroggersPluginEditor.cpp:37`), and three tests pinning it
  (`MiniAppJuceBackendParityTests.cpp:312`,
  `PortableJuceBackendTests.cpp:1183`,
  `browser_command_buffer_tests.cpp:524`). All 7 move to the constant
  (tests included — they then pin the constant's use, not a copy of its
  value).
- The same colour as CSS: `#121416` ×3 in Sheaf's
  `browser/public/synth-browser.css:3,10,33`, pinned by
  `browser/tests/static-site.spec.ts:46`. A language boundary — CSS cannot
  read the constant — so these stay literal; the constant's comment names
  the CSS twin so a future colour change greps it, and the existing spec
  test keeps the CSS side pinned.
- `Rgb(32,34,36)`: 3 sites — badge chip (unchanged, deliberate), app plate
  (changes to the constant), app test `FroggersSurfaceTests.cpp:1681`
  (changes to assert the plate command carries the constant).
- `Rgb(36,40,42)`: 3 sites — the app's off colour (becomes derived, with
  its test assertion following), Sheaf's own readout ghost inside
  `BuildEncoderDrawCommands` (`EncoderDraw.hpp:792`) which renders over the
  CELL fill rather than a band on the root and is deliberately
  higher-contrast there (unchanged; noted for upstream), and an arbitrary
  colour input in `PortableDrawGeometryTests.cpp:285` (unchanged, not a
  pin of this concept).

## Why the label mechanisms differ (asked during this investigation)

Three label mechanisms exist and the split is deliberate, not drift:

1. `ControlStyle::caption` — the library path. ZERO uses in this app,
   because `Builder::FinishControl` (`PortableUIBuilders.hpp:428-465`) only
   places captions BEFORE a control and every label here sits BELOW.
   Tracked upstream as ask 14 (caption placement).
2. Hand-rolled `Label` nodes — 3 uses (FREEZE `:1293`, scene blend `:1444`,
   BPM `:1492`), each documented as collapsing into captions when ask 14
   lands.
3. The encoder 14-segment strips — one call site serving all 16 cells. A
   cell is a single `Draw` node; a `Label` node cannot composite into it,
   and the 14-segment style is the instrument's own readout idiom, with
   `labels.md` as the text authority.

The one member of this family that was NOT deliberate is the plate colour
this change fixes.

## Sheaf delivery

The Sheaf edit lands as its own standalone branch and upstream PR (the
`HasRestoreStartupState` pattern; expected to arrive as jvictor0/Sheaf#11),
with the frogg3rs pin bumped as its own commit afterward. If upstream rejects
the constant, the fork carries it; the pin already points at the fork.
All frogg3rs commits land directly on `main` — no feature branch.

## Impact

- `External/Sheaf`: `include/synth/Color.hpp`,
  `include/synth/EncoderDraw.hpp`, `juce/PortableJuceBackend.hpp`, the
  three tests above. New branch + PR.
- `app/`: `FroggersUiSurface.hpp` (plate + off colours),
  `FroggersSurfaceTests.cpp`.
- `app/vst/FroggersPluginEditor.cpp` (root fill reads the constant).
- Submodule pin bump, own commit.
- Gates: app suite, Sheaf synth gate, miniapp target (runtime shell),
  browser build, launcher build; the VST target builds in CI on the push
  (`vst-plugin.yml` runs on every non-docs push to `main`).
- Sequencing note: `frogg3rs-microphone-path-delivery`'s sweep scope names
  `External/Sheaf/projects/synth/tests/`; no named-file collision exists,
  and the tree is clean, but its operator items remain open while this
  change edits one file in that directory.

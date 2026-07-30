# desktop-v2-grid-layout Specification

## Purpose

Desktop v2 and VST v2 chrome SHALL use a **fixed modular grid** inspired by VCV Rack panel density — one immutable layout, not a rearrangeable rack. Every control occupies an integer rectangle on a shared cell grid so layout math is explainable without pixel hunting.

**Audit 2026-06-30:** Current code uses ad-hoc pixel constants (`DesktopV2ChromeLayout.hpp`) with manual `removeFromTop/Left` in each component. Constants do not derive from a single base unit; encoder ring size (52px) and row height (72px) are not integer multiples of the same cell. This spec defines the authoritative grid and maps existing chrome to it.

## Requirements

### Requirement: grid-base-unit

The layout grid SHALL use a base cell **u** (one unit). At default UI scale, **1u = 10px**.

All chrome dimensions SHALL be expressed as integer multiples of **u** in `DesktopV2ChromeLayout.hpp`. Runtime layout code SHALL snap bounds to the grid via `gridPx(int units) = units * kGridUnitPx`.

Body text SHALL use IBM Plex Sans at **11pt**; one lowercase “m” glyph SHALL fit inside **1u × 1u** including side bearing (cap height ≈ 0.8u, line box = 1u).

#### Scenario: constants are grid-derived

- **WHEN** `DesktopV2ChromeLayout.hpp` is read
- **THEN** `kGridUnitPx == 10`
- **THEN** `kEncoderRingSize == gridPx(5)` (50px)
- **THEN** `kEncoderRowH == gridPx(5)` (50px)
- **THEN** `kRowLabelW == gridPx(9)` (90px)
- **THEN** `kModCellW == gridPx(7)` (70px)

### Requirement: control-footprints-on-grid

Control footprints SHALL use these cell rectangles (width × height in **u**):

| Control | Footprint (u×u) | Notes |
|---------|-----------------|-------|
| Encoder ring (knob) | **5 × 5** | Interactive sheaf ring; centered in row flex area |
| Arrow button (← →) | **2 × 2** | Carousel header, sequencer prev/next |
| Icon button (dice) | **2 × 2** | Sequencer Rand-seq |
| Text button | **(textU + 2) × 3** | `textU = ceil(glyphWidth / kGridUnitPx)`; 1u horizontal padding each side |
| Toggle / scene button (S1–S3, G1–G2) | **3 × 3** | Square tap targets |
| Record arm toggle | **6 × 3** | Fits “Record” at 11pt without ellipsis |
| Sequencer transport | **≥ 11 × 3** | “Stop Sequence” at 11pt; min 110px |
| Slider (horizontal) | **trackH = 2u** | Length in u multiples; thumb 2u×2u |
| Mod source dropdown cell | **7 × 5** | Width 7u; height 5u — matches row height |
| Row label (left column) | **9 × 5** | Full row height; text left-aligned, vertically centered |
| Encoder row | **5** tall | Row height equals tallest control (5u ring / 5u mod) — no extra vertical padding |
| Step gate cell (sequencer) | **2 × 2** | 16 columns × up to 4 rows for 64 steps |
| VCO EF scope (transport-embedded) | **flex × 7** (standalone) or **128u × 5** (VST strip) | One multi-trace widget; not a scope grid column |

#### Scenario: encoder row layout

- **WHEN** `SubmodulePagePanel` lays out one parameter row
- **THEN** the row is **5u** tall (50px)
- **THEN** left **9u** is the parameter label
- **THEN** right **7u** is the mod dropdown (5u tall, vertically centered in 5u row)
- **THEN** the center hosts a **5×5** encoder ring flush to row height

#### Scenario: carousel header cluster

- **WHEN** the carousel header renders
- **THEN** layout is `[← 2u][title auto][→ 2u]` as one centered group
- **THEN** **1u** gap between arrow and title text
- **THEN** header band height is **3u**

### Requirement: panel-width-and-module-column

Default panel width SHALL be **128u** (1280px). The active module column (carousel content) SHALL span the full inner width minus chrome pad (**2u** each side).

Module pages SHALL NOT reflow into multiple columns; VCV-style **vertical signal flow** within one fixed column:

```
┌─ 128u ─────────────────────────────────────────────────────────┐
│ pad 2u │              module column (124u)              │ pad │
│        │  [header 3u: ← title →]                           │     │
│        │  [row0: label 9u | ring 5u | mod 7u]  × N      │     │
│        │  (viewport scroll when N×5u exceeds flex height)  │     │
└──────────────────────────────────────────────────────────────┘
```

#### Scenario: transport-embedded VCO scope (no scope grid)

- **WHEN** standalone desktop v2 lays out top chrome
- **THEN** one triple-VCO EF oscilloscope occupies the right flex of the **7u** transport band
- **THEN** `ScopeGridComponent` is not instantiated

#### Scenario: VST top scope strip

- **WHEN** VST v2 lays out chrome
- **THEN** a **5u** full-width VCO EF scope strip appears above the performance band
- **THEN** no separate scope grid section exists

### Requirement: vertical-chrome-stack-grid

Vertical chrome SHALL stack in **u** multiples with **kSectionGap = 1u** between sections (per `desktop-v2-scope-visualization` §0a — scope grid band deleted):

| Section | Height (u) | Hosts |
|---------|------------|-------|
| Transport + VCO scope | **7u** | Standalone: Engine, Stop, Audio, MIDI + triple-VCO EF scope |
| VCO scope strip | **5u** | VST only: full-width triple-VCO EF scope (no transport row) |
| Performance band | **7u** | Scenes, blend, G1/G2, sequencer transport, Marbles LEDs |
| Carousel (flex) | remainder | Header **3u** + N×**5u** rows in scrollable viewport |
| Sequencer panel | **13u** | Toolbar 3u + step grid 10u |
| Global strip | **4u** | Rand buttons + Crunchy 5u ring + Shift |
| Chrome pad | **1u** top + bottom | Outer inset |

`ScopeGridComponent` and `kScopeGridH` SHALL NOT exist after Phase B0.8.

Default window height SHALL be **92u** (920px). Ten-row FX pages MAY require carousel vertical scroll at default standalone height — bank paging SHALL NOT be used.

When window height is insufficient to show every row, the carousel SHALL scroll vertically. All rows remain in the document.

#### Scenario: default height on 1080p

- **WHEN** desktop v2 or VST v2 opens at 128u × 92u
- **THEN** the window frame fits within a typical 1080p usable area (taskbar/dock margin)
- **THEN** standalone carousel viewport fits **ten** encoder rows without scroll at 920px (Filter page) after scope grid removal
- **THEN** VST (5u scope strip, no transport row) shows ~10 rows visible at 920px — all FX pages fit without scroll

#### Scenario: VST omits transport row

- **WHEN** `HostedMainComponentV2` lays out chrome
- **THEN** the standalone transport row (7u) is absent
- **THEN** a **5u** VCO EF scope strip and **7u** performance band appear above carousel flex
- **THEN** carousel flex gains height versus standalone (no 7u transport band)

### Requirement: performance-band-grid

`PerformanceBandV2` SHALL lay out left-to-right on the grid:

```
[S1 3u][S2 3u][S3 3u][gap 1u][blend slider ≥12u][gap 2u]
[G1 3u][G2 3u][G1 wt slider ≥8u][G2 wt slider ≥8u][gap 2u]
["BPM" 3u][tempo slider ≥10u]["Steps" 3u][length slider ≥8u]
[Start/Stop Seq ≥11u×3u][Record 6u×3u]
```

Gaps are **1u** unless noted. No control SHALL truncate at 128u width.

#### Scenario: performance band at default width

- **WHEN** performance band renders at 128u
- **THEN** “Stop Sequence” is fully visible on the transport button
- **THEN** “BPM” and “Steps” labels are visible beside their sliders

### Requirement: sequencer-toolbar-grid

Sequencer toolbar SHALL occupy the top **3u** of the sequencer panel:

```
[← 2u][→ 2u][dice 2u][gap 2u][Step|Pattern toggle ≥12u×3u]     [step grid below]
```

Step grid SHALL use **16** columns of **2u×2u** cells; row count = `ceil(patternLength / 16)`.

#### Scenario: sequencer toolbar controls

- **WHEN** sequencer panel renders
- **THEN** prev/next/dice buttons are each **2u×2u**
- **THEN** Step/Pattern toggle is right-aligned or adjacent to dice per layout fit within 128u

### Requirement: global-strip-grid

`GlobalStripV2` SHALL place controls on the bottom **4u** band:

- Left: text buttons sized per `(textU+2)×3` grid rule
- Right: **Crunchy** label (auto width) + **5×5** encoder ring + **1u** gap + **Shift** button `(textU+2)×3`

#### Scenario: Crunchy ring footprint

- **WHEN** global strip renders
- **THEN** Crunchy uses a **5×5** encoder ring, not a plain rotary
- **THEN** ring center is vertically centered in the **4u** strip

### Requirement: vst-grid-parity

VST v2 editor (`HostedMainComponentV2`) SHALL use the **same grid constants and component footprints** as standalone desktop v2. Differences are **section presence only** (no transport row, no device/MIDI settings entry points).

#### Scenario: hosted editor grid match

- **WHEN** VST v2 editor opens at default size
- **THEN** VCO scope strip, performance band, carousel, sequencer, and global strip match desktop grid footprints
- **THEN** `PluginEditorV2` default bounds use `kDefaultWidth` × `kDefaultHeight` from grid-derived constants
- **THEN** `ScopeGridComponent` is not in the VST component tree

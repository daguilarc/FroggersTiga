## Context

```
SubModulePanel (one of six: Audio … Delay)
├── title + Randomize / Randmod
└── 8 parameter rows (7 + Crunch/FUEG)

Current row layout (layoutPanel):
  [ label ──────────────── stretch ──────────────── ] [wave?][knob][jack]
  ↑ far left                                          ↑ far right of panel

PatchCableOverlay reads m_inputJackBounds per row from SubModulePanel.
Knob drag, mod ring, wave buttons — all unchanged behavior; layout only.
```

Web reference: `web-knob-column-cells` — bordered cell per parameter, label above knob, mod UI below. Desktop transpose: **row stack** instead of column row, **jack instead of mod dropdown**.

## Data Flow

| Stage | Input | Transform | Output |
|-------|-------|-----------|--------|
| Layout | panel local bounds | `layoutPanel()` loop rows 0–7 | widget bounds + `m_rowCellBounds[i]` |
| Paint | `m_rowCellBounds`, mod state | `paint()` loop rows 0–7 | 1 px cell borders + mod ring + jack discs |
| Cables | `m_inputJackBounds[i]` | `collectInputPorts()` | unchanged overlay port list |

One loop over `row ∈ [0,7]` for layout; one loop for border paint. No per-row copy-paste blocks.

## Goals / Non-Goals

**Goals:**

- Each parameter row is visually one module: thin border, label + controls inside the same rectangle.
- Eight rows × one column grid inside every `SubModulePanel`.
- Patch jacks stay clickable targets for `PatchCableOverlay` (bounds updated in same layout pass).
- Readable at default **1440×720** (~237 px per panel per `desktop-chrome-cohesion`).

**Non-Goals:**

- Submodule group boxes (VCOs / Coupling / Output) — web removed these; desktop stays flat eight-row panels.
- Changing knob diameter, row count, or six-panel window split.
- Web UI changes.
- Resizable column grid (e.g. 4×2) — always 8×1 stack unless a follow-up change requests it.

## Decisions

### D1: Store row cell bounds; paint borders in `SubModulePanel::paint`

**Choice:** `std::array<juce::Rectangle<int>, 8> m_rowCellBounds` filled in `layoutPanel()`. `paint()` draws `g.drawRect(m_rowCellBounds[row], 1.0f)` with colour `0xff3d444d` (thin, matches dark chrome).

**Why:** No new child components; patch port math stays in one file. Matches OMNI single-pass layout.

**Alternative rejected:** Eight `ParameterRowCell` child `Component`s — cleaner encapsulation but larger diff; defer unless layout logic exceeds ~3 nesting levels.

### D2: Compact in-cell layout (label + control cluster)

**Choice:** For each row cell:

```
┌─ m_rowCellBounds[i] ─────────────┐
│ Label (centredLeft, 1 line)      │
│          [wave?] [knob] [jack]   │  ← control cluster centred or right-aligned in cell
└──────────────────────────────────┘
```

Constants reuse existing `kKnobSize` (38), `kJackSize` (20), `kWaveWidth` (28). Control cluster width = `fixedTailWidth(hasWave)`. Label uses remaining cell width above or beside cluster — prefer **label top, cluster bottom-centred** when cell height allows; if tight, label left of cluster on one line (same as today but bounded to cell width, not panel width).

**Cell height:** `kRowHeight` stays 36 unless soak test requires 38 for two-line label; bump once globally, not per row.

**Why:** Groups label with knob/jack; eliminates dead horizontal gap.

### D3: Single `layoutRowCell()` helper

**Choice:** Extract one helper used for rows 0–6 and row 7 (FUEG):

```cpp
void layoutRowCell(juce::Rectangle<int> cell, int row, bool hasWave, ...);
```

**Trigger count:** complexity (8 rows + FUEG + wave variant), domain boundary (row layout), explicit contract (inputs: cell rect + row index; outputs: bounds + jack rect). Meets one-time helper exception.

### D4: Border only on row cells, not outer panel

**Choice:** No border around entire `SubModulePanel`; only the eight inner row cells. Title/button row has no cell border.

**Why:** User asked for modules (rows) to be distinct, not heavier panel chrome.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Long labels clip at ~237 px panel width | `setMinimumHorizontalScale(0.85f)` only if needed; prefer single-line truncation with tooltip (existing label text) |
| Jack hit targets misaligned after layout | Re-run `syncPatchPorts()` already called from `MainComponent::resized()`; manual cable drag test in tasks |
| Row height increase breaks six-panel fit | Keep `kRowHeight` 36; verify 8 rows + header fit in panel height |
| Visual clutter from eight borders | 1 px low-contrast stroke; no fill change |

## Migration Plan

1. Add `m_rowCellBounds`, implement `layoutRowCell`, switch `layoutPanel` to cell stack.
2. Add border draw in `paint()` before mod rings and jacks.
3. Build desktop; verify at 1440×720 and 1680×720.
4. Manual patch-cable drag to each row on Audio + Delay panels.

## Open Questions

None — layout-only change; behavior unchanged.

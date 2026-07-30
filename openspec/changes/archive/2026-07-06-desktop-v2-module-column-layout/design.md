## Context

Desktop v2 module rows target a **four-column** geometry on a 10px grid:

```
| label 9u | encoder 5u | gap 1u | center 15u | gap 1u | mod ≥18u | pad |
```

The layout-density change added `CenterGlobalClusterV2` as specified in archived `design.md` D1: *"cluster is a sibling in PageCarouselComponent"*. That implementation choice treats the center column as a **floating layer** over a full-width encoder viewport. Mod cells position at `kModuleRowModX = 310px` inside content coordinates while the cluster positions at `carouselX + 150px` in parent coordinates — numerically adjacent but **not mutually exclusive** because:

1. JUCE paints later siblings on top (cluster occludes mod hit targets).
2. Viewport width includes center-column pixels — mod column X is correct arithmetically but visually stacked under globals.
3. Cluster vertical stack (7 controls × ~40px ≈ 280px+) spans row Y positions where mod dropdowns live.

Operator screenshot (2026-07-04) confirms Rand All…Rand Resample stack sits on mod dropdowns; Shift overlaps encoder dials; performance band shows `...`; module scrolls with empty space right and below sequencer toolbar.

## Goals / Non-Goals

**Goals:**

- Exclusive four-column layout at 1280×920 — zero bounds intersection between center cluster and mod cells.
- Audio module (8 rows) visible without vertical scroll at default height.
- No mod-source or performance-band label ellipsis at default width.
- Automated layout bounds test in desktop-v2 `ctest`.
- Hosted editor parity via shared carousel layout.

**Non-Goals:**

- Change default window size (128u × 92u stays).
- Move scene band (S1/S2/S3) into module columns.
- Revert Write Seq / sequencer semantics from layout-density change.
- Redesign mod cell visual style.

## Decisions

### D1 — Exclusive column split (replaces overlay)

**Choice:** `PageCarouselComponent::resized` partitions carousel body **horizontally** before laying out children:

```
body = carouselArea minus header
colLabelEnc = removeFromLeft(kModuleRowEncoderOffset + kEncoderRingSize)  // 14u
colCenter   = removeFromLeft(kCenterGlobalClusterW)                         // 15u
colMod      = remainder (min kModCellW)
```

- `m_submodulePanel` / `m_adsrPanel` receive **label+encoder+mod** regions via column layout API — NOT full `body` width.
- `m_centerCluster` receives **`colCenter`** bounds only — sibling of panel, **not overlapping** panel bounds.

**Alternative rejected:** Keep overlay, increase gap — does not fix z-order hit testing or Shift/encoder Y overlap.

### D2 — `ModuleRowColumnLayout` single authority (OMNI repetition)

**Choice:** Add to `DesktopV2ChromeLayout.hpp`:

```cpp
struct ModuleRowColumnLayout {
    int labelW, encoderW, gapW, labelEncoderW, centerX, centerW, modX, modW, contentW;
};
ModuleRowColumnLayout moduleRowColumns(int rowWidth) noexcept;
```

- `modX = labelW + encoderW + gap + centerW + gap` (derived, not `gridPx(31)` literal).
- `labelEncoderW = labelW + encoderW`; the encoder viewport uses only this width.
- `centerX = labelEncoderW + gapW`; the carousel alone owns the center column bounds.
- `SubmodulePagePanel`, `AdsrPagePanel`, `PageCarouselComponent` call this once per layout pass.

| Helper | Trigger | Boundary | Complexity | Contract |
|--------|---------|----------|------------|----------|
| `moduleRowColumns` | ≥3 callers | Module row geometry | O(1) arithmetic | In: rowWidth; out: column widths + modX |

Review enforcement: Trigger ≥2 **Yes** | Domain **Yes** | Complexity **Yes** | Contract **Yes** | Side effects **None** | Local **Yes**.

### D3 — Encoder viewport width = label+encoder column only

**Choice:** `m_encoderViewport` width = `labelEncoderW`, not full panel width. Mod cells move to a dedicated sibling `m_modColumnViewport` whose content width is `modW`. Each `ModSourceCell` is placed at x=0 within the mod column content; absolute page-space X derives from `moduleRowColumns(rowWidth).modX`.

**Required structure:**

```
SubmodulePagePanel
├── header row (Randomize / Randmod) — spans label+encoder cols only
├── encoderViewport (labels + rings)
├── modColumnViewport (mod cells at x=0, synced scroll Y with encoder)
└── (center cluster owned by carousel, not panel)
```

Sync vertical scroll between encoder and mod viewports when scroll is required.

**Alternative rejected:** Single viewport with all four columns — center cluster is shared across rows, not per-row; stays in carousel center column.

### D4 — Vertical scroll policy

**Choice:**

```cpp
const int docH = encoderDocumentHeight(rows);
const int availH = viewport.getHeight();
m_encoderContent.setSize(labelEncW, docH);
m_encoderViewport.setScrollBarsShown(docH > availH, false);
if (docH <= availH) m_encoderViewport.setViewPosition(0, 0);
```

Apply same policy to mod column viewport. At default 1280×920, Audio 8 rows (400px document) fits in ~480px viewport — **no scrollbar**.

### D5 — Sequencer dead-space reclaim

**Choice:** Top-align step grid in sequencer panel remainder (`gridY = toolbarBottom + pad`), not vertical center. `kSequencerH` remains fixed at 18u; extra vertical space remains with the carousel through the existing parent flex allocation.

Constants unchanged: `kSequencerH = 18u`.

### D6 — Performance band truncation

**Choice:**

| Constant | Before | After |
|----------|--------|-------|
| `kPerfMarblesLabelH` | 1u (10px) | **2u** (20px) |
| Marbles label Y | `y` (top of band) | vertically centered in band |

Scene button minimum width SHALL fit `S1·L`, `S2·R`, and `S3` labels at 1280px. Increase the shared scene-button minimum constant when the measured label width plus padding exceeds the current value.

### D7 — Layout bounds regression test

**Choice:** `LayoutBounds_test.cpp` instantiates `MainComponent` off-screen at 1280×920, calls `resized()`, selects the Audio page, collects `CenterGlobalClusterV2` bounds vs all visible `ModSourceCell` bounds in page coordinates, and asserts `!intersects`.

Gate runs in `ctest`; fails on overlay regression.

### D8 — Center cluster overflow behavior

**Choice:** `CenterGlobalClusterV2` lays out its controls inside the center column using compact vertical gaps from `DesktopV2ChromeLayout`. When preferred stack height exceeds available column height, the center cluster owns internal vertical scrolling. It never expands into encoder or mod bounds.

## Data flow (fixed layout)

```
MainComponent::resized
  → carousel body height H
PageCarouselComponent::resized
  → moduleRowColumns(body.width)
  → split: labelEnc | center | mod
  → m_centerCluster.setBounds(centerCol)
  → m_submodulePanel.setBounds(labelEnc + mod regions)
SubmodulePagePanel::layoutRows
  → labels+rings in encoder viewport (width = labelEnc)
  → mod cells in mod column (x=0..modW within modColumnViewport)
  → scroll iff docH > availH
LayoutBounds_test
  → intersect(cluster, modCells) empty
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Dual viewport scroll sync drift | Link view positions in `visibleAreaChanged` callback |
| AdsrPagePanel divergence | Task parity with SubmodulePagePanel |
| Center cluster taller than viewport | Center column uses compact spacing and internal scroll |
| Hosted editor narrower than 128u | `moduleRowColumns` clamps modW to min 18u; hosted min width already 128u |

## Migration Plan

1. Land `ModuleRowColumnLayout` + column split (no widget moves).
2. Move mod column out of encoder content X=310 path.
3. Remove overlay `setBounds` from PageCarousel.
4. Fix performance band + sequencer alignment.
5. Add LayoutBounds_test; run manual QA screenshot at 1280×920.

## Open Questions

None for proposal — column split approach is determined by screenshot + source audit.

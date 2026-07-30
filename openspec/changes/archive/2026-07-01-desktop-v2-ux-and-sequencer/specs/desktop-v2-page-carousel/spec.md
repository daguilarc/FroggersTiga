## MODIFIED Requirements

**Audit 2026-06-30:** Bank paging rejected. Row counts from `froggerstiga-desktop-v2` §10 + `V2ParamDisplayNames.hpp` + `FroggersV2ControlCore::rowsForPage` (page 0 → 8; pages 1–6 → 10 until Pair-AR Phase P → 7).

### Requirement: v2-module-row-counts

| Page | Module | Rows | Source |
|------|--------|------|--------|
| 0 | Audio | 8 | `ParamDisplayNames` — 7 musical + Crispy (unchanged v1) |
| 1–5 | Random, Reverb, Filter, Drive, Delay | **10 each** | v1 rows **0–6** (7 knobs) + v2 expansion rows **7–8** + Crispy **9** |
| 6 | Pair-AR | **7** (Atk/Rel pairs per VCO + Crispy) | Phase P — was ADSR 10 |

**Filter (page 3) ten rows — not arbitrary:**

| Row | Label | Origin |
|-----|-------|--------|
| 0–6 | Comb offset … Comb LP | v1 `ParamDisplayNames` rows 0–6 (core comb + peak params) |
| 7 | Comb/Peak | v2 parallel topology mix (`froggerstiga-desktop-v2` design §10b) |
| 8 | Scoop | v2 expansion row |
| 9 | Crispy | fuego (moved from v1 row 7 when expansion rows were inserted) |

v1 Filter had **8** rows (7 musical + Crispy). v2 desktop added **Comb/Peak** and **Scoop**, bumped Crispy to row 9 → **10** total. Same pattern for Random, Reverb, Drive, Delay with different row 7–8 labels.

All rows for the active page SHALL be present in carousel content. Bank paging SHALL NOT exist.

#### Scenario: Expanded FX pages show ten rows

- **WHEN** the user opens Filter at default window size
- **THEN** ten encoder rows exist in the carousel document
- **THEN** no bank controls appear

#### Scenario: Audio unchanged

- **WHEN** the Audio module is active
- **THEN** eight Audio encoder rows exist

#### Scenario: Pair-AR seven rows

- **WHEN** the Pair-AR module is active
- **THEN** seven rows exist: Atk1, Rel1, Atk2, Rel2, Atk3, Rel3, Crispy

## ADDED Requirements

**Merged from `v2-ux-and-operator-docs`.** Carousel header layout open in Phase C task 4.1.

### Requirement: v2-carousel-header-layout

Carousel previous/next controls SHALL be positioned adjacent to the module title, not at the far horizontal edges of the carousel panel.

#### Scenario: Arrows flank title

- **WHEN** desktop v2 or VST v2 renders the module carousel header
- **THEN** the previous button appears immediately left of the module title text area
- **THEN** the next button appears immediately right of the module title text area

### Requirement: v2-performance-band-placement

Desktop v2 and VST v2 SHALL render a performance band between the top scope area and the module carousel containing scene, gesture, and sequencer performance controls.

#### Scenario: Band order in layout (standalone)

- **WHEN** desktop v2 main window is shown
- **THEN** vertical order is: transport + VCO EF scope (7u) → performance band → module carousel → sequencer panel → global strip
- **THEN** no dedicated scope grid section exists between transport and performance band

#### Scenario: Band order in layout (VST)

- **WHEN** FroggersTigaPluginV2 editor is shown
- **THEN** vertical order is: VCO EF scope strip (5u) → performance band → module carousel → sequencer panel → global strip

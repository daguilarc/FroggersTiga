## MODIFIED Requirements

### Requirement: v2-module-row-counts

Module pages SHALL use ten-row layouts where specified in `desktop-v2-module-expansion`. **Audio** is excluded from row additions.

At default desktop v2 window size (`DesktopV2ChromeLayout::kDefaultWidth` × `kDefaultHeight`), all rows for the active module SHALL be visible without bank paging.

#### Scenario: Audio unchanged

- **WHEN** the Audio module is visible
- **THEN** the Audio page shows its existing row layout with no v2 expansion rows
- **THEN** all eight Audio encoder rows are visible without bank paging

#### Scenario: Expanded FX pages show ten rows

- **WHEN** any module page 1–5 is visible at default window size
- **THEN** exactly ten encoder rows are shown without bank paging
- **THEN** Crispy is row 9

#### Scenario: ADSR ten rows

- **WHEN** the ADSR module is visible at default window size
- **THEN** exactly ten encoder rows are shown without bank paging (rows 0–8 A/S/R, row 9 Crispy)

#### Scenario: Labels from display authority

- **WHEN** any module row is rendered
- **THEN** labels come from `V2ParamDisplayNames` without duplicate tables in UI code

## ADDED Requirements

### Requirement: v2-carousel-header-layout

Carousel previous/next controls SHALL be positioned adjacent to the module title, not at the far horizontal edges of the carousel panel.

#### Scenario: Arrows flank title

- **WHEN** desktop v2 renders the module carousel header
- **THEN** the previous button appears immediately left of the module title text area
- **THEN** the next button appears immediately right of the module title text area

### Requirement: v2-performance-band-placement

Desktop v2 SHALL render a performance band between the scope grid and the module carousel containing scene, gesture, and sequencer performance controls.

#### Scenario: Band order in layout

- **WHEN** desktop v2 main window is shown
- **THEN** vertical order is: transport → global strip (randomize + Crunchy + Shift) → scope grid → performance band → module carousel

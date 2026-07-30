## ADDED Requirements

### Requirement: Faceplate silkscreen visible without hover

Every FroggersTiga VCV module panel SHALL display functional silkscreen text on the faceplate at 100% rack zoom without requiring hover. This includes a **header strip** (tiny frog logo + “FroggersTiga” name), column titles, row labels, **five mod-rack cell names**, CC/MIDI/I/O port names, and **randomize button labels**. Wave-morph widgets use nanovg; mod-rack cells use green LEDs only (no scope silkscreen or trace widgets).

#### Scenario: Product header on faceplate

- **WHEN** any FroggersTiga module is placed at 100% zoom
- **THEN** a tiny frog logo and “FroggersTiga” name are visible on the panel shell (top area)
- **THEN** the header does not obscure knobs, jacks, or screws

#### Scenario: Rack-compatible SVG text

- **WHEN** Rack loads panel SVGs via `setPanel`
- **THEN** silkscreen geometry is present as `<path>` elements that nanosvg renders
- **THEN** no live SVG `<text>` remains in shipped assets

#### Scenario: Label authority

- **WHEN** silkscreen strings are generated
- **THEN** they are parsed from `ParamDisplayNames.hpp` (not duplicate Python tables)
- **THEN** anchor positions derive from `VcvPanelLayout.hpp` / `FieldParityWidget` formulas only — no undeclared magic mm offsets in Python

#### Scenario: Label alignment and panel bounds

- **WHEN** the module is viewed at 100% rack zoom after silkscreen generation
- **THEN** row labels sit above or beside their knobs/ports — not centered on jack positions
- **THEN** bottom I/O row and Crispy row are fully inside panel bounds (widget bbox, not center point only)
- **THEN** the primary 24 HP band shows labels aligned to the voicing column widgets, not empty gray with misplaced text

#### Scenario: PM3 parity

- **WHEN** the Audio column silkscreen is inspected
- **THEN** row 7 from the top reads **Phase mod 3** and row 8 reads **Crispy**

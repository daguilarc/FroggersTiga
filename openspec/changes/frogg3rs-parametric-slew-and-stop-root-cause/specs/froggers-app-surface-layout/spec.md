# Delta — `froggers-app-surface-layout`

Two independent, operator-reported UI defects (`tasks.md` S5.2, S6), both scoped to per-cell chrome
inside the already-resolved 6×6 grid layout. Neither disturbs the grid's own row/column weights,
margins, or gaps.

## ADDED Requirements

### Requirement: The drill-level header is legible against the scope's live traces
Whenever the active modulation drill level is greater than 0, the VCO scope cell SHALL draw a
`"Modulation Level N"` header (`N` the current level) that reads as its own heads-up label rather
than competing with the three live oscillator traces beneath it. The header SHALL be drawn on an
opaque backing band sized to the header's own text bounds, in a text size larger than the surface's
unstyled default, and SHALL be drawn after — so, on top of — the scope's waveform commands for that
cell. At drill level 0, no header SHALL be drawn.

Operator, 2026-08-07: *"i still don't see a header label counting the drilldown levels."* The header
text and its draw order were already correct before this fix — a default-styled, backdrop-less label
over the scope's near-black background is close to maximum contrast in isolation — but a small,
unstyled corner label sitting directly over three colour-shifting traces, inside a cell that resolves
to roughly 130–284px tall across the sizes this surface is tested at, read as scribble on the graph,
not as a header. The defect was prominence, not colour or draw order, so the fix is an explicit style
plus a backing band, not a re-ordering.

#### Scenario: No header at the top level
- **WHEN** the operator has not drilled into any parameter's modulation view
- **THEN** the scope cell shows no drill-level header

#### Scenario: The header names the current level and stays legible
- **WHEN** the operator drills to level 1, then 2, then 3
- **THEN** the header at each level reads `"Modulation Level 1"`, `"Modulation Level 2"`, and
  `"Modulation Level 3"` respectively
- **THEN** at every level the header's text style is explicitly larger than the surface's own
  default, and an opaque backing band immediately precedes the header text in the cell's draw order
  (`FroggersSurfaceTests.cpp`'s
  `drill_level_header_shown_only_while_drilled_in_and_matches_the_level`)

#### Scenario: The header does not disturb the grid
- **WHEN** the header and its backing band are drawn
- **THEN** they are confined to the scope cell's own already-resolved bounds
- **THEN** no row, column weight, margin, or gap of the encoder grid changes

### Requirement: Encoder cells render without a card frame that collides with the modulation ring
Encoder cells in the sixteen-slot grid SHALL render without the framework's default rounded-rect card
frame (`EncoderDrawState::wantsFrame = false`). At this grid's cell size, Sheaf's own geometry places
the frame's stroke inside the painted outer edge of the per-cell modulation ring by a measured margin
(reported: **1.80px**), so the two strokes visibly collide. Suppressing the frame is the app-side
compensation for upstream geometry the app cannot patch directly, because `External/Sheaf` is pinned
and unpatchable; if the geometry is judged worth correcting upstream, that need is filed to
`/UPSTREAM-SHEAF-ASK.md` rather than worked around a second time.

#### Scenario: No card frame collides with the ring
- **WHEN** any encoder cell in the grid is rendered
- **THEN** no rounded-rect card frame is drawn around it
- **THEN** the modulation ring's stroke is not crossed by a frame stroke
- **THEN** the cell's draw commands contain no `StrokeRoundedRect`, while its body and ring fills are
  still present (`FroggersSurfaceTests.cpp`'s `encoder_cell_never_emits_a_frame_draw_command`)

#### Scenario: The grid is otherwise unchanged
- **WHEN** the frame is suppressed
- **THEN** the 6×6 grid's row/column layout, cell sizing, and every other per-cell visual element are
  unaffected

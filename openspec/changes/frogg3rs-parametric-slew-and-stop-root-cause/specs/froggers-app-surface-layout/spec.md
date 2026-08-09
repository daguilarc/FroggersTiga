# Delta — `froggers-app-surface-layout`

Two independent, operator-reported UI defects (`tasks.md` S5.2, S6), both scoped to per-cell chrome
inside the already-resolved 6×6 grid layout. Neither disturbs the grid's own row/column weights,
margins, or gaps.

## ADDED Requirements

### Requirement: The drill-level indicator renders inside the region the operator is looking at
Whenever the active modulation drill level is greater than 0, the drill-level indicator SHALL resolve
to bounds fully contained within the encoder-grid block — the region that displays the modulation
sources during drill-in — and SHALL be drawn on the Target/Back cell, which Sheaf's
`OpenModulationView` places at `physicalLayout.back()` at every drill depth. It SHALL be drawn on an
opaque backing band, appended last so it paints on top of that cell's own chrome. At drill level 0,
no indicator SHALL be drawn.

**This requirement is written against a defect that took three attempts to fix, and the wording is
deliberate: containment in the visible region is the property, not existence in the draw tree.**
The indicator originally lived on the VCO scope cell, justified by that being the only
non-interactive cell in the grid. Two successive fixes proved it existed, then proved it was appended
last and not overdrawn within its node — and the operator still could not see it. The reason,
measured at the real window size: `kVcoScope` resolves to `{16, 16, 284.67, 181.33}`, in the LEFT
block, while the modulation-source grid resolves to `{314.67, 16, 569.33, 600}` in `kRightBlock`.
Two physically disjoint columns separated by a 14px gap. The header rendered correctly the whole
time, in a column the operator's attention never crosses while reading the grid.

**Do not move this back to the scope cell.** Existence and non-overdraw are both insufficient
assertions; the test that encodes this requirement asserts geometric containment in the grid region.

Note the cell's built-in `shortLabel` is NOT a viable carrier: it renders through a 14-segment
display capped at 4 characters (`EncoderDraw.hpp`'s `UpperShortLabel(..., maxChars=4)`), so longer
text truncates silently — the same present-but-not-legible failure this requirement exists to stop.

#### Scenario: No indicator at the top level
- **WHEN** the operator has not drilled into any parameter's modulation view
- **THEN** no drill-level indicator is drawn

#### Scenario: The indicator names the current level and is inside the visible grid region
- **WHEN** the operator drills in and the modulation-source grid is displayed
- **THEN** the indicator names the current drill level
- **THEN** its resolved absolute bounds are fully contained within the encoder-grid block's own
  resolved bounds, asserted by computed containment rather than by the command's existence
  (`FroggersSurfaceTests.cpp`'s
  `drill_back_badge_resolves_inside_the_grid_region_the_operator_actually_sees`)
- **THEN** a populated modulation-source cell also resolves inside that same region, proving the
  region under test is the real one and the assertion cannot pass vacuously (OMNI §9.1)

#### Scenario: The indicator does not disturb the grid
- **WHEN** the indicator and its backing band are drawn
- **THEN** they are confined to the Target/Back cell's own already-resolved bounds
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

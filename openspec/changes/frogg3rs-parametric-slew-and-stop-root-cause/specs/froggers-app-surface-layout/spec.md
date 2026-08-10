# Delta — `froggers-app-surface-layout`

Two independent, operator-reported UI defects (`tasks.md` S5.2, S6). S6 is per-cell chrome inside the
already-resolved grid. S5.2, as of STEP 1 (2026-08-09), is not: on the operator's own instruction the
drill-level indicator now occupies a **dedicated row** in the right block, which supersedes the
earlier "must not disturb the 6×6 grid" constraint for this one case. Neither defect's fix changes any
row or column **weight**, margin, or gap; the added row is the block's only fixed-size row, so the
pre-existing weighted rows simply divide the space that remains.

## ADDED Requirements

### Requirement: The drill level is shown as a header bar above the modulation parameters
Whenever the active modulation drill level is greater than 0, a header bar SHALL render across the
full width of the encoder-grid block, positioned BELOW the bank-tabs row and ABOVE the first row of
modulation-parameter cells, reading `"Modulation Level N"`. It SHALL be its own dedicated
non-interactive row (`FroggersCellMap::RightKind::Header`, node `kModulationHeader`), structurally
independent of any button or parameter cell. Its reserved space SHALL persist at drill level 0 —
drawn empty — so entering or leaving a drilldown never reflows the bank tabs or the parameter grid.
At drill level 0 no text SHALL be drawn.

**Operator, 2026-08-08, rejecting the third attempt:** *"i don't know why you thought i wanted the
header to be 'Back' and by the back button, instead of a HEADER above all the modulation parameters,
below the bank button row?? ... nothing needs to be labeled 'back' there, that implementation
sucks."* A header is a title bar over the content it titles. Nothing in this region is labelled
"Back".

**This requirement took four attempts, and each failed assertion is recorded so the sequence is not
repeated.** (1) The indicator lived on the VCO scope cell; the test asserted the text command
EXISTS. (2) It was restyled; the test asserted it was appended last and NOT OVERDRAWN within its
node. (3) It moved to the Target/Back cell; the test asserted CONTAINMENT in the grid block. Each
assertion was true and each shipped something the operator rejected. The measured reason (1) and (2)
were invisible at all: `kVcoScope` resolves to `{16, 16, 284.67, 181.33}` in the LEFT block, while
the modulation grid resolves to `{314.67, 16, 569.33, 600}` in `kRightBlock` — disjoint columns
14px apart. Attempt (3) was visible and was rejected on SUBSTANCE, which no geometric assertion
could have caught: a badge on the one cell whose job is to LEAVE a level conflated two different
facts.

**Do not move this onto a cell.** A header is a bar above the content it titles; that is the
requirement, and the test asserts its position relative to the bank row and the parameter rows.

Note a cell's built-in `shortLabel` was also rejected as a carrier: it renders through a 14-segment
display capped at 4 characters (`EncoderDraw.hpp`'s `UpperShortLabel(..., maxChars=4)`), so longer
text truncates silently — the same present-but-not-legible failure this requirement exists to stop.

#### Scenario: No level text at the top level
- **WHEN** the operator has not drilled into any parameter's modulation view
- **THEN** the header row is present but draws no level text
- **THEN** the bank-tabs row and the parameter grid occupy the same bounds as when drilled in

#### Scenario: The header sits between the bank tabs and the parameters
- **WHEN** the operator drills in and the modulation-source grid is displayed
- **THEN** the header names the current drill level
- **THEN** its resolved absolute bounds lie below the bank-tabs row's bottom edge and above the
  first parameter row's top edge, asserted by computed geometry rather than by the command's
  existence or by its mere containment in the grid block -- three prior attempts each asserted a
  weaker property and each shipped an indicator the operator could not see
- **THEN** the bank-tabs row and at least one populated parameter cell resolve where the test claims
  they do, so the comparison is against real geometry and cannot pass vacuously (OMNI 9.1)

#### Scenario: Nothing in the grid is labelled "Back"
- **WHEN** the tree is built at any drill level
- **THEN** no drawn text anywhere in the tree contains "BACK"

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

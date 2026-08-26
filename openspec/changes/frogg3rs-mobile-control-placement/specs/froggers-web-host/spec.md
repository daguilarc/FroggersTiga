# Delta — `froggers-web-host`

The previous delta on this requirement said the Randomize and Reset rows go
"ABOVE the grid, with the other transport and scene controls". Its scenarios
tested only "above the grid", so an implementation that hoisted them inside
the ENCODER column passed every check while missing the point: the operator
asked for them beside the oscilloscope and sliders, in the empty half of the
viewport the chrome block leaves.

The requirement is restated so the placement is unambiguous, and the
scenarios now test the clause that was dropped — including one that fails if
the rows land in the encoder column.

## MODIFIED Requirements

### Requirement: Mobile viewport stacks around a full-width encoder grid
On mobile-width viewports, THE published site SHALL render the
sixteen-slot (4×4) encoder grid spanning the full viewport width, with
every other control placed above or below the grid, never beside it.
The legacy site's mobile stacking is the reference behavior; on small
screens, full-width placement takes precedence over grid element size.

The chrome block that carries the oscilloscope, transport, scenes, scene
blend and BPM SHALL itself span the full viewport width on those viewports.
It SHALL NOT render at a fraction of the width with the remainder left empty,
because that empty region is the only space on a phone large enough to hold
the transport-adjacent buttons without pushing the encoder grid off-screen.

The Randomize Page, Randomize All, Reset Page and Reset All buttons SHALL be
placed WITHIN that chrome block, beside the oscilloscope and the sliders,
occupying the width that would otherwise be empty. They SHALL NOT be placed
in the encoder column, above or below the encoder rows: hoisting them there
pushes encoder rows past the fold, which trades a control the operator
touches occasionally for controls they touch constantly.

Those four buttons SHALL be sized to their labels rather than stretched to a
share of the block width, so that four of them fit in the space beside the
sliders.

Widening the chrome block SHALL NOT push the encoder grid off the first
screen. The shell stacks the blocks vertically and scales them together, so a
chrome block that keeps its full-page height while doubling in width takes half
again as much vertical space and carries the grid down with it. The narrow
chrome block SHALL therefore declare a height that keeps its rows at the density
they were laid out for, so that the encoder grid still begins above the fold and
a full row of encoders is reachable without scrolling.

#### Scenario: Phone-width layout stacks
- **WHEN** the site loads at a phone-width viewport
- **THEN** the encoder grid spans the viewport width
- **THEN** no other control renders beside the grid — everything else
  sits above or below it

#### Scenario: The chrome block uses the whole width
- **WHEN** the site loads at a viewport width of 720px or less
- **THEN** the chrome block's rendered width is within 5% of the encoder
  grid block's rendered width
- **AND** neither block leaves an empty region wider than 10% of the viewport
  beside it

#### Scenario: The four buttons sit beside the sliders, not above the grid
- **WHEN** the site loads at a viewport width of 720px or less
- **THEN** each of the Randomize Page, Randomize All, Reset Page and Reset
  All buttons has its horizontal centre to the RIGHT of the BPM slider's
  horizontal centre
- **AND** each of their vertical centres falls within the chrome block's own
  top and bottom edges
- **AND** all four are inside the chrome block's bounding box

#### Scenario: They are NOT in the encoder column
- **WHEN** the site loads at a viewport width of 720px or less
- **THEN** no Randomize or Reset button falls inside the encoder grid
  block's bounding box
- **AND** exactly one node exists for each of the four buttons, so the narrow
  layout moved them rather than adding a second copy

#### Scenario: The encoder grid is still reachable near the top of the page
- **WHEN** the site loads at a viewport width of 720px or less
- **THEN** the first encoder row is fully within the viewport without
  scrolling

#### Scenario: Buttons are sized to their labels
- **WHEN** the site loads at a viewport width of 720px or less
- **THEN** no Randomize or Reset button is wider than half the chrome
  block's width

#### Scenario: Wide viewports keep the existing order
- **WHEN** the site loads at a viewport width greater than 720px
- **THEN** the Randomize and Reset rows remain below the encoder grid
- **AND** the chrome block keeps its existing narrower weighting

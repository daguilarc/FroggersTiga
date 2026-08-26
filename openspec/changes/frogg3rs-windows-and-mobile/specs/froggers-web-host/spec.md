# Delta — `froggers-web-host`

`froggers-web-host` already owns the published site's mobile layout: its
"Mobile viewport stacks around a full-width encoder grid" requirement says
every control other than the grid sits above or below it. It does not say
which, and Randomize/Reset landing below the whole grid satisfies it while
still burying them. That requirement is refined here rather than
re-specified in a second capability, so one document owns where mobile
controls go.

## MODIFIED Requirements

### Requirement: Mobile viewport stacks around a full-width encoder grid
On mobile-width viewports, THE published site SHALL render the
sixteen-slot (4×4) encoder grid spanning the full viewport width, with
every other control placed above or below the grid, never beside it.
The legacy site's mobile stacking is the reference behavior; on small
screens, full-width placement takes precedence over grid element size.

The Randomize and Reset rows SHALL be placed ABOVE the grid, with the other
transport and scene controls, rather than below it. Placing them below
satisfies the stacking rule while leaving them past the full height of the
grid and the sidebar, which is where they were found.

#### Scenario: Phone-width layout stacks
- **WHEN** the site loads at a phone-width viewport
- **THEN** the encoder grid spans the viewport width
- **THEN** no other control renders beside the grid — everything else
  sits above or below it

#### Scenario: Randomize and Reset are reachable without scrolling past the grid
- **WHEN** the site loads at a viewport width of 720px or less
- **THEN** the Randomize row's bounding-box top is above the first encoder
  row's bounding-box top
- **AND** the Reset row sits below Randomize and still above the first
  encoder row

#### Scenario: Wide viewports keep the existing order
- **WHEN** the site loads at a viewport width greater than 720px
- **THEN** the Randomize and Reset rows remain below the encoder grid

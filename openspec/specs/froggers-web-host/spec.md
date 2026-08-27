# froggers-web-host Specification

## Purpose
The same Sheaf app browser build replaces the public Froggers website; the legacy web and wasm trees have been retired entirely; publication is gated on the repository rename, which does not affect operator-visible product naming.
## Requirements
### Requirement: The new app is the public web build
The published Froggers website SHALL be served by the new Sheaf app's browser build. The previously deployed web application SHALL no longer be built or deployed. The same app type SHALL serve the desktop host, the Sheaf launcher package, and the public site, with no host-specific branching in the app core.

#### Scenario: The site serves the new app
- **WHEN** a visitor loads the published site
- **THEN** the new Sheaf app loads and audio runs
- **THEN** no artifact of the previous web application is served

#### Scenario: One app core, three surfaces
- **WHEN** the desktop build, the Sheaf catalog package, and the public site are compared
- **THEN** all three are produced from the same app type
- **THEN** the app core contains no branch selecting between them

### Requirement: Publication is gated on the repository rename
No public build SHALL be committed, pushed, or released under the old repository name. The repository SHALL be renamed first, so that the site URL, the published catalog URL, and the package artifact URLs are all minted under the new name and never have to be reissued.

#### Scenario: Rename precedes publication
- **WHEN** the first public build is prepared
- **THEN** the repository has already been renamed
- **THEN** the site base path, catalog URL, and artifact URLs all reflect the new name

#### Scenario: No stale-origin URL is handed out
- **WHEN** the catalog URL is given to the Sheaf launcher operator
- **THEN** that URL is on the renamed origin
- **THEN** it does not require a later redirect or reissue

### Requirement: Product naming survives the rename
The rename SHALL affect repository, origin, and URL identifiers only. Operator-visible product naming in the application's own surface SHALL be unaffected.

#### Scenario: Displayed name is unchanged
- **WHEN** the app renders its header
- **THEN** the displayed product name is unchanged by the rename

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

### Requirement: Site links carry the legacy roles under new references
THE published site SHALL present the same operator-facing link roles the
legacy site presents (desktop downloads, license, manual), each pointing
at the current product's equivalent: URLs minted under the renamed
origin, release links referencing the release being published, and the
manual link pointing at the manual that documents the published app.

#### Scenario: Link roles preserved, references renewed
- **WHEN** the published site is compared to the legacy site
- **THEN** every legacy link role is present and resolves
- **THEN** no link target carries the old repository name

### Requirement: Playwright layout regression for the published site
THE repository SHALL provide a Playwright suite for the new site — its
own harness, since the legacy `web/` tree and its suite are gone — with
mobile-emulated and desktop-emulated tests
asserting the mobile stacking scenario and the link roles without
starting audio, runnable in CI before any deploy.

#### Scenario: CI layout tests gate the deploy
- **WHEN** the site workflow runs
- **THEN** the mobile-emulated stacking assertion and the link-role
  check pass before the deploy step is reachable

### Requirement: Deploys happen only from the default branch
THE site workflow SHALL reach its deploy step only for the default
branch: a manual dispatch from any other ref SHALL build and test
without deploying, so the working branch can be exercised end to end
while the live site stays untouched until merge.

#### Scenario: Branch dispatch is a dry run
- **WHEN** the workflow is dispatched manually from a non-default branch
- **THEN** the site builds and its tests run
- **THEN** the deploy step does not execute and the live site is
  unchanged

### Requirement: Parameter controls are legible before audio starts
WHEN the published site loads, THE parameter controls SHALL show their
names and current values without requiring the visitor to press Play or
interact at all, matching the guarantee the retired web sim's mobile
layout made. Audio SHALL still not start without a user
gesture.

#### Scenario: Knobs are readable on arrival
- **WHEN** a visitor loads the site and does nothing
- **THEN** every encoder cell shows its parameter name and value
- **AND** no audio has started

### Requirement: Automated checks assert rendered visibility
THE site's automated checks SHALL assert that the surface is actually
VISIBLE — non-zero rendered extent and painted content — and SHALL NOT
rely on element geometry alone, which reports full bounding boxes for
content clipped to invisibility. Each such assertion SHALL be
demonstrated to fail against a build carrying the defect it guards.

#### Scenario: A blank page fails the suite
- **WHEN** a regression clips or blanks the rendered surface while
  leaving element geometry intact
- **THEN** the automated checks fail


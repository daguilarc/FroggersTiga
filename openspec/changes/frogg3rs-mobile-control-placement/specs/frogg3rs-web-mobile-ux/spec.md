# Delta — `frogg3rs-web-mobile-ux`

The surface-owned-topology requirement was written when the only mobile
difference was the right column's row order. The narrow topology now also
changes the chrome block's internal arrangement and the outer split's
weights, so the requirement is widened to cover them — and to say that a
narrow table which merely reorders rows is not, on its own, evidence the
requirement is met.

## MODIFIED Requirements

### Requirement: The surface owns its own mobile topology

A mobile-only difference in what the surface emits SHALL be expressed as data
consumed by the surface's existing emission code — a row table, a block
weight, or a nested arrangement — selected by a flag that defaults to false,
rather than as a shell-side CSS or DOM rearrangement. A shell that moves,
clips, or duplicates emitted controls makes the rendered tree disagree with
the surface that produced it, and the surface's own tests can no longer tell
what a viewer sees.

The narrow topology SHALL be free to differ in the OUTER split weights and in
a block's internal arrangement, not only in the order of rows within one
block. Where the shell derives a single scale from one block, a narrow
topology that leaves another SURFACE-EMITTED block narrower than the viewport
SHALL be treated as an incomplete topology rather than as a shell defect. This
covers the blocks this surface emits. Sheaf's own runtime sidebar is stacked by
the same shell but emitted by Sheaf, so this surface has no weight to declare
for it and it is out of scope here.

The flag SHALL be set only by the browser host. No desktop, standalone, or
plugin path SHALL set it.

#### Scenario: No duplicate controls at any width

- **WHEN** the surface renders at any viewport width
- **THEN** exactly one node exists for each of Randomize Page, Randomize All,
  Reset Page and Reset All
- **AND** the container carrying them is the only one: the wide layout's two
  rows and the narrow layout's column never both exist

#### Scenario: Other hosts are unaffected

- **WHEN** the standalone, VST, or AU host builds the surface
- **THEN** the narrow-viewport flag is false and the desktop layout is
  emitted, with the desktop split weights

#### Scenario: A narrow topology leaves no surface block short of the viewport

- **WHEN** the browser host sets the narrow-viewport flag
- **THEN** every block this surface emits renders at substantially the full
  viewport width under the shell's shared scale

### Requirement: The page scrolls, and a test proves it

On mobile-width viewports the published page SHALL scroll when its content is
taller than the viewport, and a scroll offset once set SHALL persist rather
than returning to the top on a later animation frame.

The suite SHALL assert this directly. The existing "page scrolling still
works" scenario has never had an assertion behind it, which is how a build
that cannot scroll at all reached the published site. An assertion SHALL
check the offset after several animation frames, not immediately, because the
offset survives the first frames and is lost afterwards.

#### Scenario: A scroll offset survives the render loop

- **WHEN** the page is scrolled down at a viewport width of 720px or less
- **AND** several animation frames elapse
- **THEN** the scroll offset is still where it was put

#### Scenario: Controls below the fold are reachable

- **WHEN** the content is taller than the viewport at a phone-width viewport
- **THEN** every emitted control can be brought into view by scrolling

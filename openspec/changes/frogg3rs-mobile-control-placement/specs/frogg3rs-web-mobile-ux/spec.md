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
topology that leaves another block narrower than the viewport SHALL be
treated as an incomplete topology rather than as a shell defect.

The flag SHALL be set only by the browser host. No desktop, standalone, or
plugin path SHALL set it.

#### Scenario: No duplicate controls at any width

- **WHEN** the surface renders at any viewport width
- **THEN** exactly one Randomize row and one Reset row exist in the node tree

#### Scenario: Other hosts are unaffected

- **WHEN** the standalone, VST, or AU host builds the surface
- **THEN** the narrow-viewport flag is false and the desktop layout is
  emitted, with the desktop split weights

#### Scenario: A narrow topology leaves no block short of the viewport

- **WHEN** the browser host sets the narrow-viewport flag
- **THEN** every stacked block renders at substantially the full viewport
  width under the shell's shared scale

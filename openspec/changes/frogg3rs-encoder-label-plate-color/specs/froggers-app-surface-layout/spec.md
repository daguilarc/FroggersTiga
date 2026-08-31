# Delta — `froggers-app-surface-layout`

## MODIFIED Requirements

### Requirement: Encoder labels are legible, natural, and never obscure the encoder
Each encoder cell SHALL render its parameter's NATURAL display label — the short form where that form is the parameter's canonical name (`A1`, `D1`, `S1`, `R1` and their siblings), the readable full name where the short form is merely a truncation (`Comb offset`, never only `CmbOff`) — per an operator-approved label list covering every parameter of every bank; neither blanket expansion nor blanket abbreviation satisfies this requirement (corrected 2026-08-17 after the first draft of this requirement mandated short names universally and the operator rejected it). No label rendering SHALL overlap the encoder's ring: label draw commands occupy vertical space disjoint from the ring's drawn arc in every cell of every bank. The operator's on-screen confirmation is the acceptance criterion, exercised on a proposed mock — including the full label list — BEFORE the change is built and again on the built result.

The label band's plate SHALL match the surface background. The plate stays
opaque — it is what keeps the glyphs legible when a visualizer underlay runs
under the band — but on a cell with nothing behind it the band is
indistinguishable from the background around it. The surface background SHALL
have a single named definition, read by the window's root fill, the encoder
cell fill, and the label plate, so the three cannot drift apart. Unlit ghost
segments derive their colour from the plate's, not from a second literal. The
badge chips on the encoder rings keep their own chip colour; a chip sits on
the knob, not on the surface, and is deliberately visible.

#### Scenario: Label plates sit flush on the background

- **WHEN** an encoder cell with no visualizer underlay renders
- **THEN** the pixels of its label plate match the surface background, and
  the band reads as glyphs on the surface, not as a lighter strip

#### Scenario: Glyphs stay legible over an underlay

- **WHEN** a visualizer underlay is visible on a cell
- **THEN** the plate occludes the underlay across the whole label band, and
  the glyphs render on the plate rather than on the moving trace

#### Scenario: One definition of the surface background

- **WHEN** the surface background colour is changed in a future edit
- **THEN** the root fill, the encoder cell fill, and the label plate all
  present the changed colour, because all three read the same single
  definition

# Delta — `froggers-app-surface-layout`

**Added 2026-08-17.** The predecessor's label rework was scoped to the ~25 over-length labels and shipped
as an unconditional long-name treatment that covers ~95% of every ring's lower semicircle. The operator
ruled it a failure on sight and their earlier ruling — short names like `A1 D1 S1 R1` need no expansion —
is binding here (`../../proposal.md` §2 W4).

## ADDED Requirements

### Requirement: Encoder labels are legible, natural, and never obscure the encoder
Each encoder cell SHALL render its parameter's NATURAL display label — the short form where that form is the parameter's canonical name (`A1`, `D1`, `S1`, `R1` and their siblings), the readable full name where the short form is merely a truncation (`Comb offset`, never only `CmbOff`) — per an operator-approved label list covering every parameter of every bank; neither blanket expansion nor blanket abbreviation satisfies this requirement (corrected 2026-08-17 after the first draft of this requirement mandated short names universally and the operator rejected it). No label rendering SHALL overlap the encoder's ring: label draw commands occupy vertical space disjoint from the ring's drawn arc in every cell of every bank. The operator's on-screen confirmation is the acceptance criterion, exercised on a proposed mock — including the full label list — BEFORE the change is built and again on the built result.

#### Scenario: Canonical short names render short
- **WHEN** a parameter's short form is its canonical name on the approved list
- **THEN** the cell renders that short form in the native single-row idiom
- **THEN** no expanded treatment is applied to it

#### Scenario: Truncations never stand alone
- **WHEN** a parameter's short form is a truncation rather than a name
- **THEN** the cell renders the readable label the approved list assigns it
- **THEN** that label is fully legible without cut-off characters, in space that does not overlap the ring's drawn arc

#### Scenario: The ring is never covered
- **WHEN** any cell of any bank is rendered, with or without an expanded label
- **THEN** no label draw command's bounds intersect the ring's circle

#### Scenario: The operator gate runs before the build
- **WHEN** a change to label rendering or cell geometry is proposed
- **THEN** the operator sees and approves a mock of the geometry before implementation begins
- **THEN** the built result is confirmed on screen by the operator before the change is considered done

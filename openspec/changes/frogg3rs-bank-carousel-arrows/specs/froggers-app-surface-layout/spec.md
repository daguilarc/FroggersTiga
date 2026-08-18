# Delta — `froggers-app-surface-layout`

**Added 2026-08-18, at the operator's instruction** (screenshot: arrows
centered between the page-bank buttons and the top encoder row). The existing
requirement already rules that arrow paging is never the primary navigation;
this delta carves in the secondary arrow pair explicitly rather than letting
it coexist unstated.

## MODIFIED Requirements

### Requirement: Bank selector with direct selection
The surface SHALL provide direct selection among banks. Arrow-based paging SHALL NOT be the primary navigation. Exactly one bank SHALL be active at a time, with a single authority for that selection.

The surface SHALL additionally provide a back/forward arrow pair as secondary navigation, horizontally centered within the band between the bank row and the encoder grid (the modulation-header row's reserved space, whose outer geometry SHALL NOT change in any state). The back arrow SHALL step the active bank to the previous index and the forward arrow to the next, wrapping at both ends, routed through the same single selection authority as direct selection; the bank-button highlight SHALL reflect an arrow-driven change identically to a button-driven one. WHILE a modulation drill-in is active, the arrows SHALL NOT render and SHALL NOT accept input, and the band SHALL render its drill-level title exactly as before this change.

#### Scenario: Direct bank selection
- **WHEN** the operator selects a bank
- **THEN** that bank's parameters populate the encoder grid
- **THEN** no second, divergent bank-selection state exists

#### Scenario: Forward arrow steps and wraps
- **WHEN** the operator clicks the forward arrow repeatedly from the first bank
- **THEN** the active bank advances one index per click, the highlight following each step
- **THEN** a click on the last bank wraps the selection to the first

#### Scenario: Back arrow steps and wraps
- **WHEN** the operator clicks the back arrow on the first bank
- **THEN** the selection wraps to the last bank, with exactly one bank highlighted

#### Scenario: Arrows yield to the drill-in title
- **WHEN** a modulation drill-in is active
- **THEN** the band shows the drill-level title with no arrow rendered and no arrow hit target
- **THEN** the band's bounds are identical to its bounds at the top level

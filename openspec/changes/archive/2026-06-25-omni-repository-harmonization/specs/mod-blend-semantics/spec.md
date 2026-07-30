## ADDED Requirements

### Requirement: Pair-AR uses the shared modulation path
Audio pair-AR effective values SHALL delegate the existing crossfade, clamp, and external-source availability behavior to `ModMgr::Modulate`. `AudioPairArState` SHALL NOT retain a second inline blend formula or accept only the raw mod array for effective-value calculation.

#### Scenario: Disabled external source
- **WHEN** a pair-AR route references external mod index 0 or 1 while that source's availability flag is false
- **THEN** its effective value equals the stored base knob value

#### Scenario: Enabled external source
- **WHEN** the same external source becomes available
- **THEN** the pair-AR effective value follows the shared `mod-blend-semantics` crossfade formula

#### Scenario: Internal source
- **WHEN** pair-AR references internal mod index 4, 5, or 6
- **THEN** its effective value uses the same shared blend path without external-presence gating

### Requirement: Shared modulation index handling is safe
`ModMgr::Modulate` SHALL return the base value for None, negative, or out-of-range indices and SHALL validate the index before reading either the availability array or mod array.

#### Scenario: None assignment
- **WHEN** `ModMgr::Modulate` receives the None sentinel
- **THEN** it returns the base knob value without array access

#### Scenario: Negative or oversized index
- **WHEN** `ModMgr::Modulate` receives an index below zero or at/above `x_numMods`
- **THEN** it returns the base knob value without undefined memory access

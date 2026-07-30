## ADDED Requirements

### Requirement: Fifteen-lane CV history is the UI sample authority
Desktop v2 SHALL maintain one UI-thread history store with fifteen rings, one per permanent modulation lane. Each UI tick SHALL push `GetCvOut(lane)` into that store once per lane. The global oscilloscope and detail-grid underlays SHALL read those histories; the oscilloscope refresh path SHALL NOT call `GetCvOut` again for the same tick.

#### Scenario: Single push per tick
- **WHEN** the UI timer runs while audio is running and a host is bound
- **THEN** each lane 0..14 receives exactly one sample push into the shared history store from `GetCvOut`
- **THEN** global oscilloscope traces update from that store for their bound lanes

### Requirement: Detail-grid lane encoders show source activity underlay
When parameter-detail is open, each of the fifteen source-depth encoder cells SHALL paint a display-only waveform underlay from that lane’s history ring, using the manifest lane color. The Target (Back) cell SHALL have no underlay. Underlays SHALL NOT receive pointer events; ring drag remains `ParamTurn` and center MOD remains `ModDrillIn`.

#### Scenario: Detail open shows lane underlays
- **WHEN** the operator opens modulation detail for a row
- **THEN** each source lane cell shows an underlay from the shared history for that lane index
- **THEN** the Target (Back) cell has no underlay

#### Scenario: Detail closed clears underlays
- **WHEN** the operator leaves parameter detail
- **THEN** all encoder underlay bindings are cleared

#### Scenario: Unavailable lanes stay dimmed
- **WHEN** a detail lane cell is greyed/unavailable
- **THEN** the underlay paints with the same reduced availability alpha as the encoder chrome

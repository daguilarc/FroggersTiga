## MODIFIED Requirements

### Requirement: Parameter detail uses a 4x4 encoder grid
Desktop v2 parameter-detail modulation view SHALL render sixteen encoder cells at the default standalone size: fifteen source-depth encoders plus one dedicated **Target (Back)** encoder. Each source-depth cell SHALL display a display-only CV activity underlay from the shared fifteen-lane CV history for that lane. The Target (Back) cell SHALL NOT show an underlay. Encoder interaction SHALL remain: ring annulus → `ParamTurn`; center MOD → `ModDrillIn`; Target press → exit detail.

#### Scenario: Source cells show activity underlay
- **WHEN** parameter detail is open and the shared CV history has samples for a lane
- **THEN** that lane’s depth encoder paints the underlay beneath encoder chrome
- **THEN** turn and ModDrillIn hit targets are unchanged

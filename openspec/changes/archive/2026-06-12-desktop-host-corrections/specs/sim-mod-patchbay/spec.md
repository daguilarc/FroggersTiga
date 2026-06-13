## MODIFIED Requirements

### Requirement: Sim mod source set (no external CV)

Sim host UI SHALL name mod sources as:

| Core index | Desktop / web label |
|------------|---------------------|
| 0 | MIDI (desktop only) |
| 4 | **VCO level** |
| 5 | Marbles 1 |
| 6 | Marbles 2 |

#### Scenario: Web mod dropdown

- **WHEN** the user opens a mod dropdown on web
- **THEN** options are `None`, **VCO level**, `Marbles 1`, `Marbles 2`

#### Scenario: Desktop mod rack

- **WHEN** the user views mod rack boxes
- **THEN** the VCO-derived source is labeled **VCO level**, not "VCO feat"

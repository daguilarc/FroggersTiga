## ADDED Requirements

### Requirement: Knob columns are the sole live parameter surface

The web sim SHALL display parameter values, names, and mod routing only through the eight knob columns (label, rotary knob, mod-source select). It SHALL NOT render a mod-route text summary above the knob field or an OLED duplicate panel below the knob field on any page, transport state, or viewport width.

#### Scenario: Page load before Play

- **WHEN** the user loads the sim before clicking Play
- **THEN** only knob columns and page chrome appear in the field area
- **AND** no bordered text summary appears above the knobs
- **AND** no black OLED panel appears below the knobs

#### Scenario: Play with active mod patches

- **WHEN** the user clicks Play and at least one row has mod source not **None**
- **THEN** knob positions and mod selects update from `screen` messages
- **AND** no additional route-summary strip appears above the knobs
- **AND** no eight-row OLED panel appears below the knobs

#### Scenario: Stop after Play

- **WHEN** the user clicks Stop
- **THEN** knob columns and labels remain visible
- **AND** no OLED panel appears or expands

### Requirement: Mod routing visible only in column controls

Mod assignment SHALL be readable and editable only via each column's mod-source dropdown and the knob's live position under CV. The sim SHALL NOT duplicate routing as prose lines (`Parameter ← Source · depth%`).

#### Scenario: Patched VCO1 to Marbles 1

- **WHEN** VCO1 mod source is Marbles 1 with non-zero depth
- **THEN** the VCO1 mod select shows Marbles 1
- **AND** the VCO1 knob animates with mod CV while playing
- **AND** no separate summary line lists `VCO1 ← Marbles 1 · N%`

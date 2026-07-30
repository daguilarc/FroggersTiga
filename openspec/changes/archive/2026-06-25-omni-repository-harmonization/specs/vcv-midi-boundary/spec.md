## ADDED Requirements

### Requirement: Froggers Tiga VCV modules expose no MIDI boundary
VCV modules in this repository SHALL NOT own Rack MIDI input/output queues, MIDI port widgets, MIDI CC enable controls, MIDI CC latches, or MIDI-specific saved state. MIDI-to-CV conversion SHALL be delegated to another Rack module.

#### Scenario: Primary module panel
- **WHEN** a Froggers Tiga primary VCV module is created
- **THEN** its panel and engine contain no MIDI In, MIDI Out, CC 1, or CC 2 control

#### Scenario: Existing patch migration
- **WHEN** an older patch containing Froggers MIDI or CC fields is loaded
- **THEN** obsolete MIDI fields are ignored while supported audio, knob, and CV state still loads

#### Scenario: Legacy parameter IDs remain stable
- **WHEN** a patch saved before removal of the two CC enable switches is loaded
- **THEN** the versioned loader discards old parameter IDs 1 and 2, maps every old voicing-knob ID `n >= 3` to new ID `n - 2`, leaves Random at ID 0, and preserves each supported knob value on its original semantic control

#### Scenario: Migrated patches are not remapped twice
- **WHEN** a patch saved by the MIDI-free module is loaded
- **THEN** its schema-v2 marker causes parameter IDs to load directly without applying the legacy mapping again

### Requirement: Per-parameter VCV inputs consume voltage directly
Each per-parameter VCV modulation input SHALL combine with the target's stored internal modulation route. The host SHALL first calculate `internalEffective = ModMgr::Modulate(base, modIndex, depth)`, then, when the jack is connected, calculate `finalEffective = clamp(internalEffective + voltage / 10, 0, 1)`. With the jack disconnected, `finalEffective` SHALL equal `internalEffective`. The CV path SHALL NOT quantize voltage into a shared internal mod-source index or mutate the base knob, stored `modIndex`, or stored depth.

#### Scenario: Positive modulation voltage
- **WHEN** a parameter has base value `0.25`, no internal route, and its jack receives `5 V`
- **THEN** its normalized effective value is `0.75`

#### Scenario: Jack disconnected
- **WHEN** a parameter CV jack is not connected
- **THEN** its effective value is calculated from its base knob and any stored internal mod route

#### Scenario: Connected CV combines with an internal route
- **WHEN** a parameter's stored internal route produces `internalEffective = 0.4` and its jack receives `5 V`
- **THEN** its final effective value is `0.9`, with the stored route evaluated once and the normalized jack voltage added afterward

#### Scenario: Combined modulation clamps once
- **WHEN** internal modulation plus normalized jack voltage falls below `0` or exceeds `1`
- **THEN** the final effective value is clamped to the normalized range after both contributions are combined

#### Scenario: External MIDI-to-CV module
- **WHEN** a user patches another Rack module's MIDI-to-CV output into a Froggers parameter jack
- **THEN** Froggers responds to the voltage without receiving or decoding MIDI

### Requirement: VCV modulation topology contains internal sources only
The VCV mod rack and random-mod source pool SHALL contain VCO Envelope, Random 1, and Random 2 at indices `4, 5, 6`. MIDI CC indices `0` and `1` SHALL be unavailable, absent from the panel, and excluded from random assignment.

#### Scenario: VCV mod rack construction
- **WHEN** the primary module panel is constructed
- **THEN** it presents exactly the three internal mod outputs in order `4, 5, 6`

#### Scenario: VCV random modulation
- **WHEN** VCV randomizes a mod assignment
- **THEN** the result is None or one of indices `4, 5, 6`, never `0` or `1`

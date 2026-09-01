# Delta — `froggers-sheaf-parameter-model`

## ADDED Requirements

### Requirement: The pitch range excludes the inaudible end

VCO pitch SHALL map its knob exponentially across a named range whose
ceiling stays in pitched, audible territory — high enough to clear the top
of the piano with headroom, low enough that no part of the knob's travel is
spent above what a listener can hear as a pitch. The floor is unchanged and
launch sits on it. The range has one named definition read by the pitch
mapping, so the ceiling cannot drift back by way of a repeated literal. The
filter-frequency ceilings deliberately keep the full audible span: a filter
opened entirely out of the way is intended behaviour, and their ranges are
not this requirement's.

#### Scenario: The top of the knob is a pitch, not a whistle

- **WHEN** any VCO pitch knob sits at the top of its travel
- **THEN** the oscillator's fundamental is a high but audibly pitched note,
  above the top of the piano yet far below the audibility limit

#### Scenario: The floor and launch are untouched

- **WHEN** the instrument launches with the default patch
- **THEN** each pitch knob sits at the unchanged floor and the launch sound
  is identical to before the ceiling moved

#### Scenario: One definition of the pitch range

- **WHEN** the pitch range is changed in a future edit
- **THEN** the mapping and every check that pins it move together, because
  all read the same named constants

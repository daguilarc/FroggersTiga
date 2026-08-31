# Delta — `froggers-transport-and-reset-controls`

## ADDED Requirements

### Requirement: New returns the instrument to its fresh-launch state

New SHALL leave the instrument in the same state a fresh launch presents,
including every modulation depth the launch state carries. New is reached
through the runtime's File page rather than the app's own surface, but it
restores the app's state and is governed here alongside Reset.

Restoring the launch state SHALL NOT be reconstructible from per-parameter
registration alone. A parameter's registered default is a single value on one
parameter and cannot express a modulation depth, which is a relationship between
a target parameter and a source slot. Any path claiming to restore the default
state SHALL therefore restore it from the state the application actually
established at startup, not from registration.

#### Scenario: New restores the cross-oscillator modulation depths

- **WHEN** the operator presses New
- **THEN** the cross-oscillator pitch modulation depths are present at the same
  values a fresh launch shows, not absent and not neutral
- **THEN** the three oscillators are not left in unison

#### Scenario: New, Reset All and launch agree

- **WHEN** the instrument's state is captured after a fresh launch, after New,
  and after Randomize All followed by Reset All
- **THEN** all three states are identical, both in which parameters and
  modulation depths exist and in the values they carry

#### Scenario: Drift in any one of the three paths is caught

- **WHEN** any one of launch, Reset All, or New is changed so that it no longer
  produces the same state as the other two
- **THEN** a check fails

## MODIFIED Requirements

### Requirement: Reset restores the default patch
THE Reset controls SHALL revert to the instrument's fresh-launch
default patch — the values a first launch presents, which are 0 for
most parameters but not all — never to a flat all-zeros state no
launch ever shows. Reset All SHALL be global: every bank's page
parameters, every parameter's modulation depths, every bank's local
Crispy, and the shared global Crunchy all revert to their defaults.
Reset Page SHALL revert the currently shown bank's slice of that same
default patch, including that bank's Crispy, and SHALL NOT touch other
banks. From a drilled-in modulation grid, reset SHALL revert the
selected parameter's modulation depths to their default-patch values.
The default patch SHALL have a single definition shared by launch,
reset, and New, so the three can never drift apart.

Equality with a fresh launch SHALL be evaluated over which parameters and
modulation depths EXIST as well as the values they carry. A depth parameter that
was materialized by an operation and left at a neutral value is not equal to one
that was never materialized.

Equality SHALL further be observable in the instrument's AUDIO OUTPUT, not only
in its stored values. A reset that leaves every stored value correct while the
instrument goes on sounding differently has not restored the default patch. This
clause exists because parameter-level equality has held while the instrument
audibly did not decay, so the two are not interchangeable evidence.

#### Scenario: Reset All lands exactly on a fresh launch
- **WHEN** the operator has changed parameters, depths, Crispy, and
  Crunchy — including via Randomize All — and presses Reset All
- **THEN** the instrument's entire state equals a fresh launch's
  default patch, field for field, in both scene poles
- **THEN** the set of materialized modulation depth parameters equals a fresh
  launch's set, with no extra depths left over from the operations before it
- **THEN** Crispy on every bank and global Crunchy are at their
  default values

#### Scenario: Reset All restores the envelope's decay, not only its values

- **WHEN** the operator presses Randomize All, and then presses Reset All on a
  later block than the randomize landed on
- **THEN** the instrument decays to silence on the same schedule a fresh launch
  does, measured in the audible band rather than as a broadband level
- **THEN** this holds however many blocks separate the two presses, so that a
  reset arriving on the same block as the randomize is not the only case that
  works

#### Scenario: Reset Page restores that page's defaults, not zeros
- **WHEN** the Audio page's parameters have been edited and Reset Page
  is pressed while the Audio page is shown
- **THEN** the Audio bank's parameters return to their default-patch
  values — including the non-zero VCO shape defaults and the default
  cross-VCO pitch modulation depths — and the bank's Crispy returns to
  its default
- **THEN** every other bank's state is untouched

#### Scenario: One definition of the default patch
- **WHEN** the default patch is changed in a future edit
- **THEN** launch, reset, and New all present the changed defaults, because
  all three read the same single definition

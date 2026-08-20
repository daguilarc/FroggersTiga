# Delta — `froggers-transport-and-reset-controls`

**Added 2026-08-19 by operator instruction — and closing a broken
promise found at audit: this spec's Purpose has claimed "reset
semantics for parameters and depths" while carrying no reset
requirement at all.**

## ADDED Requirements

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
The default patch SHALL have a single definition shared by launch and
reset, so the two can never drift apart.

#### Scenario: Reset All lands exactly on a fresh launch
- **WHEN** the operator has changed parameters, depths, Crispy, and
  Crunchy — including via Randomize All — and presses Reset All
- **THEN** the instrument's entire state equals a fresh launch's
  default patch, field for field, in both scene poles
- **THEN** Crispy on every bank and global Crunchy are at their
  default values

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
- **THEN** launch and reset both present the changed defaults, because
  both read the same single definition

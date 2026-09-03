# Delta — `froggers-modulation-slate`

## MODIFIED Requirements

### Requirement: Randomized source count is biased toward few, and depth storage is allocated once
The randomizer SHALL draw its source count geometrically, each count half as likely as the one below it, from a floor that depends on where the press happened: on a parameter page the floor is zero, so about half of all calls draw no sources at all; at a drilled-in modulation level the floor is one, so every call draws at least one source and one source is the most likely outcome. Depth storage for a given source SHALL be allocated once, on first use, rather than accumulating additional storage across repeated randomization presses.

A parameter the page-level draw leaves at zero sources SHALL carry no modulation
depth and SHALL therefore show no modulation badge, so that a randomized bank
reads as a set of deliberate choices rather than as everything touched at once.

A drilled-in press SHALL NOT be a no-op: the floor of one applies both to the
selected parameter's own depths and to the one-level descent that materializes
the level below it.

#### Scenario: Some parameters come out of a page randomize untouched
- **WHEN** Randomize All is pressed while a parameter page is active
- **THEN** about half the parameters carry no modulation depth
- **AND** those parameters show no modulation badge
- **AND** the remaining parameters carry at least one non-neutral depth

#### Scenario: A drilled-in randomize always moves something
- **WHEN** Randomize All is pressed while a modulation detail grid is active
- **THEN** the selected parameter carries at least one non-neutral depth, on every press
- **AND** one source is the most common outcome, two about half as often, three about half as often again
- **AND** each depth the press materializes is itself randomized under the same floor

#### Scenario: Wide draws stay rare
- **WHEN** modulation depths are randomized repeatedly on a parameter page
- **THEN** four or more sources are affected on about one call in sixteen

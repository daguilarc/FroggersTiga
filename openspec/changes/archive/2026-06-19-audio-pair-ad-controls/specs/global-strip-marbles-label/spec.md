## ADDED Requirements

### Requirement: Global strip labels name their target

The simulator global strip SHALL expose four actions with labels that state what each control affects. The marbles step action (steps both random S&H bags) SHALL NOT use the bare label **Random**.

#### Scenario: Marbles button labeled Rand Resample on desktop

- **WHEN** the user views the desktop global strip
- **THEN** the marbles step button reads **Rand Resample**
- **THEN** the sibling buttons read **Rand All**, **Rand Mods**, and **Rand waveforms**

#### Scenario: Marbles button labeled Rand Resample on web

- **WHEN** the user views the browser global strip
- **THEN** the marbles step button reads **Rand Resample** (not **Random**)

### Requirement: Single label authority for global strip

Global strip button labels SHALL come from `ParamDisplayNames::forGlobalStrip` — not hardcoded strings in `GlobalStrip.cpp`, `index.html`, or manuals diverging from code.

#### Scenario: Desktop reads ParamDisplayNames

- **WHEN** `GlobalStrip` constructs its buttons
- **THEN** button text is assigned from `ParamDisplayNames::forGlobalStrip`

### Requirement: Manual and quick-dict use Rand Resample

Operator docs SHALL use the button label **Rand Resample** and explain that it resamples both random S&H channels by **drawing new values from each bag** (marbles step — no internal clock).

#### Scenario: Manual global strip table

- **WHEN** the user reads the global controls table in `SIM_MANUAL.md`
- **THEN** the marbles row is labeled **Rand Resample** with description “Resample both random S&H channels (draws from bags)”

#### Scenario: Quick Dict entry

- **WHEN** the user reads `QUICK_DICT.md` global controls
- **THEN** the line reads `Rand Resample — Resample both S&H channels (draws from bags)`

#### Scenario: Keyboard hint updated

- **WHEN** the user reads the web keyboard hint line
- **THEN** `m` is documented as **Rand Resample**, not bare **Random**

# Delta — `global-strip-marbles-label`

`SIM_MANUAL.md` is retired in this change (its mirror apparatus has no
remaining reader — see the `sim-operator-doc-parity` delta). The one
requirement here that names it as a SHALL target needs to drop that name;
the button-label fact itself is untouched and still lives in `QUICK_DICT.md`,
which stays.

## MODIFIED Requirements

### Requirement: Manual and quick-dict use Rand Resample

Operator docs SHALL use the button label **Rand Resample** and explain that
it resamples both random S&H channels by **drawing new values from each
bag** (marbles step — no internal clock).

#### Scenario: Quick Dict entry

- **WHEN** the user reads `QUICK_DICT.md` global controls
- **THEN** the line reads `Rand Resample — Resample both S&H channels (draws from bags)`

#### Scenario: Keyboard hint updated

- **WHEN** the user reads the web keyboard hint line
- **THEN** `m` is documented as **Rand Resample**, not bare **Random**

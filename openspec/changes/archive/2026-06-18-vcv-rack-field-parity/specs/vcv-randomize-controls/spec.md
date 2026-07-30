## ADDED Requirements

### Requirement: Randomize buttons match web/desktop

VCV SHALL provide physical controls for every randomize action available on web and desktop.

| Faceplate label | Host | Engine |
|-----------------|------|--------|
| Randomize | Per voicing column + FX Reverb/Delay | `RandomizePage(page)` |
| Randmod | Per voicing column + FX Reverb/Delay | `RandomizePageMod(page)` |
| Rand All | Main global strip | `RandomizeAllPages()` |
| Rand Mods | Main global strip | `RandomizeAllMod()` |
| Rand Resample | Main global strip | marbles step (`ButtonCallback(0)`) |
| Rand waveforms | Main global strip | `RandomizeVcoMorphs()` |

Delay column on FX uses `DelayState::randomizeKnobs` / `randomizeMod` instead of page manager APIs.

#### Scenario: No silent omission

- **WHEN** a user compares VCV faceplate to desktop global strip and submodule headers
- **THEN** every desktop randomize button has a corresponding VCV control with path silkscreen label

#### Scenario: Miswire regression blocked

- **WHEN** the main module **Rand Resample** control fires
- **THEN** `RandomizeAllPages()` is not called
- **WHEN** **Rand All** fires
- **THEN** `RandomizeAllPages()` is called

#### Scenario: Silkscreen on new buttons

- **WHEN** randomize buttons are added to the widget tree
- **THEN** matching path labels appear on the SVG at the same grid anchors

### Requirement: Randomize actions from one dispatch table

All momentary randomize params SHALL register in a single action table processed by one rising-edge handler in `process()` — not separate copy-paste `if` blocks per button.

#### Scenario: Rand Resample lives in global strip only

- **WHEN** the mod rack row is inspected after apply
- **THEN** no marbles toggle occupies a mod cell slot
- **THEN** **Rand Resample** is a global-strip button wired to `ButtonCallback(0)`

#### Scenario: One handler for all randomize edges

- **WHEN** any randomize param transitions low→high
- **THEN** the shared dispatcher invokes the mapped engine call exactly once

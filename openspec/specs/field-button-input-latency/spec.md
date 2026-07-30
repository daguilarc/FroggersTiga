# field-button-input-latency Specification

## Purpose

Daisy Field firmware control-loop architecture for responsive tactile switches and randomize buttons under audio load: fast input polling decoupled from OLED refresh, queued heavy randomize, and toolchain parity with proto Froggers.

Hardware diagnostic findings (e.g. SW1 stuck-input on a specific unit) are recorded in `docs/daisy-field-diagnostics.md` and are out of scope for latency acceptance on affected hardware.

## Requirements

### Requirement: Fast control poll decoupled from OLED refresh

Field firmware SHALL sample tactile switches (SW1/SW2), keyboard edges (B1–B5, A1–A8), and ADC knobs on a fast main-loop path that is not blocked by a full SSD1306 redraw every iteration. OLED updates SHALL be throttled (maximum ~30 full frames per second) or driven by a dirty flag after page/param changes.

#### Scenario: SW1 polled while OLED would block

- **WHEN** the main loop would previously spend longer than 10 ms in `UpdateScreen()`
- **THEN** `ProcessAllControls()` still runs at least once before the next full OLED refresh

#### Scenario: Page change marks dirty

- **WHEN** SW1 or SW2 triggers `PagePrevious` or `PageNext`
- **THEN** the OLED refresh occurs on the next allowed frame boundary without delaying switch edge detection

### Requirement: Heavy randomize is queued

Field firmware SHALL enqueue `RandomizeAllPages` (B2) and `RandomizeAllPagesMod` (B4) on rising edge and apply them incrementally across main-loop iterations. The control poll loop SHALL NOT run either operation synchronously to completion in a single iteration. Duplicate pending Rand-All requests SHALL coalesce to one pending mutation.

#### Scenario: B2 under audio load

- **WHEN** the user presses B2 while `FroggersEngine` is processing a full block
- **THEN** SW1/SW2 remain responsive within the same session (no multi-hundred-ms poll gap attributable to Rand All)

#### Scenario: B1 remains immediate

- **WHEN** the user presses B1 (randomize current page knobs)
- **THEN** current-page knob values update in the same main-loop iteration (no queue)

### Requirement: SW1 and SW2 work during mod-assign

Field firmware SHALL process SW1/SW2 page switches regardless of `PageManager::m_modIndex`. A page switch SHALL exit mod-assign mode per existing `PageNext`/`PagePrevious` behavior.

#### Scenario: SW1 while A1 held

- **WHEN** the user holds A1 (mod-assign) and presses SW1
- **THEN** the page changes and mod-assign ends

### Requirement: Toolchain parity with proto Froggers baseline

Firmware builds SHALL use `APP_TYPE=BOOT_NONE`, `OPT_LEVEL=-Os`, `USE_LTO=1`, and Arm GNU Toolchain 14.3.rel1 as documented in `src/mk/config.mk`. No switch to bootloader app types is required for this capability.

#### Scenario: Release build flags

- **WHEN** `make` runs in `src/FroggersTiga` without overrides
- **THEN** the effective compile flags match the proto baseline documented in the change design (no accidental `-O0`, no `BOOT_SRAM` default)

### Requirement: Acceptance bench for button latency

Release verification SHALL include a manual bench: rapid SW1/SW2 taps (≥5 presses in 2 s) with audio playing and knobs mid-travel; page title on OLED and SW tactile LEDs SHALL track presses without sustained unresponsiveness (>200 ms dead window).

#### Scenario: Full-load SW test

- **WHEN** audio is playing with external input gated or VCO mix active and user taps SW1 repeatedly
- **THEN** at least 4 of 5 presses change the visible page name on OLED within one throttle frame of the press

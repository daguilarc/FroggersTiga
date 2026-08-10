## ADDED Requirements

### Requirement: Control-rate param apply is per audio block

Field firmware audio engine SHALL apply control-rate parameter target updates (`UpdateParams`) at most once per audio block, after block-level param reads. The per-sample path SHALL NOT re-run those control-rate target sets.

#### Scenario: ProcessSample does not re-apply control-rate targets

- **WHEN** `FroggersEngine::ProcessBlock` processes N samples
- **THEN** control-rate `UpdateParams` work runs once for that block, not N times

### Requirement: LED bus transmit is throttled

Field firmware SHALL compute keyboard/SW/mod LED levels on the fast poll path but SHALL NOT call LED driver `SwapBuffersAndTransmit` on every poll iteration. LED frames SHALL transmit when LED state is dirty or on the same ~30 Hz budget used for OLED refresh.

#### Scenario: Rapid poll without LED I2C every lap

- **WHEN** `ProcessControls` runs many times within one OLED throttle window with unchanged LED intent
- **THEN** LED transmit occurs on the throttle floor only (at most one transmit per `kScreenThrottleMs` while unchanged)

#### Scenario: Dirty LED transmits without waiting full throttle

- **WHEN** LED intent changes (dirty) after a prior transmit in the same OLED window
- **THEN** the next `ProcessControls` may transmit once for that dirty frame (zero or one transmit per poll; dirty path is allowed inside the OLED window)

### Requirement: Dry reverb early-out without removing reverb page

Field firmware SHALL keep the reverb page, parameters, and delay buffers. After advancing smoothed reverb mix each sample, the audio path SHALL apply hysteresis: enter dry bypass when mix ≤ `1e-4`, leave dry bypass when mix ≥ `5e-4`. While in dry bypass, the audio path SHALL skip reverb delay-line processing, zero wet L/R, and pass dry signal.

#### Scenario: RVMX dry skips delay-line work

- **WHEN** dry bypass is active (mix at or below enter threshold, or still below exit threshold after prior enter)
- **THEN** output is dry and reverb delay lines are not advanced for that sample path

#### Scenario: Reverb page remains operable

- **WHEN** the operator navigates to the reverb page and raises smoothed RVMX to or above the exit threshold
- **THEN** reverb wet processing resumes without requiring a firmware rebuild or page removal

## MODIFIED Requirements

### Requirement: Heavy randomize is queued

Field firmware SHALL enqueue `RandomizeAllPages` (B2) and `RandomizeAllPagesMod` (B4) on rising edge and apply them incrementally across main-loop iterations. Each drain step SHALL randomize at most one page of parameters (or mod routes). The control poll loop SHALL NOT complete an entire all-pages randomize in a single iteration. Duplicate Rand-All requests of the same type SHALL coalesce: while a drain of that type is active, or when the last queued entry has the same type, enqueue SHALL no-op without resetting the page cursor. B1/B3 (current-page) SHALL remain immediate. OLED SHALL mark dirty after every successful one-page drain.

#### Scenario: B2 under audio load

- **WHEN** the user presses B2 while `FroggersEngine` is processing a full block
- **THEN** SW1/SW2 remain responsive within the same session (no multi-hundred-ms poll gap attributable to Rand All)

#### Scenario: B2 drain is one page per poll

- **WHEN** a pending Rand All mutation spans multiple pages
- **THEN** one `DrainOne` call updates exactly one page before returning to the poll loop

#### Scenario: B1 remains immediate

- **WHEN** the user presses B1 (randomize current page knobs)
- **THEN** current-page knob values update in the same main-loop iteration (no queue)

### Requirement: Acceptance bench for button latency

Release verification SHALL include a manual bench with audio playing: (1) rapid SW2 taps (≥5 in 2 s) with page title tracking without >200 ms dead window; (2) repeated B2/B4 presses every few seconds without sustained unresponsiveness of SW2 or B1–B4 (>200 ms dead window). SW1 may be N/A on units with diagnosed stuck hardware.

#### Scenario: Full-load SW test

- **WHEN** audio is playing with external input gated or VCO mix active and user taps SW2 repeatedly
- **THEN** at least 4 of 5 presses change the visible page name on OLED within one throttle frame of the press

#### Scenario: Rand All spam remains responsive

- **WHEN** the user presses B2 or B4 repeatedly every few seconds with audio playing
- **THEN** SW2 and B1–B4 continue to register presses without a sustained >200 ms dead window attributable to randomize or OLED/LED work

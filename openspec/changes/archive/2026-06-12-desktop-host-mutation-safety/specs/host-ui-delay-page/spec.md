## MODIFIED Requirements

### Requirement: Randomize parity

Per-panel and global randomize actions SHALL include Delay parameters with the same UX as core pages.

#### Scenario: Per-panel Randomize mod on Delay

- **WHEN** the user clicks **Randomize mod** on the Delay panel
- **THEN** Delay mod **sources** and **depths** SHALL randomize using only sim-valid indices `{255, 0, 4, 5, 6}` on the audio thread

#### Scenario: Global Randomize mod includes Delay sources

- **WHEN** the user clicks global **Randomize mod (all)**
- **THEN** core pages and Delay mod sources and depths SHALL randomize with sim-valid indices only via one queued mutation

#### Scenario: Delay randomize mod updates overlay cables

- **WHEN** Delay **Randomize mod** assigns VCO Envelope to DTIM row
- **THEN** overlay draws cable from VCO Envelope output to DTIM input

### Requirement: Patch cables on Delay panel

Desktop patch cables SHALL assign mod sources to Delay rows through `DelayState`, not through `PageManager` or `SetPageModSource` with page index 5. Delay panel SHALL use the **same** bidirectional VCV drag rules as core panels (empty input, output, connected input).

#### Scenario: New cable to Delay row from empty input

- **WHEN** the user drags from an empty Delay input jack to a mod rack output
- **THEN** `DelayState` SHALL store the mod source for that row via the mutation queue

#### Scenario: Disconnect Delay cable

- **WHEN** the user drags a connected Delay plug to void
- **THEN** that row's `modSource` SHALL become `255`

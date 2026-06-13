## ADDED Requirements

### Requirement: Engine bootstrap runs on Play click

The web sim SHALL NOT call `initWorklet()` on page load. The first **Play** click SHALL fetch WASM, compile, add the AudioWorklet module, connect the graph, and resume the AudioContext if suspended.

#### Scenario: Page load does not bootstrap

- **WHEN** the sim page loads
- **THEN** no WASM fetch or `addModule` runs
- **AND** knob labels are already visible (static labels)
- **AND** status shows an idle message such as "Click Play to start"

#### Scenario: First Play bootstraps engine

- **WHEN** the user clicks **Play** for the first time
- **THEN** status shows "Loading engine..." during bootstrap
- **AND** on success status shows **Playing** with sample rate
- **AND** VCO output is audible with External off

#### Scenario: Second Play after Stop reuses worklet

- **WHEN** the user clicks **Stop** then **Play** again in the same session
- **THEN** bootstrap does not re-fetch WASM unless the worklet was torn down
- **AND** audio resumes without full reload delay

### Requirement: External can trigger bootstrap if needed

When **External** is turned on before audio has started, the sim SHALL call `initWorklet()` if no worklet exists, consistent with `web-external-audio-permission`.

#### Scenario: External before Play

- **WHEN** the user clicks **External** on before **Play** and no worklet exists
- **THEN** `initWorklet()` runs from the External click handler
- **AND** pessimistic External UI rules still apply (no On until stream connects)

### Requirement: Bootstrap failure leaves Play retryable

If the first Play bootstrap fails, the sim SHALL re-enable the Play button and show a readable error status. Play SHALL NOT remain permanently disabled when `engineReady` is false.

#### Scenario: Failed first Play

- **WHEN** the user clicks Play and WASM fetch or `addModule` fails
- **THEN** status shows an error with retry hint
- **AND** Play is enabled again for a subsequent attempt

### Requirement: Page Randomize requires engine

Page **Randomize** controls SHALL NOT call WASM until the worklet is ready. Before `engineReady`, buttons SHALL be disabled or show **Click Play first** on activation.

#### Scenario: Randomize before Play

- **WHEN** the user clicks page Randomize before the engine has bootstrapped
- **THEN** no worklet message is sent
- **AND** status indicates Play is required first

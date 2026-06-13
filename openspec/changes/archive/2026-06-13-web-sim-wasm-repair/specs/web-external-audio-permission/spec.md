## ADDED Requirements

### Requirement: External requests mic permission on user gesture

The web sim SHALL call `navigator.mediaDevices.getUserMedia({ audio: … })` only when the user clicks **External** to turn it on. Play with **External: Off** SHALL NOT trigger a microphone permission prompt.

#### Scenario: Play without mic prompt

- **WHEN** the user clicks **Play** with **External: Off**
- **THEN** no microphone permission dialog appears
- **AND** VCO output is audible

#### Scenario: External on triggers permission

- **WHEN** the user clicks **External** to turn it on
- **THEN** the browser shows a microphone permission prompt (unless previously denied)
- **AND** `getUserMedia` is invoked from the click handler (user gesture)

#### Scenario: External on before Play

- **WHEN** the user clicks **External** to turn it on before **Play**, and WASM bootstrap has finished or is still loading
- **THEN** the click handler ensures a worklet exists (`await initWorklet()` when needed) before `getUserMedia`
- **AND** **External: Off** remains shown until the mic stream connects

### Requirement: Pessimistic External UI

The **External: On** label and WASM `external: true` SHALL apply only after `getUserMedia` succeeds and the mic stream is connected to the AudioWorklet.

#### Scenario: Permission pending

- **WHEN** the user clicks **External** and the permission dialog is open
- **THEN** the button remains **External: Off** until the stream connects

#### Scenario: Permission granted

- **WHEN** the user grants microphone access
- **THEN** the button shows **External: On**
- **AND** external audio feeds the ring-mod path while audio is running

### Requirement: Denied and blocked mic recovery

When microphone access is denied, not found, or unavailable (insecure context), the status line SHALL show a human-readable message with recovery steps. External SHALL remain **Off**.

#### Scenario: User denies permission

- **WHEN** the user denies the microphone prompt or the site permission is **denied**
- **THEN** status explains that the microphone is blocked
- **AND** instructs the user to allow microphone access in browser site settings and click **External** again
- **AND** **External: Off** is shown

#### Scenario: No microphone device

- **WHEN** `getUserMedia` fails with no input device
- **THEN** status states that no microphone was found
- **AND** **External: Off** is shown

#### Scenario: Insecure context

- **WHEN** the page is not a secure context and `getUserMedia` is unavailable
- **THEN** status states that HTTPS is required for external audio
- **AND** **External: Off** is shown

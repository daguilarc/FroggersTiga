## ADDED Requirements

### Requirement: MIDI in before audio block

`CvMidiBridge::drainMidiIn()` (or equivalent) SHALL run inside host `tickControls()` **before** `ProcessBlock`, then `applyCvPresence()` SHALL run on mod values.

#### Scenario: CC visible in same block

- **WHEN** a MIDI CC arrives immediately before an audio callback
- **THEN** `ModMgr::Modulate` in that block's `ProcessBlock` sees the updated mod value

### Requirement: Sim hosts — one MIDI mod bus

On **desktop sim** (and web N/A), external modulation SHALL enter through **one** MIDI input mapping:

- One listen **channel** (default 1)
- One listen **CC** number (user-configurable, default CC 1)
- Value 0–127 → `m_mods[0]` as float ∈ [0, 1]
- `m_externalCvActive[0]` reflects MIDI presence on that CC

Sim hosts SHALL NOT map four separate CCs to `m_mods[0..3]` in the UI or settings panel. VCV phase 2 MAY retain four CV jacks for Field parity (separate from sim desktop profile).

#### Scenario: Single CC drives MIDI mod source

- **WHEN** MIDI CC 64 arrives on the configured channel and CC number
- **THEN** `m_mods[0]` is approximately 0.5 and the desktop MIDI module meter reflects it

#### Scenario: Desktop MIDI settings simplified

- **WHEN** the user opens MIDI settings on desktop sim
- **THEN** one in-channel picker, one in-CC field, and envelope out settings are shown — not four M1–M4 CC fields

### Requirement: Envelope to MIDI CC out

`CvMidiBridge::tickMidiOut()` SHALL run **after** each audio block, read `FroggersEngine::GetEnvelopeLevel()` (last sample in the block), and emit MIDI CC (0–127) proportional to envelope ∈ [0,1]. Default: channel 1, CC 74; user-configurable in desktop settings.

#### Scenario: Envelope drives MIDI out

- **WHEN** external audio envelope rises to 0.5 during processing
- **THEN** MIDI CC out sends approximately 64

### Requirement: Sim hosts only (firmware)

`CvMidiBridge` SHALL be used by desktop and VCV hosts only. `DaisyIO.hpp` SHALL NOT send or receive MIDI.

#### Scenario: Firmware unchanged

- **WHEN** inspecting `DaisyIO.hpp` after implementation
- **THEN** no MIDI send/receive calls exist

### Requirement: Engine envelope accessor

`FroggersEngine` SHALL expose `GetEnvelopeLevel()` returning the envelope follower output used by `m_extGate`, updated per `ProcessSample`.

#### Scenario: Envelope matches gate input

- **WHEN** a 1 kHz tone is fed to `ProcessSample` at −12 dBFS
- **THEN** `GetEnvelopeLevel()` is non-zero and correlates with input amplitude

# midi-cc-to-mod-cv Specification

## Purpose
Map incoming MIDI CC values to normalized mod CV levels consumed by the shared modulation manager across sim hosts.
## Requirements
### Requirement: Two independent CC inputs drive mod CV

The host SHALL accept two configurable MIDI input pairs. Each pair SHALL consist of an In Channel (1–16) and a CC number (0–127). Matching Control Change messages SHALL set latched CV on the corresponding mod index: pair 1 → `mods[0]`, pair 2 → `mods[1]`. CV SHALL equal `controllerValue / 127.0f`.

#### Scenario: MIDI CC 1 updates mod zero

- **WHEN** a CC message arrives on MIDI CC 1 channel with MIDI CC 1 number set to 1 and value 64
- **THEN** `mods[0]` equals approximately 0.504 (64/127)

#### Scenario: MIDI CC 2 updates mod one

- **WHEN** a CC message arrives on MIDI CC 2 channel with MIDI CC 2 number set to 2 and value 127
- **THEN** `mods[1]` equals 1.0

#### Scenario: Latched CV holds between messages

- **WHEN** a matching CC updated `mods[0]` and no further matching CC 1 messages arrive
- **THEN** `mods[0]` retains the last value

### Requirement: Notes ignored

Note-on and note-off messages SHALL NOT affect `mods[0]` or `mods[1]`.

#### Scenario: Notes have no effect

- **WHEN** note messages arrive on either configured in channel
- **THEN** mod CV values change only from matching CC messages

### Requirement: No QWERTY piano input

The desktop host SHALL NOT provide computer-keyboard MIDI input.

#### Scenario: Hardware input only

- **WHEN** the user opens MIDI Settings
- **THEN** the input device list excludes a computer keyboard option

### Requirement: CC number controls display fully

MIDI CC 1 and MIDI CC 2 number sliders SHALL show 0–127 without ellipsis at default dialog size.

#### Scenario: Three-digit CC visible

- **WHEN** the user sets MIDI CC 2 to 127
- **THEN** the control shows **127** without truncation

### Requirement: v2-desktop-unified-midi-cv
Desktop v2 SHALL replace the v1 two CC-pair `CvMidiBridge` settings UI with a unified MIDI CV assignment table on the single primary MIDI input.

#### Scenario: v1 desktop CC bridge unchanged
- **WHEN** `SimHostKind::Desktop` opens MIDI Settings
- **THEN** two CC pairs with enable toggles remain as today

#### Scenario: v2 assigns CC to control-core external slot
- **WHEN** desktop v2 maps hardware CC 1 to external mod slot A
- **THEN** CC 1 level feeds the control-core external modulator, not hard-wired `ModMgr` index 0


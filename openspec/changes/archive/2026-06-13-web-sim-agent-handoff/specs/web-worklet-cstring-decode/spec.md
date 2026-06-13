## ADDED Requirements

### Requirement: Worklet decodes WASM C strings without TextDecoder

The AudioWorklet processor SHALL decode null-terminated strings from WASM linear memory using APIs available in `AudioWorkletGlobalScope`. It SHALL NOT reference `TextDecoder`, `TextEncoder`, or other main-thread-only globals for row or delay parameter names.

#### Scenario: postScreen after audio starts

- **WHEN** `process()` reaches a screen post interval while audio is running
- **THEN** `readCString` returns each row name without throwing
- **AND** the worklet continues processing audio on subsequent frames

#### Scenario: ASCII parameter names

- **WHEN** WASM returns a name pointer to bytes `VCO1\0`
- **THEN** the decoded string equals `VCO1`

### Requirement: Screen posts include row names for all pages

Each `screen` message SHALL include eight `rows` entries with `name`, `value`, `badge`, `modSource`, and `modDepth` whether the host page is a core page or Delay.

#### Scenario: Delay page names

- **WHEN** the host page is Delay (page index 5)
- **THEN** row names come from `froggers_delay_row_name` decoded in the worklet
- **AND** the main thread renders knob labels and OLED without error

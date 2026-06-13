## ADDED Requirements

### Requirement: MIDI In section describes mod rack routing

MIDI Settings SHALL label the input section **MIDI In** with default **Computer keyboard**. Copy SHALL state that MIDI In (QWERTY piano or selected hardware source) feeds the **MIDI** mod jack on the rack via patch cables — not a physical MIDI output port.

#### Scenario: User reads MIDI In purpose

- **WHEN** the user opens MIDI Settings
- **THEN** the UI explains that MIDI In drives the internal **MIDI** mod module

### Requirement: MIDI Out section labeled VCO Envelope

MIDI Settings SHALL label the output section **MIDI Out (VCO Env)**. It SHALL configure the physical MIDI output device, channel, and CC used to send **VCO Envelope** level only.

#### Scenario: User reads MIDI Out purpose

- **WHEN** the user opens MIDI Settings
- **THEN** the UI states that MIDI Out (VCO Env) sends envelope to an external MIDI device when one is selected and open

#### Scenario: No physical out device

- **WHEN** no MIDI output device is selected or open
- **THEN** the app does not send VCO Envelope MIDI to a physical port

### Requirement: MIDI In device list and refresh

The MIDI In dropdown SHALL list **Computer keyboard** first, then hardware inputs from `juce::MidiInput::getAvailableDevices()`. **Refresh devices** SHALL re-enumerate lists for mid-dialog hot-plug.

#### Scenario: Hot-plug while dialog open

- **WHEN** a device is connected while MIDI Settings is visible and the user clicks **Refresh devices**
- **THEN** the new device appears in the MIDI In list

### Requirement: Hardware MIDI In open failure status

MIDI Settings SHALL show when the selected hardware MIDI In device failed to open. QWERTY SHALL remain available when **Computer keyboard** is selected.

#### Scenario: Hardware open failure

- **WHEN** the selected hardware MIDI In cannot be opened
- **THEN** the status indicates failure

### Requirement: QWERTY piano layout legend

MIDI Settings SHALL document white keys **A S D F G H J K L**, black keys **W E T Y U O P**, and that they feed the **MIDI** mod jack (not MIDI Out VCO Env).

#### Scenario: User reads layout help

- **WHEN** the user opens MIDI Settings
- **THEN** the legend shows the piano map and mod-rack destination

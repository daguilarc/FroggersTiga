## ADDED Requirements

### Requirement: Persistent VST/AU controls have stable host parameter IDs
Every persistent continuous VST/AU control SHALL be exposed as a JUCE host parameter with a stable, label-independent ID. An independent semantic inventory SHALL enumerate each page and Delay knob/depth, pair-AR knob/depth, and continuous morph control together with its stable ID, range, and legacy/new-state default. The host registry SHALL match that inventory exactly. UI editing, DAW automation, state recall, and DSP SHALL use the same parameter authority.

#### Scenario: DAW scans plugin parameters
- **WHEN** a DAW loads the VST3 or AU plugin
- **THEN** every persistent continuous control appears with a stable automatable parameter ID

#### Scenario: Display label changes
- **WHEN** a visible parameter label changes in a later release
- **THEN** the stable host parameter ID and existing DAW automation binding remain unchanged

#### Scenario: Registry completeness
- **WHEN** the host-parameter registry is validated
- **THEN** comparison with the independent semantic inventory reports no missing, duplicate, or extra stable IDs and every entry has an explicit range and default

### Requirement: DAW owns unrestricted MIDI-to-parameter routing
VST/AU SHALL rely on standard DAW parameter mapping rather than two private MIDI CC pairs. The plugin SHALL permit the DAW to map any of MIDI's 16 channels and 128 CC numbers to exposed parameters and SHALL impose no global two-source mapping limit.

#### Scenario: More than two MIDI sources
- **WHEN** a DAW maps three or more distinct channel/CC sources to Froggers parameters
- **THEN** all mappings can control their targets without enabling CC pair 1 or CC pair 2 inside Froggers

#### Scenario: Multiple MIDI channels
- **WHEN** the DAW maps CC traffic from several MIDI channels to different parameters
- **THEN** each host-parameter automation stream updates its assigned Froggers target

### Requirement: Hosted mode has no fixed CC bridge surface
VST/AU hosted mode SHALL NOT expose the standalone MIDI Settings dialog, fixed CC 1/CC 2 enable toggles, fixed CC mod-rack cells, or CC latches at mod indices `0` and `1`. Its mod rack SHALL contain only VCO Envelope, Random 1, and Random 2.

#### Scenario: Hosted editor opens
- **WHEN** the plugin editor is shown in a DAW
- **THEN** no fixed MIDI CC settings or CC mod cells are present and the internal mod rack uses indices `4, 5, 6`

### Requirement: Host parameter application is realtime-safe
DAW parameter changes SHALL reach DSP through bounded, allocation-free pending-value storage keyed by stable parameter ID. Standard JUCE parameter notifications SHALL be coalesced per ID and applied in deterministic registry order at the next render-block boundary. This capability SHALL NOT claim sample-accurate parameter offsets that JUCE does not expose portably through `AudioProcessor`. Audio processing SHALL NOT resize a parameter container or acquire a blocking lock.

#### Scenario: Dense automation before a render block
- **WHEN** many parameter notifications arrive before the next process block
- **THEN** the latest value for each stable ID is applied once in deterministic registry order at the block boundary without heap allocation or blocking on the audio thread

#### Scenario: Continuous target smoothing
- **WHEN** an automated continuous parameter changes abruptly at a render-block boundary
- **THEN** its target changes at that boundary and the DSP value transitions through the existing per-sample smoothing path rather than jumping discontinuously or pretending the JUCE callback supplied a sample offset

### Requirement: DAW MIDI learn does not require raw plugin MIDI input
Hosted VST3/AU SHALL receive DAW MIDI-to-control mappings as ordinary host-parameter automation. After fixed CC bridge removal, the plug-in SHALL NOT advertise or consume a raw MIDI input solely for MIDI learn; JUCE build metadata and `acceptsMidi()` SHALL report no hosted MIDI input. Desktop standalone MIDI behavior remains unchanged.

#### Scenario: DAW maps CC to a parameter
- **WHEN** the DAW maps a MIDI channel/CC source to an exposed Froggers parameter
- **THEN** the resulting parameter automation controls Froggers without a `MidiBuffer` message, MIDI Settings dialog, or fixed CC latch inside the plug-in

#### Scenario: Host scans MIDI capabilities
- **WHEN** a host scans the VST3 or AU target after fixed CC removal
- **THEN** the target reports no raw MIDI input while all automatable host parameters remain available

### Requirement: Hosted format identity and optional audio input are explicit
The harmonized VST3 and AU SHALL remain a single instrument/generator product identity. The processor SHALL render correctly with zero input channels. A mono input MAY be used when an instrument host exposes it, but documentation and verification SHALL treat that bus as host-dependent and SHALL NOT promise it in every DAW.

#### Scenario: Instrument host provides no input bus
- **WHEN** the host instantiates Froggers with zero audio input channels
- **THEN** the plug-in renders its generated stereo output correctly without reading an input buffer

#### Scenario: Host exposes optional mono input
- **WHEN** a supported host enables the declared mono input bus
- **THEN** Froggers accepts that input using the documented hosted external-audio path

### Requirement: Plugin state remains backward compatible
The VST/AU state envelope SHALL persist stable parameter values and sim routing state, SHALL read existing v1/v2 `SimPresetSnapshot` bytes with documented defaults, and SHALL ignore unknown future parameter IDs safely.

#### Scenario: Legacy session recall
- **WHEN** a DAW restores a session saved with a v1 or v2 snapshot
- **THEN** existing knob and route state loads and fields absent from that snapshot receive the exact defaults declared by the independent semantic inventory

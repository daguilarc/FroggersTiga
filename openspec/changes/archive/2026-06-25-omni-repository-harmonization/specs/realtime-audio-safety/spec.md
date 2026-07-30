## ADDED Requirements

### Requirement: Owned realtime storage does not allocate or grow
`WasmSimHost::processBlock`, JUCE standalone/VST render callbacks, VCV `process`, and recorder producer paths SHALL perform no repository-controlled heap allocation, deallocation, dynamic-capacity growth, file I/O, or blocking synchronization during steady-state audio processing. The Web Audio worklet DSP portion SHALL perform no WASM `malloc/free` or typed-array backing allocation per render quantum.

#### Scenario: Repeated render blocks
- **WHEN** each host processes repeated blocks after its prepare/start phase
- **THEN** owned-allocation instrumentation records zero allocations, frees, or capacity growth from the native/WASM realtime path

### Requirement: Web Audio telemetry is bounded honestly
Worklet screen telemetry MAY create a bounded JavaScript message at a fixed decimated cadence. Its row, mod, pair-AR, and scope payload sizes SHALL be capped by shared protocol constants, and scope copying SHALL reuse prepared WASM storage. Browser-internal structured-clone allocation and garbage collection SHALL NOT be represented as repository-controlled or covered by the zero-owned-allocation assertion.

#### Scenario: Telemetry tick
- **WHEN** the worklet reaches its configured screen-update interval
- **THEN** it emits at most one fixed-shape screen payload and performs no WASM `malloc/free` or scope-buffer growth

#### Scenario: Non-telemetry quantum
- **WHEN** the worklet processes a render quantum between screen updates
- **THEN** it performs DSP using persistent WASM buffers without building a screen payload

### Requirement: Render storage is prepared outside realtime callbacks
Each host SHALL allocate or size render buffers during construction, `prepareToPlay`, `audioDeviceAboutToStart`, or recording start. Variable block sizes SHALL be handled by prepared capacity or bounded chunking, not callback-time resize.

#### Scenario: VST maximum block size
- **WHEN** `prepareToPlay` receives the host's maximum expected block size
- **THEN** subsequent blocks up to that size use already-prepared input and mono buffers

#### Scenario: Web render quantum
- **WHEN** the worklet processes one or more render quanta
- **THEN** it reuses persistent WASM input/output/scope buffers and refreshes only typed-array views after memory-buffer replacement

### Requirement: Recording growth is off the audio thread
Recording SHALL use a bounded producer buffer on the audio thread and move accumulation or file-writing work to a non-realtime consumer. Overflow SHALL stop or truncate recording explicitly rather than allocate or block.

#### Scenario: Long recording
- **WHEN** captured audio exceeds the producer's immediately available chunk capacity
- **THEN** the audio callback remains non-blocking and the recorder reports truncation or backpressure according to its documented policy

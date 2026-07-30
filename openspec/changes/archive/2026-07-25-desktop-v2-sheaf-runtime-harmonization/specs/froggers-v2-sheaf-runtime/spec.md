## ADDED Requirements

### Requirement: Desktop boots through vendored Sheaf Runtime
Desktop standalone v2 SHALL start through a vendored Sheaf-compatible `Runtime` / shell (`SYNTH_RUNTIME_MAIN` or equivalent) hosting a Froggers `SynthApplication`. The Application surface SHALL be the Froggers product UI. Runtime SHALL own audio device lifecycle, MIDI device connection shell, File/Audio/Controllers pages, and the audio/message pump. Product builds SHALL NOT FetchContent Sheaf from the network; all Runtime scaffolding SHALL live in the FroggersTiga tree with a reviewable adoption inventory.

#### Scenario: No network Sheaf dependency
- **WHEN** desktop v2 configures and builds
- **THEN** the build does not fetch Sheaf from the network
- **THEN** Runtime sources used by the product are present in-tree

#### Scenario: Runtime owns device chrome
- **WHEN** desktop v2 launches
- **THEN** audio device selection and Controllers/File shell pages are Runtime-owned
- **THEN** `MainComponent` is not the primary host that owns `AudioEngine` / device lifecycle for standalone desktop

### Requirement: FroggersApp satisfies SynthApplication
Froggers desktop v2 SHALL expose `Config`, `Init`, `ProcessBlock`, and `PortableSurface` such that it satisfies the Sheaf `SynthApplication` contract while delegating DSP to the existing AudioEngine and control-core path during migration. Optional Sheaf hooks (`ProcessFrame`, `PrepareToPlay`) are implemented only when the vendored Engine initialize path requires them for a clean build — not as product features.

#### Scenario: Engine pumps Froggers audio
- **WHEN** Runtime delivers an audio block to `Engine<FroggersApp>`
- **THEN** `FroggersApp::ProcessBlock` produces finite stereo output through the existing engine path within declared deterministic tolerance

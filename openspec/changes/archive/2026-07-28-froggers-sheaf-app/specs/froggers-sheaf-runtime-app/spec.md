## ADDED Requirements

### Requirement: Froggers is a Sheaf SynthApplication
The Froggers app SHALL be a type satisfying `synth::SynthApplication` — providing `Config()`, `Init(context)`, `ProcessBlock(block)`, and `PortableSurface()` — and SHALL assert that conformance at compile time. The app core SHALL NOT depend on JUCE.

#### Scenario: Concept conformance is enforced at build time
- **WHEN** the app target is compiled
- **THEN** a static assertion confirms the app type satisfies `synth::SynthApplication`
- **THEN** the app core translation units include no JUCE header

#### Scenario: Headless process produces audio
- **WHEN** a test harness calls `Init` then `ProcessBlock` without any host shell
- **THEN** the block is filled with finite stereo samples

### Requirement: Desktop hosting through the Sheaf launcher
Froggers SHALL be launchable on desktop by registering with the `sheaf-patch` launcher, reaching `Runtime<App>` through the launcher's generic registration path. Froggers SHALL NOT define its own JUCE application entry point.

#### Scenario: App appears in the launcher and runs
- **WHEN** the operator selects Froggers in the `sheaf-patch` launcher
- **THEN** a Runtime session is created for the Froggers app
- **THEN** audio processes and the portable surface renders

### Requirement: Browser hosting through the Sheaf browser ABI
The same app type SHALL be hostable in the Sheaf browser/patcher host via the browser app entry macro, with no app-core changes between the desktop and browser hosts.

#### Scenario: One app core serves both hosts
- **WHEN** the browser build is produced from the same app type as the desktop build
- **THEN** no host-specific branching exists in the app core
- **THEN** both hosts drive the identical `ProcessBlock` and `PortableSurface`

### Requirement: Frozen legacy trees
This capability SHALL NOT modify `desktop-v2/`, `desktop/`, `src/`, `sim/`, `wasm/`, `vcv/`, or `web/`. Outside the new app tree, only the submodule declaration and the existing Pages workflow may change. The app SHALL consume Sheaf only through the `External/Sheaf` submodule and SHALL NOT resolve Sheaf headers through the frozen vendored slice under `desktop-v2/`.

#### Scenario: Daisy firmware is unaffected
- **WHEN** the full change is applied
- **THEN** no file under `src/` (including the Daisy firmware) differs from its recorded pre-change baseline
- **THEN** the Daisy firmware's raw binary image artifact hash matches its recorded baseline
- **THEN** the other tracked firmware build artifacts are excluded from this hash comparison because they embed the absolute build path and would differ for that reason alone, not because they are unimportant

#### Scenario: Verification does not itself write to a frozen tree
- **WHEN** the frozen-tree proof is performed
- **THEN** the firmware is built to a scratch location outside the repository trees
- **THEN** the tracked build artifacts under `src/` are left untouched by the verification

#### Scenario: Vendored slice is not referenced
- **WHEN** the app is compiled
- **THEN** no Sheaf header resolves through `desktop-v2/External/Sheaf`

# froggers-sheaf-runtime-app Specification

## Purpose
Froggers satisfies `synth::SynthApplication`; it runs under Sheaf Runtime via `sheaf-patch` launcher registration and under the browser host via the browser app entry macro, with a JUCE-free app core and the Daisy firmware under `src/` left untouched.
## Requirements
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

### Requirement: Operator documentation ships with the app
THE app SHALL carry its manual and quick dictionary locally in every host
it ships in — standalone, VST3 and AU — and SHALL let the operator open
both from inside the app without a network connection. The documents SHALL
be embedded from the repository's single copy at build time, so that no
second checked-in copy exists to drift from the first. The browser build
MAY instead link to the published documents, because it is already running
in a browser with the network available.

Neither the way the operator reaches the documents nor the way the app
locates them SHALL assume a single platform's conventions. Where a host's
platform has no equivalent of the macOS main menu, the app SHALL present the
same entries by that platform's own means; where it has no application
bundle, the app SHALL find the documents where that platform's build places
them.

#### Scenario: Reading the manual offline in a DAW
- **WHEN** the plugin is loaded in a DAW on a machine with no network
- **THEN** the operator can open the manual and the quick dictionary from
  the plugin itself
- **AND** the content matches the repository's copy for that build

#### Scenario: The standalone app carries its own documentation
- **WHEN** the standalone app is opened with no network
- **THEN** both documents are reachable from inside the app

#### Scenario: Every shipped standalone platform reaches its documents
- **WHEN** the standalone app is opened on any platform it is released for
- **THEN** the manual and quick dictionary open from inside the app
- **AND** the files opened are the ones that build placed, not a path that
  resolves only on the platform the feature was written on

#### Scenario: One copy, not two
- **WHEN** the manual or the quick dictionary is edited in the repository
- **THEN** the next build carries the edit
- **AND** no checked-in duplicate of either document has to be re-synced


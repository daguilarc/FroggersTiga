## ADDED Requirements

### Requirement: Five-cell mod rack matches desktop topology

The VCV main module mod rack SHALL expose five patchable mod outputs matching desktop `ModRackPanel`: MIDI CC 1 (mod 0), MIDI CC 2 (mod 1), VCO Envelope (mod 4), Random 1 (mod 5), Random 2 (mod 6).

#### Scenario: Mod output count and indices

- **WHEN** the mod rack row is inspected
- **THEN** five output jacks are present with labels from `ParamDisplayNames::forModSource`
- **THEN** each jack carries CV from the matching mod index (0, 1, 4, 5, 6)

#### Scenario: LED-only mod feedback on VCV

- **WHEN** any mod cell CV exceeds 0.55 while audio runs
- **THEN** the corresponding green LED illuminates
- **THEN** no oscilloscope or CV trace widget is present on the VCV module

#### Scenario: Mod rack labels on faceplate

- **WHEN** the module is at 100% zoom without hover
- **THEN** path silkscreen identifies all five mod cells
- **THEN** labels match `ParamDisplayNames` strings used on desktop

### Requirement: Mod rack supersedes truncated three-output layout

VCV SHALL NOT ship the current three-output mod rack (VCO Env, Random 1, Random 2 only) as the final field-parity layout.

#### Scenario: MIDI CC mod outputs present

- **WHEN** a user patches MIDI CC 1 mod to an Audio row on desktop
- **THEN** the same mod index and patch topology is available from the VCV mod rack MIDI CC 1 output

### Requirement: VCV does not implement desktop CV scopes

VCV mod rack feedback SHALL remain LED-only per `modIndicatorModeForVcv()` in `HostPanelLayout.hpp`. Desktop, web, and VST retain `CvScopeDisplay` traces; that is an intentional host split, not a VCV gap.

#### Scenario: No scope widgets in VCV build

- **WHEN** the VCV plugin source is inspected
- **THEN** no `ModCvScopeWidget` or equivalent CV trace component exists
- **THEN** mod activity is indicated by green LEDs only

#### Scenario: Mod rack driven by cell table

- **WHEN** mod rack widgets, outputs, and LEDs are implemented
- **THEN** a single static table defines mod index, output param, and light id for all five cells
- **THEN** widget construction and `process()` iterate that table — no hardcoded 3-wide arrays

## ADDED Requirements

### Requirement: v2-scope-transport-row-width-cap

In standalone desktop v2, the VCO envelope-follower oscilloscope in the top transport row SHALL respect `DesktopV2ChromeLayout::kTransportScopeMaxWidth`. Scope flex growth SHALL NOT shrink Play, Stop, MIDI, Audio, or **Record audio** export cluster below their minimum readable widths.

#### Scenario: Scope capped at default width

- **WHEN** the main window is wider than 1280 px
- **THEN** transport-row scope width stays at `kTransportScopeMaxWidth` (320 px initial)
- **THEN** extra horizontal space remains empty or is reserved for future transport controls, not assigned to scope

#### Scenario: Scope still updates at UI rate

- **WHEN** audio is running and scope is visible in the transport row
- **THEN** trace repaints at ≥15 Hz per `v2-scope-grid-for-ef-sources`
- **THEN** width cap does not disable ring-buffer reads

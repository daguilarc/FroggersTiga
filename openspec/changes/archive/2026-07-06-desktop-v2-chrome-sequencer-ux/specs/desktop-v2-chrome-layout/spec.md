## ADDED Requirements

### Requirement: v2-chrome-layout-authority

Desktop v2 and VST v2 SHALL share a single layout authority header `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` defining transport, module-row, and mod-cell geometry constants. `MainComponent`, `HostedMainComponentV2`, `SubmodulePagePanel`, and `ModSourceCell` SHALL read these constants instead of duplicating magic numbers.

#### Scenario: Constants consumed by both hosts

- **WHEN** standalone `MainComponent` and `HostedMainComponentV2` lay out the top transport row
- **THEN** both use `DesktopV2ChromeLayout::kTransportScopeMaxWidth` for the VCO EF scope budget
- **THEN** both use the same module-row column offsets from `DesktopV2ChromeLayout`

#### Scenario: No center-float encoder rows

- **WHEN** `SubmodulePagePanel::layoutRows` positions label, encoder, and mod cells for a row
- **THEN** cells align to left-anchored column X offsets derived from row width
- **THEN** encoders are not placed with `withSizeKeepingCentre` in leftover horizontal space

### Requirement: v2-transport-row-control-budget

The standalone top transport row SHALL allocate horizontal space in fixed order: **Play**, **Stop**, **MIDI**, **Audio**, **Record audio** (`RecordButton` only), then **VCO EF scope** capped at `kTransportScopeMaxWidth`, with remaining width unused (not given to scope). Export format toggles are not in this row — they live in the **Audio** menu.

#### Scenario: Scope does not consume Play cluster

- **WHEN** the main window is at default minimum width (1280 px per `desktop-v2-global-controls`)
- **THEN** Play and Stop button labels are fully visible
- **THEN** scope width is ≤ `kTransportScopeMaxWidth` (initial value 320 px)

#### Scenario: Transport constants in chrome header

- **WHEN** `layoutStandaloneTransportRow` lays out Play, Stop, MIDI, Audio, and Record audio
- **THEN** button widths and gaps come from `DesktopV2ChromeLayout` (`kTransportPlayStopW`, `kTransportSettingsW`, `kTransportGapSm`, `kTransportGapMd`, `kRecordButtonMinWidth`)
- **THEN** no duplicate width literals exist in the transport layout helper

#### Scenario: VST omits audio Play row

- **WHEN** `HostedMainComponentV2` renders
- **THEN** Play/Stop audio transport buttons are absent
- **THEN** scope cap and MIDI/Audio controls follow the same chrome constants where applicable

### Requirement: v2-mod-cell-fixed-footprint

Each `ModSourceCell` SHALL use a fixed outer height whether the source is **None** or assigned. The label strip height SHALL use `DesktopV2ChromeLayout::kModLabelStripH` and SHALL NOT change between None and assigned states.

#### Scenario: None vs assigned same cell height

- **WHEN** a mod cell shows **None**
- **THEN** its bounds height equals `DesktopV2ChromeLayout::kModCellHeight`
- **WHEN** the operator assigns Random 1 S&H
- **THEN** cell height is unchanged; only inner label and LED/scope content updates

#### Scenario: Row layout stable on assignment

- **WHEN** the operator cycles a row mod source from None to an assigned source
- **THEN** sibling encoder and label Y positions do not shift

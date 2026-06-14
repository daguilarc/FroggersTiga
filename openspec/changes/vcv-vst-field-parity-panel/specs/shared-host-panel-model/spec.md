## ADDED Requirements

### Requirement: Single panel layout authority

The repository SHALL define one MIT header (`sim/HostPanelLayout.hpp` or equivalent) that enumerates host pages, rows per page, mod source indices, and desktop pixel layout constants consumed by all sim hosts. Row and mod source display names SHALL come from `ParamDisplayNames.hpp` — not duplicated string tables per host.

#### Scenario: Desktop includes shared layout

- **WHEN** desktop `SubModulePanel` or `ModRackPanel` is built
- **THEN** layout dimensions and mod indices are read from the shared header, not duplicated literals in multiple files

#### Scenario: VCV includes shared layout

- **WHEN** the VCV widget positions knobs and jacks
- **THEN** row names and mod indices match the same header tables used by desktop

#### Scenario: Mod rack labels use ParamDisplayNames

- **WHEN** desktop or VCV renders mod source 4 label
- **THEN** the label is `ParamDisplayNames::forModSource(4)` ("VCO Envelope"), not a hardcoded duplicate string

### Requirement: Page-indexed host APIs on all sim hosts

`PagedHostIO` SHALL expose page-indexed knob and mod operations matching `DesktopHostIO` semantics so all-six-columns-at-once UIs (VCV expander stack, desktop) write the correct page without switching `m_currentPage`.

#### Scenario: Per-page knob write on VCV

- **WHEN** VCV UI sets knob row 3 on Filter page (page index 3)
- **THEN** the host writes `SetPageKnob(3, row, value)` and the engine Filter params update without changing the current page pointer

#### Scenario: Per-page mod assignment on VCV

- **WHEN** VCV UI assigns Random 1 to Audio row 0 with depth 50%
- **THEN** `SetPageModSource(0, 0, 5)` and `SetPageModDepth(0, 0, 0.5)` match desktop patchbay behavior for the same connection

#### Scenario: Sim display names on PagedHostIO

- **WHEN** any sim host queries row 7 name on Audio page
- **THEN** the result is **Phase mod 3** from `ParamDisplayNames`, not firmware name `OLVL`

### Requirement: Host panel backend contract

Shared hosts SHALL expose knob, mod source, and mod depth operations through the existing `IPanelBackend` interface (or a renamed equivalent in the shared header) backed by `DesktopHostIO` or extended `PagedHostIO`.

#### Scenario: Desktop backend unchanged

- **WHEN** desktop `DesktopPanelBackend` sets a knob
- **THEN** it continues to call `DesktopHostIO::SetPageKnob` with no behavior regression

### Requirement: Delay FX sidecar on VCV

VCV field-parity modules SHALL wire `DelayState` and `FroggersEngine::SetSimFxInsert` on the shared engine instance, matching desktop `AudioEngine` and web `WasmSimHost`.

#### Scenario: Delay column drives DelayState

- **WHEN** user turns Delay column knob row 0 on VCV
- **THEN** `DelayState` receives the update and stereo delay FX matches desktop for the same knob value

### Requirement: Mod indicator mode per host

The shared layout header SHALL define mod indicator mode per host. **VCV** field-parity modules SHALL use `LedOnly` for mod sources 4–6. **VST** and desktop standalone SHALL use `ScopeAndLed` (VCO Envelope scope + Random LEDs).

#### Scenario: VCV host flag

- **WHEN** host is VCV expander stack
- **THEN** no oscilloscope or CV trace component is allocated for any mod source

#### Scenario: VST host flag

- **WHEN** host is JUCE VST/AU plugin
- **THEN** VCO Envelope mod box includes `CvScopeDisplay`; Random 1/2 use LEDs as on desktop

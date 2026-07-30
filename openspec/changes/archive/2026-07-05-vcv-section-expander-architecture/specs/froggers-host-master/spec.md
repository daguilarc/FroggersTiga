## ADDED Requirements

### Requirement: VCV host contract is section and expander based
VCV Rack SHALL be modeled as a section/expander host, not a paged host. The Rack-facing VCV code SHALL use named sections and extension snapshots for Audio, Random, Filter, Drive, Reverb, Delay, Global, and VCO AR controls. VCV SHALL NOT use `m_currentPage`, page navigation, or shared hardware/current-page knob positions to apply Rack controls.

#### Scenario: VCV control surface has no selected page
- **WHEN** a VCV Rack knob changes on any visible section
- **THEN** the host applies the value to named VCV section state
- **THEN** there is no selected-page transition or current-page replay

#### Scenario: Shared engine compatibility remains internal
- **WHEN** VCV section state is applied to the shared engine
- **THEN** any internal legacy page/bank mapping remains behind a VCV-safe adapter
- **THEN** desktop, web, and VST host behavior remains unchanged

### Requirement: VCV global Crunchy participates in host differences
VCV Rack SHALL expose global Crunchy as a main-module knob with a CV input. The effective value SHALL clamp the sum of knob value and normalized Rack CV. This global control SHALL coexist with section-local Crispy controls.

#### Scenario: VCV global Crunchy appears as global control
- **WHEN** a user places the VCV main module
- **THEN** global Crunchy knob and CV input are available on the main module
- **THEN** per-section Crispy rows remain available where those sections expose Crispy

## MODIFIED Requirements

### Requirement: External MIDI and parameter control differ by host
Each sim host SHALL expose external MIDI, continuous parameters, and mod-assignment UI according to its column in the matrix below.

| Host | External MIDI / CC | Continuous parameter surface | Mod assignment UI |
|------|-------------------|------------------------------|-------------------|
| **Desktop standalone** | Two hardware CC pairs via MIDI Settings (CC 1 default On, CC 2 default Off); QWERTY → CC 1 | On-panel knobs + patch cables | Patch cables from mod rack |
| **Web** | Web MIDI CC 1 when enabled | Expanded pages 1–5 + **global Crunchy**; v1 mod dropdowns | Dropdown per knob; v1 four-cell mod bay |
| **VST / AU v1** | **None** in plugin — `acceptsMidi()` false; DAW maps any MIDI/CC to **107** host parameters | DAW automation + plugin knobs | Patch cables; no hosted MIDI Settings |
| **VCV Rack** | **None** in module — use Rack MIDI-to-CV → per-parameter jacks | Section knobs + per-parameter CV inputs + global Crunchy CV; no selected page | Internal mod routes + CV jacks |
| **Desktop v2** | One assignable MIDI input for pitch/gate/CC targets | Carousel knobs + mod grid + control core | Lit cells + dropdown |
| **VST / AU v2** | DAW MIDI → any `HostParameterInventoryV2` parameter | Full v2 inventory + carousel UI | Lit cells + dropdown; DAW maps MIDI to parameters |

#### Scenario: VST v2 accepts MIDI for parameter modulation
- **WHEN** a DAW sends MIDI to FroggersTigaPluginV2
- **THEN** `acceptsMidi()` is true
- **THEN** parameter changes from MIDI arrive through JUCE host parameter mapping, not raw `ModMgr` CC slots 0/1

#### Scenario: VCV exposes no MIDI boundary
- **WHEN** a VCV main or extension module is placed
- **THEN** the module has no Froggers-owned MIDI port, MIDI queue, CC-enable control, or MIDI-specific saved state
- **THEN** external MIDI must arrive as ordinary Rack CV from another module

### Requirement: VCV per-parameter CV combines with internal modulation
VCV SHALL compute `internalEffective = ModMgr::Modulate(base, modIndex, depth)` once per target, then for connected jacks `clamp(internalEffective + voltage / 10, 0, 1)`. Disconnected jacks SHALL use `internalEffective` only. Base, route, and depth SHALL NOT mutate during jack evaluation. VCV SHALL pass this combined value as temporary effective state and SHALL NOT write the combined value back into the stored base knob while the internal route remains active.

#### Scenario: Negative CV clamps
- **WHEN** a negative CV is patched to a parameter jack
- **THEN** the effective value clamps at 0 after addition

#### Scenario: Disconnected internal route is not doubled
- **WHEN** a VCV target has base `0.0`, internal route source `5`, depth `1.0`, source value `0.4`, and no jack connected
- **THEN** the effective value supplied to the engine is `0.4`
- **THEN** the stored base value remains `0.0`
- **THEN** the route source `5` is not applied a second time by a later engine read

## ADDED Requirements

### Requirement: SimHostKind includes v2 surfaces
The repository SHALL add `SimHostKind::DesktopV2` and `SimHostKind::VstV2` for the forked desktop and VST products. v1 kinds remain unchanged.

#### Scenario: v2 mod catalog query
- **WHEN** `IsSimModSourceAvailable` is queried for index 7 with `SimHostKind::DesktopV2`
- **THEN** the source is available
- **WHEN** queried for index 0 with `SimHostKind::VstV2`
- **THEN** the legacy CC1 scope cell is unavailable (v2 uses host-parameter MIDI only on VST)

### Requirement: v2 mod rack replaced by source grid
Desktop v2 and VST v2 SHALL NOT use `HostPanelLayout::kModRackCatalog` five-cell mod rack. They SHALL use the eight-source catalog in `desktop-v2-mod-source-grid`.

#### Scenario: v1 desktop mod rack unchanged
- **WHEN** `SimHostKind::Desktop` renders mod rack
- **THEN** five-cell patch-cable rack behavior is unchanged

### Requirement: v2-host-page-count-and-adsr
v2 sim hosts SHALL use **seven** host pages (indices 0–6). Index 6 is the ADSR module. v2 hosts SHALL use `VcoAdsrState` instead of `AudioPairArState`.

#### Scenario: Host page count on v2
- **WHEN** `SimHostKind::DesktopV2` queries host page count
- **THEN** the count is 7 including ADSR at index 6

#### Scenario: Global Crunchy on v2
- **WHEN** v2 host applies global Crunchy
- **THEN** `Fuegoize` runs on every persisted row on every module page including all Crispy instances (`CrispyRowForPage`: row 7 on Audio; row 9 on expanded modules 1–5 and ADSR) and all musical rows
- **THEN** per-page Crispy rows remain present on every module page

#### Scenario: Per-page Crispy retained on v2
- **WHEN** Audio module (page 0) is visible
- **THEN** row 7 Crispy is present and applies page-local fuego to rows 0–6 after global Crunchy
- **WHEN** any expanded module page 1–5 is visible
- **THEN** row 9 Crispy is present and applies page-local fuego to rows 0–8 after global Crunchy

#### Scenario: pair-AR inactive on v2
- **WHEN** `SimHostKind::DesktopV2` processes audio
- **THEN** `AudioPairArState` is not ticked
- **THEN** `VcoAdsrState` supplies gated ADSR envelope shaping per VCO

### Requirement: v2-sequencer-on-v2-hosts
v2 sim hosts SHALL include `SequencerState` integrated with the control core and `VcoAdsrState` gate input.

#### Scenario: Sequencer gate drives ADSR
- **WHEN** sequencer playback is active on DesktopV2
- **THEN** per-step gate merges with MIDI gate before `VcoAdsrState` tick

## MODIFIED Requirements

### Requirement: Mod rack topology differs by host
Mod rack cells SHALL come from `HostPanelLayout::kModRackCatalog` for hosts `Desktop`, `Web`, `Vst`, and `Vcv`. Hosts `DesktopV2` and `VstV2` SHALL use `V2ModSourceCatalog` (indices 7–14) with UI projection defined in v2 specs; they SHALL NOT render v1 mod rack cells.

| Mod index | Source | Desktop | Web | VST/AU | VCV | DesktopV2 | VstV2 |
|-----------|--------|---------|-----|--------|-----|-----------|-------|
| 0 | MIDI CC 1 | yes | yes | no | no | no | no |
| 1 | MIDI CC 2 | yes | no | no | no | no | no |
| 4 | VCO Envelope (legacy sum) | yes | yes | yes | yes | no | no |
| 5 | Random S&H 1 | yes | yes | yes | yes | no | no |
| 6 | Random S&H 2 | yes | yes | yes | yes | no | no |
| 7–12 | Per-VCO / pair EF | no | no | no | no | yes | yes |
| 13 | Random S&H 1 | no | no | no | no | yes | yes |
| 14 | Random S&H 2 | no | no | no | no | yes | yes |

**Cell counts:** desktop **5**; web **4**; VST/AU v1 **3**; VCV **3**; DesktopV2 **8** (6 scopes + 2 LEDs); VstV2 **8**.

#### Scenario: v2 desktop excludes v1 CC scope cells
- **WHEN** desktop v2 renders mod sources
- **THEN** indices 0 and 1 do not appear as scope cells
- **THEN** external MIDI CV is configured through v2 MIDI assignment UI

#### Scenario: Cross-host LED curve parity on Random S&H v2
- **WHEN** `GetCvOut(13)` is `0.3` on DesktopV2 with audio running
- **THEN** Random S&H 1 LED brightness equals `ModLedDisplayBrightness(0.3, true)`

### Requirement: External MIDI and parameter control differ by host
Each sim host SHALL expose external MIDI, continuous parameters, and mod-assignment UI according to its column in the matrix below.

| Host | External MIDI / CC | Continuous parameter surface | Mod assignment UI |
|------|-------------------|------------------------------|-------------------|
| **Desktop standalone** | Two hardware CC pairs via MIDI Settings (CC 1 default On, CC 2 default Off); QWERTY → CC 1 | On-panel knobs + patch cables | Patch cables from mod rack |
| **Web** | Web MIDI CC 1 when enabled | Expanded pages 1–5 + **global Crunchy**; v1 mod dropdowns | Dropdown per knob; v1 four-cell mod bay |
| **VST / AU v1** | **None** in plugin — `acceptsMidi()` false; DAW maps any MIDI/CC to **107** host parameters | DAW automation + plugin knobs | Patch cables; no hosted MIDI Settings |
| **VCV Rack** | **None** in module — use Rack MIDI-to-CV → per-parameter jacks | Knobs + CV inputs | Internal mod routes + CV jacks |
| **Desktop v2** | One assignable MIDI input for pitch/gate/CC targets | Carousel knobs + mod grid + control core | Lit cells + dropdown |
| **VST / AU v2** | DAW MIDI → any `HostParameterInventoryV2` parameter | Full v2 inventory + carousel UI | Lit cells + dropdown; DAW maps MIDI to parameters |

#### Scenario: VST v2 accepts MIDI for parameter modulation
- **WHEN** a DAW sends MIDI to FroggersTigaPluginV2
- **THEN** `acceptsMidi()` is true
- **THEN** parameter changes from MIDI arrive through JUCE host parameter mapping, not raw `ModMgr` CC slots 0/1

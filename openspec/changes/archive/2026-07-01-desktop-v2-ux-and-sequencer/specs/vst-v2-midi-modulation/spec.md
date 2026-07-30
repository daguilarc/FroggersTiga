## MODIFIED Requirements

**Audit 2026-06-30:** Editor embeds v2 chrome (`HostedMainComponentV2`) but UX polish in `desktop-v2-ux-and-sequencer` (grid layout, Crunchy scene ring, sequencer toolbar, performance band naming) is not yet applied. DAW MIDI/automation paths are implemented; standalone MIDI settings UX is correctly absent.

### Requirement: vst-v2-editor-parity-with-desktop-v2

FroggersTigaPluginV2 editor SHALL embed v2 chrome including Crunchy **scene** encoder ring, Pair-AR module carousel page, encoder rings, fixed grid layout, **full sequencer panel** (edit-step toolbar, step grid with single/double-click and right-click menu), and performance band — minus standalone transport/device/MIDI settings.

Sequencer behavior SHALL be **identical** to standalone desktop v2 except where DAW I/O differs (no **Engine** row; DAW starts/stops audio processing; MIDI Start/Stop may toggle **Start Sequence**).

#### Scenario: Sequencer step interaction parity

- **WHEN** the operator uses FroggersTigaPluginV2 sequencer panel
- **THEN** **single-click** selects edit step without toggling gate
- **THEN** **double-click** toggles step gate (lit/rest)
- **THEN** **right-click** offers **Reset** and **Randomize** for that step only
- **THEN** toolbar **←** / **→**, dice, and Step/Pattern scope match standalone
- **THEN** gate cells dim when **Start Sequence** is off and render at full brightness when playing (per `v2-sequencer-gate-cell-stopped-dim`)

#### Scenario: Sequencer gate and default audio parity

- **WHEN** the DAW is processing audio and **Start Sequence** is off
- **THEN** internal VCOs drive output continuously (same default-open gate policy as standalone)
- **WHEN** **Start Sequence** is on
- **THEN** playhead step gates and DAW-routed MIDI notes shape per-VCO AR per `desktop-v2-sequencing`

#### Scenario: Hosted editor hides device settings

- **WHEN** the plugin editor opens in a DAW
- **THEN** audio device and record/export clusters are hidden
- **THEN** module carousel, Crunchy scene ring, mod grid, scopes, and sequencer remain functional

#### Scenario: Grid layout matches desktop

- **WHEN** VST v2 editor opens at default size
- **THEN** control footprints match `desktop-v2-grid-layout` (encoder 5u, arrows 2u, panel **128u×92u**)
- **THEN** `HostedMainComponentV2` does not duplicate layout constants outside `DesktopV2ChromeLayout`

#### Scenario: Performance band without Engine

- **WHEN** VST v2 performance band renders
- **THEN** sequencer transport reads **Start Sequence** / **Stop Sequence** (not Play)
- **THEN** no **Engine** control appears (DAW owns audio transport)

#### Scenario: Sequencer host parameters remain automatable

- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** BPM, pattern length, play/record arm, and current step remain exposed as host parameters
- **THEN** DAW MIDI Start/Stop may toggle sequencer playback per `AudioEngine::routeMidiMessage`

### Requirement: vst-v2-daw-midi-to-any-parameter

FroggersTigaPluginV2 SHALL accept MIDI input from the DAW and SHALL allow any MIDI message type the DAW maps to modulate any exposed parameter through host parameter routing. VST SHALL NOT expose `MidiCvSettingsComponent`; DAW mapping is the operator-facing MIDI UX.

#### Scenario: MIDI CC modulates global Crunchy

- **WHEN** the DAW maps CC 1 to `Global/Crunchy`
- **THEN** global fuegoization follows CC at block boundaries

#### Scenario: No duplicate raw MIDI mod path

- **WHEN** MIDI arrives on the plugin bus
- **THEN** modulation is applied only through host parameter mapping

#### Scenario: No standalone MIDI CV settings in VST

- **WHEN** the operator opens FroggersTigaPluginV2 in a DAW
- **THEN** there is no MIDI In device dropdown or CV assignment table UI
- **THEN** desktop-only **MIDI CC A/B** mod sources in scope grid are not required in VST (DAW CC mapping substitutes)

#### Scenario: Parameter count after Pair-AR refactor

- **WHEN** `HostParameterInventoryV2.hpp` is regenerated
- **THEN** the count is **142** (was 148; six Sus axes removed: three knobs + three mod depths)
- **THEN** `HostParameterProcessorV2_test` asserts **142**

#### Scenario: Pair-AR parameters in DAW

- **WHEN** the DAW shows automation for FroggersTigaPluginV2
- **THEN** `Pair-AR/Atk1` … `Pair-AR/Rel3` are present
- **THEN** no `ADSR/Sus1`, `ADSR/Sus2`, or `ADSR/Sus3` parameters exist

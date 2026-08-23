# desktop-v2-scope-visualization Specification

## Purpose
Desktop v2 signal visualization includes a shell-level global top-chrome oscilloscope plus source-indicator cells and encoder-integrated CV LED monitors separate from the global scope.

## Requirements
### Requirement: v2-scope-grid-for-ef-sources
Desktop v2 SHALL render source-indicator scope cells where envelope followers display color-coded traces consistent across the grid.

#### Scenario: Per-VCO color coding
- **WHEN** VCO1 EF scope is active
- **THEN** its trace uses the VCO1 canonical color from host display constants
- **WHEN** VCO1+VCO2 EF scope is active
- **THEN** its trace blends or dual-tones VCO1 and VCO2 colors per `HostPanelLayout` v2 rules

#### Scenario: Scope updates at UI refresh rate
- **WHEN** audio is running
- **THEN** each EF scope cell reads from a ring buffer fed at audio rate and repaints at the v2 UI timer rate (≥15 Hz)

#### Scenario: Pair and sum scopes show combined signal
- **WHEN** VCO2+VCO3 EF is displayed
- **THEN** the trace represents the envelope follower of `|VCO2 + VCO3|` (or documented sum-then-detect path from design)

### Requirement: v2-random-sh-gated-green-leds
Random S&H sources SHALL NOT use oscilloscope cells; they SHALL use gated green LEDs with level-proportional brightness driven by CV level and gate state.

#### Scenario: Random S&H LED brightness curve
- **WHEN** Random S&H 1 CV is 0.3 with audio running
- **THEN** LED brightness follows the level-proportional curve for CV 0.3 with the gate active

#### Scenario: EF scopes exclude Random S&H
- **WHEN** the scope grid renders
- **THEN** Random/Marbles source cells show LED widgets only, not waveform traces

### Requirement: Global top-chrome oscilloscope
Desktop v2 SHALL render one persistent shell-level oscilloscope in the transport/signal band of the top chrome stack next to the Play and Stop controls. The oscilloscope SHALL remain visible across carousel module pages and runtime pages at the default standalone size and hosted editor minimum layout.

#### Scenario: Scope signal taps are declared
- **WHEN** manifest validation runs
- **THEN** the product manifest declares scope taps for VCO 1, VCO 2, and VCO 3 after waveform morph and cross-coupling and before global reverb/delay output effects
- **THEN** the product manifest declares source-group scope taps for source groups that are inspectable in the same global oscilloscope, including LFO, VCO pair-bus, EF, Random/Marbles, and External Audio sources

#### Scenario: VCO traces are color-coded together
- **WHEN** the global oscilloscope renders its default view
- **THEN** it displays VCO 1, VCO 2, and VCO 3 as three color-coded traces in the same scope
- **THEN** the trace colors match the Audio/VCO page and permanent source rack color assignments

#### Scenario: LFO source group can use the same multi-signal scope rules
- **WHEN** the global oscilloscope is set to the LFO source group
- **THEN** it can display LFO 1, LFO 2, and LFO 3 as color-coded traces in the same scope
- **THEN** trace colors match the permanent source rack labels

#### Scenario: Sheaf-style visualization indicates audio-rate modulation
- **WHEN** a displayed source has nonzero audio-rate modulation affecting its signal or parameterization
- **THEN** the global oscilloscope changes that trace's visualization according to manifest-declared Sheaf-style visualization rules
- **THEN** the change is per-trace and does not require a second oscilloscope
- **THEN** unmodulated displayed traces keep their normal visualization

#### Scenario: External audio source taps respect availability
- **WHEN** external audio is inactive or unavailable
- **THEN** External Audio (audio rate) and External Audio (envelope follower) source taps report unavailable/off state
- **THEN** the global oscilloscope does not display stale external-audio traces as active modulation

#### Scenario: Oscilloscope persists across carousel pages
- **WHEN** the user switches from the Audio module to another carousel module
- **THEN** the global oscilloscope remains visible

#### Scenario: Oscilloscope persists across runtime pages
- **WHEN** the user opens Audio, Controllers, or File/Patch runtime pages in desktop standalone
- **THEN** the global oscilloscope remains visible

#### Scenario: Hosted editor keeps the signal monitor
- **WHEN** FroggersTigaPluginV2 opens at its minimum hosted editor size
- **THEN** the global oscilloscope is visible

### Requirement: Global oscilloscope is separate from mod-source cells
The global top-chrome oscilloscope SHALL be a shell-level signal monitor and SHALL NOT replace the v2 mod-source grid scope/LED cells or encoder-integrated CV LED modulation monitors.

#### Scenario: EF grid remains available
- **WHEN** the mod source grid renders VCO EF source lanes
- **THEN** their source cells still render according to manifest-declared EF source indicators
- **THEN** the global oscilloscope remains a separate shell-level monitor

#### Scenario: Audio page does not duplicate the scope
- **WHEN** the runtime Audio page is visible
- **THEN** it shows device, channel, bus, and level/status information
- **THEN** it does not create another oscilloscope beyond the global top-chrome oscilloscope

### Requirement: Global oscilloscope uses bounded signal buffers
The global top-chrome oscilloscope SHALL use fixed-capacity signal buffers and repaint on the named desktop v2 scope UI timer.

#### Scenario: Scope buffer remains bounded
- **WHEN** audio runs for repeated UI frames
- **THEN** global oscilloscope storage does not grow

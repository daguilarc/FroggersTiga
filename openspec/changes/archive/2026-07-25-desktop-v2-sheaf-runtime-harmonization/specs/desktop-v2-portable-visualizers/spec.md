## ADDED Requirements

### Requirement: Portable visualizer contract on Application surface
Desktop v2 Application surface SHALL compose Sheaf-style JUCE-free `Visualizer` instances (including `ScopeVisualizer` multi-layer scopes and `GangedRandomLfoVisualizer` for Random S&H mod-depth cells) under encoder cells when modulator metadata publishes a non-null visible visualizer. Visualizer instances SHALL be address-stable, non-owning of DSP state, and rendered via the portable UI backend.

#### Scenario: Random S&H depth cell shows ganged visualizer
- **WHEN** parameter-detail mod view shows the Random S&H 1 or Random S&H 2 depth cell with a published ganged visualizer
- **THEN** the visualizer draw node occupies the encoder cell bounds under the interactive encoder
- **THEN** no Step chance, Deja vu, Bag size, or Slew module-page control is required for that visualization

#### Scenario: Hidden visualizer emits no node
- **WHEN** a visualizer is intrinsically hidden or its published pointer is null
- **THEN** only the encoder node is rendered for that cell

### Requirement: Dual multi-layer oscilloscope panels
Desktop v2 Application surface SHALL present exactly two scope panels using multi-layer overlapping color-coded traces: one panel for the three VCO waveforms and one panel for the three LFO envelope-follower traces. The previous single global scope fed only by three VCO EF taps SHALL NOT be the sole product oscilloscope.

#### Scenario: VCO panel shows three layers
- **WHEN** the Application surface is visible and VCO scope layers are connected
- **THEN** the VCO scope panel draws three color-coded overlapping traces

#### Scenario: LFO EF panel shows three layers
- **WHEN** the Application surface is visible and LFO EF scope layers are connected
- **THEN** the LFO EF scope panel draws three color-coded overlapping traces

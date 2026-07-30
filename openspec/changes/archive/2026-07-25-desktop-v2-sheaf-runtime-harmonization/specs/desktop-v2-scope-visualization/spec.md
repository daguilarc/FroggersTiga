## ADDED Requirements

### Requirement: Dual scope panels replace sole EF global scope
Desktop v2 scope visualization SHALL provide two Application-surface panels: VCO multi-layer and LFO EF multi-layer, per `desktop-v2-portable-visualizers`. Manifest oscilloscope tap declarations SHALL support both panels’ layer wiring.

#### Scenario: Product surface shows two panels
- **WHEN** the operator views the Application surface
- **THEN** both the VCO scope panel and the LFO EF scope panel are present

## REMOVED Requirements

### Requirement: Single global scope fed only by three VCO EF taps as sole viz
**Reason:** Replaced by dual Sheaf-style multi-layer panels (VCO waveforms + LFO EFs).  
**Migration:** Retire `kOscilloscopeTaps` EF-only sole-viz role; wire new layer sources; update validators and docs.

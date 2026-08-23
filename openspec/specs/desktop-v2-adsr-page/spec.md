# desktop-v2-adsr-page Specification

## Purpose
Desktop v2 and VST v2 dedicate host page 6 to a ten-row ADSR module, with independent per-VCO gated envelopes driven by the shared `VcoAdsrState` engine, superseding v1's `AudioPairArState` on v2 hosts only.
## Requirements
### Requirement: v2-adsr-module-page
Desktop v2 and VST v2 SHALL provide host page index 6 (ADSR module) with ten parameter rows.

| Row | Label (abbrev) | Role |
|-----|----------------|------|
| 0 | Atk1 | Attack time VCO1 |
| 1 | Atk2 | Attack time VCO2 |
| 2 | Atk3 | Attack time VCO3 |
| 3 | Sus1 | Sustain level VCO1 |
| 4 | Sus2 | Sustain level VCO2 |
| 5 | Sus3 | Sustain level VCO3 |
| 6 | Rel1 | Release time VCO1 |
| 7 | Rel2 | Release time VCO2 |
| 8 | Rel3 | Release time VCO3 |
| 9 | Crispy | Page-local fuego for ADSR rows 0–8 (stacks after global Crunchy on musical rows; Crispy instance also receives global Crunchy) |

#### Scenario: ADSR page in carousel
- **WHEN** the user navigates the module carousel to ADSR
- **THEN** ten encoder rings are visible with the labels above

#### Scenario: Rand All includes ADSR knobs
- **WHEN** Rand All runs
- **THEN** ADSR rows 0–8 are randomized
- **THEN** ADSR row 9 Crispy is skipped (matching v1 FUEG exclusion)

### Requirement: v2-gated-adsr-per-vco
On v2 hosts, each VCO SHALL have an independent **gated ADSR** envelope driven by rows 0–8 and a gate from MIDI note input, sequencer step gate, or the host bridge.

#### Scenario: Gate on triggers attack and sustain
- **WHEN** the gate rises for VCO1
- **THEN** VCO1 amplitude attacks to sustain level and holds while gate is high

#### Scenario: Gate off triggers release
- **WHEN** the gate falls for VCO2
- **THEN** VCO2 releases on its release time; other VCOs are unaffected

#### Scenario: Independent per-VCO times and levels
- **WHEN** attack, sustain, and release differ per VCO
- **THEN** each VCO envelope follows its own A/S/R parameters on shared gate edges

### Requirement: v2-adsr-engine
The shared engine SHALL apply `VcoAdsrState` on v2 hosts, superseding `AudioPairArState`.

#### Scenario: v1 pair-AR unchanged
- **WHEN** `SimHostKind::Desktop` runs
- **THEN** `AudioPairArState` behavior is unchanged


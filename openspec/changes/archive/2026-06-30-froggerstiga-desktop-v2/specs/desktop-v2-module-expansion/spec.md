## ADDED Requirements

### Requirement: v2-ten-row-module-pages
Modules **Random (1), Reverb (2), Filter (3), Drive (4), and Delay (5)** SHALL expose **ten** encoder rows: v1 rows 0–6 unchanged, two new rows 7–8, **Crispy** at row 9. **Audio (0)** and **ADSR (6)** SHALL NOT receive row additions in this expansion.

#### Scenario: Expanded module row count
- **WHEN** any module page 1–5 is visible
- **THEN** exactly ten encoder rows are shown (indices 0–9)
- **THEN** Crispy is row 9 (not row 7)

#### Scenario: Audio unchanged
- **WHEN** the Audio module is visible
- **THEN** no v2 expansion rows are added beyond the existing Audio page layout

#### Scenario: ADSR unchanged row count
- **WHEN** the ADSR module is visible
- **THEN** ten rows remain (rows 0–8 A/S/R, row 9 Crispy)

### Requirement: v2-module-expansion-labels
New row labels SHALL be defined in `V2ParamDisplayNames.hpp`.

| Page | Row 7 | Row 8 | Row 9 |
|------|-------|-------|-------|
| Random (1) | Spread | Bias | Crispy |
| Reverb (2) | Mod depth | Hold | Crispy |
| Filter (3) | Comb/Peak | Scoop | Crispy |
| Drive (4) | Blend | Phase | Crispy |
| Delay (5) | Color | Halo | Crispy |

#### Scenario: Labels from display authority
- **WHEN** an expanded module row is rendered
- **THEN** labels match the table above from `V2ParamDisplayNames`

### Requirement: v2-filter-parallel-comb-peak
On v2 hosts, the Filter module SHALL use **parallel** Comb and Peak paths with a **Comb/Peak** crossfade (row 7). v1 serial `PureDelay → Comb → Peak` topology SHALL be replaced on v2 only.

#### Scenario: Parallel filter topology
- **WHEN** `SimHostKind::DesktopV2` or `VstV2` processes the filter stage
- **THEN** input feeds both a Comb path (PureDelay + Comb, existing rows 0 and 4–6) and a Peak path (ResonantBump, existing rows 1–3)
- **THEN** row 7 **Comb/Peak** crossfades between the two path outputs (CCW = Peak, CW = Comb)

#### Scenario: Scoop notch
- **WHEN** row 8 **Scoop** is non-zero
- **THEN** a notch centered at/near Peak freq (row 1) is applied to the mixed filter output
- **THEN** CCW = no scoop; CW = deeper mid scoop (Ruina Cancilla-style)

#### Scenario: v1 filter topology unchanged
- **WHEN** `SimHostKind::Desktop` processes audio
- **THEN** serial PureDelay → Comb → Peak order is unchanged

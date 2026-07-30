## MODIFIED Requirements

**Audit 2026-06-30:** Page 6 exposes ten rows (Atk×3, Sus×3, Rel×3, Crispy) via `V2ParamDisplayNames.hpp` L40–41 and `VcoAdsrState` sustain stage. Web sim uses **pair-AR** (attack/release only, no sustain knobs) via `AudioPairArState`. User decision: remove sustain knobs; per-VCO A/R on desktop v2 and VST v2; sustain behavior absorbed into release mapping.

### Requirement: v2-pair-ar-module-page

Desktop v2 and VST v2 SHALL provide host page index 6 as a **Pair-AR** module (carousel label **Pair-AR**, not ADSR) with **seven** parameter rows:

| Row | Label | Role |
|-----|-------|------|
| 0 | Atk1 | Attack time VCO1 |
| 1 | Rel1 | Release time VCO1 (extended range; encodes former sustain+release behavior) |
| 2 | Atk2 | Attack time VCO2 |
| 3 | Rel2 | Release time VCO2 |
| 4 | Atk3 | Attack time VCO3 |
| 5 | Rel3 | Release time VCO3 |
| 6 | Crispy | Page-local fuego for Pair-AR rows 0–5 |

Rows SHALL be ordered as **A/R pairs per VCO** (not grouped Atk×3 then Rel×3).

There SHALL be **no** sustain-level knobs. Host parameters `ADSR/Sus1`, `ADSR/Sus2`, `ADSR/Sus3` and matching mod-depth axes SHALL be removed from `HostParameterInventoryV2`.

`rowsForPage(6)` and `rowsForUiPage(6)` SHALL return **7**.

`V2ParamDisplayNames::CrispyRowForPage(6)` SHALL be **6**.

#### Scenario: Pair-AR page in carousel

- **WHEN** the user navigates the module carousel to Pair-AR
- **THEN** seven encoder rings are visible with labels Atk1, Rel1, Atk2, Rel2, Atk3, Rel3, Crispy
- **THEN** no Sustain rows appear

#### Scenario: Rand All includes Pair-AR knobs

- **WHEN** Rand All runs
- **THEN** Pair-AR rows 0–5 are randomized
- **THEN** row 6 Crispy is skipped

#### Scenario: VST host parameters match desktop

- **WHEN** `HostParameterInventoryV2` is generated after this change
- **THEN** Pair-AR exposes six attack/release knob axes plus six mod-depth axes (not nine+ nine)
- **THEN** `FROGGERS_EXPECT_HOST_PARAM_COUNT_V2` is updated (148 → **142**)
- **THEN** display names use `Pair-AR/Atk1`, `Pair-AR/Rel1`, … (or `ADSR/` prefix migrated — stable IDs for removed Sus* params are dropped; preset migration resets Sus* slots)

### Requirement: v2-gated-ar-per-vco

On v2 hosts, each VCO SHALL have an independent **attack–release** envelope (not ADSR with sustain knob):

- Gate **on** → attack toward **1.0**, hold at **1.0** while gate remains high
- Gate **off** → release from current level using that VCO's release knob
- No separate sustain level parameter

While **Start Sequence** is **not** running, envelope gate SHALL default **open** so internal VCOs drive output continuously (`desktop-v2-sequencing` / `design.md` §4.1). While **Start Sequence** is running, gate follows `m_gateHigh || activeStepGate()` for pattern/MIDI performance.

Release knob mapping SHALL extend to at least **10 s** at max (matching web pair-AR 1 ms–10 s spirit) so tail length formerly controlled by sustain level is achievable via release time/curve.

`FroggersEngine` SHALL read attack from rows `0,2,4` and release from rows `1,3,5` on page 6 (not the old `0–2` attack / `3–5` sustain / `6–8` release layout).

#### Scenario: Default open gate when sequencer stopped

- **WHEN** audio processing is active and **Start Sequence** is off
- **THEN** VCO output is at full envelope level from internal oscillators
- **THEN** knob/scene edits on Audio are audible without MIDI

#### Scenario: Gate on attacks to full level

- **WHEN** the gate rises for VCO1 during **Start Sequence** playback
- **THEN** VCO1 amplitude attacks toward **1.0** and holds at **1.0** while gate is high

#### Scenario: Gate off triggers release

- **WHEN** the gate falls for VCO2 during pattern playback
- **THEN** VCO2 releases on its Rel2 time; other VCOs are unaffected

#### Scenario: Web envelope model parity

- **WHEN** comparing desktop v2 Pair-AR to web Audio pair-AR
- **THEN** both use attack+release only (no sustain knob)
- **THEN** web retains four **pair-sum** knobs on the Audio page; desktop v2 retains six **per-VCO** knobs on the Pair-AR carousel page (layout differs; envelope family matches)
- **THEN** both allow continuous internal VCO output without requiring MIDI or sequencer gates for basic knob tweaking

### Requirement: v2-pair-ar-engine

`VcoAdsrState` SHALL be refactored to **AR** (remove `Stage::Sustain` and sustain knob argument). Rename to `VcoArState` or equivalent; sustain level is implicit **1.0** while gate is high. Default gate policy: open when sequencer stopped; performance gating while **Start Sequence** runs (`design.md` §4.1).

#### Scenario: v1 pair-AR unchanged

- **WHEN** `SimHostKind::Desktop` runs
- **THEN** `AudioPairArState` behavior is unchanged

#### Scenario: v1 desktop ADSR page N/A

- **WHEN** `SimHostKind::Desktop` or `SimHostKind::Vst` (v1) runs
- **THEN** no Pair-AR carousel page exists (v1 only)

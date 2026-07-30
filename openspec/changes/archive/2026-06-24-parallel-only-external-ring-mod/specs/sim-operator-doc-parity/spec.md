## ADDED Requirements

### Requirement: Sim manual learner-first structure

`SIM_MANUAL.md` and its mirrors (`docs/sim-manual.md`, `web/public/sim-manual.md`) SHALL present content in top-to-bottom learning order:

1. Getting sound (Play / Stop)
2. External input (optional)
3. Randomize controls (page + global strip)
4. Crispy (knob 8 on every page)
5. Brief signal-path overview
6. Audio page/column (before other pages)
7. Remaining pages in learning order: Random, Drive, Filter, Reverb, Delay
8. Mod bay
9. Per-host differences (short)
10. Appendix (version history, advanced host/MIDI reference)

The manual SHALL NOT open with layout trivia, host page index tables, or mod-bay detail before Play, External, and Randomize are explained.

#### Scenario: New reader top of manual

- **WHEN** a reader opens any sim manual mirror from the top
- **THEN** the first operational sections cover Play/Stop, optional Ext. In., and randomize buttons before page knob tables

#### Scenario: Audio before Random in page sections

- **WHEN** a reader reaches the page/column reference sections
- **THEN** Audio is documented first, then Random, then Drive, Filter, Reverb, Delay

### Requirement: Desktop layout described correctly

Sim manuals SHALL describe desktop/VST layout as **six equal columns**: **Audio → Random → Drive → Filter → Reverb → Delay**.

The manual SHALL NOT describe Delay as an overlay, popup, or separate page hidden behind the five core columns.

#### Scenario: Desktop host guide

- **WHEN** reader opens the desktop layout description
- **THEN** text states six visible columns with Delay as the rightmost column

### Requirement: Audio and Random explained in plain language

The Audio section SHALL explain, in plain language:

- Three VCOs and waveform morph (VCO1/2)
- Cross-coupler: CCW enables 1→2 coupling, CW enables 2→3, noon = off
- Phase mod knobs and their coupling gates
- External input: VCO-only when silent/off; parallel ring mod `(ext×VCO1 + ext×VCO2 + ext×VCO3) / 3` when the external gate is open

The Random section SHALL state the page is inspired by **Mutable Instruments Marbles**, describe bag/resample behavior, and link outputs to **Random 1 S&H** / **Random 2 S&H** mod sources.

#### Scenario: External ring mod without topology knob

- **WHEN** reader reads the Audio external-input explanation
- **THEN** text does not describe product ring mod, mix morph, or Crispy controlling external mix shape

#### Scenario: Marbles attribution

- **WHEN** reader opens the Random page section
- **THEN** Marbles inspiration is stated in operator language (not trademark legalese blocks)

## MODIFIED Requirements

### Requirement: PM3 vs Crispy vs VCO Envelope glossary

Sim manuals SHALL include a short note that:

- **Phase mod 3** (row 7) is a dedicated PM3 depth knob on sim hosts.
- **Crispy** (row 8) scrambles knobs 1–7 on fuego-enabled pages; it no longer controls PM3 on sim hosts and does not control external ring-mod mix on any host.
- **VCO Envelope** is a mod source (slow CV from VCO mix), shown as a scope on desktop/web mod rack — not the row 7 knob.

#### Scenario: Desktop embedded manual

- **WHEN** user opens Manual from the desktop app Help menu
- **THEN** embedded content matches root `SIM_MANUAL.md` PM3/Crispy/VCO Envelope distinction

#### Scenario: Web help modal

- **WHEN** user opens Manual on the website
- **THEN** fetched `sim-manual.md` matches the same PM3 row 7 semantics

#### Scenario: No external mix topology in Crispy gloss

- **WHEN** reader opens Global controls / Crispy section in any sim manual mirror
- **THEN** text does not describe product ring mod, parallel ring mod morph, or FUEG/Crispy blend of mix topologies

## REMOVED Requirements

### Requirement: Mix topology documented in operator signal flow

**Reason:** Product ring mod and FUEG/Crispy morph removed from engine; only parallel ring mod remains when external gate is open.

**Migration:** Describe external path as parallel ring mod when Ext. In. is active; VCO-only when silent. Remove mix-topology tables from `MANUAL.md` and sim manuals.

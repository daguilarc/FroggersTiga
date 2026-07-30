# sim-operator-doc-parity Specification

## Purpose
Keep operator documentation mirrors synchronized with sim host control labels and behavior described in SIM_MANUAL and web help copies.
## Requirements
### Requirement: Operator docs match ParamDisplayNames

All sim operator documentation (`SIM_MANUAL.md`, `QUICK_DICT.md`, `docs/sim-manual.md`, `docs/quick-dict.md`, `web/public/sim-manual.md`, `web/public/quick-dict.md`) SHALL list Audio page row 7 as **Phase mod 3** with the gloss: VCO2 → VCO3 PM depth when cross-coupler is CW (2→3).

#### Scenario: docs sim-manual Audio table

- **WHEN** a reader opens `docs/sim-manual.md` Page 1 — Audio table
- **THEN** row 7 is **Phase mod 3**, not VCO level or VCO Envelope

#### Scenario: docs quick-dict Audio section

- **WHEN** a reader opens `docs/quick-dict.md` Audio section
- **THEN** an entry for **Phase mod 3** exists and no entry implies row 7 is OLVL/VCO-only level

#### Scenario: Crispy naming consistency

- **WHEN** sim manuals reference knob 8 on any page
- **THEN** the sim-facing name is **Crispy** (matching UI), with Field OLED name FUEG noted parenthetically where relevant

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

### Requirement: VCO Envelope scope documented

Operator docs SHALL state that the VCO Envelope mod rack scope displays **mod CV** (0–100% slow envelope of VCO mix level), not an audio waveform.

#### Scenario: Mod bay scope description

- **WHEN** user reads Mod sources / Mod bay section
- **THEN** VCO Envelope is described as continuous CV scope trace of slow level from VCO mix, distinct from Phase mod 3 knob

### Requirement: Mod blend semantics documented in sim manuals

All sim operator documentation (`SIM_MANUAL.md`, `QUICK_DICT.md`, `docs/sim-manual.md`, `docs/quick-dict.md`, `web/public/sim-manual.md`, `web/public/quick-dict.md`) SHALL include, in or under **Mod bay**, a **Mod depth & blend** subsection stating:

- Effective value crossfades between stored base and mod source via mod depth (not `knob × CV`).
- At depth 0 → base only; depth 1 → mod only; between → mix.
- UI shows live effective value while idle; drag edits mod depth.
- M1–M4 ignored when CV input inactive.

#### Scenario: sim-manual Mod bay section

- **WHEN** reader opens `web/public/sim-manual.md` Mod bay
- **THEN** Mod depth & blend subsection is present

#### Scenario: quick-dict mod depth entry

- **WHEN** reader opens `web/public/quick-dict.md`
- **THEN** mod depth defined as crossfade amount

#### Scenario: All manual copies in sync

- **WHEN** change is applied
- **THEN** `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md` contain equivalent text

### Requirement: Mod-then-fuego pipeline documented

Sim manuals SHALL state that on fuego-enabled pages (including Delay), modulation is applied first, then Crispy scrambles low bits of the result. Crispy itself can be modulated (scramble intensity follows effective Crispy). Pair-AR knobs are not fuegoized.

#### Scenario: Crispy mod gloss in manual

- **WHEN** reader opens Delay or Global controls section
- **THEN** text explains Crispy mod affects scramble intensity on rows 1–7

#### Scenario: Pair-AR exclusion noted

- **WHEN** reader opens Audio pair-AR section
- **THEN** pair-AR knobs described as moddable but not fuegoized

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

#### Scenario: No external mix topology in Crispy gloss

- **WHEN** reader opens Crispy section in any sim manual mirror
- **THEN** text does not describe product ring mod, parallel ring mod morph, or FUEG/Crispy blend of mix topologies

### Requirement: Quick Dict desktop v2 boot outcome

`QUICK_DICT.md` SHALL state that a healthy desktop v2 standalone launch keeps the main window open (instant exit indicates a fault). Engine and Stop transport controls are already documented in §Transport; this requirement adds the boot-outcome gloss only.

#### Scenario: Boot outcome documented

- **WHEN** an operator reads `QUICK_DICT.md` for desktop v2 standalone behavior
- **THEN** they find that the window remaining open after launch is the expected healthy boot outcome

### Requirement: Quick Dict desktop v2 carousel page navigation

`QUICK_DICT.md` SHALL document carousel left/right arrow buttons for module page changes. Rand / Rand Mod on the carousel header are already summarized in §Transport; this requirement adds explicit navigation controls only.

#### Scenario: Carousel navigation documented

- **WHEN** an operator reads the desktop v2 Quick Dict section
- **THEN** they find that left/right arrows on the carousel header change the active module page

### Requirement: Help doc mirrors stay synchronized

After Quick Dict edits, `scripts/sync-help-docs.sh` SHALL be run so `docs/`, `web/public/`, and embedded binary copies match.

#### Scenario: Sync script run after doc change

- **WHEN** `QUICK_DICT.md` is updated for this change
- **THEN** mirrored paths are updated via `scripts/sync-help-docs.sh` before merge

### Requirement: v2-operator-center-cluster-docs

Operator documentation SHALL describe the center global cluster and updated Write Seq. workflow.

#### Scenario: Center cluster documented

- **WHEN** reader opens desktop v2 layout documentation
- **THEN** Rand All / Crunchy / Shift are documented in the module center column, not the bottom strip

#### Scenario: Write Seq. playing workflow documented

- **WHEN** reader opens Write Seq. documentation
- **THEN** text explains: arm Write Seq., Start Sequence, edit highlight follows playhead while playing, each beat saves the step being left, first beat after start does not duplicate step-0 capture, stopped navigation saves on step change


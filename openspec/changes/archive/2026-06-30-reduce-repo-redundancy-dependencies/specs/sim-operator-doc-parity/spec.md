## ADDED Requirements

### Requirement: Sim docs mirrors are publication outputs

`docs/` and `web/public/` copies of sim operator manuals SHALL be treated as publication mirrors of root sim documentation, not independent editing surfaces. The sync/check workflow SHALL identify `SIM_MANUAL.md`, `QUICK_DICT.md`, and `LICENSE` as authorities for their mirrors.

#### Scenario: Root sim manual drives mirrors

- **WHEN** root `SIM_MANUAL.md` changes
- **THEN** `docs/sim-manual.md` and `web/public/sim-manual.md` are regenerated or docs sync verification fails

#### Scenario: Web public quick dict mirrors root

- **WHEN** root `QUICK_DICT.md` changes
- **THEN** `docs/quick-dict.md` and `web/public/quick-dict.md` are regenerated or docs sync verification fails

### Requirement: Sim manual public host scope is launch-gated

`SIM_MANUAL.md` and its mirrors (`docs/sim-manual.md`, `web/public/sim-manual.md`) SHALL describe launched desktop standalone and web sim behavior only. The manuals SHALL NOT present VST/AU, plugin, VCV, or Rack as supported user surfaces until those hosts are tested and launched by a later explicit documentation change.

#### Scenario: Public sim manual excludes pre-launch hosts

- **WHEN** a reader opens any sim manual mirror
- **THEN** it documents desktop standalone and web sim behavior
- **THEN** it does not present VST/AU, plugin, VCV, or Rack as available host options

#### Scenario: Internal specs can retain pre-launch host contracts

- **WHEN** VST/AU or VCV implementation/testing work continues
- **THEN** internal OpenSpec specs may retain those requirements
- **THEN** public sim manual mirrors remain desktop/web-only until launch

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
9. Desktop/web differences (short)
10. Appendix (version history and launched-host reference)

The manual SHALL NOT open with layout trivia, host page index tables, or mod-bay detail before Play, External, and Randomize are explained. The manual SHALL NOT include VST/AU, plugin, VCV, or Rack user-facing sections until those hosts are launched by a later explicit documentation change.

#### Scenario: New reader top of manual

- **WHEN** a reader opens any sim manual mirror from the top
- **THEN** the first operational sections cover Play/Stop, optional Ext. In., and randomize buttons before page knob tables

#### Scenario: Audio before Random in page sections

- **WHEN** a reader reaches the page/column reference sections
- **THEN** Audio is documented first, then Random, then Drive, Filter, Reverb, Delay

### Requirement: Desktop layout described correctly

Sim manuals SHALL describe desktop layout as **six equal columns**: **Audio -> Random -> Drive -> Filter -> Reverb -> Delay**. Web layout SHALL be described as a paged interface.

The manual SHALL NOT describe Delay as an overlay, popup, or separate desktop page hidden behind the five core columns. The manual SHALL NOT describe VST/AU layout until the plugin host is launched and documented by a later change.

#### Scenario: Desktop host guide

- **WHEN** reader opens the desktop layout description
- **THEN** text states six visible columns with Delay as the rightmost column

#### Scenario: Web host guide

- **WHEN** reader opens the web layout description
- **THEN** text describes pages rather than desktop columns

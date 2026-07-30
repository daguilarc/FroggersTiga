# froggers-app-surface-layout Specification

## Purpose
The portable Froggers surface (dual VCO scope band + global chrome, sixteen-slot encoder grid, in-place modulation-detail swap) ported from the v2 layout design, built entirely with Sheaf's portable UI builder so the same description drives both the desktop and browser hosts.

## Requirements
### Requirement: Portable surface built with Sheaf UI
The Froggers surface SHALL be constructed entirely with Sheaf's portable UI builder. No JUCE component from the legacy desktop trees SHALL be reused.

#### Scenario: Surface is portable
- **WHEN** the app renders in either the desktop or browser host
- **THEN** the same portable surface description drives both
- **THEN** no legacy JUCE panel class participates

### Requirement: Scope band with global chrome
The surface SHALL present a band containing the VCO scope panels alongside transport and global controls: the **Randomize All** control, scene controls, and a tempo (BPM) slider positioned immediately beside the scene slider, so the two sliders sit together. The **Randomize Page** control SHALL NOT appear in this band; it SHALL instead appear in the per-page/bank header. Global Crunchy SHALL NOT appear in this band either (corrected 2026-07-27 — operator: "why is there a fucking slider for crunchy between the randomize buttons, i never asked for that. It duplicates bank slot 15"); Crunchy's only control surface is bank slot 15 in the encoder grid (see the "Sixteen-slot encoder grid" requirement below), reachable like any other bank parameter and, as a deliberately accepted trade-off, unreachable while a modulation view is open (slot 15 is then Target/Back).

#### Scenario: Scopes and chrome share the band
- **WHEN** the surface is displayed
- **THEN** the VCO scope panels are visible
- **THEN** transport, Randomize All, scene, and tempo controls are reachable without leaving the main view
- **THEN** no global Crunchy control appears in this band

#### Scenario: Tempo slider sets and displays the master clock tempo
- **WHEN** the operator adjusts the tempo slider
- **THEN** the master clock's tempo is set to the slider's value
- **THEN** the slider displays the active tempo

#### Scenario: External MIDI clock makes the tempo slider read-only
- **WHEN** the host is slaved to an external MIDI clock
- **THEN** the tempo slider becomes read-only and inert, and no longer accepts input
- **THEN** the slider displays the recovered external tempo instead

#### Scenario: Exactly two randomize controls exist in the whole surface
- **WHEN** the entire surface is enumerated for randomize controls
- **THEN** exactly two exist: Randomize All in the global chrome band and Randomize Page in the per-page/bank header
- **THEN** no other randomize control appears anywhere

### Requirement: No dedicated waveform-randomize or manual random-source controls
The surface SHALL provide no dedicated waveform-randomize control and no manual random-source step/resample control. The former is retired because the waveform Shape controls are now ordinary bank parameters covered by Randomize Page. The latter is retired because the random sources are driven by the master clock rather than by manual stepping.

#### Scenario: Neither control appears anywhere
- **WHEN** the entire surface is enumerated for controls
- **THEN** no dedicated waveform-randomize control exists
- **THEN** no manual random-source step/resample control exists

### Requirement: Bank selector with direct selection
The surface SHALL provide direct selection among banks. Arrow-based paging SHALL NOT be the primary navigation. Exactly one bank SHALL be active at a time, with a single authority for that selection.

#### Scenario: Direct bank selection
- **WHEN** the operator selects a bank
- **THEN** that bank's parameters populate the encoder grid
- **THEN** no second, divergent bank-selection state exists

### Requirement: Sixteen-slot encoder grid with in-place modulation swap
The surface SHALL render the active bank as a sixteen-slot encoder grid indexed `0..15`, with the local Crispy control at slot index 14 and global Crunchy at slot index 15 in every bank, and empty cells wherever the bank has no parameter. Entering a modulation view SHALL replace the grid contents in place with the modulation detail cells, occupying the same region; it SHALL NOT open a separate window or push a new page.

#### Scenario: Empty cells render as empty
- **WHEN** the active bank uses fewer than fourteen parameter slots
- **THEN** the unused cells render as empty
- **THEN** Crispy and Crunchy still occupy slot indices 14 and 15

#### Scenario: Drill-in swaps in place
- **WHEN** the operator opens a parameter's modulation view
- **THEN** the modulation detail cells occupy the same grid region
- **THEN** the surrounding scope band and chrome remain in place

#### Scenario: Return restores the parameter grid
- **WHEN** the operator activates Target/Back from any modulation level
- **THEN** the bank's parameter grid is restored in the same region

### Requirement: Layout integrity at the target window size
The surface SHALL lay out without overlap or clipping at the target window size, and its regions SHALL be verified by automated bounds tests.

#### Scenario: No overlapping regions
- **WHEN** the surface is laid out at the target size
- **THEN** the scope band, chrome, bank selector, and encoder grid regions do not overlap
- **THEN** every encoder cell lies fully within the grid region

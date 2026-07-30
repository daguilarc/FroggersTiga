## 1. Audit Guards And Tests

- [x] 1.1 Add a VCV section adapter unit test proving section writes do not mutate `m_currentPage` or shared `m_knobPositions`
- [x] 1.2 Add a VCV effective-value test covering disconnected internal route, connected CV addition, clamp high/low, and no double internal-route application
- [x] 1.3 Add a VCV Random All test proving randomization reads section state directly and does not depend on `m_knobPositions`
- [x] 1.4 Add a VCV Randmod test proving assignments are None or internal sources `4`, `5`, `6` only
- [x] 1.5 Add a global Crunchy CV test covering disconnected input, positive CV addition, negative CV, and clamp behavior
- [x] 1.6 Extend VCV boundary checks to reject VCV wrapper calls to current-page/latch APIs such as direct `SetPageKnob`, `KnobUpdateOnPage`, or `m_currentPage` use outside the adapter

## 2. Section Adapter

- [x] 2.1 Define VCV section identifiers for Audio, Random, Filter, Drive, Reverb, Delay, Global, and VCO AR
- [x] 2.2 Implement a fixed-storage VCV section state/snapshot type for base values, internal routes, route depths, CV connectivity, and CV voltages
- [x] 2.3 Implement section-to-shared-engine mapping behind a VCV-only adapter without exposing page terminology to `vcv/src`
- [x] 2.4 Implement temporary effective-value evaluation for per-parameter Rack CV without persisting physical CV to base knobs
- [x] 2.5 Ensure adapter operations are allocation-stable in steady-state audio processing

## 3. Main Module Ownership

- [x] 3.1 Refactor `FroggersTigaModule` so main collects all VCV section snapshots before calling the shared engine process function
- [x] 3.2 Remove direct Rack wrapper writes that mutate shared current-page hardware latch state
- [x] 3.3 Move right-extension section application out of right-extension audio callback mutation/restore flow and into main-owned snapshot consumption
- [x] 3.4 Preserve main audio/CV/gate I/O, Random trigger, and internal mod outputs `4`, `5`, `6`
- [x] 3.5 Preserve schema-v2 patch migration or add a new migration if global Crunchy changes saved parameter IDs

## 4. Global Crunchy

- [x] 4.1 Add global Crunchy knob and CV input IDs to the VCV main module without introducing MIDI or DAW host parameter concepts
- [x] 4.2 Wire effective global Crunchy into VCV section processing while preserving per-section Crispy behavior
- [x] 4.3 Add panel widget placement for global Crunchy knob and CV input from shared layout constants
- [x] 4.4 Document and test the global Crunchy and per-section Crispy processing order

## 5. Left And Right Extensions

- [x] 5.1 Define the left VCO AR extension model and snapshot contract
- [x] 5.2 Implement or stage `Froggers Tiga VCO AR` registration with six Attack/Release controls, local Crispy, Randomize, Randmod, and optional CV inputs
- [x] 5.3 Ensure main consumes left VCO AR defaults when the extension is absent
- [x] 5.4 Update the right FX extension to publish Reverb/Delay section state and stereo I/O to main without owning engine state
- [x] 5.5 Add expander-link tests or local smoke coverage for absent, left-only, right-only, and both-extension configurations

## 6. Panels And Docs

- [x] 6.1 Add or update VCV display-name helpers for section titles, global Crunchy/CV, and VCO AR labels
- [x] 6.2 Update `VcvPanelLayout.hpp` with section/extension/global Crunchy layout constants
- [x] 6.3 Update `vcv/scripts/generate_panels.py` to generate page-free path-based silkscreen
- [x] 6.4 Regenerate `vcv/res/*.svg` and verify no live `<text>` remains
- [x] 6.5 Update `vcv/README.md` and `vcv/DEVELOPMENT.md` to describe main plus optional left/right extensions and remove stale MIDI/CC-enable/page wording
- [x] 6.6 Update `desktop/PACKAGING.md` and host docs only where they describe VCV local-only boundaries

## 7. Verification

- [x] 7.1 Run VCV sim/unit tests including the new section adapter, effective-value, Random/Randmod, global Crunchy, and patch migration tests
- [x] 7.2 Run `sim/check_vcv_midi_boundary.sh`
- [x] 7.3 Run `sim/check_vcv_license_boundary.sh`
- [x] 7.4 Run `sim/check_vcv_panel_bounds.sh`
- [x] 7.5 Run `sim/check_vcv_panel_svg.sh`
- [x] 7.6 Run `scripts/check_openspec_hygiene.sh --post-closure` or the nearest available OpenSpec hygiene command
- [x] 7.7 If local Rack SDK is available, run the VCV build smoke without installing new dependencies

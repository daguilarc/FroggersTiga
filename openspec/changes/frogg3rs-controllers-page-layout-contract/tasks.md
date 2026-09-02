# Tasks — `frogg3rs-controllers-page-layout-contract`

Task 1 is the fresh-context preflight; the operator read its report and
gave the go on 2026-09-02. Dispatches are sequential. No fix
outside this text: a defect this text does not name stops execution and
supersedes the change.

## 1. Preflight (fresh context), before any code

- [x] 1.1 Verify the proposal's table "The tree as it stands" against
      `git -C External/Sheaf diff` file by file: every edit listed is in
      the diff, nothing in the diff is unlisted, and each "verified by"
      test exists and passes (run the built binaries; rebuild only if
      their mtimes are older than the sources).
- [x] 1.2 Verify the citations in "What went wrong in the page" and
      "What the operator saw" (file:line, constants, sums), exact / off
      by n / wrong.
- [x] 1.3 Read the fits-within fixture and confirm it covers only
      collapsed rows (or say which states it opens); list the section
      and row states the proposal's item 1 names and confirm each is
      reachable through the view model's actions the fixture can
      dispatch (name the action ids).
- [x] 1.4 Confirm the Playwright spec runs in CI (which workflow, which
      job) and how a per-state overflow assertion would drive the page's
      states there (the actions the harness dispatches), or say it
      cannot and why.
- [x] 1.5 Hygiene sweep of the Impact files: dead code by invocation
      search, stale citations naming line numbers of files this change
      touched, junk.
- [x] 1.6 Audit against the omni rule: claims cited and read; reuse; any
      behavioural claim only a run settles; overlap with other active
      changes; scope against "What the operator saw".
- [x] 1.7 Report to the operator; stop.

## 2. Sheaf, on branch `app-midi-catalog`

- [x] 2.1 Extend `TestControllersRowFitsWithinFroggersNarrowestHost`
      (`tests/portable_ui_tests.cpp`) to the open states: Generic row
      (index 1) expanded with Encoders open (a Turn row and a Push row
      added), System Messages open (one row), Analogs open (one Gesture
      row, one App action row); Launchpad row (index 2) expanded with
      System Messages open; Twister (index 0) expanded with Encoders
      open; zero fits-within violations after every step; the Generic
      system row's Message combo offers the catalog passed through
      `callbacks.messageCatalog` (a frogg3rs-shaped catalog of 5 library
      kinds and 19 actions, 24 choices from `MakeUISystemMessageChoices`).
      The fixture needs, beyond what it has today: a `commitInstrument`
      callback that assigns the committed config back into `instrument`
      and resizes `connection.controllers` (the pattern at
      `tests/controllers_page_ui_tests.cpp:209-219`; without it every add
      is refused at `ControllersPageUI.hpp:1288-1292`);
      `callbacks.analogActionCatalog = MakeAnalogAppActionChoices(catalog)`
      (an empty one makes the App actions group refuse adds,
      `src/MidiConfigViewModel.cpp:3733-3737`); and the actions, dispatched
      through `surface.DispatchAction` then `RefreshOnTick()` before each
      `BuildTree()`: `Actions::kToggleConfig` with value `"<ix>"`;
      `Actions::kToggleSection` with `"<ix>:encoders|system_messages|analogs"`;
      `Actions::kAddSingle` with `"<ix>:<section>:<group>"` where group is
      `encoder_turn`, `encoder_push`, `system`, `analog_gesture` or
      `analog_app_action` (`ControllersPageUI.hpp:664-684`). Gate:
      `portable_ui_tests`, `controllers_page_ui_tests`, the JUCE
      simulation (`apps/miniapp/build/controllers_page_simulation_tests`).
      Record the fixture's open states as a scenario under sru-61 and as
      task 1.21 in Sheaf's `openspec/changes/app-midi-catalog/`.
- [x] 2.2 Add `tests/ui-backend.spec.ts` to the spec files Sheaf's
      `.github/workflows/synth-browser-pages.yml` runs: the workflow has
      two `playwright test` invocations naming three spec files; the one
      that runs against the built browser bundle is the build job's
      "Run local cross-origin publication gates" step (line 57), after
      `npm run build` has produced `dist/src/ui.js` the spec imports. Add
      the file there. Run that spec locally the same way (`npx playwright
      test tests/ui-backend.spec.ts` in `projects/synth/browser`; the
      headless shell Playwright 1.62 wants, build 1234, is installed, so no
      install is needed); if it still cannot run, say so and leave it to
      CI. Record it as task 1.22 in Sheaf's `openspec/changes/app-midi-catalog/`.
- [x] 2.3 Push `app-midi-catalog` to the fork; update PR #13's description
      (two-line row, display names, captions and legend, overlay sizing,
      fits-within criterion, the rendered state list).
      Done 2026-09-02: Sheaf commit 2c27f35c on `app-midi-catalog`, pushed
      to the fork; PR #13 description carries the section.

## 3. frogg3rs

- [x] 3.1 Pin bump as its own commit; `MANUAL.md` MIDI controllers section
      says "MIDI in", "MIDI out", "Device"; `README.md` gains a short MIDI
      controllers paragraph pointing to the manual; `nice make -j2 -C app
      test`; browser build; push `main`; Pages and VST workflows green.
      Done 2026-09-02: pin 4654525, docs 9b31d16; `make -C app test` ten
      binaries green; browser build green; pushed; GitHub Pages run
      33687442876 and VST Plugin run 33687442860 both succeeded.

## 4. Postflight, before any commit

- [x] 4.1 Fresh-context Sonnet reviewer: the whole Sheaf diff against the
      proposal's table and the two spec deltas; the rendered state list
      with the overflow count per state from the lead's browser pass at
      1000 × 820 on the packaged build; divergences reported strictly.
      Commit only when it passes.
      Done 2026-09-02: PASS on all seven items (table vs diff, task 2.1
      and 2.2 vs implementation, spec checks by name, gates fresh and
      green, rendered pass not stale since no page file moved after
      13:36, no duplicate definition of any new symbol); a second pass on
      the MANUAL and README edits: PASS. Positive control for the
      fixture: 29 violations at 700 px, none at 900.

## 5. Operator, then archive

- [ ] 5.1 Desktop and deployed site: with a Twister, a Generic and a
      Launchpad row, every control visible in every state; the Layout
      combo offers the three layouts and Custom; the add row reads
      "Device"; ports read "MIDI in"/"MIDI out" with the legend above.
- [ ] 5.2 Desktop with the Twister: choosing the Twister layout installs
      it; each side button does what the manual says; a push drills in
      and stops at the cap; editing any row flips the combo to Custom.
- [ ] 5.3 Desktop with the APC40 in the Generic layout: each button and
      knob does what the manual says; SHIFT held plus a knob turn drills
      that knob without moving it.
- [ ] 5.4 UNCONFIRMED until this passes. APC40 in the Ableton layout:
      press any Track Select button and turn device knob 1; encoder 9
      still moves; buttons are dark.
- [ ] 5.5 Save a patch, change a mapping, reload the patch: the mapping is
      back. Relaunch: the runtime config's map loads.
- [ ] 5.6 The manual's MIDI section, followed on a Twister and an APC40 at
      factory settings with nothing else to go on.
- [ ] 5.7 Archive on confirmation.

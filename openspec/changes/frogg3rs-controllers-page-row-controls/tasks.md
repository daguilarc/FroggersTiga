# Tasks — `frogg3rs-controllers-page-row-controls`

Task 1 is the preflight and is DONE, inline, 2026-09-03. Tasks 2 onward are
PLANNED ONLY and do not execute before the operator's go. No fix outside this
text: a defect this text does not name stops execution and supersedes the change.

## 1. Preflight — done

- [x] 1.1 Every citation resolves at c81727b9, except one, now corrected in the
      proposal: `:3243` was named as the Variant combo and is a continuation line
      of the `BuildLayoutOptions` call. The Variant combo is `if (hasVariant)` at
      `:3254`, gated by `:3220`, emitting at `:3260`. Verified as written:
      `ControllersPageUI.hpp:893`/`:915` (`kLayoutCustomOptionLabel`), `:898-918`
      (`BuildLayoutOptions`), `:873` (`BuildLaunchpadVariantOptions`), `:972-989`
      (`BuildAddPresetOptions`), `:3242` (the row combo's call site), `:3350` and
      `:3354` (Blacklist emission and enable); `src/MidiConfigViewModel.cpp:831`
      (`hasCompleteEndpointPair`); `src/MidiReconcile.cpp:64-86`;
      `app/FroggersMidiCatalog.hpp:74, 150, 161, 163`; `MANUAL.md:269`.
- [x] 1.2 Finding 2 confirmed by reading `LaunchpadVariantIndex` (`:3867-3891`)
      and `SetLaunchpadVariant` (`:3893-3969`) end to end. The model is derived,
      never stored; the setter rewrites existing associations and writes nothing
      when a row has none. The removal in 2.1 is sound.
- [x] 1.3 §5 table completed per file; it is the table in the proposal.
- [x] 1.4 `NodeIds::ControllerLayout` / `kControllerLayout` / `BuildLayoutOptions`
      / `kLayoutCustomOptionLabel`: 28 references found across 4 files — page 12,
      `juce/ControllersPageSimulationTests.cpp` 7, `tests/controllers_page_ui_tests.cpp` 8,
      `tests/browser_runtime_contract_tests.cpp` 1. All 28 are disposed by 2.3 and
      2.6; found versus changed is reported again in 2.6 against the diff.
- [x] 1.5 The Blacklist wording is NOT page-local. Nine user-visible strings, in
      two files, are enumerated in the proposal's finding 4 table; the persisted
      token and the two corrupt-file validation messages are read and deliberately
      unchanged. This widened 2.5.
- [x] 1.6 Consumers of a row's Launchpad model enumerated by operand
      (`launchpadPosition`, `LaunchpadController`, `LaunchpadGridPosition`,
      `LaunchpadShapeSupports`, `CurrentLaunchpadVariant`). The consumer that
      matters is grid editing: `AddSingle` (`:3425`) and `AddBlock` (`:3633`) both
      derive the model through `CurrentLaunchpadVariant`, which returns Launchpad X
      on a row with no associations (`:3162-3168`). A Launchpad preset shipping no
      mappings would therefore build every pad as a Launchpad X regardless of which
      model was chosen. This is why task 3.1's presets carry a real per-model
      mapping; each association records the model, and the derivation keeps working
      after edits.
- [x] 1.7 Hygiene swept, every Impact directory named, in the proposal's preflight
      record. The eight Sheaf Impact files are clean.
- [x] 1.8 Found in preflight, not planned: `wizardId` fuses provenance with
      pristine state and every edit clears it. Recorded as finding 6; it changes
      the design of Restore and adds task 2.0.

## 2. Sheaf, on branch `app-midi-catalog`

- [x] 2.0 Split provenance from divergence. Remove the four `slot.wizardId.reset()`
      calls in `ApplyMappingEdit` (`:2714`), `DeleteRow` (`:2934`), `AddSingle`
      (`:3529`) and `AddBlock` (`:3734`), and the comment at `:2714` that explains
      the old behaviour. Add one predicate that answers whether a slot's config
      still matches its descriptor's generated profile, by generating through
      `InstallDescriptorProfile` into a scratch slot and comparing
      `ToJSON(arena, config)` output. Compute it once per row while the row VM is
      built, next to `hasResolvedWizard` (`:825-831`), and carry it as a bool on
      the row VM; it is O(n) and read per render, so it is not recomputed at the
      call site. Do not add `operator==` to any struct in `MidiController.hpp`.
- [x] 2.1 Remove the Variant control and its mechanism:
      `BuildLaunchpadVariantOptions`, `LaunchpadVariantIndex`, `SetLaunchpadVariant`,
      `LaunchpadVariantCatalog`, `RewritePresentationLaunchpadVariant`,
      `Actions::kVariantSelect`, `NodeIds::ControllerVariant`, the `hasVariant`
      local at `:3220` and the emission block at `:3254-3266`. Run the inbound half
      first: enumerate every caller and every test naming any of them, with a
      disposition each, zeros included, and report found versus changed.
      `CurrentLaunchpadVariant` STAYS — it is the derivation `AddSingle` and
      `AddBlock` use, and 3.1's presets are what feed it. Of the six
      `LaunchpadShapeSupports` uses, only `:3265` and `:3944` go.
- [x] 2.1b Two items the diff itself created, per §9's re-run of §5 against the diff.
      First: delete the now-dead `CurrentLaunchpadVariant(const SectionPresentation&)`
      overload at `src/MidiConfigViewModel.cpp:3262`. Its only caller was inside
      `LaunchpadVariantIndex`, which 2.1 removes; the four surviving call sites all
      pass `systemMessages` and bind the `:3194` overload. It currently emits
      `-Wunused-function`. Second: make the page simulation's per-step invariant
      wizard-aware — `VerifyTreeAndRenderer`
      (`juce/ControllersPageSimulationTests.cpp:259-278`) requires a controller row
      per controller and `kAddRow` after every action, and neither exists while a
      wizard session is open, which `BuildTree` (`ControllersPageUI.hpp:1033-1037`)
      returns instead of the page. Skip those checks for exactly the steps where the
      wizard form is showing, detected from the tree. Do NOT pin the walk seed, do
      NOT delete the assertions, and do NOT shorten the walk: the checks must still
      run on every step that renders the page.

- [x] 2.1c The row's minimum-width computation still pays for the two deleted
      combos. `kActiveHeaderLine1Width`
      (`include/synth/ControllersPageUI.hpp:531-534`) sums `kLifecycleLayoutWidth`
      (`:507`, 200.0f) and `kVariantFieldWidth` (`:481`, 200.0f) for controls that
      no longer render, and its comment still describes them. That sum feeds
      `kActiveControllerHeaderWidth` -> `kControllerHeaderMinWidth` (`:553`), which
      the page enforces as a real minimum at `:2524`, so the row reserves about
      408px for nothing and the published 740px figure is held up by phantom
      controls. Nothing overflows, so the fits-within fixture cannot see it.
      Remove both constants, correct line one's sum and its comment to the controls
      that actually render (disclosure, name, kind), and RE-DERIVE the resulting
      minimum. `kActiveLifecycleWidth` (`:518`) is already correct — it includes
      `kLifecycleRestoreWidth` — do not touch it.
      Then check `kBlacklistedBadgeWidth` against the badge's new text: it is sized
      for "Blacklisted" and now renders "Released".
      Report the new minimum. Every place that quotes 740 — the Sheaf specs and the
      PR draft at `pr13-description.md` — is updated to the re-derived number, or
      confirmed still correct. Re-derive the fits-within fixture's 700px violation
      count too, since the PR text quotes "29 violations at 700 px".

- [x] 2.2 The APC40 keeps both descriptors, Generic and Ableton, unchanged. They
      are two creation-time templates and nothing about them moves in this change.
- [x] 2.3 Remove the row's preset combo and everything that served only it:
      `BuildLayoutOptions`, `kLayoutCustomOptionLabel`, `HandleControllerLayout`
      (`:2188`), `Actions::kControllerLayout` (`:441`, `:1306`, `:1474`),
      `NodeIds::ControllerLayout` (`:312`) and the emission at `:3239-3253`.
      Nothing replaces it: no dropdown and no readout. Presets are named on the add
      row only.
- [x] 2.4 Add Restore: one action, one node id, one call site. It reinstalls the
      row's own preset through `InstallDescriptorProfile` (`:928`), the existing
      single definition site, rather than a second install path. It is emitted when
      the row has a resolved wizard id AND 2.0's predicate says the config has
      diverged — which is now a reachable state, and was not before 2.0.
      Restore keeps the row intact: `InstallDescriptorProfile` passes
      `{.name, .input, .output}` from the slot into `GenerateProfile` and writes
      back only `kind`, `config` and `wizardId`, so the row's name, bound ports and
      disposition survive. Assert that in 2.6 rather than assuming it.
      Its header comment (`:919-926`) names `HandleControllerLayout` and the add row
      as the only two callers. 2.3 deletes the first and this task adds the second,
      so rewrite the comment to name the add row and Restore.
- [x] 2.5 Blacklist becomes Release and the released row's Remove becomes Reclaim,
      at all nine user-visible strings in the proposal's finding 4 table, across
      `ControllersPageUI.hpp` and `src/MidiConfigViewModel.cpp`. Release is emitted
      when both endpoints are bound: replace `if (rowVm.hasResolvedWizard)` at
      `:3350` with the endpoint-pair condition and drop `blacklist.enabled` at
      `:3354`. The `hasResolvedWizard` gate on Configure at `:3199` stays. Action
      names, node ids and the persisted disposition token are unchanged; prove that
      with a round trip that writes a released record and reads it back.
- [x] 2.6 Tests.
      - The row no longer offers another kind's preset. Assert directly, since it
        is finding 1 and must fail without 2.3.
      - A mapping edit no longer clears `wizardId` — the direct check on 2.0, and
        it must fail without it.
      - Restore returns an edited row to its preset, is absent on an untouched row,
        and is absent on a row never made from a preset.
      - Release is absent on a row with no bound device, present on a bound one,
        and present on a bound row whose mappings have been edited. The last case
        is the `:3350` regression finding 6 names and must fail without 2.5.
      - Configure is present on a blacklisted row that has been edited.
      - A row added from a preset carrying `openSysEx` sends exactly that message
        when its output connects, and one added from a preset without it sends
        none. This is the wiring at `src/MidiController.cpp:3108-3110`; the
        processor itself is already covered by `instrument_tests.cpp:1438` and
        `:1464`, so a failure here is wiring, not the processor.
      - Report found versus changed for the 28 references 1.4 enumerated.
        RESULT: 28 found, 28 changed, 0 remaining — independently reproduced by
        the postflight reviewer, which grepped every one of
        `NodeIds::ControllerLayout`, `kControllerLayout`, `BuildLayoutOptions` and
        `kLayoutCustomOptionLabel` across the tree and found none.
- [x] 2.7 Gates, each built and run by path, `nice make -j2` and never more:
      `portable_ui_tests`, `controllers_page_ui_tests`, `runtime_main_component_tests`,
      `browser_audio_device_tests`, `browser_runtime_contract_tests`,
      `viewmodel_tests`, `instrument_tests`; then
      `nice make -j2 -C apps/miniapp build/controllers_page_simulation_tests` run
      from `projects/synth`. `make test` is not trusted to reach them.
- [x] 2.8 Overflow is measured by the existing fits-within fixture, not by reading
      screenshots. `requireFits` (`tests/portable_ui_tests.cpp:3136-3152`) runs
      `FitsWithinViolations` against `froggersContentBounds` and already carries a
      positive control at 600px (`:3154-3167`) proving the instrument is live —
      keep that control and report its count alongside the results, so a zero
      cannot come from a dead instrument.
      Extend its named states to cover every row state this change creates, one
      `requireFits` call each: a row showing Restore (diverged) and one not showing
      it (pristine); a row showing Release (bound and resolved) and ones not showing
      it (unbound, and bound-but-unresolved); a released row with its badge, Reclaim
      and Configure; and a Launchpad row collapsed and expanded.
      One Launchpad row is sufficient here and the earlier wording asking for all
      three presets was wrong: the three presets live in the frogg3rs catalog,
      which this Sheaf fixture cannot see, and the row header is gated on
      `kind == Launchpad` rather than on the model, so all three render an
      identical header now that the Variant combo is gone. Per-preset coverage
      lives in frogg3rs's own catalog tests. Report the violation count per state, zeros included.
      This runs inside `portable_ui_tests`, already in 2.7's gate list.
- [ ] 2.8a NOT DONE, and not to be done by an agent: the operator withheld permission to
      rebuild or restart the browser app on 2026-09-04, having it already running. The
      headless fits-within fixture in 2.8 owns the overflow evidence and covers every new
      row state, so nothing is unverified by skipping this; the PR text no longer claims a
      browser render for this row. If a visual check is wanted later it is an operator
      step. Original scope, for reference: one browser pass at 1000 x 820 for visual confirmation only — the
      headless fixture above owns the overflow numbers. Screenshot a Twister row and
      a Launchpad row, collapsed and expanded, and a row showing Restore and Release
      together. BEFORE building, copy `app/browser/dist/` aside: `build-browser.sh`
      runs `rm -rf "$OUT_APPS_DIR/$APP_ID"` at `:48` under `set -euo pipefail`, so a
      compile failure after that line leaves the live site deleted. Restore the copy
      if the build fails. Build with `nice` and never more than `-j2`. Install
      nothing: `e2e/node_modules` and the Playwright browser cache are already
      present, and if something is missing, report BLOCKED rather than installing.
      The add row's combo open is an operator check, not this one: Chromium does not
      composite a native select's list into a screenshot.
- [x] 2.9 Sheaf openspec: sru-4, sru-60 and sru-62 rewritten;
      `openspec validate app-midi-catalog --strict`. Every SHALL on a requirement's
      FIRST body line.

## 3. frogg3rs

- [x] 3.1 `app/FroggersMidiCatalog.hpp`: add three Launchpad descriptors of kind
      Launchpad through ONE shared helper taking the model and its programmer-mode
      message, so the three differ only in those two things:
      `froggers.launchpad.x` "Launchpad X" `F0 00 20 29 02 0C 0E 01 F7`;
      `froggers.launchpad.promk3` "Launchpad Pro MK3" `F0 00 20 29 02 0E 00 11 00 00 F7`;
      `froggers.launchpad.minimk3` "Launchpad Mini MK3" `F0 00 20 29 02 0D 0E 01 F7`.
      Every association carries `launchpadPosition` at that descriptor's model, so
      the model is recorded on every pad and survives edits.

      Pads are built by a pad-addressed sibling of `AppActionButton` (`:49-59`):
      same body, setting `launchpadPosition` instead of `control`, with
      `press = synth::MessageIn::AppAction(0, 0, 0.0f)`, the `appAction` and
      `appActionValue` strings, and `outputFeedback = false`. The index in `press`
      stays 0 here: `ResolveAppActionsAgainstCatalog`
      (`External/Sheaf/projects/synth/include/synth/Engine.hpp:1040-1054`) resolves
      `(appAction, appActionValue)` against the running catalog and writes the real
      index into `press.appActionIx`. It branches on `press.type`, not on how the
      association is addressed, so a pad resolves exactly as a CC button does.

      That function ERASES any association whose (action, value) the catalog does
      not declare, so every pair below must exist in `catalog.actions` — all of
      them do, and a test in 3.1a pins that.

      The map, using only positions valid on all three models:

      | position | action | value |
      |---|---|---|
      | `(0,-1)`..`(3,-1)` | `kPlay`, `kStop`, `kFreeze`, `kRecord` | `""` |
      | `(4,-1)`, `(5,-1)` | `kSceneSelect` | `"0"`, `"1"` |
      | `(6,-1)`, `(7,-1)` | `kRandomizePage`, `kResetPage` | `""` |
      | `(8,0)`..`(8,5)` | `kBankSelect` | `"0"`..`"5"` |

      The bank column is `kFroggersBankCount` tall — that constant is 6
      (`app/FroggersParameters.hpp:66`), so the column is `(8,0)` to `(8,5)` and NOT
      eight pads. Derive the extent from the constant rather than writing 6.
      The 8x8 grid is left unmapped: frogg3rs declares no pad-to-parameter action,
      and a preset is a starting point. Do NOT call `LaunchpadDefaultProfileConfig`
      — it emits library `SceneSelect` and `SelectParamBank` messages, and frogg3rs
      drives scenes through its own `froggers.scene.select` app action.
- [x] 3.1a Assert each byte sequence against its preset in
      `app/FroggersMidiCatalogTests.cpp`, so a later edit cannot silently swap two
      models, and assert every association in each Launchpad preset carries that
      preset's model. Also assert every (appAction, appActionValue) pair in the
      three presets resolves against `FroggersMidiCatalog().actions`, since an
      unresolved pair is silently erased at runtime rather than reported.
- [x] 3.2 `MANUAL.md`: MIDI control becomes its own top-level section, and its
      content is rewritten for what this change delivers.

      Structure. `### MIDI controllers` (`:216`) is promoted to `## MIDI
      controllers` — SAME heading text, one level up. Do not reword it: `README.md:109`
      links `[MIDI controllers](MANUAL.md#midi-controllers)`, and the anchor derives
      from the text, so promoting the level keeps it resolving while renaming would
      break it silently. `## Audio and MIDI configuration` (`:176`) becomes
      `## Audio configuration`; Standalone, Plugin and Browser build stay under it.
      The existing bold lead-ins inside the MIDI section become `###` subsections.

      Inbound references to repair, all four enumerated 2026-09-03:
      - `MANUAL.md:8` "see Audio and MIDI configuration, below" — repoint to the
        MIDI controllers section, since it is about controller configuration.
      - `MANUAL.md:14` "in Audio and MIDI configuration" — check which of the two
        sections it means and repoint accordingly.
      - `MANUAL.md:183` "see MIDI controllers, below" — still resolves, confirm.
      - `QUICK_DICT.md:15` "see MANUAL.md's Audio and MIDI configuration section" —
        this one is about EXTERNAL AUDIO INPUT and must keep pointing at the audio
        section under its new name. Do not repoint it at MIDI.

      Content. Rewrite for: one preset per device or mode; no preset control on a
      row; Restore and when it appears; Release and Reclaim replacing Blacklist;
      no Variant control. Then document the three Launchpad presets to the same
      standard the Twister and APC40 entries already set — those name every mapped
      control, and the Twister entry also gives the Midi Fighter Utility settings,
      so a half-documented Launchpad would be the only device a reader cannot set
      up from the manual alone. That standard is operator check 6.10's bar. For the
      Launchpad, give the pad map from 3.1 (top row Play/Stop/Freeze/Record, Scene 1
      and 2, Randomize Page, Reset Page; right column Bank 1 to `kFroggersBankCount`;
      the 8x8 grid unmapped and free to map), say the app switches the unit into
      programmer mode when its output connects, and say the pads stay dark because
      programmer mode hands LED control to the host — the same tell the APC40
      Ableton entry already documents.

- [x] 3.2a `README.md:107-109`: line 108 says presets exist "for the MIDI Fighter
      Twister and the Akai APC40 mkII", which stops being the whole list once the
      three Launchpad presets ship. Update it. Line 109's anchor keeps working if
      3.2's heading text is left alone; confirm it resolves rather than assuming.

## 4. Postflight, before any commit

- [x] 4.1 Fresh-context Sonnet reviewer over the whole diff in both repos, against
      this proposal's Design, its §5 table and the rendered states from 2.8.
      Divergences reported strictly. Commit only when it passes.

## 5. Delivery, only after 4.1 passes

- [x] 5.1 Commit Sheaf; push the fork; PR #13 description updated.
- [x] 5.2 frogg3rs: pin bump as its own commit; `nice make -j2 -C app test`;
      browser build; commit docs and artifacts; push `main`; Pages and VST
      workflows green.

## 6. Operator, then archive

The superseded change's checks carry, since none has been run yet.

- [ ] 6.1 On a Twister row, there is no way to turn it into an APC40.
- [ ] 6.1a Adding a Launchpad is one press: the add row lists the three models by
      name and there is no second menu anywhere asking which one you have.
      ALSO REPORT THE PORT NAMES. Alias matching is exact, case-insensitive
      (`MatchesAnyAlias` -> `CaseInsensitiveEquals`,
      `External/Sheaf/projects/synth/src/ControllerWizard.cpp:227-231`), and the
      aliases shipped in 3.1 are taken from Novation's manuals rather than from a
      unit, because none was reachable. With a Launchpad connected, say exactly how
      its input and output ports are named on the machine. If the row adds with its
      ports reading "(none)", the preset is fine and only the alias list is wrong;
      send the real names and 3.1's alias lists get corrected. This is the one
      thing in this change that reading could not settle.
- [ ] 6.1b With a Launchpad connected: adding it as your model switches the unit
      into programmer mode when the output connects, once and not again on the next
      tick. Add the WRONG model deliberately and it does not switch — the message is
      per device and a mismatch should be visibly inert. Programmer mode hands LED
      control to the host and the preset sets `outputFeedback = false`, so the
      visible proof is the pads going dark on connect and staying dark, the same
      tell the APC40 Ableton mode already documents. This is the `openSysEx` check
      PR #13 has been asking for and the only one runnable on hardware anyone has;
      the APC40 Ableton path stays unconfirmed until a unit is available.
- [ ] 6.1c The Launchpad preset map in 3.1 is the one thing in this change chosen
      rather than forced by a defect, and the operator approved it 2026-09-03:
      Play/Stop/Freeze/Record, two scenes, Randomize Page and Reset Page on the top
      row; banks down the right column; the 8x8 grid left free. Confirm it plays
      the way it reads once hardware is available.
- [ ] 6.2 Edit a mapping, then Restore: the factory mapping comes back. Restore is
      not offered on a row you have not edited, nor on a Custom row that was never
      made from a template. Edit a mapping and set it back by hand: Restore goes
      away on its own.
- [ ] 6.3 The only control that lists devices is the add row.
- [ ] 6.4 With an APC40: a row added from the Generic preset leaves the unit as it
      powers up; one added from the Ableton preset switches it on connect, device
      knobs stay on encoders 9 to 16 whatever track is selected, and the buttons
      stay dark.
- [ ] 6.5 Release a bound controller: it lets the ports go, another application can
      take it, and Reclaim brings it back with its mappings intact. Release is not
      shown on a row with nothing bound, and IS shown on a row whose mappings you
      have edited.
- [ ] 6.6 Rename from inside the editor: the editor stays open on the same row with
      its sections as they were.
- [ ] 6.7 Add a Twister connected and unplugged; press Add without touching the
      selector and the preset it displays is the one added.
- [ ] 6.8 Save a patch, change a mapping, reload: the mapping is back. Relaunch:
      the runtime config's map loads.
- [ ] 6.9 A first visit to the deployed site in a fresh profile: the UI arrives.
      Automation grants autoplay, so nothing in this change speaks to it.
- [ ] 6.10 The manual's MIDI section followed on a Twister and an APC40 at factory
      settings with nothing else to go on.
- [ ] 6.11 Archive on confirmation, with the superseded
      `frogg3rs-controllers-page-name-in-the-editor` and
      `frogg3rs-controllers-page-user-story`.

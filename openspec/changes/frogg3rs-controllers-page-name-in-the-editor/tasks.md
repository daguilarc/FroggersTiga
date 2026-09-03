# Tasks — `frogg3rs-controllers-page-name-in-the-editor`

Task 1 is the preflight, run inline. Task 2 is sequential on one implementer.
Task 3 is parallel per-file subagents. No fix outside this text.

## 1. Preflight, run inline 2026-09-02

- [x] 1.1 Every citation in "The four findings" and "Design" resolves at the
      WORKING TREE: `MidiConfigViewModel.hpp:711` (`PresentationKey` is
      `pair<string, MidiConfigSection>`), `:725` (`expandState_`), `:732`
      (`presentations_`); `src/MidiConfigViewModel.cpp:843-847` (expand state
      read back by name), `:852-872` (the orphan sweep and its stated reason),
      `:892-907` (the presentation orphan sweep and its two-pass reason),
      `:910-912` (`StateFor`); `ControllersPageUI.hpp:1116-1125`
      (`DispatchAction`), `:1250` (`RefreshOnTick`'s `m_vm.Rebuild`),
      `:1801` (the throwaway view model), `:1823-1845` (`HandleRenameController`),
      `:3445-3476` (the Name row emitted only when expanded), `:972-977`
      (`BuildAddPresetOptions`); `src/MidiController.cpp:3325-3332`
      (`MidiProfileKindName`); `juce/ControllersHarnessApp.cpp:30`;
      `tests/browser_runtime_contract_tests.cpp:954, 972-983`;
      `tests/controllers_page_ui_tests.cpp:1594-1602`;
      `juce/ControllersPageSimulationTests.cpp:1178-1186`;
      `Makefile:2, 188, 194, 200, 203, 204, 209`. Two frames were mixed in the
      original text and are corrected: it cited `HandleRenameController` at the
      base commit and `m_vm.Rebuild` at the working tree.
- [x] 1.2 Behavioural premise settled by reading the path end to end, and it
      holds. `HandleRenameController` calls `CommitLifecycleAction`, which
      mutates a THROWAWAY `MidiConfigViewModel` built from the snapshot, then
      calls `Commit`, `RefreshDiscoveryFromCallbacks` and
      `SaveCommittedWizardAction`. None of the three rebuilds `m_vm`: the page's
      only `m_vm.Rebuild` is `RefreshOnTick`'s. The original claim that it "runs
      on a later tick" is WRONG — `DispatchAction` calls `HandleAction` and then
      `RefreshOnTick(false)` in the same call. The hook point is unaffected: it
      sits inside `HandleAction`, before the refresh. Both names are in scope
      there (`identity->second` and `name`). `RenameController` refuses an
      unchanged name and a name already taken, so the re-key cannot collide.
- [x] 1.3 §5 table re-run per FILE over the whole Sheaf repo, at the working
      tree rather than the base commit, since that is what the executor edits.
      Every row restated. New hits the original table did not carry:
      `expandState_` in `docs/superpowers/plans/2026-07-05-rework-controllers-block-editing.md`
      (2, a plan quoting existing code, untouched), and `SetAddPresetDraft`,
      `StateForConst`, `DiscardPresentation`, `BrowserRuntimeAbi` added as rows.
      `NoteControllerRenamed`, `DEPFLAGS`, `-MMD` and `-MP` are zero everywhere.
- [x] 1.4 Include graph recomputed with `c++ -MM`: exactly six sources reach
      `ControllersPageUI.hpp` — `portable_ui_tests.cpp`,
      `runtime_main_component_tests.cpp`, `controllers_page_ui_tests.cpp`,
      `browser_audio_device_tests.cpp`, `browser_runtime_contract_tests.cpp`,
      `browser/cpp/BrowserRuntimeAbi.cpp`. Four targets list it
      (`Makefile:188, 194, 200, 209`); `$(BROWSER_CONTRACT_TEST_BIN)` (`:203`)
      does not.
- [x] 1.5 The depfile mechanism was RUN, not assumed, before being planned.
      `-MMD` with `-o build/NAME` writes `build/NAME.d` targeting `build/NAME`,
      so no `-MT` is needed. A single invocation with two sources writes ONE
      depfile recording only the LAST source — demonstrated both ways
      (`c++ -MMD -MP t.cpp o.cpp -o build/t_bin` → `build/t_bin: o.cpp`;
      sources reversed → `build/t_bin: t.cpp`). `$(BROWSER_CONTRACT_TEST_BIN)`
      is the tree's only multi-source rule, so it must be split. This also
      invalidated the original 2.4 control, which could not have failed.
- [x] 1.6 Hygiene sweep of every Impact directory, both repos.
      `apps/controllers_harness` is invoked by nothing — no CI workflow, no
      script, no Makefile target; its only mentions are July planning docs and
      past verification reports. Task 1.5 of the original text called it "live"
      because its Makefile builds the source, which is a structural fact
      standing in for an invocation trace. §8.0's fork was put to the operator,
      who chose removal. `juce/ControllersPageHarness.hpp` stays: the simulation
      tests and `scripts/check_ui_boundary.sh` use it.
      frogg3rs carries the uncommitted deletion of the layout-contract change's
      directory (archived under the gitignored `openspec/changes/archive/`),
      folded into this change's Impact. 160 planning references in shipping
      comments were counted per file; the operator scoped the sweep to the files
      this change edits in code.
- [x] 1.7 Sheaf's eight active changes match the list in the proposal.
      `rework-controllers-block-editing` plans an `editSessions_` cache keyed by
      a slot-derived key that will need the same rename treatment; recorded as a
      note, not acted on, since the symbol does not exist in the tree.
- [x] 1.8 No finding rejects the change. Eleven corrections applied to the
      artifacts, listed under "What preflight changed".

## 2. Sheaf code, on branch `app-midi-catalog`, sequential

- [x] 2.1 `MidiConfigViewModel::NoteControllerRenamed(const std::string& from,
      const std::string& to)`: move the `expandState_` entry when one exists,
      and re-key every `presentations_` entry whose key's first element is
      `from`. The presentation re-key collects keys in one pass and rewrites
      them in a second, matching the two existing orphan sweeps and for the
      reason they state. Declared beside `StateFor`, non-const, no other
      behaviour changed.
- [x] 2.2 Call it from `HandleRenameController`'s success path
      (`ControllersPageUI.hpp:1823-1845`), inside the `if (CommitLifecycleAction(
      ...))` block, with `identity->second` as `from` and `name` as `to`.
- [x] 2.3 Delete `juce/ControllersHarnessApp.cpp` and `apps/controllers_harness/`
      (`Makefile`, `Info.plist`, `README.md`, and the gitignored `build/`).
      Keep `juce/ControllersPageHarness.hpp`. Then run the inbound half: grep
      the tree for `controllers_harness` and `ControllersHarnessApp` and report
      every hit with a disposition, zeros included. Hits inside
      `docs/superpowers/plans/`, `.superpowers/sdd/`, `analysis/` and
      `data/agents/` are historical records and stay.
- [x] 2.4 `Makefile`: add `DEPFLAGS := -MMD -MP` (a new variable — NOT appended
      to `CXXFLAGS`, which is `?=` and overridable). Apply it to the five
      test-binary rules that reach `include/synth/ControllersPageUI.hpp`
      (`:188, 194, 200, 203, 209`) and to the browser ABI unit. Split
      `$(BROWSER_CONTRACT_TEST_BIN)` (`:203-204`): compile
      `browser/cpp/BrowserRuntimeAbi.cpp` to `$(BUILD_DIR)/BrowserRuntimeAbi.o`
      with its own depfile, and link the test against that object, because one
      invocation with two sources writes a single depfile recording only the
      last source (task 1.5). Write depfiles in `$(BUILD_DIR)` and `-include`
      them. Delete the hand-written header prerequisites those rules no longer
      need.

      Positive control, two legs, both numbers reported:
      (a) `touch include/synth/ControllersPageUI.hpp`, then
      `make build/browser_runtime_contract_tests` REBUILDS it, where before the
      change the same command reported it up to date.
      (b) Pick a header the TEST unit reaches and the ABI unit does not —
      compute the two sets with `c++ -MM` on
      `tests/browser_runtime_contract_tests.cpp` and
      `browser/cpp/BrowserRuntimeAbi.cpp` and name the header you chose. Touch
      it and confirm a rebuild. Leg (b) is what leg (a) cannot test: the ABI
      unit reaches the page header too, so leg (a) passes even under the broken
      single-depfile version.
      A control that shows a rebuild both before and after means the instrument
      is dead — say so and re-run.
- [x] 2.5 Tests.
      `tests/browser_runtime_contract_tests.cpp`: assert the rename nodes are
      absent while controller 2 is collapsed, dispatch `kToggleConfig` with
      value `"2"`, then run the existing rename block (`:972-983`).
      `tests/viewmodel_tests.cpp`: renaming an expanded row keeps it expanded
      and keeps its section presentation; renaming a collapsed row leaves it
      collapsed; a same-name re-add after a delete still starts fully collapsed
      (the behaviour `Rebuild`'s orphan sweep exists for).
      `tests/controllers_page_ui_tests.cpp`: delete the
      `ToggleConfig`-after-rename workaround at `:1602` and the comment at
      `:1594-1601` that explains it, and require the editor to still be open
      after the rename.
      `juce/ControllersPageSimulationTests.cpp`: there is NO workaround to
      delete here — the simulation renames at `:1183` and deletes at `:1186`
      without re-expanding. Add, between those two lines, an assertion that
      `ControllerRenameDraft(0)` and `ControllerRename(0)` still exist after the
      rename commit.
      The `controllers_page_ui_tests` and simulation assertions are the
      regression check for finding 1 and must fail without 2.1: run them once
      with 2.1 reverted and report the failure text.
- [x] 2.6 Gates, each built and run by path, `nice make -j2` and never more:
      `portable_ui_tests`, `controllers_page_ui_tests`,
      `runtime_main_component_tests`, `browser_audio_device_tests`,
      `browser_runtime_contract_tests`, `viewmodel_tests`, `instrument_tests`;
      `nice make -j2 miniapp` and its ControllersPage simulation. `make test` is
      not trusted to reach any of them. `apps/controllers_harness` is not a gate
      — it is removed by 2.3.

## 3. Comment hygiene, parallel per-file subagents, after task 2

Remove planning-doc references from comments, test names and assertion strings —
`sru-NN`, `Finding N`, `Task N.N`, `task group N`, `design.md`, `DN`, `brief
finding N`, `reviewer finding N`. Keep every comment's technical content and
drop only the label; where a label carries the only meaning, replace it with
what the code does. Change no code and no assertion logic. Report references
FOUND and references CHANGED per file. Counts at the working tree before task 2:

- [x] 3.1 `tests/viewmodel_tests.cpp` (49)
- [x] 3.2 `src/MidiConfigViewModel.cpp` (27)
- [x] 3.3 `tests/portable_ui_tests.cpp` (23)
- [x] 3.4 `include/synth/MidiConfigViewModel.hpp` (21)
- [x] 3.5 `juce/ControllersPageSimulationTests.cpp` (19)
- [x] 3.6 `tests/controllers_page_ui_tests.cpp` (9)
- [x] 3.7 `include/synth/ControllersPageUI.hpp` (5)
- [x] 3.8 `tests/runtime_main_component_tests.cpp` (4)
- [x] 3.9 `tests/browser_runtime_contract_tests.cpp` (3)
- [x] 3.10 Re-run task 2.6's gates after the sweep, since it touches strings the
      tests print and headers they compile.

## 4. Sheaf openspec and render

- [x] 4.1 Sheaf openspec at the repo root: sru-4, sru-60, sru-61 and sru-62
      rewritten to this design, including the rename-keeps-the-row scenario;
      tasks 1.23-1.26; `openspec validate app-midi-catalog --strict`. Put every
      SHALL/MUST on a requirement's FIRST body line — the validator only finds
      it there.
- [x] 4.2 Render every state the superseded design named, in the browser build
      at 1000 x 820, and record the overflow count per state. Nine states, zero
      overflow in each. Caveat recorded: Chromium does not composite a native
      select's open list into a screenshot, so state 1 captured the Preset combo
      focused rather than open. That one is an operator check.

## 5. Extraction found by the postflight

- [x] 5.1 `ControllersLayout::EmitStatusDot` extracted from the row's per-port
      dot and the legend's dot, which carried byte-identical draw blocks. One
      `FillEllipse` remains in the page. The pass-through lambda the extraction
      left behind is inlined. Gates re-run.

## 6. Postflight, BEFORE any commit

- [ ] 6.1 Fresh-context Sonnet reviewer: the whole Sheaf diff — the tree state
      this change inherited as well as the work done here, since none of it has
      been committed or independently reviewed — against this proposal's Design
      and §5 table, the spec deltas, and the rendered state list from 4.2.
      Divergences reported strictly. Its first pass REJECTed on three findings,
      all three now fixed; a re-run decides.

## 7. Delivery, only after 6.1 passes

Nothing in this section runs before the postflight passes. The earlier ordering
put the push in task 4 and gated commits on the postflight in task 6, which
contradicted itself; the postflight governs.

- [ ] 7.1 Commit Sheaf on `app-midi-catalog`; push to the fork; PR #13
      description updated.
- [ ] 7.2 frogg3rs: pin bump as its own commit; `nice make -j2 -C app test`;
      browser build.
- [ ] 7.3 frogg3rs: commit `MANUAL.md`, `README.md`, this change's artifacts,
      and the superseded layout-contract change's directory deletion; push
      `main`; Pages and VST workflows green.

## 8. Operator, then archive

- [ ] 8.1 Desktop and deployed site: with a Twister, a Generic and a Launchpad
      row, every control visible in every named state; the add row reads Preset
      with the three presets then the Custom entries; each port's dot sits
      before its combo; the Name row is in the editor.
- [ ] 8.2 Rename a controller from inside its editor: the name changes and the
      editor stays open on the same row, with its open sections still open.
- [ ] 8.3 Add MIDI Fighter Twister with the unit connected: the row's ports bind
      to it and its dots go green; add it again with the unit unplugged: ports
      read "(none)". Press Add without touching the Preset combo: the preset the
      combo displays is the one that gets added.
- [ ] 8.4 Desktop with the Twister: choosing the Twister preset installs it;
      each side button does what the manual says; a push drills in and stops at
      the cap; editing any row flips the combo to Custom.
- [ ] 8.5 Desktop with the APC40 in the Generic preset: each button and knob
      does what the manual says; SHIFT held plus a knob turn drills that knob
      without moving it.
- [ ] 8.6 UNCONFIRMED until this passes. APC40 in the Ableton preset: press any
      Track Select button and turn device knob 1; encoder 9 still moves; buttons
      are dark.
- [ ] 8.7 Save a patch, change a mapping, reload the patch: the mapping is back.
      Relaunch: the runtime config's map loads.
- [ ] 8.8 The manual's MIDI section, followed on a Twister and an APC40 at
      factory settings with nothing else to go on.
- [ ] 8.9 A first visit to the deployed site in a fresh profile or a hard-reloaded
      private window: the UI actually arrives. Automation grants autoplay, so no
      green run in this change speaks to this.
- [ ] 8.10 Archive on confirmation, together with the superseded
      `frogg3rs-controllers-page-user-story`.

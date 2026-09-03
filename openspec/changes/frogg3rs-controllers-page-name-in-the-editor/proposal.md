# Proposal — `frogg3rs-controllers-page-name-in-the-editor`

**Created 2026-09-02. Preflight run and artifacts rewritten 2026-09-02; see
"What preflight changed" below. Supersedes `frogg3rs-controllers-page-user-story`,
which superseded `frogg3rs-controllers-page-layout-contract` (executed the same
day: Sheaf 2c27f35c, frogg3rs 4654525..507a947), which superseded
`frogg3rs-controllers-page-fits-its-host`, which superseded
`frogg3rs-midi-mappings-for-froggers`. Operator tasks 5.x carry into task 5
here.**

Sheaf paths are under `External/Sheaf/projects/synth/` unless the text says
otherwise; the Sheaf openspec is at the Sheaf repo root, NOT under
`projects/synth/`. Branch `app-midi-catalog`, base commit 2c27f35c, upstream
PR #13.

**Line numbers are WORKING-TREE reads at 2026-09-02, not base-commit reads.**
Five files under `External/Sheaf` are modified relative to 2c27f35c, and the
executor edits the working tree, so every citation here resolves in the tree as
it stands. Where a number matters and the base differs, both are given.

## Starting point: what is already in the working tree

The superseded change executed its tasks 2.1-2.4 before it was stopped. That
work is uncommitted and is this change's starting point, documented here rather
than silently adopted. In `External/Sheaf`, five files are modified:
`include/synth/ControllersPageUI.hpp` and the four test files
`tests/controllers_page_ui_tests.cpp`, `tests/portable_ui_tests.cpp`,
`tests/runtime_main_component_tests.cpp`,
`juce/ControllersPageSimulationTests.cpp`. It delivers the whole design the
superseded change specified — the Preset add row, the per-port status dots on
line two, the Name row inside the expanded editor, `InstallDescriptorProfile`
single-sourcing the descriptor install, `EffectiveAddPresetId` single-sourcing
the add row's default, `kStatusDotWidth` shared with the legend, and
`kControllerHeaderMinWidth` at 740 — and its five named gates pass.

That design is not reopened here. This change carries it forward unchanged and
adds what stopping revealed: one product defect and three enumeration defects,
all four found by re-running the superseded change's own §5 enumeration at FILE
granularity instead of per-directory counts.

## The four findings

**1. Renaming a controller collapses its row, so the Name field closes the
editor it now lives in.** `MidiConfigViewModel` keys per-row expand state by
controller NAME (`expandState_`, `MidiConfigViewModel.hpp:725`; `StateFor`,
`src/MidiConfigViewModel.cpp:910-912`; read back in `Rebuild` at `:843-847`),
and `Rebuild` erases every entry whose name is no longer present in the
instrument (`:852-872`) so that a same-name re-add starts collapsed. A rename
therefore presents `Rebuild` with a name it has never seen expanded: the row
comes back collapsed. `presentations_` is keyed the same way
(`std::pair<std::string, MidiConfigSection>`, `MidiConfigViewModel.hpp:711,732`;
orphan sweep at `src/MidiConfigViewModel.cpp:895-907`), so a rename also throws
away the row's section presentation. Two name-keyed caches, one rename.

The rebuild is synchronous, not deferred: `ControllersPageSurface::DispatchAction`
calls `HandleAction(action)` and then `RefreshOnTick(/*respectFocusGuard=*/false)`
in the same call (`ControllersPageUI.hpp:1116-1125`), and `RefreshOnTick`'s
`m_vm.Rebuild` is at `:1250`. So the collapse lands before the dispatch that
caused it returns.

The collapse predates every change in this chain. What is new is that the Name
field is now reachable only with the editor open, so the collapse is the field's
own outcome: open the editor, type a name, press Rename, and the editor shuts.
The behaviour is observed, not inferred — the superseded change's executor had
to insert `surface.ViewModel().ToggleConfig(0)` after the rename in
`tests/controllers_page_ui_tests.cpp:1602`, and wrote the mechanism into the
comment at `:1594-1601` beside it. A test that re-expands what the product
collapses records the defect; it does not fix it.

**2. A non-test source calls a method the tree has already deleted.**
`juce/ControllersHarnessApp.cpp:30` calls `surface_.SetAddControllerDraft(name,
"generic")` and then dispatches `Actions::kAddController`; its "Add Generic"
button is exactly the add row's Custom (Generic) path. `SetAddControllerDraft`
was removed with the name-and-kind add row and now survives only at this one
call site (`git grep -c SetAddControllerDraft` over the tree: 1, this file).
Evidence: `nice make -j2 -C apps/controllers_harness` fails with
`error: no member named 'SetAddControllerDraft' in
'synth::runtime_ui::ControllersPageSurface'`, and
`apps/controllers_harness/build/` holds today's JUCE module objects with no
`ControllersHarnessApp.o` and no `ControllersHarness.app` — the compile died
exactly there. The symbol appeared in no §5 table; the file and the
`apps/controllers_harness` target appeared in no Impact or gate list.

**3. A fifth test file drives the rename ids from the header.**
`tests/browser_runtime_contract_tests.cpp:972-983` dispatches
`ControllerRenameDraft(2)` and `ControllerRename(2)` and asserts the draft text
survives a browser frame. Those nodes are now inside controller 2's collapsed
editor: the page emits the Name row only after `if (rowVm.disposition ==
Blacklisted || !rowVm.configExpanded) continue;` (`ControllersPageUI.hpp:3445-3449`,
the row emitted at `:3452-3476`). Evidence: freshly built, the binary aborts —
`browser lifecycle node has a portable action`, the `Require` at `:954`, because
`FindNode` returned null. The superseded change's §5 row counted
`ControllerRenameDraft` at 6 test hits and matched the tree exactly; the 6 are 4
in `controllers_page_ui_tests.cpp` and 2 here, and a per-directory count cannot
see that.

**4. The gate that catches finding 3 does not rebuild when the page changes.**
`$(BROWSER_CONTRACT_TEST_BIN)`'s prerequisite list (`Makefile:203`) does not
name `include/synth/ControllersPageUI.hpp`, though the test reaches it through
`RuntimeMainComponent.hpp`. Evidence: `make` reported the binary up to date
after the header changed, and the stale binary passed; after `rm`, it rebuilt
and aborted. Six sources include the header transitively
(`portable_ui_tests.cpp`, `runtime_main_component_tests.cpp`,
`controllers_page_ui_tests.cpp`, `browser_audio_device_tests.cpp`,
`browser_runtime_contract_tests.cpp`, `browser/cpp/BrowserRuntimeAbi.cpp`,
computed with `c++ -MM`); four targets list it (`Makefile:188,194,200,209`) and
this one does not. A hand-mirrored prerequisite list is a family that must stay
in sync with the real include graph, and §5 says such a family gets a check that
fails on drift, not a one-time correction.

## Design

**Finding 1 — the rename keeps the row.** After a rename commits, the two
name-keyed caches move with the name instead of being orphaned. Add
`void MidiConfigViewModel::NoteControllerRenamed(const std::string& from,
const std::string& to)`: it moves `expandState_`'s entry from `from` to `to`
when one exists, and re-keys every `presentations_` entry whose key's first
element is `from`. The presentation re-key collects its keys in one pass and
rewrites them in a second, the way the two existing orphan sweeps do and for
the same reason they say: erasing a `std::map` element while a range-for is
iterating that element is undefined behaviour (`src/MidiConfigViewModel.cpp:892-894`).

It is called by `ControllersPageUI`'s rename handler (`HandleRenameController`,
`ControllersPageUI.hpp:1823-1845`; base commit `:1752-1772`) on the success path
only, after `CommitLifecycleAction` returns true, so `Rebuild`'s orphan sweep
sees the entries already under the new name and erases nothing. The ordering is
verified, not assumed: `CommitLifecycleAction` mutates a throwaway
`MidiConfigViewModel` (`:1801`), then calls `Commit`, `RefreshDiscoveryFromCallbacks`
and `SaveCommittedWizardAction`, none of which rebuilds `m_vm` — the page's ONLY
`m_vm.Rebuild` is `RefreshOnTick`'s at `:1250`, which `DispatchAction` reaches
after `HandleAction` has returned (`:1116-1125`). Both names are in scope in the
handler: the old one as `identity->second` from the lifecycle token, the new one
as `name`. `RenameController` refuses an unchanged name and refuses a name
already taken (`src/MidiConfigViewModel.cpp`, `RenameController`), so the re-key
never collides with a live entry. Renaming into a collapsed row leaves it
collapsed; nothing else about `Rebuild` changes, and a same-name re-add still
starts fully collapsed because no rename moved anything to that name.
`RenameController` stays `const` and untouched: it derives `out` and does not
own the caches.

The two caches are re-keyed in one function because they are one concept —
per-row UI state keyed by a name the user can change. Splitting them would put
the same rename in two places.

**Finding 2 — the harness app is removed, not repaired.** §8.0's fork was traced
rather than assumed. Nothing invokes `apps/controllers_harness`: no CI workflow,
no script, no Makefile target. Its only mentions are July planning docs under
`docs/superpowers/plans/` and past verification reports under `.superpowers/sdd/`,
which are historical records. Its one unique story — looking at the Controllers
page drawn by the real JUCE widget backend without launching the synth — has no
holder: the operator's acceptance path (tasks 5.1-5.8) is the real Frogg3rs
desktop app, and the surface-API compile check the harness would give is already
given by `juce/ControllersPageSimulationTests.cpp`, which is in the gate list.
So `juce/ControllersHarnessApp.cpp` and `apps/controllers_harness/` go.
`juce/ControllersPageHarness.hpp` STAYS — it is the fixture, and
`ControllersPageSimulationTests.cpp` and `scripts/check_ui_boundary.sh` use it.
`apps/controllers_harness` therefore does not join the gate list.

**Finding 3 — the browser contract test.** Its rename block expands
controller 2 first (`Actions::kToggleConfig`, value `"2"`, the same dispatch
the page's disclosure sends) so the nodes exist, and asserts they are absent
before the expand. That makes the test state what the page now does rather than
work around it.

**Finding 4 — the check that fails on drift.** The five test-binary rules and
the browser ABI translation unit get their dependency lists generated instead of
written: compile with `-MMD -MP`, write the depfiles beside the binaries in
`$(BUILD_DIR)`, and `-include` them. That removes the hand-mirrored list rather
than correcting one entry, so no future header can be reached without being
depended on.

Two mechanics were settled by running the compiler rather than assuming:

- `-MMD` with `-o build/NAME` writes `build/NAME.d` whose target line reads
  `build/NAME:`, matching the make target. No `-MT` is needed.
- **A single compiler invocation with two source files writes ONE depfile, and
  it records only the LAST source.** Run both ways: `c++ -MMD -MP t.cpp o.cpp -o
  build/t_bin` produced `build/t_bin: o.cpp`, and with the sources reversed,
  `build/t_bin: t.cpp`. `$(BROWSER_CONTRACT_TEST_BIN)` is the tree's only
  multi-source rule — it compiles `tests/browser_runtime_contract_tests.cpp` and
  `browser/cpp/BrowserRuntimeAbi.cpp` in one command (`Makefile:204`) — so
  `-MMD` alone there would silently record the ABI unit's headers and drop the
  test unit's. That rule is therefore split: `browser/cpp/BrowserRuntimeAbi.cpp`
  compiles to `$(BUILD_DIR)/BrowserRuntimeAbi.o` with its own depfile, and the
  test links against that object.

`-MMD -MP` goes in a new `DEPFLAGS := -MMD -MP` used explicitly on those six
compiles. It does NOT go into `CXXFLAGS`, which is declared `?=` (`Makefile:2`)
and so is replaceable from the environment.

The positive control (task 2.4) runs on two legs, because one leg cannot fail:
`BrowserRuntimeAbi.cpp` reaches `ControllersPageUI.hpp` too, so touching that
header would rebuild the binary even under the broken single-depfile version.
The second leg touches a header the TEST unit reaches and the ABI unit does not.

Scope limit: the depfile change covers the test-binary and ABI rules that this
change's header actually reaches. The `$(OBJ)` library rules keep their explicit
lists; converting those is a separate change and is named here so it is not
mistaken for an omission.

**Hygiene — planning references in shipping comments.** §8.0's sweep of this
change's Impact found 160 planning-doc references in comments, test names and
assertion strings across the files this change edits in code — `sru-NN`,
`Finding N`, `Task N.N`, `design.md`, `DN`, `reviewer finding N`. They describe
history the reader never saw rather than what the code does. They are removed in
this change, keeping each comment's technical content and dropping only the
label. Counted per file at the working tree:
`tests/viewmodel_tests.cpp` 49, `src/MidiConfigViewModel.cpp` 27,
`tests/portable_ui_tests.cpp` 23, `include/synth/MidiConfigViewModel.hpp` 21,
`juce/ControllersPageSimulationTests.cpp` 19,
`tests/controllers_page_ui_tests.cpp` 9,
`include/synth/ControllersPageUI.hpp` 5,
`tests/runtime_main_component_tests.cpp` 4,
`tests/browser_runtime_contract_tests.cpp` 3, `Makefile` 0. Files this change
does not edit in code are out of scope and get their own change.

## §5 forward enumeration, per FILE

Counts are `git grep -c` over the Sheaf repo at the WORKING TREE, one line per
file, because the superseded change's per-directory counts matched the tree
exactly and still hid findings 2 and 3. Every concept the change touches or
creates is listed, zeros included.

| concept | files at the working tree | disposition |
|---|---|---|
| `kAddName`, `kAddNameDraft`, `kAddKind`, `kAddKindDraft` | 0 everywhere | removed; done in the tree |
| `BuildAddControllerKindOptions`, `KindFromAddOptionId` | 0 everywhere | removed; done in the tree |
| `SetAddControllerDraft` | **juce/ControllersHarnessApp.cpp 1** | the declaration is already gone; the last call site goes with the file (finding 2) |
| `SetAddPresetDraft` | page 2 | the add row's draft setter, kept; its only remaining caller is the page's own action handler once the harness is removed |
| `ControllerStatusDots` | 0 everywhere | replaced by the two per-port ids; done in the tree |
| `ControllerRenameDraft` | page 6; juce sim 1; controllers_page 5; **browser_runtime_contract 2** | ids kept, nodes moved; the browser contract call site is finding 3, fixed here |
| `ControllerRename(` | page 2; juce sim 2; controllers_page 3; **browser_runtime_contract 1** | as above |
| `kStatusDotsWidth`, `kStatusLegendDotWidth` | 0 everywhere | replaced by `kStatusDotWidth`; done in the tree |
| `kLifecycleDraftWidth`, `kLifecycleRenameWidth` | page 5, 5 | kept for the editor's Name row; done in the tree |
| `AvailableControllerName` | page 3 | reused by the add path; done in the tree |
| `kAddController` (action) | page 5; juce harness 1; juce sim 2; controllers_page 1 | unchanged; the harness hit goes with the removed file |
| `expandState_` | viewmodel hpp 1, cpp 5; **docs plan 2** | read and re-keyed by the new `NoteControllerRenamed`; no second expand-state store. The two doc hits are a July plan quoting existing `ToggleSection` code: not code, untouched |
| `presentations_` | viewmodel hpp 1, cpp 11 | re-keyed by the same function; not a second rename path |
| `PresentationKey` | viewmodel hpp 2, cpp 6 | its `first` element is the name the re-key rewrites |
| `StateFor` / `StateForConst` | hpp 1 each, cpp 2 each | the only expand-state accessors; `NoteControllerRenamed` joins them rather than adding a third store |
| `DiscardPresentation` | hpp 1, cpp 3 | the existing single-entry presentation drop; the re-key does not use it and does not duplicate it |
| `NoteControllerRenamed` (new) | 0 everywhere | created; one caller, the page's rename success path |
| `DEPFLAGS`, `-MMD`, `-MP` | 0 everywhere | created; no existing depfile machinery to collide with |
| `BrowserRuntimeAbi` | Makefile 2 (both on the one multi-source rule) | that rule splits; the ABI unit gets its own object and depfile |

`analysis/sdd-model-analysis/data/timelines/*.md` and `data/agents/task-analyzer.*`
hits on `SetAddControllerDraft`, `kStatusDotsWidth`, `kAddController` and
`ControllersHarness` are archived agent transcripts, not code: untouched.

## Gates, derived from the include graph rather than named by hand

`portable_ui_tests`, `controllers_page_ui_tests`, `runtime_main_component_tests`,
`browser_audio_device_tests`, `browser_runtime_contract_tests` (the six sources
`c++ -MM` reports as reaching the page header, less the ABI object which those
targets compile), plus `instrument_tests`, `viewmodel_tests` (it owns the expand
state's own tests), and the miniapp JUCE simulation. Each is run by path after
building; `make test` is not trusted to reach them.
`apps/controllers_harness` is NOT a gate: it is removed by finding 2.

## What preflight changed

Preflight ran inline on 2026-09-02 and did not reject the change. It corrected
the artifacts on eleven points, recorded here so the diff can be read against a
text that matches the tree:

1. Line numbers were mixed frames — `HandleRenameController` cited at the base
   commit (`:1752`) and `m_vm.Rebuild` at the working tree (`:1234`). All
   citations are now working-tree reads; `HandleRenameController` is at `:1807`.
2. "`Rebuild` runs on a later tick" was false. `DispatchAction` calls
   `RefreshOnTick` synchronously (`:1100-1109`). The hook point still holds.
3. The single-depfile-per-invocation defect (finding 4's design) was found by
   running the compiler; without splitting the browser contract rule the check
   would have recorded the wrong translation unit and still passed its control.
4. The 2.4 positive control needed a second leg, for the same reason.
5. `-MMD -MP` was headed for `CXXFLAGS`, which is `?=` and overridable.
6. Task 2.5 told the executor to delete a `ToggleConfig`-after-rename workaround
   in `juce/ControllersPageSimulationTests.cpp`. There is none — the simulation
   renames at `:1183` and deletes at `:1186` without re-expanding. That file
   gets a new assertion instead of a deletion.
7. §8.0's fork on the harness app was named and traced; the operator chose
   removal over repair.
8. The §5 table was stated at the base commit while the executor edits the
   working tree; it is restated at the working tree.
9. The `presentations_` re-key needs two passes; task 2.1 did not say so.
10. `"custom.generic"` was verified as a real preset id (`BuildAddPresetOptions`,
    `ControllersPageUI.hpp:972-977`, over `MidiProfileKindName`,
    `src/MidiController.cpp:3325-3332`) before the harness decision made it moot.
11. The comment-hygiene sweep was enumerated per file and scoped to the files
    this change edits in code.

## Found during execution

Two things surfaced after the artifacts were rewritten. Both are recorded here
rather than left in a subagent's report.

**A blacklisted row can no longer be renamed, and that is now a decision.**
Moving the Name row into the expanded editor removed the rename draft and the
Rename button from blacklisted rows, which have no editor. The superseded
change's executor did it deliberately and wrote the reason into
`kBlacklistedLifecycleWidth`'s comment, but no proposal in this chain named it,
and the previous `sru-4` required blacklisted records to expose Rename. The
capability is gone: rename a record after removing it from the blacklist. Put
to the operator during execution and accepted, so `sru-4` now states it.
`kBlacklistedLifecycleWidth` drops from draft + Rename + Configure + Remove to
Configure + Remove, which is part of why the header minimum falls to 740.

**One spec sentence asserted the opposite of the mechanism.** The first draft of
the depfile scenario said `BrowserRuntimeAbi.cpp` "never reaches the header".
It does — that is precisely why leg (a) of the positive control could not fail
on its own, and why leg (b) exists. Corrected in place: both translation units
reach `ControllersPageUI.hpp`, and the split is needed because one invocation
over two sources writes a single depfile recording only the last of them.

**The same false claim sat in two files and only one was fixed.** The
"never reaches the header" error was corrected in the Sheaf spec delta and left
standing in that change's `tasks.md`, where the postflight found it. §5 says to
close out every hit in the same breath; correcting one instance of a wrong
sentence without grepping for its siblings is the same defect as de-duplicating
one call site and leaving the other. Both now read the same, and the grep that
should have run the first time is in the record.

**A status-dot draw block is duplicated, and this change's §5 table missed it.**
`constexpr float kDotSize = 8.0f` with its centred-ellipse bounds and
`EndpointStatusColor(status)` appears twice: inside the `emitPortDot` lambda
(`ControllersPageUI.hpp:3268-3272` before the fix) and in the legend (`:3435-3442`). The base
commit had three such sites — two adjacent inline draws in the combined
`ControllerStatusDots` node (`:3159-3165`) plus the legend (`:3367`) — so the
inherited work reduced three to two rather than introducing a copy. The pairing
with the legend predates this chain. It is still a §5 violation, and §4's 2-of-4
is met (reused twice; prevents repetition of structurally similar code). The
table missed it because it enumerated by symbol name — `kStatusDotsWidth`,
`kStatusLegendDotWidth`, `ControllerStatusDots` — where §5 says to search by
OPERANDS: grepping `FillEllipse` or `EndpointStatusColor` surfaces all three
sites at once.

## Other active changes

- Sheaf `app-midi-catalog` (PR #13): this change's commit lands on it. Its spec
  deltas sru-4, sru-60, sru-61 and sru-62 are rewritten here.
- Sheaf `rework-controllers-block-editing`: names `ControllersPageUI.hpp` and
  `projects/synth/src` in its own Impact but owns the scroll viewport and
  row-identity model, not the header or the add row. It does not touch
  `MidiConfigViewModel`'s expand state today. It DOES plan an `editSessions_`
  cache keyed by a slot-derived key (`docs/superpowers/plans/2026-07-05-rework-controllers-block-editing.md:211-218`),
  which is a third name-derived cache and will need the same rename treatment
  `NoteControllerRenamed` gives the other two. `editSessions_` does not exist in
  the tree yet, so nothing is done about it here beyond this note.
- Sheaf `bank-addressed-absolute-write`, `fix-out-of-tree-app-gaps`,
  `ui-state-before-audio`, `browser-slider-value-readout`,
  `shorten-deadline-readout-window`, `fix-task-analyzer-plan-derived-tasks`:
  none edits the page, the wizard registry, the add path or the view model's
  caches. Nothing.
- frogg3rs `frogg3rs-guitar-and-solo-variants`: `src/`, `test/firmware/`,
  `DAISY_MANUAL.md`, uncommitted, excluded.

## Impact

- Sheaf `projects/synth/`: `include/synth/ControllersPageUI.hpp`,
  `include/synth/MidiConfigViewModel.hpp`, `src/MidiConfigViewModel.cpp`,
  `Makefile`; `tests/controllers_page_ui_tests.cpp`,
  `tests/portable_ui_tests.cpp`, `tests/runtime_main_component_tests.cpp`,
  `tests/browser_runtime_contract_tests.cpp`, `tests/viewmodel_tests.cpp`;
  `juce/ControllersPageSimulationTests.cpp`; and, as the inbound half of the
  harness deletion, `docs/coverage.md` and `scripts/check_ui_boundary.sh`.
  Deleted: `juce/ControllersHarnessApp.cpp` and `apps/controllers_harness/`.
- Sheaf repo root: `openspec/changes/app-midi-catalog/` — proposal paragraph,
  tasks, and sru-4, sru-60, sru-61, sru-62.
- frogg3rs: pin bump; `MANUAL.md` lines 219-224 and 227-230; `README.md` lines
  108-109; this change's artifacts; and the uncommitted deletion of
  `openspec/changes/frogg3rs-controllers-page-layout-contract/` (executed and
  moved into the gitignored `archive/`), committed here rather than riding along
  in an unrelated commit.

## Delivery

One Sheaf commit on `app-midi-catalog` after the postflight, pushed to the fork;
PR #13 description updated. frogg3rs: pin bump as its own commit, then docs and
artifacts, push `main`, Pages and VST workflows green. Builds `nice make -j2`,
never more.

## Dispatch

Preflight ran inline (a large change: coverage, not independence). Code changes
sequential on one implementer. The comment sweep runs after the code changes, as
parallel per-file subagents — the files are independent and the work is
mechanical. Implementers on Sonnet, the per-file sweepers on Haiku; the
postflight reviewer on Sonnet with fresh context. No fixes outside this text: a
defect this text does not name stops execution and supersedes the change.

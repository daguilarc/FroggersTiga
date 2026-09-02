# Proposal — `frogg3rs-controllers-page-layout-contract`

**Created 2026-09-02. Preflighted the same day by a fresh-context
subagent, and again by the lead before execution (four corrections: the
spec delta cited a per-state Playwright assertion this change does not
add; task 2.2 counted three `playwright test` invocations where the
workflow has two; task 2.1 lacked the fixture's commit callback, analog
catalog and action value formats; the rendered pass is carried into the
postflight because no page-rendering input moves in tasks 2-3). First
preflight: the tree table matches the diff file by file, every listed test
passes, the citations are exact; three corrections applied (the active
line-one sum is 776, the Playwright spec runs in no CI, the Sheaf
active-change list was incomplete). Supersedes `frogg3rs-controllers-page-fits-its-host`
(archived as `2026-09-02-frogg3rs-controllers-page-fits-its-host`), which
itself superseded `frogg3rs-midi-mappings-for-froggers`. Execution go
given 2026-09-02; tasks 2-4 execute in order, operator tasks 5.x stay
open.**

All Sheaf paths are under `External/Sheaf/projects/synth/` on branch
`app-midi-catalog` (last commit 02895a84, upstream PR #13). frogg3rs paths
are under `app/`. Line numbers are 2026-09-02 reads.

## Why a second supersession

The superseded change was preflighted and executed task by task, and its
design was wrong in three places the render then exposed. Its design
section said "Whether either backend clips a combo's text to its box is
not settled by reading; task 2.4 settles it by rendering", which was
right; but it did not say what happens when the render finds more, and
the lead treated the findings as bounded follow-ups instead of stopping.
Three fixes were dispatched outside the proposal's text:

1. The browser backend sized a combo's `<select>` and a text field's
   `<input>` to their text, not to their node's box
   (`browser/src/ui.ts:120` sets the wrapper's width; `:204-205` create
   the children with no size). The rename field ran under the Rename
   button and the Launchpad row's Layout combo ran over the "Variant"
   caption. Fixed in `browser/public/synth-browser.css` (a `width: 100%`,
   `height: 100%`, `overflow: hidden`, `text-overflow: ellipsis` rule for
   both elements).
2. The Layout combo's box beside its caption was 67 px, too narrow for
   "MIDI Fighter Twister". `kLifecycleLayoutWidth` 140 → 200.
3. The status legend placed its three dots by character-advance
   arithmetic over one label; the render put one dot at the left edge,
   one over "offline", one past the words. Rebuilt as three in-flow
   dot-and-word pairs.

The omni rule's system model (execution runs only the approved proposal;
an incomplete prompt stops execution) was broken by the lead, not by an
executor. This change records the tree as it now stands, states what is
verified and how, closes the remaining gaps as tasks, and adds the rule
the previous change lacked: a page change's proposal is written AFTER the
page is rendered in every state at the app's width, and a defect found
during execution supersedes the change instead of being patched.

## What the operator saw, and what each thing was

The deployed page with one Twister row read `> MIDI Fighter Twiste
twister · · Input [Midi Fighter Twister (offline)] Output [Midi Fighter
Twister (offl…] [MIDI Fighter Twister] [R`, with "Refused: name is
unchanged" below and an add row captioned "Kind".

| what it showed | what it was | where |
|---|---|---|
| "MIDI Fighter Twiste" | the controller's name in a 120 px label | `ControllersPageUI.hpp`, `kControllerNameWidth` |
| "twister" | the profile kind's JSON token used as a label | `MidiProfileKindName`, `src/MidiController.cpp:3325-3331` |
| two dots | MIDI in and MIDI out port status, no legend | `EndpointStatusColor` |
| "Input" / "Output" | the controller's MIDI in and MIDI out ports; the app sends encoder-ring feedback, Launchpad colours and connect-time SysEx on the out port | smi-1, `MidiControllerSlot::input/output` (`MidiController.hpp:970-971`) |
| third "MIDI Fighter Twister" box | the rename draft, a text field prefilled with the name, styled like a combo | `renameDraftFor(rowVm.name)` |
| "R" under the sidebar | the Rename button; Delete, Reconfigure/Layout, Blacklist were past the window | row width 1240 against 892 of content |
| "Refused: name is unchanged" | Rename pressed with the unchanged draft | `Actions::kControllerRename` |
| "Kind" | the add row's device selector caption | `:3297` at the pin |

## What went wrong in the page, traced (unchanged from the superseded change, verified by its preflight)

- The page is 900 px wide on every host and cannot grow:
  `config.uiWidth = 900` (`app/FroggersAppCore.hpp:222`; miniapp and
  braid-4 compile the same), set once into each page's content bounds at
  construction (`include/synth/RuntimeMainComponent.hpp:71-75`);
  `SetContentExtent` stores the live extent and calls nothing; the
  browser shell scales the fixed root down (`browser/src/ui.ts:420-431`);
  the 96 px sidebar (`RuntimePages.hpp:404`) paints over anything past
  900 (`RuntimeMainComponent.hpp:216`).
- The active controller row's fixed widths summed to 1240 (1194 before
  the Layout combo; 1448 with a Launchpad's Variant) against 892
  (`ControllersPageUI.hpp:469-527` at the pin). `kEndpointFieldWidth =
  220` since 663392be (2026-08-01).
- No test could see it: `scrollWidth = max(contentWidth,
  kControllerHeaderMinWidth)` laid rows out at their own minimum, so
  every node was inside its parent and `ContainmentViolations` passed;
  nothing related the tree to `m_contentBounds`; the JUCE simulation
  drives nodes by id.
- Captions are sibling labels inside the control's own width
  (`PortableUI.hpp:229-234`, `PortableUIBuilders.hpp:435-475`); with
  text-sized overlays the "Output" caption read on top of the Input
  combo's text.

## The tree as it stands (uncommitted on `app-midi-catalog`)

`git -C External/Sheaf diff --stat`: 13 files, 660 insertions, 205
deletions, plus the untracked `openspec/changes/app-midi-catalog/specs/
synth-portable-runtime-shell/`. By file, what each edit is and what
verifies it:

| file | edit | verified by |
|---|---|---|
| `include/synth/ControllersPageUI.hpp` (+470/−) | two-line controller header (identity line: disclosure 24, name 200, device 100, dots 32, Layout 200, Variant 200 for a Launchpad; ports line: MIDI in 220, MIDI out 220, "Rename to" field 160, Rename 72, Delete 66, Blacklist 78; blacklisted rows likewise); `kControllerHeaderLineHeight = 36`, header 72; header width constants as the max of the two lines (active line one 776, line two 840; blacklisted 890; minimum 890); captions "MIDI in"/"MIDI out"/"Rename to"/"Device"; device label from `MidiProfileKindDisplayName`; legend row of three dot-and-word pairs; `kHeaderControlsX` removed | `portable_ui_tests` (fits-within fixture, criteria, control count 63), `controllers_page_ui_tests`, the JUCE simulation |
| `include/synth/MidiController.hpp`, `src/MidiController.cpp` | `MidiProfileKindDisplayName(kind)`: WRLD.Bldr, MF Twister, Launchpad, Generic; JSON token unchanged | `instrument_tests` `KindDisplayNameCoversEveryKind` |
| `browser/public/synth-browser.css` | select and text input fill their node box; select text clips | `browser/tests/ui-backend.spec.ts` select-fills-wrapper assertion (Playwright; NOT run by any CI today: Sheaf's `synth-browser-pages.yml` runs two other spec files by name, and frogg3rs's Pages e2e runs `app/browser/e2e/`) |
| `browser/tests/ui-backend.spec.ts`, `tests/instrument_tests.cpp` | the assertion above; `KindDisplayNameCoversEveryKind` | themselves |
| `tests/support/VisualCriteria.hpp` | `FitsWithinViolations(tree, bounds)` | used by the fixture |
| `tests/portable_ui_tests.cpp` | `TestControllersRowFitsWithinFroggersNarrowestHost`: 900 × 620, Twister + Generic + Launchpad + Blacklisted, three app layouts, long device names; caption-exception list and out-of-flow list updated | passes: zero violations (30 before the row change) |
| `tests/controllers_page_ui_tests.cpp` | row kind Section, row height 72, display-name label test, caption tests, legend test | passes |
| `juce/ControllersPageSimulationTests.cpp` | out-of-flow list | passes |
| `openspec/changes/app-midi-catalog/` | proposal paragraph, tasks 1.16-1.20, `synth-runtime-ui` delta sru-61/sru-62, new `synth-portable-runtime-shell` delta sprs-18 | `openspec validate --strict` valid |

Rendered 2026-09-02 on the locally packaged browser build (buildId
02a87702…) at a 1000 × 820 viewport, content scaled 0.964, with a Twister,
a Generic and a Launchpad row: collapsed rows; each row expanded; the
Encoders section with a Turn row; the System Messages section with a
row (Addr combo, Ch, CC, Message combo of 24 targets, Arg, ×); the
Analogs section with an App actions row (Ch, CC, Target "BPM", ×) and the
Gestures group; the add row. In every state the number of elements whose
right edge passed the content edge (891 px in the viewport) was 0; the
overlays measured equal to their boxes (Layout select 123 = wrapper 123,
port selects 135/127, rename input 61); the legend read "● online ●
offline ● not set" with each dot before its word. The desktop host was
not rendered: the JUCE backend sizes every component to its node bounds
(`juce/PortableJuceBackend.hpp:966, :991`, `HostLocalBounds` `:790-814`),
and the fits-within fixture is host-independent, so the desktop row is
covered by reading and by the fixture, and the operator's desktop check
is task 5.1.

## Remaining gaps, and the design that closes them

1. **The fixture covers collapsed rows only.** The states rendered above
   (expanded rows, each section open, one row per group) have no
   fits-within assertion. Task 2.1 extends the fixture: for the Generic
   row, open Encoders and add a Turn and a Push row; open System
   Messages and add a row; open Analogs and add a Gesture and an App
   action row; for the Launchpad, open System Messages; assert zero
   violations in each state, and assert the Message combo of the
   Generic system row offers the app catalog's 24 targets in the fixture
   whose callbacks carry the frogg3rs-shaped catalog.
2. **The rendered check has no automated form, and the Playwright
   assertion runs nowhere.** The browser pane the lead drives is not
   reachable from `make`, and `browser/tests/ui-backend.spec.ts` renders a
   hand-authored command-buffer frame, not the real page; no workflow runs
   it. Two things close this: the lead's browser pass at 1000 × 820 over
   the state list of item 1, recorded in the postflight report with the
   overflow count per state before the PR is pushed (task 4.1); and task
   2.2, which adds `ui-backend.spec.ts` to the spec list Sheaf's
   `synth-browser-pages.yml` runs, so upstream CI at least runs the
   select-fills-wrapper assertion. A per-state overflow assertion over
   the real page in Playwright would need the pattern `midi-flow.spec.ts`
   uses (a compiled app driven through the DOM) and is not in this change;
   the per-state fits-within fixture of item 1 is the check that covers
   every state, in C++, in `make test`.
3. **PR #13 and the app.** Task 2.3 pushes the branch and updates the PR
   description with the two-line row, the display names, the captions,
   the overlay sizing and the fits-within criterion, in the library's
   terms. Task 3.1: frogg3rs pin bump; `MANUAL.md`'s MIDI controllers
   section says "MIDI in", "MIDI out", "Device"; `README.md` gains a short
   MIDI controllers paragraph pointing to the manual.
4. **Postflight before commit.** Task 4.1 is an independent fresh-context
   review of the WHOLE Sheaf diff against this proposal's table and the
   two spec deltas, with the rendered state list, before any commit.

## Process rule this change adds to the frogg3rs rulings

Recorded 2026-09-02 in the operator's persistent rulings (the memory entry
"defects found mid-execution: supersede, don't patch"), which every
session on this tree loads.

A change that touches a page is proposed only after the page has been
rendered in every reachable state at the app's compiled width, and the
proposal's design names each state. A defect found during execution that
the proposal does not name stops execution; the lead supersedes the
change with the new evidence, dispatches a fresh preflight, and waits for
the go. No bounded fixes outside the proposal.

## Other active changes, enumerated

- Sheaf `app-midi-catalog` (PR #13): this change's commits land on it.
- Sheaf `rework-controllers-block-editing` (upstream): owns the block
  rows' editing model; the section rows are rendered here but their
  editing logic is untouched.
- Sheaf `bank-addressed-absolute-write`, `fix-out-of-tree-app-gaps`,
  `ui-state-before-audio`, `browser-slider-value-readout`,
  `shorten-deadline-readout-window`, `fix-task-analyzer-plan-derived-tasks`:
  none edits `ControllersPageUI.hpp` or `MidiController.{hpp,cpp}` as a
  target (`fix-out-of-tree-app-gaps` cites the page as prior art;
  `bank-addressed-absolute-write` touches `MessageTypeName`). Nothing.
- frogg3rs `frogg3rs-guitar-and-solo-variants`: `src/`, `test/firmware/`,
  `DAISY_MANUAL.md`, uncommitted, excluded.

## Impact

- Sheaf: the files in the table above; `tests/portable_ui_tests.cpp`
  (task 2.1), `browser/tests/ui-backend.spec.ts` (task 2.2),
  `openspec/changes/app-midi-catalog/` (PR text source).
- frogg3rs: pin bump, `MANUAL.md`, `README.md`, this change's artifacts.

## Delivery

Commits on Sheaf `app-midi-catalog` after the postflight: one commit for
the page (the table above plus tasks 2.1-2.2), pushed to the fork so PR
#13 carries it; PR description updated. frogg3rs: pin bump as its own
commit, then the docs commit, push `main`, Pages and VST workflows green.
Builds `nice make -j2`; gates through the gate script.

## Dispatch

Sequential. Preflight on a fresh-context Sonnet subagent. Implementers on
Sonnet; the postflight reviewer on Sonnet with fresh context, handed the
rendered state list. No parallel agents. No fixes outside this text.

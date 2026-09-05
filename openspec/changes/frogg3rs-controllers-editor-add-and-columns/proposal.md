# Proposal — `frogg3rs-controllers-editor-add-and-columns`

**Created 2026-09-04. Rewritten 2026-09-04 after preflight, which overturned
the change's central claim.**

Sheaf paths are under `External/Sheaf/projects/synth/`. Branch `app-midi-catalog`,
base commit 448dee5d (frogg3rs `main` b0c7c5f). Every number below is from
`evidence.md` beside this file, or from a cited read. Everything in `evidence.md`
sections C, D and E is a run on this machine with a positive control beside it.

## Why this change exists

Every Add and Block button in the Controllers page's Encoders editor refuses in
the shipping browser build. The operator found it in minutes of first use. The
entire test suite is green on the host, and the tests that name this defect
exist and have existed all along — they are simply never built for the target
the product ships to.

## The findings

**1. `kMaxBlockDomain` truncates to zero in the browser build, and EVERY block
add in the page refuses — not only the Encoders editor's.**

`src/MidiConfigBlocks.cpp:233` declares

    constexpr std::size_t kMaxBlockDomain = 9007199254740992ULL;  // 2^53

`StartPlusCountExceedsDomain` (`:238-243`) returns true when
`start > kMaxBlockDomain || count > kMaxBlockDomain`, else when
`kMaxBlockDomain - start < count`. The browser target is wasm32 — no `MEMORY64`
or `-sWASM64` flag exists in `app/browser/build-browser.sh` or
`browser/Makefile` — so `std::size_t` is 32 bits there. 2^53 is an exact
multiple of 2^32, so the constant truncates to exactly **0** and the guard reads
`start > 0 || count > 0`. Nearly every block operation refuses.

Run on both targets from one source file (`evidence.md` C): host `sizeof=8`,
constant 2^53, every case false; wasm32 `sizeof=4`, constant **0**,
`exceeds(0,2)` and `exceeds(0,16)` **true**, `exceeds(0,0)` false. That last
case is the control that identifies the cause rather than fitting it: it
predicts a plain Add succeeds on an EMPTY row (nothing to re-expand) and refuses
on a populated one, which is the asymmetry the operator observed before it was
run.

The guard has THREE callers, not one: `ExpandEncoderBlock`
(`src/MidiConfigBlocks.cpp:286`), `ExpandAnalogBlock` (`:320`) and
`ExpandSystemBlock` (`:415`). All three are reached from the one dispatcher,
`MidiConfigViewModel::AddBlock` (`src/MidiConfigViewModel.cpp:3508-3701`), whose
Encoders, Analogs and SystemMessages branches all pass through it. Only the Grid
branch escapes, because `ExpandGridBlock` (`:629`) uses a separate,
correctly-typed `std::uint64_t` guard (`:639`).

The cap was chosen as JavaScript's safe-integer bound. It is written in a type
whose width on that very platform destroys it.

**2. The Encoders group header has a zero gap, and one label overruns its box
by 0.3px.**

Re-measured live at the preflight (`evidence.md` B), correcting the first
draft's numbers in both directions:

- The header row is built with a literal `0.0f` gap
  (`include/synth/ControllersPageUI.hpp:2906-2909`, in the `emitGroupHeader`
  lambda at `:2891-2952`), applied uniformly between every child. The last
  column's right edge and the Add button's left edge are both 443.4. Zero.
- `FieldEditorWidth` allocates 58px for `BlockStartPos` (`:592`). "Start Pos"
  renders at 58.3px at the label's own `600 13px system-ui`, so it is
  ellipsised — in both the Turn and Push headers. The other four columns have
  4px to 41px of slack; the first draft's claim that every column was clipped
  was wrong.

The two together are what the operator saw: an ellipsised label running
straight into a button with nothing between them.

**3. The suite already catches finding 1. Nothing ever builds it for the
target.** This reverses what the first draft of this proposal claimed, and it
is the finding that matters most.

Sheaf's own test binaries, unmodified, built from the same sources for wasm32
and run under node (`evidence.md` D):

| binary | host | wasm32, before the fix |
|---|---|---|
| `blocks_tests` | 102 pass / 0 fail | **79 pass / 23 fail** |
| `viewmodel_tests` | 0 fail | **121 pass / 26 fail** |

All 49 failures are the three `Expand*Block` functions returning false. Every
`ExpandGridBlock` test passes on both targets, independently confirming the Grid
path is unaffected. One `viewmodel_tests` failure quotes the operator's exact
refusal string. Each build takes about 15 seconds.

So the coverage hole is not a missing test. The gap is a missing *target*.

Two subsidiary findings, both measured, both of which change what this change
can honestly deliver:

- **A compile-time assertion would not have caught finding 1.** The first draft
  proposed a `static_assert` as the guard. Compiled for wasm32,
  `constexpr std::size_t k = 9007199254740992ULL;` produces no diagnostic under
  `-Wall -Wextra -Wpedantic` or even `-Wconversion`. The positive control is the
  list-initialised form, which is a hard error naming the value. A wasm32
  *compile* is not a guard against this class; a wasm32 *run* is.
- **The existing text-fit criterion would not have caught finding 2**, and
  pointing it at this state as the first draft proposed would produce a green
  check on a live defect. `scrollWidth` and `clientWidth` are integers and both
  read 58, so a 0.3px overflow rounds away (`evidence.md` B2, measured
  `scrollWidth > clientWidth + TOLERANCE = false`). Its state coverage is a real
  gap too — `openSurface()` visits page roots only — but coverage alone is not
  enough here; the measurement has to get finer at the same time.

A gap criterion does exist and is configured to permit finding 2's other half:
`SpacingConformance` runs against the Controllers page
(`juce/ControllersPageSimulationTests.cpp:247`) but `ControllersPageSpacing()`
(`:196-211`) lists `0.0f` among the legitimate values, so it cannot tell "flush
by design" from "flush by defect".

## Design

**Finding 1 — delete the cap; use the type's own range.**

The guard's real requirement is that `start + count` not overflow `std::size_t`.
The 2^53 figure expresses a separate wish about JavaScript's safe-integer range,
and the tree already applies that wish correctly, elsewhere and in a type that
holds it: `IsNonNegativeInteger` (`src/MidiConfigViewModel.cpp:1815-1820`)
carries 2^53 as a `double` and explicitly clamps it with
`std::min(kMaxSafeInteger, maxSizeT)`, with a comment naming the 32-bit case.
`kMaxBlockDomain`'s own comment (`:229-232`) concedes it adds nothing over that:
"any block field wide enough to reach this cap could not have been entered
through the numeric field editor anyway".

So the cap is not retyped, it is removed, and the guard becomes the idiom this
very file already uses for the same job 400 lines away — `ExpandGridBlock`'s
`width > std::numeric_limits<std::size_t>::max() / height` (`:639`), and
`IsWrapSafeSuccessor`'s `prev == std::numeric_limits<std::size_t>::max()`
(`:251`). The same idiom appears at `src/ButtonGrid.cpp:31` and
`src/ParameterModulation.cpp:443` and `:1597`. Introducing a second spelling
beside five existing ones would be the duplication §5 forbids.

What this widens, stated rather than left implicit: on a 64-bit host the old
cap refused any `start + count` above 2^53, and the new guard refuses only true
wraparound near `SIZE_MAX`. Values in that vacated band are unreachable through
the UI, whose own field editor clamps to `min(2^53, SIZE_MAX)`, but they are
reachable by constructing a block directly, as a test or a non-UI caller might.
No test covers the vacated band: `blocks_tests`' three near-`SIZE_MAX` refusal
cases all use `start = max() - 1, count = 4`, which still refuses.

This design was validated before any shipping file was touched: applied to a
scratch copy of `MidiConfigBlocks.cpp` and nothing else, `blocks_tests` goes
79/23 to **102/0** on wasm32 and `viewmodel_tests` 121/26 to **147/0**, both
exactly matching their host counts. That the counts match exactly also
establishes that all 49 failures were this one guard, with no second
width-dependent defect behind them.

**Finding 2 — widen the one column that overruns, and give the row a gap.**

`FieldEditorWidth`'s 58 is one value shared by twenty-two fields, so it is not
edited for one label. `BlockStartPos` gets its own case sized to its header
text. The header row's `0.0f` gap becomes the library's standard gap. Both
resulting numbers are reported and their effect on
`kControllerHeaderMinWidth` (`:548-549`, currently 724) confirmed rather than
assumed.

**Finding 3 — build the existing suite for the target that ships.**

The deliverable is not a new test. It is a gate that builds `blocks_tests` and
`viewmodel_tests` for wasm32 and runs them under node, in Sheaf's own
`Makefile`, inside `make test`. It goes red on this defect today, which is the
§6.1 control, and it needs no new test to do it. Emscripten is already vendored
and already required to ship this product at all.

It fails loudly when `emcc` is absent rather than skipping. A gate that skips
silently is how a guard ends up pointed at nothing, which is the shape of the
hole this change exists to close.

For finding 2, the existing text-fit criterion gains both the missing state and
a sub-pixel measurement. Measured across four surfaces, the finer measurement
flags exactly the two "Start Pos" labels and nothing else (`evidence.md` B2), so
tightening it does not light up unrelated pages.

The Controllers spacing allowance is NOT narrowed, and the reason is a
measurement: removing `0.0f` produces 84 violations of the form "gap 0.00
between X and its leading edge" — container padding, legitimately zero — against
only 5 sibling-gap pairs, all deliberate. `SpacingConformance` checks padding
and sibling gaps against one permitted set, so it cannot express "zero padding
yes, zero gap between a label and a button no" without splitting that set, and
even split the page still needs zero for those 5 pairs. That is a wider change
than this one, with its own Impact across every page using the criterion. What
covers the defect instead is the browser criterion above and a direct assertion
in `tests/controllers_page_ui_tests.cpp` that the group header separates its
last column from the Add button by the named gap.

## Questions the preflight settled

- **Does the 2^53 intent need to survive?** No. Its genuine consumer is the
  numeric field editor's double-domain check, where it already lives, correctly
  typed (`src/MidiConfigViewModel.cpp:1816`).
- **Can any part of the suite run for wasm32 today?** Yes, in about 15 seconds
  per binary, with the vendored emsdk at `.emsdk/` in the frogg3rs root and node
  v26. Proven by doing it.
- **Where can a Controllers page test using frogg3rs's real catalog live?** In
  frogg3rs `app/`, with no build change. `app/Makefile` already passes
  `-I$(SHEAF_SYNTH_DIR)/include` and links `libsynth.a`, which carries
  `MidiConfigViewModel::AddBlock` and `ExpandEncoderBlock`;
  `ControllersPageUI.hpp` is header-only. Proven by compiling and linking a
  probe that built the real catalog and its wizard registry: 6 device defaults,
  6 registry entries. The first draft's premise that frogg3rs "does not
  currently link the page" was wrong.

## Impact

- Sheaf `projects/synth/`: `src/MidiConfigBlocks.cpp`; `Makefile` (the wasm32
  gate); `include/synth/ControllersPageUI.hpp` (the `BlockStartPos` width and
  the group header's gap); `browser/tests/visual-criteria.spec.ts` (state
  coverage and sub-pixel measurement); `tests/controllers_page_ui_tests.cpp`
  (the group header's gap assertion).
  `juce/ControllersPageSimulationTests.cpp` was in an earlier Impact for a
  spacing-allowance change that the measurement above ruled out; it is not
  touched.
- frogg3rs: `app/` (the real-catalog page test and its Makefile target); this
  change's artifacts.

`tests/portable_ui_tests.cpp` was in the first draft's Impact and is dropped:
nothing in this design touches it. `tests/blocks_tests.cpp` and
`tests/viewmodel_tests.cpp` are read and built but not edited — the point of
finding 3 is that they already say the right thing.

## Overlap with other active changes

Enumerated across all five other open changes. No other change touches
`src/MidiConfigBlocks.cpp` or `browser/tests/visual-criteria.spec.ts` at all.

Three changes name `include/synth/ControllersPageUI.hpp` —
`frogg3rs-controllers-page-row-controls`,
`frogg3rs-controllers-page-name-in-the-editor` and
`frogg3rs-controllers-page-user-story` — but every region they cite is the
**per-controller-row** header (the disclosure/name/kind and ports/lifecycle
lines, and `kControllerHeaderMinWidth`). This change edits `emitGroupHeader`
(`:2891-2952`) and one `FieldEditorWidth` case (`:592`), which none of them
touch. No line conflict; the sequencing concern is confined to
`kControllerHeaderMinWidth`, which this change confirms rather than moves.

`frogg3rs-controllers-page-user-story` is superseded and dead pending archive
(its own `proposal.md:313`). `frogg3rs-drilled-in-randomize-floor` and
`frogg3rs-guitar-and-solo-variants` are disjoint subsystems.

Two bookkeeping corrections, neither of which this change acts on:

- The first draft said `row-controls` "reads 8/42 tasks complete". It reads
  **27/42**; the unticked remainder is its operator checks plus one deliberately
  withheld browser rebuild. Its code is delivered at Sheaf 448dee5d.
- `frogg3rs-guitar-and-solo-variants` is delivered in the frogg3rs working tree
  but **uncommitted** — it accounts for every modified and untracked file under
  `src/` and `test/firmware/` in `git status`. This change must not sweep those
  into its own commit.

## Delivery

Committed and pushed on both repos, then checked by the operator on the live
URL.

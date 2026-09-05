# Measured evidence — 2026-09-04

Two collections, both measured, neither inferred:

- **A/B** — the lead, by DOM inspection against the deployed site
  (https://daguilarc.github.io/frogg3rs/, Pages deployment of frogg3rs b0c7c5f,
  Sheaf 448dee5d).
- **C/D/E** — the preflight, by compiling and running the library for the
  browser's own target on this machine. Every table below is a run, and every
  run has a positive control beside it.

## A. `Add` on a stock Twister row refuses

State: Controllers page, one row added from the "MIDI Fighter Twister" preset,
editor expanded, Encoders section open, nothing else touched.

Pressing the Turn group's Add button
(`runtime.controllers.row.0.section.0.body.header.0.add_single`) sets the page
status to, verbatim:

    Refused: encoder block start position is too large for its cell count

The button is not inert and not obscured: `disabled=false`,
`aria-disabled=null`, `pointerEvents=auto`, `zIndex=1`, and
`document.elementFromPoint` at the button's own centre returns the button
itself. It receives the press and refuses. That is what "the buttons do
nothing" is.

The status line persists after the refusal, which is why the message appeared
to belong to whichever field was edited next.

That string is emitted at `src/MidiConfigBlocks.cpp:287`.

## B. The Encoders group header: a zero gap, and one label 0.3px over

Re-measured live at the preflight, driving the deployed page itself: Controllers
page, one row added from the "MIDI Fighter Twister" preset, row expanded,
Encoders section open. An earlier draft of this section reported 56px boxes
around 58px text for every column. Both numbers were wrong; the corrected
measurement is below and it is smaller and more specific.

Every column label is allocated **58px** and carries no padding and no border
(`box-sizing: border-box`, `padding: 0`, `border-width: 0`). Rendered text width
is measured with `canvas.measureText` at the label's own computed font,
`600 13px system-ui`:

| node | text | allocated | text needs | over by |
|---|---|---|---|---|
| `header.0.column.0` | Ch | 58 | 17.4 | — |
| `header.0.column.1` | Start CC | 58 | 53.9 | — |
| `header.0.column.2` | End CC | 58 | 46.3 | — |
| `header.0.column.3` | Slot | 58 | 25.0 | — |
| `header.0.column.4` | **Start Pos** | 58 | **58.3** | **0.3** |
| `header.1.column.5` | **Start Pos** | 58 | **58.3** | **0.3** |

So **one** label overflows, in both the Turn and the Push header, by 0.3px, and
it is ellipsised (`overflow: hidden`, `text-overflow: ellipsis`). The other four
have 4px to 41px of slack. 58 is what `FieldEditorWidth` allocates for
`BlockStartPos` (`include/synth/ControllersPageUI.hpp:592`).

The gap is the larger half of the defect, and it is exactly as first reported:

| node | left | right |
|---|---|---|
| `header.0.column.4` | 387.5 | **443.4** |
| `header.0.add_single` | **443.4** | 503.1 |
| `header.0.add_block` | 503.1 | 562.9 |

Zero. The header row is built with a literal `0.0f` gap
(`include/synth/ControllersPageUI.hpp:2906-2909`, inside the `emitGroupHeader`
lambda at `:2891-2952`), applied uniformly between every child. So an ellipsised
"Start Pos" runs directly into the Add button with nothing between them, which
is what reads as the button sitting on top of the label.

The earlier draft's 56 and 60 were screen rects read off a page rendered at a
non-unit scale; the layout values are 58 and 62 (`kAddButtonWidth`, `:480`).

## B2. The existing text-fit criterion cannot see it

This is the finding that changes what the check has to be. Measured on the same
live state, for both overflowing labels:

    scrollWidth = 58   clientWidth = 58   scrollWidth > clientWidth + TOLERANCE = false

`scrollWidth` and `clientWidth` are integers, so a 0.3px overflow rounds away.
The criterion at `browser/tests/visual-criteria.spec.ts:412` and `:766` is
`scrollWidth > clientWidth + TOLERANCE`; pointing it at this state, as this
change first proposed, produces a **green check on a live defect**.

A sub-pixel measurement does see it. Running `canvas.measureText` against
`clientWidth` minus padding, over the same node kinds the existing criterion
uses (`label`, `status-text`), across this state:

| state | text nodes measured | flagged |
|---|---|---|
| Controllers, row expanded, Encoders open | 28 | **2** — both "Start Pos", 0.3px |
| Audio I/O | 16 | 0 |
| Sync | 18 | 0 |
| File | 15 | 0 |

It flags the defect and nothing else, so tightening the measurement does not
light up unrelated surfaces. That is the positive-and-negative control for the
criterion change.

One observation outside this change's scope, recorded rather than acted on: with
the node-kind filter widened to `button`, `runtime.controllers.row.0.rename`
("Rename", 48.8px in 46px) also overflows. It is a `button`, which the existing
criterion's text-bearing kinds exclude, and it belongs to the per-controller-row
header that `frogg3rs-controllers-page-row-controls` owns.

## C. The truncation, run on both targets

`constexpr std::size_t kMaxBlockDomain = 9007199254740992ULL;` and
`StartPlusCountExceedsDomain` copied verbatim from `src/MidiConfigBlocks.cpp`,
compiled twice from one source file:

| | host (clang++) | wasm32 (emcc) |
|---|---|---|
| `sizeof(std::size_t)` | 8 | **4** |
| `kMaxBlockDomain` | 9007199254740992 | **0** |
| `exceeds(0, 2)` — empty Analogs row | false | **true** |
| `exceeds(0, 16)` — Twister row re-expand | false | **true** |
| `exceeds(0, 0)` — empty row, no block to re-expand | false | false |

2^53 is an exact multiple of 2^32, so a 32-bit `std::size_t` holds exactly 0
and the guard reads `start > 0 || count > 0`.

The third row is the control that identifies the cause rather than fitting it:
it predicts that a plain Add succeeds on an EMPTY row (nothing to re-expand,
count 0) and refuses on a populated one (count 16), which is the asymmetry A
observed before this was run.

## D. The existing suite already fails on the browser's target

This is the finding that matters, and it reverses what an earlier draft of this
file claimed. Sheaf's own test binaries, **unmodified**, built from the same
sources for wasm32 and run under node:

| binary | host | wasm32, before the fix | wasm32, after the fix |
|---|---|---|---|
| `blocks_tests` | 102 pass / 0 fail | **79 pass / 23 fail** | 102 pass / 0 fail |
| `viewmodel_tests` | 0 fail | **121 pass / 26 fail** | 147 pass / 0 fail |

All 49 failures are `ExpandEncoderBlock`, `ExpandAnalogBlock` or
`ExpandSystemBlock` returning false. Every `ExpandGridBlock` test passes on both
targets, which independently confirms that the Grid path's own `std::uint64_t`
guard is unaffected. One `viewmodel_tests` failure names A's string exactly:

    [FAIL] EveryEditableFieldOnEveryDefaultProfileRowSucceeds: controller 0
    section 0 row 0 field 0 failed: encoder block start position is too large
    for its cell count

So the coverage hole is not a missing test. The tests exist, they cover these
paths, and they name the defect on sight. Nothing ever builds them for the
target the product ships to. Each build is about 15 seconds.

The "after" column is the proposed fix applied to a scratch copy of
`MidiConfigBlocks.cpp` and nothing else, so the design was validated before any
shipping file was touched. Both binaries reach exactly their host counts, which
also establishes that all 49 failures are this one guard and no second
width-dependent defect is hiding behind them.

## E. A compile-time assertion would not have caught this

Proposed earlier in this change as the guard. It was measured, and it does not
hold:

| declaration, compiled for wasm32 | diagnostic |
|---|---|
| `constexpr std::size_t k = 9007199254740992ULL;` under `-Wall -Wextra -Wpedantic` | **none** |
| the same under `-Wconversion` | **none** |
| `constexpr std::size_t k = {9007199254740992ULL};` (list-initialised) | hard error: "constant expression evaluates to 9007199254740992 which cannot be narrowed to type 'std::size_t'" |

The last row is the positive control: the compiler can see the truncation, and
says nothing about the form the code actually uses. A wasm32 *compile* check is
therefore not a guard against this defect class; a wasm32 *run* is, and D shows
it already works.

## F. Why no existing check sees B

`criteria::SiblingOverlapViolations` is correct to report zero: the boxes abut
exactly, they do not overlap. A width sweep of that state from 600 to 1400 in
steps of 10 reported zero overlaps at all 81 widths, identical at 448dee5d's
parent c81727b9. `FitsWithinViolations` does not see it either — the labels are
inside their boxes and the boxes are inside the section.

For the 0.3px overflow, B2 above is the answer: the criterion exists, it runs,
and its integer measurement rounds the defect away. Extending its state coverage
alone would not have caught this; the measurement has to get finer at the same
time.

For the zero gap, the criterion also exists and is configured to permit it.
`SpacingConformance` measures every sibling gap and does run against the
Controllers page (`juce/ControllersPageSimulationTests.cpp:247`), but
`ControllersPageSpacing()` (`:196-211`) lists `0.0f` among the legitimate
values, so a zero gap cannot be distinguished from a deliberate one.

The Controllers fits-within fixture additionally samples one width only
(`froggersContentBounds`, 900x620); the eight states added by
`frogg3rs-controllers-page-row-controls` inherit that single-sample limit.

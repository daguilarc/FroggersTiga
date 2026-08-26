# Proposal — `frogg3rs-mobile-control-placement`

**Created 2026-08-26.** The published site wastes half its width on a phone,
and the four Randomize/Reset buttons that should live in that space are
stacked full-width below everything instead.

Supersedes `frogg3rs-windows-and-mobile`, whose Windows, logo, touch-gesture
and hygiene work is DELIVERED and must be archived rather than dropped (see
task 0.1). What it got wrong is where mobile controls go, and why.

## What the operator asked for

Verbatim intent, restated so it cannot be read two ways: on a phone there is
a large empty column to the RIGHT of the oscilloscope and the Scene
blend / BPM sliders. If the buttons were made narrower — less space to the
left and right of the text centred in each — all four of Randomize Page,
Randomize All, Reset Page and Reset All would fit comfortably in that empty
column, beside the oscilloscope and sliders.

The controls move SIDEWAYS into unused space. They do not move UP into the
encoder column.

## Why the empty column exists

Traced, not inferred.

- `app/browser/site/mobile-stack.mjs:309` computes ONE scale for all three
  stacked blocks, from the grid block alone:
  `const sharedScale = viewportWidth / measurements[gridIndex].extent.width;`
- The outer split Row's weights are `kLeftBlockWeight = 2.0f` and
  `kRightBlockWeight = 4.0f` (`app/FroggersUiSurface.hpp`, `FroggersCellMap`),
  so the chrome block is 2/6 of the surface and the grid block is 4/6.
- Scaling so the grid block exactly fills the viewport therefore renders the
  chrome block at exactly HALF the viewport width. The empty column is the
  other half.

That column is OUTSIDE the chrome block's own element. Nothing placed inside
the chrome block as it is currently weighted can reach it, which is the
constraint the fix has to satisfy.

## Why the buttons are so wide

`AppendTwoButtonRow` (`app/FroggersUiSurface.hpp:1888`) gives each button
`leftStyle.layout.main = synth::ui::Extent::Weight(2.0f)` and the row
`cross = Weight(1.0f)`. Button width is weight-driven, so each button
stretches to its share of the full block width and the label is centred in
whatever that produces. The padding the operator is describing is not a style
value to reduce — it is the gap between a short label and a button sized by
weight. `cross` already uses `Extent::Intrinsic()`; whether `main` accepts
Intrinsic is UNVERIFIED and task 2.2 checks it before relying on it.

## Why the last change built the wrong thing

This is the part worth reading, because the same failure will recur
otherwise.

The superseded change's `froggers-web-host` delta said:

> The Randomize and Reset rows SHALL be placed ABOVE the grid, **with the
> other transport and scene controls**, rather than below it.

`Transport` and `Scenes` are `LeftKind` rows in the CHROME block
(`FroggersCellMap::kLeftRows`). "With the other transport and scene controls"
means the chrome block. The implementation instead added
`kRightRowsNarrow`, a reordered table in the RIGHT block, putting the rows
above the encoder rows inside the encoder column.

That satisfies the sentence's first clause and drops its second.

It survived review because every scenario tested only the first clause:

> **THEN** the Randomize row's bounding-box top is above the first encoder
> row's bounding-box top

A hoist inside the right block passes that. So did the Playwright assertion
written from it, and so did a C++ surface test written from it. Three green
checks, all measuring the weaker half of the requirement.

The preflight verified that the chrome block stacks above the grid block
(`mobile-stack.mjs:59`) and concluded the placement claim held. The question
it never asked was which BLOCK the rows land in — one grep from the answer,
and the phrase naming the answer was already in the requirement it was
auditing.

**The spec was not wrong. Its scenarios did not test what its prose
required.** A requirement whose scenarios only cover the easy clause is a
requirement that will be implemented to the easy clause.

## What Changes

- **Narrow block weights.** A narrow variant of the outer split weights so
  the chrome block scales to the full viewport width instead of half. Without
  this, no amount of internal rearrangement reaches the empty column.
- **Narrow chrome topology.** In narrow mode the chrome block becomes a Row
  of two Columns: the existing `kLeftRows` stack, and a second column
  carrying Randomize Page, Randomize All, Reset Page and Reset All.
- **Intrinsic button width** for those four, so a short label does not force
  a full-width button.
- **`kRightRowsNarrow` is DELETED.** The right block returns to a single
  table. `narrowViewport_` survives and is consumed by `AppendLeftBlock`
  instead of `AppendRightBlock`.
- **Scenarios that test the intent**, including a negative one that fails if
  the rows land in the encoder column.

## Non-goals

- Changing the desktop or plugin layout. `narrowViewport_` defaults false and
  only the browser host sets it.
- Changing the Windows build, the site logo, or the touch-gesture rule. All
  delivered by the superseded change and archived, not revisited.
- Shell-side DOM rearrangement. The surface owns its own topology; a shell
  that moves emitted controls makes the rendered tree disagree with the
  surface that produced it.

## Impact

- Affected specs: `froggers-web-host` (where mobile controls go, with
  scenarios that test it), `frogg3rs-web-mobile-ux` (the surface-owned
  topology requirement now covers the chrome block, not just the right
  column).
- The phone layout stops wasting half its width, and the four buttons become
  reachable without scrolling past the encoder grid.

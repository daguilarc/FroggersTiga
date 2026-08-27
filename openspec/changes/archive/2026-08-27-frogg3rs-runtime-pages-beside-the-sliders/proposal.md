# Proposal — `frogg3rs-runtime-pages-beside-the-sliders`

**Created 2026-08-26.** Two things, both about Sheaf's runtime sidebar on a
phone: where its buttons sit, and what the first one is called.

## Where they sit

The narrow layout stacks three blocks: the chrome block, the encoder grid, and
Sheaf's own sidebar (Audio / Controllers / Sync / File plus the CPU readout)
below both. That puts four buttons a full page-scroll away from everything else.

There is already room for them. Measured at 390x844 against the current build,
in the surface's own design px:

- the chrome block is 427 wide and 453 tall
- its Randomize/Reset column is 137.5 wide, starting at x 305.5
- that column's four buttons occupy 154 of those 453 px of height
  (4 x 28 plus 3 x 14 gaps), leaving 285 px under them
- Sheaf's sidebar is 96 x 200

So the sidebar fits under the Randomize/Reset buttons, inside the chrome block's
own footprint, with 85 px to spare and without taking a single pixel from the
sliders. Stacking it there instead of below the grid also removes a block from
the stack, which shortens the page.

## Why the shell places it and the surface does not

`runtime.sidebar.root` is emitted by Sheaf's `SidebarSurface`
(`External/Sheaf/projects/synth/include/synth/RuntimePages.hpp:1496-1502`), a
sibling of `froggers.root` under the composite `runtime.main.root`. It is not in
this surface's node tree, so `AppendLeftBlock` cannot contain it. The shell
already positions it — `mobile-stack.mjs`'s `STACK_SELECTORS` lists it as the
third stacked block — so this changes where the shell puts it, not who puts it.
`frogg3rs-web-mobile-ux`'s "the shell must not move emitted controls" clause is
about controls THIS surface emits; the sidebar has always been the shell's to
place, and the requirement is widened to say so rather than left to be read
either way.

The shell should not hardcode where the empty space starts. The surface already
owns the column: `froggers.layout.left.buttons` is emitted by
`AppendNarrowButtonColumn` (`app/FroggersUiSurface.hpp`). It currently declares
`cross = Extent::Weight(1.0f)`, so its box is the block's full height even
though its buttons stop a third of the way down. Declaring `cross =
Extent::Intrinsic()` makes the column's own box end where its last button ends.
Mind which axis that is: the chrome block is a Row, so its CROSS axis is
vertical, and vertical is the column's own MAIN axis. `IntrinsicForNode` sums
children plus gaps only along a container's main axis
(`PortableUILayout.hpp:534-551`), so this resolves to 4 x 28 plus 3 x kGap
exactly because those two axes coincide here. The shell then reads that box live
— the same way it already reads every block's wire extent — instead of computing
the offset from a button id or a hardcoded number.

## What the first button is called

`BuildSidebarTree` hardcodes the label:

    builder.Button(NodeIds::kSidebarAudio, "Audio", ...)   // RuntimePages.hpp:695

"Audio" is also the name of this instrument's first parameter bank, so the
sidebar button and the bank button read as the same thing and are not. The page
behind it selects the output device AND the input device, so it is renamed
**Audio I/O** rather than "Input", which would name half of it.

The rename is a Sheaf change, and a generic one: the gap is that a host app
whose own vocabulary already uses a runtime page's name has no way to rename
that page. `RuntimeConfig` is where an app already declares how the runtime
shell should present it (`appName`, `uiWidth`, `uiHeight` —
`AppContext.hpp:31-40`), and `SidebarSnapshot::registeredPageTitle`
(`RuntimePages.hpp:186-190`) is the existing precedent for an optional,
host-supplied sidebar label that defaults to unset and changes nothing when it
is. This follows both: a `std::optional<std::string> audioPageTitle` on
`RuntimeConfig`, threaded to the sidebar the way `registeredPageTitle` already
is, and `value_or("Audio")` at the one build site.

It lands on the standing PR, `jvictor0/Sheaf#9` ("Fix out-of-tree app gaps"),
whose branch `fix-out-of-tree-app-gaps` is what the submodule already points at.
No frogg3rs special-casing: the field is the app-facing API, and an app that
never sets it gets today's sidebar.

Label width is not a risk. Sidebar buttons take their width from the sidebar,
not from the label (`style.layout.main` is a fixed row height and `cross`
defaults to a fill weight), and "Audio I/O" is nine characters against
"Controllers"' eleven, which already ships in the same column.

## What Changes

- **Sheaf:** `RuntimeConfig::audioPageTitle`, carried through
  `RuntimeMainComponent` into `SidebarSnapshot`, read by `BuildSidebarTree`.
- **frogg3rs:** `FroggersAppCore::Config()` sets it to "Audio I/O". Applies to
  the browser and the standalone alike, which is correct — the bank is called
  Audio in both.
- **The narrow button column** declares an intrinsic cross extent, so its box
  reports where its buttons actually end.
- **The shell** places the sidebar under that column when narrow, and stacks two
  blocks instead of three.
- **Assertions** for the placement, and for the label reaching the page.

## Non-goals

- The wide layout, at any width above 720px. The sidebar keeps its normal place.
- Renaming Controllers, Sync or File. The mechanism is one field for the one
  page that collides; a second collision can add a second field.
- The Audio page's own contents.

## Impact

- Affected specs: `frogg3rs-web-mobile-ux`.
- Affected submodule: `External/Sheaf`, on the branch PR #9 already tracks. The
  pin moves with this change.
- Overlaps `frogg3rs-reverb-wetness-and-damping-floor`, the other active change,
  in ONE file: both edit `app/FroggersAppCore.hpp`, this one near `Config()`
  (~line 195) and that one at `kMaxReverbWetMix` (~line 1827). Different
  regions, so no conflict, but this change lands first by the operator's own
  sequencing and will shift that line number. That change's task 0.2 already
  says to grep by name rather than by line, which is what makes it survive.
- Four runtime-page buttons stop being a page-scroll away from the controls they
  sit beside, and the page gets shorter.

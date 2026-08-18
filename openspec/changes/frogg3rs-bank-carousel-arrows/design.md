# Design — frogg3rs-bank-carousel-arrows

All file:line anchors verified 2026-08-18 at FroggersTiga HEAD (post
`aef79d7`) unless marked UNVERIFIED. Paths repo-root relative.

## Data flow (input → transform → output)

Click on arrow node → `FroggersUiSurface::DispatchAction`
(`app/FroggersUiSurface.hpp:787-792`) → `HandleAction` string-match chain
(`:1682+`, `kBankSelect` branch at `:1858-1861`) → NEW branch computes
`(CurrentBankIndex() ± 1 + kFroggersBankCount) % kFroggersBankCount` →
`app_->RequestBankSelect(ix)` (`app/FroggersAppCore.hpp:557-559`, pending
atomic) → `ProcessFrame` drain applies `activeBankIx_` (`:627-641`, state at
`:2369`) → publish path sets `uiState->banks[ix].selected` → next
`BuildTree()` renders the moved highlight via `style.selected =
BankSelected(bankIx)` (`app/FroggersUiSurface.hpp:1216`, reader at
`:1656-1661`). **The highlight-sync requirement is therefore free**: arrows
inject into the same pipeline as button clicks; no second selection state
exists (spec's single-authority clause holds by construction).

`CurrentBankIndex()` (`:1670-1680`) is the UI-side read the new branch uses —
same source the surface already renders from.

## Placement — inside the header row, no new vertical extent

The band between the bank row and the encoder grid is NOT free space: it is
the modulation-header row, `FroggersCellMap::kRightRows[1]` (`:442`), emitted
by `AppendModulationHeaderRow` (`:1288-1307`), fixed `Px(26)`
(`kModulationHeaderRowHeight`, `:1278`), drawing nothing at drill level 0 and
a full-width centered "Modulation Level N" title when drilled
(`kModulationHeaderTextStyle` is `TextAlign::Center`, `:1284-1286`). Its
comment (`:1270-1277`) binds the contract: the row's space is always
reserved; only its content changes.

- **Structure:** `AppendModulationHeaderRow` becomes a `Row` wrapper carrying
  the existing id `FroggersNodeIds::kModulationHeader`, `main = Px(26)`,
  `cross = Weight(1)` — outer geometry identical in both drill states, so the
  load-bearing geometry test
  (`modulation_header_sits_below_bank_row_and_above_parameter_cells`,
  `app/FroggersSurfaceTests.cpp:878-955`, which finds the node by id and
  asserts bounds) keeps passing. Children by drill level:
  - **Level 0:** `[spacer Weight(1)] [prev arrow Px] [next arrow Px]
    [spacer Weight(1)]` — symmetric weighted spacers center the pair; the
    pair's internal separation uses the row's `gap` (`FroggersPageLayout::kGap
    = 14.0f`, `:259`). Arrows are `Draw` nodes with `ControlStyle.action`,
    the transport-plate idiom (`kTransportPlateSize = 28.0f` square Draw with
    action, `:484`, `:940-941`); size here is bounded by the 26px band —
    `Px(26)` square with the glyph inset, exact inset at implementation.
    Glyphs are left/right triangles via `DrawCommand` fills, colors matched
    to the transport plates' palette at implementation.
  - **Drilled (level > 0):** one full-width child, `Weight(1)`, the EXACT
    existing title Draw (band fill + centered text), under a new inner id
    `kModulationHeaderTitle`; the arrows are not emitted at all — no draw
    commands, no action nodes, so no hit target exists while drilled. This is
    a child-structure switch inside the row, which the row's own contract
    permits (sibling geometry cannot move; the outer Row is fixed Px(26)).
- **Rejected: `overlayOf` overlays.** An overlay resolves to the FULL bounds
  of its in-flow target (`External/Sheaf/projects/synth/include/synth/PortableUILayout.hpp:63-71`,
  collection at `:589-592`) — two full-band overlapping hit targets, no way
  to center a small pair. Rejected before implementation.

## Behavior decisions

- **Wrap-around:** next from bank 5 → 0; previous from 0 → 5 ("toggle
  through the carousel" reads as cyclic; a dead-end at the ends would need a
  disabled visual state this band has no precedent for).
- **Drilled-in stepping: none.** Arrows vanish while drilled (above). Bank
  buttons already define the drilled-in bank-change semantics
  (`clicking_the_active_bank_while_drilled_in_exits_to_the_top_level_grid`,
  `app/FroggersSurfaceTests.cpp:683+`); the arrows do not add a second one.
- **The action branch MUST gate on drill level 0 (preflight finding,
  2026-08-18):** `DispatchAction` → `HandleAction` matches on action NAME
  unconditionally (`:787-792` — no node-presence check), so hiding the arrow
  nodes alone does not satisfy the spec's "SHALL NOT accept input" while
  drilled: a synthetic dispatch would still switch banks (and, per the drain
  at `app/FroggersAppCore.hpp:627-641`, reconstruct `drillIn_` — silently
  exiting the drill). The new branch runs only when `app_->DrillLevel() == 0`
  (the same source `AppendModulationHeaderRow` reads, `:1291`); otherwise it
  returns without effect, and a test pins that.
- **New constants:** `FroggersActions::kBankPrevious/kBankNext`
  (`froggers.bank.previous`/`froggers.bank.next`) beside `kBankSelect`
  (`:213`); `FroggersNodeIds::kBankPrevArrow/kBankNextArrow`
  (`froggers.bank.prev`/`froggers.bank.next` node ids) and
  `kModulationHeaderTitle`, following the `inline constexpr const char*`
  convention (`:107-220`).

## Testing

- Behavior (imitating `bank_buttons_are_button_kind_with_selected_flag...`,
  `FroggersSurfaceTests.cpp:565-612`): dispatch `kBankNext` → exactly one
  selected bank, index +1; wrap 5→0 and 0→5 via `kBankPrevious`; highlight
  invariant (exactly one `selected==true`) after every step.
- Geometry (imitating `modulation_header_sits_below_bank_row_...`,
  `:878-955`, `AbsoluteBounds` at `:255-283`): at level 0 the arrow pair is
  horizontally centered on the band (pair midpoint == band midpoint within
  tolerance), both arrows fully inside the band's bounds; the band's own
  bounds equal the pre-change band's bounds.
- Drilled state: at level > 0 the tree contains no arrow action nodes and the
  title child renders the existing fill+text commands; the outer band bounds
  unchanged.
- Existing tests: re-anchor any test that pins the header node's KIND or its
  draw commands (the geometry test pins bounds by id — unaffected by design).

## Risks

- The header row is a band with a documented history of rejected placements
  (`:842-897`, `:940-954` relocation notes) — mitigated: the operator
  specified this placement themselves (screenshot, 2026-08-18), and the
  arrows vanish exactly when the band's title needs the space.
- Test `bank_buttons_...` counts/asserts node kinds in the bank row only —
  the arrows live in a different row; no interference expected, verified at
  implementation.

## Gate

`cd app && nice make -j2 test` green — baseline 274/274 (a67b3fbf) plus this
change's new tests; NEVER above -j2 on this machine. Operator acceptance in
the built app (`./app/build-launcher.sh`) before archive.

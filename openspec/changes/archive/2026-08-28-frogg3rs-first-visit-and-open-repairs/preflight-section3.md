# Preflight: form-grid control-width plan (Section 3)

**VERDICT: PROCEED WITH AMENDMENTS** — the core mechanism (add `ControlStyle.controlWidth`,
consumed at `FinishControl:466`, read back by `ApplyFormGrid`) is sound and does reach the
right node. But the plan's own line citations for Q2 are stale, and Q1's fix needs to be a
real `min()` clamp, not a bare conditional skip. Amendments: (1) implement Q1's clamp with
an actual `min()`, not "skip if not Weighted"; (2) correct the ColumnAlignment caller lines
in the change description before implementing Q2; (3) add an explicit assertion that
`kAudioInputRetry`'s width is intrinsic-sized (Q4 gap). Repo confirmed clean at
`80d9f4bb` on `fix-out-of-tree-app-gaps`.

## Q1 — the overflow remedy

`ApplyFormGrid` (`PortableUILayout.hpp:813-857`): the first loop (818-835) computes
`labelColumnWidth` = max label width across rows. The second loop (837-856) sets, per row,
`cells[1]->bounds.x = controlOffset` (844, **unconditional**) and
`cells[1]->bounds.width = row->bounds.width - controlOffset - rowOpts.padding` (845,
currently unconditional too).

`cells[1]` is the control node itself, not a wrapper — verified via
`Builder::FinishControl` (`PortableUIBuilders.hpp:438-480`): a captioned control's
`.row` (451-456) is what gets appended to the grid column and *is* the form-grid row
`ApplyFormGrid` iterates; `AppendChild(std::move(node))` (472/476) puts the control itself
as that row's second in-flow child.

Traced the 459.72 figure precisely for `TestFormGridAlignsLabelAndControlColumns`
(`portable_ui_layout_tests.cpp:446-470`, root 400px): `ComboBox` intrinsic width is a fixed
160px leaf (`PortableUIMetrics.hpp:50-51`), `TextWidth` uses `advance=size*0.62`
(`PortableUIMetrics.hpp:26-33`), `kSpacing = {padding:12, gap:8, labelGap:8}`
(`PortableUILayout.hpp:23-27`). `"A considerably longer caption"` (29 chars) →
`29*8.68+16 = 267.72` = `labelColumnWidth`. `controlOffset = 12+267.72+8 = 287.72`.
r1.control's **pre-ApplyFormGrid** width is its raw leaf intrinsic, 160 (from the normal
top-down resolve, unaware of the cross-row label alignment). The previous attempt made
845 conditional on `Weighted` *without a `min()`* — it left an Intrinsic-mode cell's width
untouched at its stale 160 while `844` still moved `x` to the new shared offset:
`287.72 + 160 = 447.72`, `+padding(12) = 459.72` — exact match to the reported failure.

**Does `min(resolvedWidth, remainingWidth)` fix it?** Yes, and it's not test-specific luck
— it's a mathematical guarantee: `edge = controlOffset + min(resolvedWidth, remainingWidth)
<= controlOffset + remainingWidth = row.width - padding = contentEdge`, always. Re-derived
`TestFormGridUsesRowLocalPadding` (472-497) the same way — also never overflows under this
clamp, by the same inequality, regardless of which cell resolvedWidth/remainingWidth wins.
**The bug in "the previous attempt" was skipping the assignment, not that clamping is
unsound** — the plan's Q1 framing ("clamping... actually fixes it") is basically right,
provided the fix literally computes `min(...)`, not "leave as-is."

One caveat: this clamp reads the *cell's own* `.main` mode (Weighted vs Intrinsic) as the
signal. Confirmed by grep that only two production containers use `formGrid=true`
(`RuntimePages.hpp:773`, `:884` — `kSyncForm`, `kAudioForm`), and every control in both is
built via `Field()`/`FormButton()`/`Toggle()` (`RuntimePages.hpp:410-467`), never a bare
uncaptioned control — so the case where clamping shrinks an *uncaptioned* control below a
wide remaining column (which would be wrong for a real selector) does not currently exist
in production; it only shows up in synthetic tests, which don't assert width there.

## Q2 — callers of `criteria::ColumnAlignment`

Grep found exactly **3** call sites, not the lines the operator guessed:
- `tests/portable_ui_layout_tests.cpp:1297` and `:1304` — both inside
  `TestColumnAlignmentCheckCatchesAControlLeavingItsColumn` (`:1281-1309`). (Operator's
  guess of `:1333`/`:1340` is stale — those lines don't contain this call today.)
- `tests/portable_ui_tests.cpp:2604` — inside the per-page/app loop over
  `surface.formGrids`, part of `TestNamedVisualCriteriaHoldOnEveryPageAndApp`.

Per caller, what retiring the equal-width half (`VisualCriteria.hpp:478-484`) stops
catching, and whether shared-x (`:472-477`) still catches it:
- **`:1297` (clean report)**: only asserts `violations.empty()` and row/column counts.
  Retiring width changes nothing observable here.
- **`:1304` (misaligned report)**: see Q3 — the injected fault is x-only, so shared-x still
  catches it and the width half was never load-bearing for this assertion.
- **`:2604` (per-page/app loop)**: asserts `violations.empty()` across every fixture's
  `formGrids`. This is the one that *would* start missing a real defect: a control column
  cell whose `x` is correct but whose `width` diverges from its row-mates (e.g. a future
  regression that resizes one row's control without moving it) would no longer be flagged.
  Shared-x cannot catch a width-only divergence by construction — it doesn't inspect width.

## Q3 — the test that could kill the plan

`TestColumnAlignmentCheckCatchesAControlLeavingItsColumn`
(`portable_ui_layout_tests.cpp:1281-1309`, not `:1317` as guessed — verified by grep, no
other definition exists). The fault injection is line 1303:
```
MutableNode(misaligned, "toggle.2").bounds.x += 4.0f;
```
**Width is untouched.** It misaligns by `x` only. `report.violations.size() == 1`
(1305) and the message must mention `toggle.2`/`toggle.0` (1306-1308) — both satisfied by
the shared-x check alone; width is not involved on either side of this test.

**Conclusion: this test does NOT kill the plan.** Retiring the equal-width check does not
change this test's pass/fail outcome. All three toggles in the fixture use default
`ControlStyle` (only `.caption` set, `controlWidth` stays at its Weight(1.0f) default), so
their widths are equal before *and* after the plan regardless — the width check was never
exercised as the actual mechanism catching this particular fault, only as an unrelated
passenger. The operator's stated risk does not materialize; do not reject on this basis.

## Q4 — coverage of the actual button

`showInputRetry = true` occurs at 3 sites (verified, matches operator's first two, found a
third):
- `tests/portable_ui_tests.cpp:1523` — `TestOfflineInputCaptureOffersACaptionedRetryRow`
  (`:1517-1546`). Builds the real `BuildAudioPageTree`. Asserts (via local helpers
  `ColumnXOffsetsOf`/`ColumnWidthsOf`, `:135-165`, which index `row.children[column]` —
  the control node directly, confirming Q1's cell model): x-offsets equal for column 0 and
  1 (1540, 1542), width equal for column **0 only** (1544). **Column-1 (control) width
  equality is conspicuously not asserted** — consistent with anticipating a narrower
  button — but nothing asserts the button actually *is* narrow/intrinsic-sized either.
- `tests/portable_ui_tests.cpp:1626` — `TestAudioPageWithNoAppSectionIsByteIdenticalToBeforeTheChange`
  (`:1618-1650`). Compares against `ReferenceAudioPageTreeBeforeAppSection` (`:1572-1601`),
  which is a **verbatim copy calling the same `PageControls::FormButton()`**
  (`:1596-1599`) — both sides move together under this plan, so this test cannot detect a
  width regression in `FormButton`; it only guards the unrelated app-section-splicing
  refactor.
- `tests/portable_ui_tests.cpp:1554` — `TestLiveInputCaptureHidesTheRetryRow` sets
  `showInputRetry = false`, not relevant.

None of these three, nor `TestNamedVisualCriteriaHoldOnEveryPageAndApp`
(`:2604`, `criteria::ColumnAlignment`), was confirmed to run with an Audio fixture that has
`showInputRetry = true` — UNVERIFIED whether `surface.formGrids`/the fixture table ever
sets it for that broad test; not traced due to scope, but neither Audio-page test found
here goes through `criteria::ColumnAlignment` at all. **Net: no existing test asserts the
narrowed button's actual geometry (e.g. `retry.bounds.width` ≈ its own label width). Add
one** — amendment (3) above.

## Q5 — existing "don't fill the container" sites

- `RuntimePages.hpp:434-439` (`BackButton`) — verified: `style.layout.cross =
  Extent::Px(Layout::kBackButtonWidth)`. Uses `.cross`, not `.main`, and is placed as a
  direct root child (`kAudioBack`, line 883), never inside a `formGrid` column. Different
  axis, different container — does **not** become a copy of the new concept; orthogonal.
- `ControllersPageUI.hpp:2244-2251` (`buttonStyle` lambda) — verified: `style.layout.main =
  Extent::Px(width)`. Used inside an `actionsRow` with fixed height, part of the
  Controllers wizard pages, which have no `formGrid` container at all (grepped — only
  `kSyncForm`/`kAudioForm` set `formGrid=true`, both in `RuntimePages.hpp`). Pre-existing,
  independent idiom for the same *principle* ("this control has its own width") but not
  the same *mechanism* — it will keep existing side-by-side with `controlWidth` and is not
  retrofitted by this change. Not broken by it, but confirms the codebase already has
  ≥2 unrelated ways to express "don't stretch"; this change adds a third, scoped to
  form-grid + caption only.
- Grep for `.layout.main =`, `.layout.cross =`, and `bounds.width =` across
  `RuntimePages.hpp`/`ControllersPageUI.hpp` turned up ~15 more sites (`RuntimePages.hpp:513,
  514, 536, 546, 557, 569, 687, 707, 1042`; `ControllersPageUI.hpp:2183, 2186, 2191, 2255,
  2365-2393`) — all in Browser/Sidebar/Controllers page code, none inside a `formGrid`
  container. None compete with or duplicate the new concept.

## Q6 — other contradictions found while reading

- The plan's phrase "`FormButton` declares `Extent::Intrinsic()`" is easy to misread as a
  change to `style.layout.main`; it is not — `CompactFormRowLayout()` (`RuntimePages.hpp:
  412-418`) already leaves `.main` at its type default (`Intrinsic()`,
  `PortableUILayout.hpp:54`). The actual, necessary edit is `ControlStyle.controlWidth` on
  the style object `Button()`/`FormButton()` passes, consumed via `FinishControl:466-467`
  into `layoutByNodeId_[controlId]` — same node `ApplyFormGrid` treats as `cells[1]`. This
  was verified to line up correctly; flagging only because the plan's own wording invites
  the wrong edit site.
- The Toggle carve-out (`RuntimePages.hpp:452-457`, `Toggle` currently == `FormButton`) is
  not optional polish — it is verified necessary: the four Sync toggles
  (`:774-793`) and the PPQN `Field` (`:794-798`) sit in the same `kSyncForm` grid; without
  it, all five rows would go identically-Intrinsic per the plan's own account of the
  earlier failure (equal-width violation) — the carve-out is load-bearing for Sync,
  independent of Q3.

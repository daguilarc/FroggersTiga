# Step C report — captioned control width

## STOP: the design as specified breaks two pre-existing, unrelated tests

I implemented the design exactly as written -- new field, both fixed call
sites, `FormButton`'s `Extent::Intrinsic()`, new test assertions -- and
confirmed the fix mechanism works (positive control below). But applying it
literally breaks two tests that predate this change and aren't in the brief:
`TestFormGridAlignsLabelAndControlColumns` (layout tests, no captions
involved) and the "Sync" surface of `TestNamedVisualCriteriaHoldOnEveryPage
AndApp` (app tests, via `PageControls::Toggle`). See \S5. I did not invent a
workaround for either -- that's a design call, not an implementation one.

## 1. The new field

`Extent controlWidth` on `ControlStyle` (`PortableUIBuilders.hpp`), default
`Extent::Weight(1.0f)`.

Not `.cross`: `layout.cross` answers "how wide is the flow slot this control
occupies" for an UNcaptioned control, on the ROW's parent's cross axis
(`PageControls::BackButton`, `RuntimePages.hpp:437`). `controlWidth` answers
"how wide is the CONTROL within its own row," on the row's MAIN axis, and
only exists once `FinishControl` has synthesized a `.row` (caption non-empty).
Reusing `.cross` would make one name answer two questions about two different
nodes. I grepped every `.layout.main =` / `.layout.cross =` site first (\S7);
none address a captioned control's width, so there was nothing to unify with.

## 2. Diff hunks

`PortableUIBuilders.hpp` — new field:
```diff
     LayoutOptions layout{};
+    // The CONTROL node's own main-axis extent inside the implicit caption
+    // row -- distinct from `layout`, which FinishControl spends entirely on
+    // the `.row` wrapper. Named apart from `.cross` on purpose: see header.
+    Extent controlWidth = Extent::Weight(1.0f);
 };
```
`PortableUIBuilders.hpp` — `FinishControl` (was hardcoded):
```diff
         LayoutOptions controlLayout;
-        controlLayout.main = Extent::Weight(1.0f);
+        controlLayout.main = style.controlWidth;
         layoutByNodeId_[controlId] = controlLayout;
```
`PortableUILayout.hpp` — `ApplyFormGrid` (was unconditional):
```diff
             cells[1]->bounds.x = controlOffset;
-            cells[1]->bounds.width = std::max(0.0f, row->bounds.width - controlOffset - rowOpts.padding);
+            const LayoutOptions& controlOpts = LayoutFor(layoutByNodeId, cells[1]->id, fallback);
+            if (controlOpts.main.mode == Extent::Mode::Weighted)
+            {
+                cells[1]->bounds.width = std::max(0.0f, row->bounds.width - controlOffset - rowOpts.padding);
+            }
             float extraCursor = cells[1]->bounds.x + cells[1]->bounds.width + rowOpts.gap;
```
`RuntimePages.hpp` — `FormButton`:
```diff
     style.layout = CompactFormRowLayout();
+    style.controlWidth = ui::Extent::Intrinsic();
     return style;
```
`Field` and the device-selector `ComboBox` sites: untouched, still default
to `Weight(1.0f)`, still fill the control column.

## 3. Positive control

New test in `portable_ui_layout_tests.cpp`:
`TestCaptionedButtonSizesToItsLabelWhileFieldsStillSpanTheColumn` — one
form-grid page, a captioned `Field` (default) plus a captioned `Button`
declaring `controlWidth = Intrinsic()`; asserts button width < 150 and
field width > 400 at a 700px root.

Before the fix (field present, but `FinishControl`/`ApplyFormGrid` still
today's behaviour):
```
libc++abi: terminating due to uncaught exception of type std::runtime_error:
a captioned button declaring Intrinsic controlWidth sizes near its label,
not the control column
```
After the fix, run in isolation (why "isolation" matters: \S5): `EXIT=0`.
Fix mechanism confirmed correct; \S5 is about two OTHER tests.

## 4. STOP CONDITION `portable_ui_tests.cpp:1618`

**Did not fire.** `ReferenceAudioPageTreeBeforeAppSection` copies the
pre-app-section *builder call sequence*, not frozen numbers — both it and
`BuildAudioPageTree`'s no-app-section path call the same live
`PageControls::FormButton`/`Field`, so any drift lands on both sides
identically. Verified by extracting the one test into a standalone harness
(temp file, not committed, linked against the same `libsynth.a`): `EXIT=0`.
Needed isolation because an earlier test in that binary's `main()` aborts
first (\S5b).

## 5. Two contradictions found

**(a) `TestFormGridAlignsLabelAndControlColumns`** (layout tests,
pre-existing, no captions). Builds a raw Row+`Label`+`ComboBox` (default
style, so `layout.main` = library-default `Intrinsic`) inside a `formGrid`
column. Before this change `ApplyFormGrid` filled `cells[1]` unconditionally
regardless of its own mode, which is what let the row's initial per-row
sizing survive label-column realignment. With the conditional, an
Intrinsic-mode cell keeps its pre-alignment width; the final-bounds overflow
check then fires:
```
portable UI container overflows its horizontal extent: 'r1' has 376.00 and
its in-flow children need 459.72; the first child that does not fit is
'r1.control'. Declare a ScrollArea or one weighted in-flow child ...
```
`RequireContainerHoldsItsChildren`'s own comment documents the contract this
breaks: "a form grid re-columns its rows ... so a row whose allocated cells
overrun can still be laid out correctly" — i.e. recovery was meant to cover
any control, not only Weighted ones.

**(b) `TestNamedVisualCriteriaHoldOnEveryPageAndApp`, Sync surface**
(app tests, pre-existing). Sync's form grid mixes four `Toggle` rows with one
`TextField` row. `Toggle` is `FormButton` under an alias
(`RuntimePages.hpp:454-457`) — the brief edits `FormButton` and says nothing
about `Toggle`, but they are the same function. `criteria::ColumnAlignment`
requires every control in a grid's column 1 to share width, not just offset:
```
Sync: runtime.sync.ppqn width=478.98 leaves column 1 width 72.00
```
72.00 is the button intrinsic-width floor; the PPQN field still fills.
Confirmed `TestSyncPageAlignsThroughTheFormGrid` itself would have passed
(it only checks offsets and column-0 width) — the failure is the generic
cross-page criteria check.

I have not carved `Toggle` out of `FormButton`, nor touched either test —
both are plausible fixes but are design calls the brief doesn't make.

## 6. Gate

`nice make -j2 -C projects/synth test`: **EXIT=2**, not meaningfully green.
The `test` target is one linear recipe; `BRAID4_DEADLINE_TEST_BIN` runs and
fails BEFORE `PORTABLE_UI_TEST_BIN`/`PORTABLE_UI_LAYOUT_TEST_BIN` in that same
list, so make aborts there and never reaches either broken test (confirmed
with `-k` too — irrelevant within one target's own sequential recipe).
Printed failures (both confirmed `braid4_*_deadline*`):
```
[FAIL] braid4_meets_96000hz_256_frame_deadline_and_continuity: ...0.60
[FAIL] braid4_sparse_modulation_meets_96000hz_256_frame_deadline: ...0.60
make: *** [test] Error 1
```
Reporting that as "green modulo braid4" would be false. Built and ran both
binaries directly (fresh, against this tree):
```
./build/portable_ui_layout_tests  -> EXIT=134 (r1/r1.control overflow, §5a)
./build/portable_ui_tests         -> EXIT=134 (Sync column width, §5b)
```
Total pass/fail counts for the full suite can't be produced — the run never
gets there. Not certifying this gate green.

## 7. Forward enumeration

Grepped every `.layout.main =` / `.layout.cross =` in `RuntimePages.hpp`,
`ControllersPageUI.hpp`, `PortableUIStandardLayout.hpp`.
- `BackButton` `.cross` (:437): uncaptioned control's row-slot width, a
  different node/axis. Not a duplicate; untouched.
- `ControllersPageUI.hpp` wizard `buttonStyle` (:2244-2250): plain children
  of a horizontal row, never captioned, never through `FinishControl`'s
  caption branch — `.layout.main` already means their own width there. A
  third, independent, pre-existing mechanism; untouched.
- All other `.layout.main =` hits set a ROW's height or an uncaptioned
  node's own axis — none address a captioned control's width.
- No other production site narrows a CAPTIONED control's width; before this
  change none could. `FormButton` (and `Toggle`, \S5b) is the only caller.

## 8. Contradictions with this prompt

- \S5a/\S5b: literal design breaks two pre-existing tests not in the brief.
- The brief never names `Toggle`, but `Toggle` is `FormButton`
  (`RuntimePages.hpp:452-457`); editing `FormButton` silently changes
  `Toggle`, causing \S5b — a mechanical consequence, not a scope choice.
- Everything else matched: `Extent::Intrinsic()`/`Mode::Intrinsic` exist and
  are `LayoutOptions::main`'s default (`PortableUILayout.hpp:32,39,54`); the
  ten `FinishControl` call sites are at the stated lines; `BackButton`'s
  `.cross` usage is exactly as described.

## Files modified

- `projects/synth/include/synth/PortableUIBuilders.hpp` — new
  `ControlStyle::controlWidth`; `FinishControl` uses it.
- `projects/synth/include/synth/PortableUILayout.hpp` — `ApplyFormGrid`
  conditional overwrite.
- `projects/synth/include/synth/RuntimePages.hpp` — `FormButton` sets
  `controlWidth = Extent::Intrinsic()`.
- `projects/synth/tests/portable_ui_layout_tests.cpp` — new test +
  call line.

```
$ git -C External/Sheaf status --short
 M projects/synth/include/synth/PortableUIBuilders.hpp
 M projects/synth/include/synth/PortableUILayout.hpp
 M projects/synth/include/synth/RuntimePages.hpp
 M projects/synth/tests/portable_ui_layout_tests.cpp
```
No other files changed; `projects/synth/build/` is gitignored; no commits,
no staging, nothing outside `External/Sheaf` touched.

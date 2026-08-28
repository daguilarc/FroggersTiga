# Step C2 report — captioned control width (amended plan)

The amended plan (clamp instead of skip; carve `Toggle` out of `FormButton`)
is implemented exactly as specified. Both traps step C's preflight found are
closed: `TestFormGridAlignsLabelAndControlColumns` and the Sync surface of
the visual-criteria check both still pass.

## 1. Positive control

Test: `TestCaptionedButtonIsSizedToItsOwnCaptionInsteadOfTheColumn`
(`portable_ui_layout_tests.cpp`).

Before the fix (field, `FinishControl`, `FormButton`/`Toggle` all in place,
but `ApplyFormGrid` still doing today's unconditional overwrite — exactly
the "previous attempt" state):

```
libc++abi: terminating due to uncaught exception of type std::runtime_error:
a captioned button declaring Intrinsic controlWidth sizes to its own label, not the column
EXIT=134
```

After restoring the clamp: `EXIT=0` (both binaries, \S4).

## 2. The clamp, as a diff hunk

`PortableUILayout.hpp`, `ApplyFormGrid`:

```diff
             cells[1]->bounds.x = controlOffset;
-            cells[1]->bounds.width = std::max(0.0f, row->bounds.width - controlOffset - rowOpts.padding);
+            // Weighted -> fill the column, as before. Non-weighted -> keep the
+            // already-resolved width (e.g. Intrinsic sized to its caption),
+            // clamped to whatever the column has left, so it never overruns.
+            const float remainingWidth = std::max(0.0f, row->bounds.width - controlOffset - rowOpts.padding);
+            const LayoutOptions& controlOpts = LayoutFor(layoutByNodeId, cells[1]->id, fallback);
+            cells[1]->bounds.width = controlOpts.main.mode == Extent::Mode::Weighted
+                                          ? remainingWidth
+                                          : std::min(cells[1]->bounds.width, remainingWidth);
             float extraCursor = cells[1]->bounds.x + cells[1]->bounds.width + rowOpts.gap;
```

Weighted branch is byte-identical to the old expression (default resolves
exactly as before). Non-weighted: `controlOffset + min(x, remainingWidth) <=
controlOffset + remainingWidth == row->bounds.width - rowOpts.padding`
always, so the row can never overrun on this account.

## 3. Both stop conditions

**`TestColumnAlignmentCheckCatchesAControlLeavingItsColumn`
(`portable_ui_layout_tests.cpp:1281-1309`): did not fire** — passed in the
full 47-test run (`LAYOUT_EXIT=0`, \S4). Can't be touched by either change:
`ColumnAlignment` no longer checks width at all (edit 4), and the
misalignment it injects (`bounds.x += 4.0f`) never touched width either.

**`portable_ui_tests.cpp:1618-1650`
(`TestAudioPageWithNoAppSectionIsByteIdenticalToBeforeTheChange`): did not
fire** — passed in the full 50-test run (`UI_EXIT=0`, \S4).
`ReferenceAudioPageTreeBeforeAppSection` calls the same live
`PageControls::FormButton`/`Field` the real builder calls, not frozen
numbers, so the change lands identically on both sides. Not edited.

## 4. Both binaries, run directly

```
$ nice make -j2 -C projects/synth build/portable_ui_tests build/portable_ui_layout_tests
$ ./build/portable_ui_layout_tests; echo EXIT=$?
EXIT=0
$ ./build/portable_ui_tests; echo EXIT=$?
EXIT=0
```

Neither binary prints per-test output on success (`Require` throws and
aborts on first failure; silence + exit 0 means every registered `main()`
call passed). Pass counts from the hand-maintained call lists:
`portable_ui_layout_tests` 47 calls (45 pre-existing + 2 new — the button
test above and `TestContentWiderThanTheColumnIsClampedAndTheRowStillHoldsItsChildren`);
`portable_ui_tests` 50 calls, unchanged — the real-Audio-page assertion (\S5)
extends an existing test body rather than adding a new call.

## 5. Real-Audio-page assertion

Added to `TestOfflineInputCaptureOffersACaptionedRetryRow`
(`portable_ui_tests.cpp`), which already builds the real
`BuildAudioPageTree` with `showInputRetry = true`:

```cpp
Require(retry.bounds.width < inputSelector.bounds.width,
        "the retry button is narrower than the full-width input device selector");
Require(std::fabs(retry.bounds.x - inputSelector.bounds.x) <= 0.0001f,
        "the retry button's left edge matches the input device selector's left edge");
```

Actual numbers (900x560 root, both selectors + retry shown), via the
standalone harness (\S9) against the real `BuildAudioPageTree`: output
x=128.78 width=771.22; input x=128.78 width=771.22; retry x=128.78
width=104.66 — retry shares the selectors' left edge and is well under a
third of their width.

## 6. Sync toggles unchanged — how verified

- **By construction:** `Toggle` calls `FormButton` (now
  `controlWidth = Intrinsic()`) then unconditionally overwrites
  `controlWidth = Weight(1.0f)` before returning — bit-identical, on this
  field, to what `FormButton` alone produced before this change.
- **By measurement:** standalone harness (\S9) built the real
  `BuildSyncPageTree` (900x560 root, all four toggles on, PPQN field
  present) and printed every control-column cell: all five —
  `send_clock`, `receive_clock`, `send_transport`, `receive_transport`,
  `ppqn` — resolve to the identical `x=161.02 width=738.98`. Toggles still
  fill the full control column, unchanged.

## 7. Files modified

- `PortableUIBuilders.hpp` — new `ControlStyle::controlWidth` field (default
  `Weight(1.0f)`); `FinishControl` uses it instead of the hardcoded weight.
- `PortableUILayout.hpp` — `ApplyFormGrid`'s control-cell width is now the
  weighted/clamped branch (\S2).
- `RuntimePages.hpp` — `FormButton` sets `controlWidth = Intrinsic()`;
  `Toggle` carved out to reset it to `Weight(1.0f)`; `Field`/selectors untouched.
- `tests/support/VisualCriteria.hpp` — `ColumnAlignment` drops the
  equal-width check (keeps shared-x); comment restates the rule.
- `tests/portable_ui_layout_tests.cpp` — two new tests (\S1, \S4) + call lines.
- `tests/portable_ui_tests.cpp` — two new assertions inside the existing
  `TestOfflineInputCaptureOffersACaptionedRetryRow` (\S5); no new call line.

```
$ git -C External/Sheaf status --short
 M projects/synth/include/synth/PortableUIBuilders.hpp
 M projects/synth/include/synth/PortableUILayout.hpp
 M projects/synth/include/synth/RuntimePages.hpp
 M projects/synth/tests/portable_ui_layout_tests.cpp
 M projects/synth/tests/portable_ui_tests.cpp
 M projects/synth/tests/support/VisualCriteria.hpp
```

No other files changed; `build/` is untracked/gitignored; nothing staged,
committed, or touched outside `External/Sheaf`.

## 8. Fuller gate

`nice make -j2 -C projects/synth test`: **does not reach the two portable-UI
binaries.** `BRAID4_DEADLINE_TEST_BIN` runs 14th in the linear recipe, fails,
and `make` stops (`Error 1`) before `PORTABLE_UI_TEST_BIN`/
`PORTABLE_UI_LAYOUT_TEST_BIN` (21st/22nd). The 13 binaries before it all
passed clean (920 `[PASS]`, 0 unexpected `[FAIL]`).

Failures, all five confirmed `braid4_*_deadline*`
(`tests/braid4_deadline_tests.cpp`, timing assertions unrelated to this
change): `braid4_meets_44100hz_256_frame_deadline_and_continuity`,
`braid4_meets_48000hz_256_frame_deadline_and_continuity`,
`braid4_meets_96000hz_256_frame_deadline_and_continuity`,
`braid4_sparse_modulation_meets_48000hz_256_frame_deadline`,
`braid4_sparse_modulation_meets_96000hz_256_frame_deadline` (`make: ***
[test] Error 1`) — each fails a `p99Seconds`/`averageSeconds <=
blockSeconds * 0.80/0.60` timing budget, not a correctness assertion.

The two portable-UI binaries were verified directly instead (\S4), both
`EXIT=0`. Not certifying "make test" itself green — it isn't, for reasons
unrelated to this change.

## 9. Inner-loop method (for the next person)

Rebuilding `portable_ui_tests`/`portable_ui_layout_tests` per edit is slow —
header-only layout headers pulled into a ~4,900-line binary linking the whole
synth library: touching `portable_ui_tests.cpp` and rebuilding just that
binary measured **16.3s wall** (`nice make -j2 build/portable_ui_tests`, warm
cache). A standalone `.cpp` including only `synth/RuntimePages.hpp` (pulls in
`PortableUIBuilders.hpp`/`PortableUILayout.hpp`), building the real page tree
and printing the bounds in question, compiles **link-free** in **2.2s wall**
(`c++ -Iinclude -std=c++20 file.cpp -o file`, no `libsynth.a`). Used that for
\S5/\S6's numbers; built/ran the two real binaries only once the change
looked correct (\S4), plus once more after a comment fix. Scratch files,
deleted afterward.

## 10. Contradictions with this prompt

None found. Every cited line number, the "two jobs" `ApplyFormGrid`
contract, and both stop-condition premises checked out against the tree.

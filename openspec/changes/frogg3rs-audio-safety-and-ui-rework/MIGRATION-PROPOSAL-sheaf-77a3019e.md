# Migration proposal — `External/Sheaf` `1940ddcb` → `77a3019e`

Written 2026-08-01, after the pin bump and before any code edit (OMNI §7/§13 — the predecessor's
recorded process failure was starting call-site edits with no proposal behind them).

Pipeline position (OMNI §13): OpenSpec = `design.md` + the RE-CHECK section of
`/UPSTREAM-SHEAF-ASK.md`. **This document is the proposal layer.** Preflight gate is at the bottom.
Execution is subagent-driven and sequential; postflight compares against this document only.

---

## §1 planning checklist — answered here, not deferred to an implementer

Every claim below cites the `file:line` where it was read at the stated revision (OMNI §1, TRACE
DON'T ASSERT). App paths are relative to `app/`; Sheaf paths to `External/Sheaf/projects/synth/`.

### Data flow — what actually breaks

The build fails in exactly **two** app files — *for the translation units that were compiled*.
Complete error surface for those TUs: **20 errors, 4 warnings**, and clang's 20-error cap was
**not** reached (`4 warnings and 20 errors generated.`, no `too many errors emitted` line). An
earlier report called this list truncated; that was wrong and is corrected here.

**ERRATA (2026-08-01, found by Phase 1 execution).** The "complete" claim above was itself
incomplete, in a second dimension: `-ferror-limit=0` exhausts one TU's errors, but `make test` has
no `-k` and aborts at the first failing *binary*, so eight of the ten test TUs were never compiled
when this table was drawn. Phase 1's implementer, correctly stopping rather than improvising, found
a **21st error in a 3rd file**: `FroggersSurfaceTests.cpp:898` reads `Node::variant`, retired
upstream. A follow-up static sweep of every app TU for the retired/renamed symbols (`.variant`,
`DrawCommand::Kind::Stroke/Ellipse/RoundedRect`, style-less Builder calls) found **no further
site** — `FroggersVisualizerTests.cpp`'s match is a comment plus an accessor whose type resolves
through the new include. Lesson, recorded for the next enumeration: **"complete" means every TU
compiled or swept, not one TU un-capped.** The aborts-at-first-binary trap the handoff documents
for *test runs* applies equally to *error enumeration*.

| # | Error kind | Count | Sites | Real or cascade |
|---|---|---|---|---|
| 1 | `no template named 'ScopeVisualizer' in namespace 'synth::ui'` | 1 | `FroggersAppCore.hpp:1470` | **real** — one missing include |
| 2 | `Button()` has no matching overload | 6 | `FroggersUiSurface.hpp:756, 757, 850, 858, 993, 1014` | real |
| 3 | `Visualizer()` too few args (expected 3, have 2) | 2 | `FroggersUiSurface.hpp:768, 901` | real |
| 4 | `Label()` too few args (expected 3, have 2) | 2 | `FroggersUiSurface.hpp:1049, 1107` | real |
| 5 | `Slider()` too few args (expected 8, have 7) | 2 | `FroggersUiSurface.hpp:1051, 1106` | real |
| 6 | `StatusText()` too few args (expected 3, have 2) | 1 | `FroggersUiSurface.hpp:1064` | real |
| 7 | `DrawInteractive()` too few args (expected 6, have 4) | 1 | `FroggersUiSurface.hpp:918` | real |
| 8 | `Build()` missing `rootExtent` | 1 | `FroggersUiSurface.hpp:807` | real |
| 9 | `FroggersApp` → `FroggersAppCore` conversion / `SynthApplication` concept | 4 | `Froggers.hpp:45, 49, 58`, `FroggersHeadlessTests.cpp:73` | **cascade of #1 — zero edits** |

**Error 9 is a cascade, traced and proven, not assumed.** `vcoScopeVisualizer_` is a *member
declaration* of `FroggersAppCore` (`FroggersAppCore.hpp:1470`). An unresolved type in a member
declaration makes the enclosing class invalid; `FroggersApp final : public FroggersAppCore`
(`Froggers.hpp:42`) therefore has a broken base, so every `FroggersApp`→`FroggersAppCore`
conversion fails and the `SynthApplication<FroggersApp>` static_assert fails with it. A build agent
reported error 9 as an unexplained "app composition vs inheritance" API change and stated it found
no link to error 1. **That conclusion is wrong** — clang simply does not cross-reference a broken
base class. There is no app-composition change in Sheaf. Fixing error 1 removes all four sites.

**Error 1 is a header move, not an API change.** `ScopeVisualizer` was
`template <typename LayerState> class ScopeVisualizer final : public Visualizer` at
`include/synth/PortableUIBuilders.hpp:225-238` (revision `1940ddcb`) and is now the **same
template with a byte-identical constructor** at `include/synth/PortableScopeVisualizer.hpp:222-229`
(revision `77a3019e`). The app obtained it transitively via
`#include "synth/PortableUIBuilders.hpp"` (`FroggersAppCore.hpp:91`); it now needs the header
named directly. No use-site change.

**Errors 2-7 are one API change with one shape.** Builder control methods take a trailing
`ControlStyle` parameter object (`include/synth/PortableUIBuilders.hpp:20-33`; `Button` overloads at
`:194-213`). `ControlStyle` carries `color`, `textStyle`, `borderColor`/`borderWidth`/
`cornerRadius`, `selected`, `enabled`, `action`, `pointerDragAction`, `doubleClickAction`,
`caption`, and `layout`.

**Error 8 is the one with design weight.** `Build()` is now `NodeTree Build(Bounds rootExtent)`
(`include/synth/PortableUIBuilders.hpp:352`), with an in-source rationale that the old form "let a
producer resolve against a surface size it had already baked into its own tree, which is the
compiled-in surface size sru-50 forbids" (`:350-351`). The app calls bare `builder.Build()` at
`FroggersUiSurface.hpp:807`.

### Reuse vs creation — every definition site enumerated (OMNI §1)

15 call sites in `FroggersUiSurface.hpp` and 1 include in `FroggersAppCore.hpp` = **16 edit sites,
2 files.** The list above is exhaustive: it comes from a build with `-ferror-limit=0` whose own
summary line confirms 20 total errors, of which 4 are the cascade and 16 are edits. No other app
file is touched. The frozen trees (`desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`) are not
involved.

### Dependencies

All required Sheaf API is public and verified present at `77a3019e`:
`ControlStyle` (`PortableUIBuilders.hpp:20-33`), `Build(Bounds)` (`:352`),
`ScopeVisualizer` (`PortableScopeVisualizer.hpp:222`), and — for Phase 2 only —
`IntrinsicFor(const Node&)` (`PortableUIMetrics.hpp:36`), `Extent`/`LayoutOptions`/`AllocateExtents`
(`PortableUILayout.hpp:31-53, :165`).

### Structure depth (OMNI §5) / helper functions (OMNI §6)

Phase 1 changes call-site arguments only; it introduces no nesting and no helper. The default
`ControlStyle{}` is passed positionally at each site rather than through a wrapper helper — a
wrapper would satisfy neither §6 condition 2 nor 3 (it isolates no transformation stage and
improves no traceability), so per §6 it is not allowed.

### Efficiency (OMNI §10/§11)

Not a factor. `Build(rootExtent)` resolves layout once per tree build, exactly as the pre-bump path
did; no loop, no repeated allocation, no O(n) check introduced.

### Defensive code (OMNI §12)

No guard is added anywhere. In particular no null/empty check is added around `rootExtent` — the
value passed is the surface's own bounds, which the app already holds unconditionally; an
unreachable branch there would be precisely the defect §12 forbids and the
`frogg3rs-dsp-recovery` spec's "No unreachable defensive clamps" requirement already names.

---

## Phase 1 — restore green. No behaviour change. UNCONDITIONAL.

Phase 1 is required under every possible answer to Phase 2's open question, so it proceeds now.
Its purpose is a green baseline that makes Phase 2's diffs attributable — the discipline this
change's history says was missing when a scope-position regression shipped green.

**P1.1** — add `#include "synth/PortableScopeVisualizer.hpp"` to `FroggersAppCore.hpp`'s include
block (alphabetical position, among the `synth/` includes at `:85-91`). Fixes errors 1 and 9
(5 of 20). No other edit in that file.

**P1.2** — pass a default-constructed `ControlStyle{}` at the 14 Builder call sites (errors 2-7),
**preserving each site's existing behaviour exactly**. Where a site currently wires an action
positionally, it keeps doing so; where it relies on post-`Build()` patching
(`WireDrawNodeActions`, `MarkSelectedBank`), that patching **stays** in Phase 1. Migrating those
into `ControlStyle` is Phase 2 work and must not be smuggled in here.

**P1.3** — pass the surface's own bounds to `builder.Build(...)` at `FroggersUiSurface.hpp:807`.
Use the extent the surface already has; do not introduce a new constant, and do not change any
computed height.

**P1.4** — full suite via subagent: `nice make -j2 test`, foreground, plus
`./app/build-launcher.sh`. **Count that all ten binaries ran** — `make test` has no `-k` and aborts
at the first failure, so an early abort hides the rest. Baseline to restore: **156/156 across ten
binaries.**

**P1.5** *(added by errata)* — `FroggersSurfaceTests.cpp:898`:
`REQUIRE_TRUE(playNode->variant.empty() && stopNode->variant.empty());` pins a field that no longer
exists. The assertion's *intent* (its own comment): nothing recolours the transport emoji — "an
emoji carries its own colour, and a variant would recolour the text and fight it (design.md A3e)."
The equivalent property in the new appearance model is that **no carried `textStyle` overrides the
glyph colour** (glyph colour comes from `textStyle` alone, `PortableUI.hpp:204`). Rewrite as:
`REQUIRE_TRUE(!playNode->textStyle.has_value() && !stopNode->textStyle.has_value());`
and update the preceding comment to name `textStyle` instead of `variant`, keeping the design.md
A3e reference. This is the handoff's own rule for pinned expectations: rewrite the pin to assert
the surviving property; do not delete the assertion.

**Explicitly out of scope for Phase 1:** deleting any workaround, changing any pixel, touching §A
audio safety, touching parameter-value randomization, or altering the frozen trees.

---

## Phase 2 — the workaround deletions the bump unlocks. NEEDS AN OPERATOR DECISION.

The bump's *point* is that several app-side workarounds now have no cause. Leaving one in place is,
per this change's own handoff, worse than the original bug: invisible, constraining the design
around a limitation that no longer exists, and indistinguishable from a deliberate choice.

Confirmed available at `77a3019e` (evidence in `/UPSTREAM-SHEAF-ASK.md` RE-CHECK):

- **Single-click encoders + restored draw-command transport icons** (ask 1 landed). Encoders are
  `DrawInteractive` + `doubleClickAction` today; `BuildPlayDrawCommands`/`BuildStopDrawCommands`
  were kept unused in the file for exactly this moment.
- **`ControlStyle::action`** replaces the post-`Build()` `SetNodeAction` field-patch helper, and
  **`ControlStyle::selected`** replaces `MarkSelectedBank`'s post-`Build()` edit.
- **`ControlStyle::caption`** replaces the hand-rolled adjacent `Label` nodes —
  `kSceneBlendLabel`/`kBpmLabel` exist only as that workaround.
- **Coloured glyphs** via `textStyle` (ask 3 landed), which is the only reason Stop is a `🟥` emoji
  rather than a coloured glyph.
- **D.6, the left-column control block**, blocked solely because positioned controls required
  `Draw` nodes and those were double-click-only.

### The open question — scope of the layout change

`FroggersUiSurface.hpp:140-152` declares `FroggersAutoFlowedChromeModel`, an explicit ~200-line
app-side **replica** of `PortableJuceBackend.hpp`'s control-sizing and greedy-wrap rules. Its
stated justification is that the app is portable code with no JUCE dependency **"and Sheaf exposes
no 'measured auto-flow extent' accessor."**

**That justification has expired.** `PortableUIMetrics.hpp` — a header that did not exist at
`1940ddcb` — provides `AdvanceFor(const TextStyle&)` (`:26`), `TextWidth(std::string_view, const
TextStyle&)` (`:31`), and `IntrinsicFor(const Node&) -> Bounds` (`:36`), all portable and
JUCE-free. `PortableUILayout.hpp` (also new) adds a declarative engine: `Extent` in Fixed /
Intrinsic / Fraction / Weighted modes with `Min`/`Max` (`:31-51`), `LayoutOptions` (`:53`),
`AllocateExtents` (`:165`), and overflow treated as an error (`:246`).

Two honest options, and this is the operator's call, not the implementer's:

**Option A — minimal.** Phase 1 plus only the workaround deletions above. Keep
`FroggersAutoFlowedChromeModel` and hand-positioning. **Cost:** the replica's per-constant citations
into `PortableJuceBackend.hpp` line ranges are now ~424 commits stale and silently wrong, and the
file keeps a comment asserting an accessor exists nowhere when it now exists. At minimum those
citations get re-verified or the comment corrected — leaving a false justification in place is the
exact failure mode this change keeps rediscovering.

**Option B — adopt the layout engine.** Additionally delete the replica and express the chrome band
with `LayoutOptions`/`Extent`, letting `Build(rootExtent)` resolve it. **Benefit:** ~200 lines of
mirrored toolkit internals go away permanently, the window-height derivation stops being a
prediction of someone else's algorithm, and D.6's positioned control block becomes a declaration
rather than arithmetic. **Cost:** materially larger; it touches the geometry that §B.1/B6 fought a
regression over, so it requires re-proving
`scope_sits_in_a_left_column_with_the_grid_to_its_right` and ends at an operator walkthrough
regardless.

**Recommendation was A now, B as its own change** — on the grounds that B is a re-architecture of
the surface layer rather than a bump consequence, and that bundling it into a change already
carrying audio safety plus two UI rounds would blur the postflight comparison.

### DECISION: **B — adopt the layout engine.** Operator, 2026-08-01.

The recommendation above is **superseded and recorded, not deleted**, so a later reader can see the
tradeoff was weighed rather than missed. B proceeds in full.

**Mitigation for the concern the recommendation raised.** The postflight-attributability problem is
real but is a *sequencing* problem, not a reason to refuse the scope. It is handled by splitting
execution into three separately-committed stages, each green before the next begins:

- **Stage 1 = Phase 1** — compile only, zero behaviour change. Green baseline.
- **Stage 2 = workaround deletions** — single-click encoders, restored draw-command transport
  icons, `ControlStyle::action`/`selected` replacing the post-`Build()` patches,
  `ControlStyle::caption` replacing `kSceneBlendLabel`/`kBpmLabel`, coloured glyphs.
- **Stage 3 = layout-engine adoption** — delete `FroggersAutoFlowedChromeModel`, express the chrome
  band as `LayoutOptions`/`Extent`, let `Build(rootExtent)` resolve it, then D.6's left-column
  control block as a declaration rather than arithmetic.

Postflight compares each stage against this document independently. A geometry regression in
stage 3 cannot then hide inside stage 2's diff, which is the failure mode §B.1/B6 already shipped
once.

**Stage 3 carries a mandatory guard obligation.** `scope_sits_in_a_left_column_with_the_grid_to_its_right`
exists precisely because nothing pinned the scope's *position* and a regression shipped with every
other assertion green. Deleting the replica changes how geometry is computed, so that guard must be
**re-proven, not merely still-passing** — and per the handoff's third process lesson, the question
to ask is what assertion would catch the bug, not what assertion is easy. The reserved-empty region
below the scope (`froggers-app-surface-layout`, "Shrinking the band does not move it") is a
requirement, not an accident, and the engine must not reclaim it.

**Spec consequence.** Stage 3 invalidates the justification written into
`FroggersUiSurface.hpp:140-152` and touches requirements framed around toolkit limitations that no
longer exist. Per the handoff's "supersede rather than patch" rule, the constraint-lifted
justification gets **recorded** — the requirement is not silently edited to match the new code.

Phase 2 is **visual and behavioural in every stage**, so it closes at the operator's eyes and
ears (C.2), never at an implementer's checkbox.

---

## Preflight gate (OMNI §14) — all must hold before any dispatch

- [x] The §1 data-flow trace exists, is complete, and every claim cites a `file:line` actually read.
- [x] ~~The error surface is complete, not truncated — confirmed by the compiler's own count with
      `-ferror-limit=0`.~~ **FALSIFIED during execution** — the count was per-TU while `make`
      aborts per-binary; see ERRATA. Re-closed by the static sweep of every app TU for retired
      symbols, and finally proven only by P1.4's full ten-binary run.
- [x] Every edit site enumerated exhaustively — **17 sites, 3 files** after the errata (16
      original + `FroggersSurfaceTests.cpp:898`), verified by sweeping all TUs rather than
      sampling the first failure.
- [x] Cascade errors distinguished from real ones by tracing the mechanism, not by pattern-matching.
- [x] Phase 1 states explicitly that §A audio safety and parameter-VALUE randomization are untouched.
- [x] Phase 1 forbids smuggling in Phase 2's workaround deletions.
- [x] The one design question is escalated to the operator rather than decided by the implementer.
- [x] Execution constraints restated for the brief: Sonnet/Haiku only, sequential, foreground
      builds, `nice -j2` never higher, count all ten binaries.

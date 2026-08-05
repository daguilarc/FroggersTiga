# Tasks — `frogg3rs-audio-safety-and-ui-rework`

## §0 Standing constraints

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch (OMNI §4, §15).
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Build/test runs go through a subagent**, reporting counts and failure tails only (OMNI §16.1).
- **`External/Sheaf` is pinned at `77a3019e` and stays clean** (bumped from `1940ddcb` 2026-08-01,
  operator-approved). We do not patch Sheaf; Sheaf-side needs → `UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees stay byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes are sequential.** No parallel implementation dispatches (operator directive,
  2026-07-28). Parallel is permitted for read-only analysis only.
- **Do not add user-visible behaviour the operator did not ask for.** Propose first.
- **An implementer may not close a task whose spec requires the operator to see or hear it.**
  The predecessor did this and cost the operator a wasted test session.
- **`screencapture` works and the resulting PNG is readable** — GUI claims can be verified directly.
  Scope captures to the app window; a full-screen capture catches the operator's other windows.
- **The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.**

### Execution order — this file is the single proposal layer

**One change, one proposal layer, one order.** Every `§X-PLAN` section below is a proposal under
OMNI §3 and is the only artifact an executor runs for its scope. Two standalone proposal documents
(`MIGRATION-PROPOSAL-sheaf-77a3019e.md`, `PROPOSAL-direct-launch.md`) existed briefly and were
folded into §F-PLAN and §G-PLAN on 2026-08-02; **commit `6701fc1`'s message still cites the old
path.** They were split out in the first place because each new scope looked self-contained — which
is exactly how a change ends up with three proposals and its execution order recorded nowhere but a
chat transcript. If a new scope arrives, it becomes a `§X-PLAN` section here.

**Remaining work, in order. The order is load-bearing, not a preference:**

1. ~~**F.2**~~ — done 2026-08-03, `84f83e7`.
2. **F.3** — adopt the portable layout engine. Rewrites the same call sites F.2 touched.
3. **F.4** — second pin bump, `77a3019e` → `508d9d68`, for ask 8. **After F.3, not before**: F.3 is
   a re-architecture of the surface against a known-good toolkit, and stacking an unmigrated 27-commit
   bump under it would make any breakage unattributable — the exact confusion F.1 existed to prevent.
   F.4 is also its own compile-then-behaviour split, for the same reason.
4. **F.5** — remove the external-audio workaround. Strictly after F.4; the API it needs does not
   exist at our current pin.
5. **D.6** — left-column control block. Depends on F.2a (positioned controls needed plain-clickable
   `Draw` nodes) and lands cleanest after F.3 makes placement declarative.
6. **G.2** — blank-window-on-failure. Independent; ordered here only because it is small.
7. **C.2 / G.3** — operator walkthrough and patch-load confirmation. **Last, and not closable by an
   implementer.** F.3 and F.5 are both operator-perceptible, so they end at the operator's eyes and
   ears regardless.

D.3 (voicing) and D.4 (publish pipeline) stay carried-open; neither blocks the above.

---

## §A Audio safety — do this first, it is why this change exists

- [x] A.1 **Comb feedback below unity.** `Comb::GetFeedback` (`app/dsp/FilterFx.hpp:381-388`)
  returns ±1.1 at the knob extremes. Change the magnitude to **±0.95**.
  This **will** fail `app/FroggersDspParityTests.cpp:528-529`, which pins the ±1.1 formula
  literally. That is the intended outcome: rewrite the pinned expectation and add a one-line note
  that the divergence from the frozen firmware is deliberate, in the style of the existing D6
  fuegoize-divergence note. Do not preserve 1.1 anywhere.
  Failing test first: drive the comb at both knob extremes, stop the input, assert the output decays
  to silence instead of sustaining.
- [x] A.2 **Resonant bump gain ceiling.** `filterChain_.peak.SetHeight(ExpMapCompute(1.0f, 10.0f, …))`
  (`app/FroggersAppCore.hpp:825`) gives up to 10× (+20 dB). Change the maximum to 4× (+12 dB).
  Check whether `scoopNotch` shares the mapping (it takes the same freq/width but its height is a
  dip and adds no gain — confirm before touching it). Update any pinned parity expectation.
  Failing test first: full-scale input at maximum resonance must not exceed the documented ceiling.
  **Amended 2026-07-29: further revised to 2× (+6 dB).** The operator heard 4× modulated and judged
  it still too harsh — the limiter was riding hard and continuously at that ceiling, and the
  sustained gain reduction was itself the harshness. See `app/dsp/FilterFx.hpp:109-122` for the
  full value history and reasoning. `kMaxResonantBumpHeight` is now a shared constant (both the
  call site and the parity test read from it) rather than a literal at each use.
- [x] A.3 **Limiter replacing the hard clamp.** `SanitizeOutputSample`
  (`app/FroggersAppCore.hpp:912-920`) currently hard-clamps at 1.0 (the predecessor's task 2.8,
  now superseded). Replace with a limiter: threshold **0.9**, gain reduction, fast attack, release
  ~100 ms.
  **Gain reduction, not per-sample waveshaping** — a signal below threshold must pass through
  bit-identical, and that is the acceptance test. **Preserve exactly** the non-finite → `0.0f`
  branch and the denormal flush.
  **Required comment at the definition (operator):** note that this stage does not need to be
  internal to a future VST/plugin build, because a plugin host owns final gain staging and usually
  supplies its own limiting — so there it is redundant and a candidate to bypass or compile out.
  Failing tests first: (a) an overdriven patch never emits above 1.0 and is not squared off;
  (b) a normal-level patch is bit-identical through the stage.
- [x] A.4 **Attack range 2.5 s → 1.0 s.** `kMaxAttackSeconds` (`app/dsp/VoiceEnvelope.hpp:35`).
  Release stays at 10 s. Update any pinned parity expectation.
- [x] A.5 **Randomize storm test.** At least 200 Randomize All draws through the **real** engine
  path, asserting after each that output is finite, never sustained above full scale, and that the
  instrument still produces audio. The predecessor's failure rate was roughly 1 in 7; this must be
  zero. This is the test that would have caught the operator's blowout.

## §B UI rework — operator feedback from the running build

- [x]· B.1 **Scope band geometry.** `kScopeWidth = 340.0f` (`app/FroggersUiSurface.hpp:316`) against
  a full-height column (`ScopeArea`, `:428`) makes the panel far taller than wide. Make it **wider
  than tall and at most one third of its current area**. Re-derive `RequiredHeight()`/`uiHeight` —
  the auto-flow model at `app/FroggersUiSurface.hpp:152` is the single source of truth and its
  assertions must be updated, not bypassed. **Operator confirmation required.**
  **Amended 2026-07-29: see B6 below.** The "give the reclaimed space to the encoder grid, leaving
  no blank region" instruction in this task's original wording was wrong and is withdrawn — the
  operator asked only for a height change, not a position change. The implementer followed this
  task's brief faithfully; the brief was the defect. Only the height shrinks; the freed space below
  the scope stays empty for a future control block (D.6).
- [x]· B.2 **Trace colours → cyan / pink / yellow.** `app/FroggersAppCore.hpp:128-130` currently sets
  Red / Orange / Yellow. `synth::Color` has `Cyan` and `Yellow` but **no** `Pink`/`Magenta`, so use
  `Color::Rgb(255, 105, 180)` for pink. Reason for the change — red-green colour blindness — goes in
  the comment so nobody "restores" the old palette.
- [x]· B.3 **Post-gate scope tap.** `dsp::Vco::Process` writes the scope
  (`app/dsp/Vco.hpp:164-167`) **before** `MixOscVoices` applies the ASR gate, so traces move before
  Play is ever pressed. Write the post-gate per-voice values instead. Extend `MixOscVoices`
  (`app/dsp/VoiceEnvelope.hpp:150-168`) with an out-parameter and have its single call site pass it
  — do **not** re-apply `adsr.apply` at a second site (OMNI §8).
  Failing test first: with the transport never started, every trace is flat.
- [x]· B.4 **Play/Stop as glyph Buttons.** Restore visible transport controls as `Button` nodes with
  `▶` and `■` as label text (operator-approved). Buttons render their labels and dispatch on single
  click, so this gets icons *and* single click with no upstream dependency. Keep
  `BuildPlayDrawCommands`/`BuildStopDrawCommands` for when plain-click lands. **Operator
  confirmation required.**
- [x]· B.5 **Remove the BPM annotation.** The label must be a constant `"BPM"`. The
  "(no effect while stopped)" text was never requested and, because chrome is auto-flowed by
  control width, it re-flows its neighbours every time the transport starts or stops.

## §B-bis Second UI round — operator feedback on the running build (2026-07-29)

The first §B round was **seen** by the operator and produced corrections. All landed inline (the
subagent dispatch path was down with repeated 529s; operator approved the inline fallback under
OMNI §-1). Suite green 151/151, `make` exit 0, all ten binaries.

- [x] B6 **Scope POSITION regression fixed.** §B.1 shrank the scope *and moved it* to a full-width
  top band. The operator asked only for a size change: *"WHEN DID I ASK FOR YOU TO CHANGE THE
  LOCATION OF IT? i said just the height should change."* Cause was my brief, not the implementer:
  it said "give the reclaimed space to the encoder grid", which the operator never asked for.
  Restored side-by-side — scope 340×64 in the upper-left, grid to its right, space below the scope
  left empty for the deferred control block (D.6). 12% of the original 340×528 area, 5.3× wider
  than tall.
  **New guard test** `scope_sits_in_a_left_column_with_the_grid_to_its_right`: nothing pinned the
  scope's *location*, which is exactly why the regression shipped green.
- [x] B7 **Transport glyphs → `▶️` / `🟥`.** Operator-chosen emoji. **Confirmed rendering: `🟥`
  draws as a real red square** — JUCE does resolve colour emoji through the system font stack.
  `▶️` falls back to a monochrome triangle (emoji presentation not applied) but reads correctly.
  Asserted as exact byte sequences incl. U+FE0F, because `label == "▶"` would silently pass on the
  wrong glyph.
- [x] B8 **Stop forces a ~50 ms fade.** Closing the gate put voices into `Stage::Release` honouring
  the *patch's* release, so Stop began a multi-second fade while the voices kept re-exciting delay
  and reverb. A `releaseKnob` lambda substitutes a fast release **only while the transport is
  stopped**; the operator's setting is untouched and applies normally on restart. Derived from
  `VcoAdsrState`'s own mapping so it stays 50 ms if the ceiling is retuned.
- [x] B9 **`kMaxReleaseSeconds` 10 → 5** (operator number).
- [x] B10 **Reverb wet capped at 0.7.** Operator: *"too fucking quiet."* The blend is
  `(1−mix)*dry + mix*wet`, so mix 1.0 removed the dry signal entirely and read as a level drop
  rather than more reverb. Capping keeps ≥30% dry at every knob position; the control still sweeps
  full travel.
- [x] B11 **External audio defaults to disconnected.** `externalInputConnected` asked only "did the
  device hand us an input channel?", permanently true on a laptop (built-in mic; log reads
  `1 in / 2 out` with nothing attached), so both external modulation sources were marked connected
  and Sheaf's randomizer — which correctly filters on `connected` — kept picking them.
  Gated behind `kExternalAudioOptedIn = false`; plumbing left intact so a real opt-in is one line.
  **Accepted cost:** the Audio config page can no longer re-enable it. Operator ruled: leave as-is
  and file the ask (`UPSTREAM-SHEAF-ASK.md` item 8, draft email in
  `upstream-email-external-audio-draft.md`).
- [x] B12 **BPM label moved to trail its slider.** Leading it put it between the two sliders and
  nearer the scene-blend one, reading as labelling the wrong control. The two labels are now
  deliberately asymmetric — do not "fix" that.

## §C Verification — none of this is self-certifiable

**Status 2026-07-29 (updated).** Suite green 151/151, all ten binaries, `make` exit 0.

**Partially verified visually.** The lead built, launched and screenshotted the running app on the
operator's third display (an earlier attempt hit a locked login screen and was abandoned rather
than worked around). Confirmed by looking: scope is upper-left, landscape, cyan/pink/yellow; space
below it empty; grid to its right; `🟥` renders as a red square; bank order signal-path with
selected-state inversion; "Scene blend" and "BPM" both visible; BPM constant. The operator has
since confirmed **Play works** and the instrument is silent until pressed.

`·` on a §B item means code landed and suite green with pixels unseen. Items confirmed above are
now plain `[x]`.

- [x] C.1 **Full suite green** via subagent, counts and failure tails only.
- [ ] C.2 **Operator walkthrough.** Audible on start; single-click Play/Stop/banks; scope landscape,
  correctly coloured, flat when silent; Randomize All repeatedly without blowout; Stop silences;
  nothing clipped. **No item in §A or §B closes without this.**

## §E-PLAN — how to execute §E under OMNI (written 2026-07-29)

The analysis (design A6/A7) is the OpenSpec layer. This is the **proposal** layer OMNI §3 requires
between it and execution, written because the first attempt at E.1 skipped straight to editing and
had to be stopped.

### Pipeline position (OMNI §13)

1. Structural intent check — done.
2. OpenSpec — done, `design.md` A6/A7.
3. **Proposal — this section.**
4. **Preflight audit (§14) — gate below. Do not dispatch until every box passes.**
5. Execution — subagent-driven, sequential.
6. Postflight (§14) — compare implementation against this proposal only.

### §1 planning checklist — answered, not deferred to the implementer

**Data flow.** Today: `RandomizePage`/`RandomizeAll` -> `detail::PressWithModifier(..., RandomMod)`
-> Sheaf `Bank::HandlePress` -> private `Bank::RandomizeModulationDepths` -> per-source
`RandomizeVisibleValue`. After: the app computes {count, distinct source set} and calls
`Parameter::EnsureModulationDepth(modIx)` then `RandomizeVisibleValue(manager.Scene(), ...)` per
chosen source. **Sheaf still performs every write** — this is D14's split, not a violation of it.

**Reuse / every definition site (OMNI §1 — enumerate ALL, not the first found).** There are
**four** `Modifier::RandomMod` dispatch sites in `app/FroggersModulation.hpp`, and the current
count is easy to re-check with
`grep -n "Modifier::RandomMod" app/FroggersModulation.hpp`:
  1. `RandomizeBankLevel1Depths` (per-parameter loop, driven by Randomize All)
  2. `RandomizePage` at drill-in level 1/2
  3. `RandomizeAll` at drill-in level 1
  4. `RandomizeAll` at drill-in level 2
**All four take the new distribution** — it is one conceptual operation and a shared helper is
mandatory here (§6: reused 2+ times, isolates a distinct transformation stage). A fix applied to
only the Page path would leave two behaviours for the same gesture, which is the duplication §8
forbids. Re-enumerate before starting; do not trust this list's line numbers.

**Dependencies (all public, all verified in A6).** `ParameterManager::Scene()`,
`::NextRandomCoin/NextRandomIndex/NextRandomValue`, `Parameter::EnsureModulationDepth`,
`::RandomizeVisibleValue`, `ParameterGroup::GetModulators().Metadata()`.

**Efficiency (§10/§11).** The eligible-source vector is O(15) per call and Randomize All makes 54
calls — trivial, but `reserve()` it and build it once per call rather than per chosen source. Do
not cache it across calls: `connected` changes at runtime.

**Structure depth (§5).** Shallow — one helper, one loop. No nesting concern.

**Defensive code (§12).** Guard `eligible.size() < 5` before the tail branch because it is
genuinely reachable (external audio off leaves 13, but a future config could leave fewer). Do NOT
add a guard for `connectedCount == 0` beyond the early return that already exists — that path is
real, unlike the divisor clamps §2.6 refused.

### Preflight gate — all must hold before any dispatch

- [x] The four call sites re-enumerated against the current file, not this list.
- [x] The distribution table in A6 reproduced in the brief verbatim (median 3, `P(1)=P(>=5)`,
      30/30 tie, geometric r=0.7 tail) — an implementer must not re-derive it.
- [x] Brief states explicitly that parameter-VALUE randomization is OUT of scope and must not be
      touched.
- [x] Brief forbids reimplementing `RandomizeVisibleValue` (D14).

**Executed. See `git log d6298f2` — "Fix modulation randomize distribution and drill-in
navigation", 2026-07-30. Full suite green, 156 tests across 10 binaries.**

### Execution rules for these dispatches

- **Sequential only** — operator directive; no parallel implementation agents.
- Model set explicitly to Sonnet or Haiku (§4, §15). Never Opus.
- **Foreground builds.** Five agents this session parked on backgrounded builds; two left the tree
  in a state their report did not describe.
- **Check the tree, not the report** (§14 postflight): after every dispatch, verify the claimed
  edits exist before believing counts.
- E.1 and E.2/E.3 touch different files (`FroggersModulation.hpp` vs
  `FroggersModulation.hpp` + `FroggersAppCore.hpp`) but E.1 and E.2 share a file — so run them in
  separate dispatches, E.1 first.
- **`make test` has no `-k`** and aborts at the first failing binary of ten; count that all ten ran.

### Postflight

Compare against this proposal only (§14 — do not reinterpret intent). Specifically: does the
implemented distribution match A6's table, did all four call sites change, is value-randomization
untouched, and did the suite run all ten binaries.

---

## §E — DONE, committed `d6298f2` (2026-07-30)

Analysis in `design.md` A6/A7. Implemented per the §E-PLAN proposal above. An earlier attempt at
E.1 was stopped mid-edit for skipping the proposal step (OMNI §7/§13 — scattered call-site edits
with no written analysis behind them); the dead helper it produced was removed before this
implementation started.

- [x] E.1 **Randomize-depth count distribution (design A6).** Implemented in
  `detail::RandomizeParameterModulationDepths` (`app/FroggersModulation.hpp:798-880`), the exact
  10/30/30/20 + geometric-r=0.7-tail table, used by all four `Modifier::RandomMod` call sites.
  Distinct-source draws via partial Fisher-Yates (fixes Sheaf's own same-source-twice bug).
  Parameter-VALUE randomization confirmed untouched. Tests:
  `randomize_depth_helper_median_count_is_three_across_1000_trials`,
  `randomize_depth_helper_draws_distinct_sources_even_from_an_adversarial_index_feed`,
  `randomize_page_mod_detail_is_never_a_no_op_across_500_trials` (`app/FroggersModulationTests.cpp`).
- [x] E.2 **Back from level 2 returns to level 1, not level 0 (design A7a).** Implemented in
  `FroggersModulationDrillIn::Back()` (`app/FroggersModulation.hpp:712-729`) — remembers
  `level1Encoder_` and re-opens it after a full `Deselect()`. Fixing this surfaced a latent bug in
  `RandomizeAll`'s own hand-rolled level-2 exit-and-reopen (written when `Back()` was always a full
  exit); removed, since one `Back()` call now does both. Test:
  `back_from_level_two_returns_to_the_same_level_one_parameter_then_back_again_exits_to_grid`
  (`app/FroggersModulationTests.cpp:250`).
- [x] E.3 **Clicking the active bank exits a drilldown (design A7b).** Implemented in
  `FroggersAppCore.hpp:415-442` — resets the drill-in via `Back()`-until-zero when the bank matches
  and level > 0; the pre-existing no-op is preserved at level 0.
- [x] E.4 **Test the navigation, not just the state.** Behavioural tests assert level transitions
  directly — see E.1/E.2 test names above, plus existing coverage in
  `app/FroggersModulationTests.cpp`.

**Commit `d6298f2`, full suite green, 156 tests across 10 binaries.** Also: Randomize All no longer
randomizes local Crispy on any bank (all-six-banks equivalent to randomizing global Crunchy, which
this app never does); Randomize Page still does, since one page's own Crispy is that page's
business.

## §F-PLAN — Sheaf pin bump `1940ddcb` → `77a3019e` (written 2026-08-01)

Folded in from a standalone `MIGRATION-PROPOSAL-sheaf-77a3019e.md` on 2026-08-02. **Commit
`6701fc1`'s message references that path; the content is here now.** It was written as a separate
document, which was a mistake: OMNI §2 generates OpenSpec once per task scope and §3 makes the
proposal the single artifact an executor runs, so three parallel proposal documents left the
execution order recorded nowhere but chat. §E-PLAN was already the right pattern and this section
follows it.

The OpenSpec layer is `design.md` plus the RE-CHECK section of `/UPSTREAM-SHEAF-ASK.md`, which
re-traced all 11 upstream asks against the new pin with file:line evidence. This is the proposal
layer.

### §1 planning checklist — answered, not deferred to the implementer

Claims cite `file:line` at the stated revision (OMNI §1). Sheaf paths are relative to
`External/Sheaf/projects/synth/`.

**Data flow — the complete break surface.** 21 errors, 3 files, **17 edit sites**:

| # | Error kind | Count | Sites | Real or cascade |
|---|---|---|---|---|
| 1 | `no template named 'ScopeVisualizer'` | 1 | `FroggersAppCore.hpp:1470` | **real** — one missing include |
| 2-7 | Builder methods need trailing `ControlStyle` | 14 | `FroggersUiSurface.hpp` — `Button` 756/757/850/858/993/1014, `Visualizer` 768/901, `Label` 1049/1107, `Slider` 1051/1106, `StatusText` 1064, `DrawInteractive` 918 | real |
| 8 | `Build()` missing `rootExtent` | 1 | `FroggersUiSurface.hpp:807` | real |
| 9 | `FroggersApp`→`FroggersAppCore` / concept | 4 | `Froggers.hpp:45, 49, 58`, `FroggersHeadlessTests.cpp:73` | **cascade of #1 — zero edits** |
| 21 | `Node::variant` retired | 1 | `FroggersSurfaceTests.cpp:898` | real — found during execution |

**Error 9 is a cascade, traced not assumed.** `vcoScopeVisualizer_` is a *member declaration* of
`FroggersAppCore` (`FroggersAppCore.hpp:1470`); an unresolved type there invalidates the class, so
`FroggersApp final : public FroggersAppCore` (`Froggers.hpp:42`) has a broken base and the
`SynthApplication` static_assert fails with it. A build agent reported this as an unlisted
"app composition vs inheritance" API change and said it found no link to error 1 — **that was
wrong**; clang does not cross-reference a broken base class. There is no app-composition change.

**Error 1 is a header move, not an API change.** `ScopeVisualizer` was at
`include/synth/PortableUIBuilders.hpp:225-238` (`1940ddcb`) and is the **same template with a
byte-identical constructor** at `include/synth/PortableScopeVisualizer.hpp:222-229` (`77a3019e`).
The app got it transitively; it now needs the header named.

**ERRATA — two enumeration defects, both mine, recorded so the next one is cheaper.**
1. A first pass called the surface "truncated at clang's 20-error cap." It was not: the compiler's
   own summary read `4 warnings and 20 errors generated.` with no `too many errors emitted` line.
2. The corrected pass was still incomplete in a *second* dimension. `-ferror-limit=0` exhausts one
   translation unit, but `make test` has no `-k` and aborts at the first failing **binary**, so
   eight of ten test TUs were never compiled when the table was drawn. Execution found the 21st
   error in a 3rd file. A follow-up static sweep for the retired symbols (`.variant`,
   `DrawCommand::Kind::Stroke/Ellipse/RoundedRect`, style-less Builder calls) found no further site.
   **Lesson: "complete" means every TU compiled or swept, not one TU un-capped.** The
   aborts-at-first-binary trap §0 documents for *test runs* applies equally to *error enumeration*.

**Dependencies, all public and verified at `77a3019e`:** `ControlStyle`
(`PortableUIBuilders.hpp:20-33`), `Build(Bounds)` (`:352`), `ScopeVisualizer`
(`PortableScopeVisualizer.hpp:222`); for F.3 additionally `IntrinsicFor(const Node&)`
(`PortableUIMetrics.hpp:36`) and `Extent`/`LayoutOptions`/`AllocateExtents`
(`PortableUILayout.hpp:31-53, :165`) — **all four of those headers are new since `1940ddcb`.**

**Structure/helpers (§5/§6).** F.1 changes call-site arguments only. `ControlStyle{}` is passed
positionally rather than through a wrapper — a wrapper satisfies neither §6 condition 2 nor 3.

**Efficiency (§10/§11).** Not a factor. `Build(rootExtent)` resolves layout once per tree build,
as the pre-bump path did.

**Defensive code (§12).** No guard added around `rootExtent`: the value passed is the surface's own
bounds, held unconditionally. An unreachable branch there is exactly what §12 and
`frogg3rs-dsp-recovery`'s "No unreachable defensive clamps" forbid.

### Preflight gate

- [x] Trace complete, every claim cited to a `file:line` actually read.
- [x] Error surface complete — every TU compiled or swept, not one TU un-capped (see ERRATA).
- [x] Cascades distinguished by tracing the mechanism, not by pattern-matching.
- [x] F.1 forbids smuggling in F.2/F.3 work.
- [x] §A audio safety and parameter-VALUE randomization declared untouched.
- [x] Layout-engine question escalated to the operator, not decided by an implementer.

### Postflight

Compare against this section only (§14). Did the suite run all ten binaries; is §A untouched; is
`External/Sheaf` clean and unpatched; did F.1 change behaviour anywhere.

---

## §F — Sheaf pin bump and migration

**Sequential. F.1 → F.2 → F.3. F.3 rewrites the very call sites F.2 touches, so the order is not
a preference.**

- [x] F.0 **Pin bumped** `1940ddcb` → `77a3019e`, operator-approved, submodule clean and unpatched.
  All 11 upstream asks re-checked against the new pin with file:line evidence — see
  `/UPSTREAM-SHEAF-ASK.md` RE-CHECK. Landed: asks 1 (plain click on `Draw` nodes), 3 (per-node
  colour, delivered as `color`/`textStyle`/`selected` rather than the `TextColourForNode` branch
  asked for), 4 (captioned audio selectors); ask 6 addressed by `ControlStyle::caption`. **Ask 8
  (external audio) did NOT land** — `AppContext`/`AudioBlock` still carry no input-routing signal,
  so `kExternalAudioOptedIn = false` stays.
- [x] F.1 **Restore green, zero behaviour change.** Committed `6701fc1`, **156/156 across ten
  binaries**. Include added to `FroggersAppCore.hpp`; `ControlStyle{}` at 14 sites;
  `Build(root)`; `DrawInteractive` given an explicit `std::nullopt` doubleClickAction preserving
  today's gesture. `WireDrawNodeActions()`/`MarkSelectedBank()` deliberately left in place.
  `FroggersSurfaceTests.cpp`'s `Node::variant` assertion rewritten to pin the same intent
  (nothing recolours the transport glyphs) via `textStyle`, since glyph colour now comes only from
  there. **The pin was rewritten, not deleted** — §0's rule.
- [x] F.2 **Delete the workarounds the bump obsoleted** — done 2026-08-03, suite **156/156 across
  ten binaries**, launcher exit 0. Each had a confirmed-dead cause; a workaround outliving its cause
  is invisible, constrains the design around a limitation that no longer exists, and reads as
  deliberate to the next person.

  **Finding 1 — "delete the workaround" needs EVERY cause dead, not the first one found.**
  F.2d's brief named the BPM label for conversion. Converting it silently reversed **B12**, an
  explicit operator instruction ("the two labels are now deliberately asymmetric — do not 'fix'
  that"): `ControlStyle::caption` always leads its control, and B12 requires this one to trail
  because leading it reads as labelling the scene-blend slider. The implementer executed the brief
  as written and **flagged it rather than absorbing it**, which is the behaviour the brief asked
  for. **The brief was the defect, not the implementation.** Scene-blend keeps the caption (its only
  cause — captions never drawn — is dead); BPM keeps its hand-rolled `Label` (a second cause is
  still live). Filed as upstream ask 14. The guard now pins the **order**, not merely the label's
  existence, or the reversal is invisible again.

  **Finding 2 — `declared_ui_height_matches_the_derived_required_extent` cannot fail.** The brief
  predicted it would break at F.2b. It did not, for a worse reason:
  `FroggersAutoFlowedChromeModel::FlowedControls()` is a build-once `static` list that **hardcodes**
  `{Kind::Button, "▶"}` / `{Kind::Button, "■"}` and never consults the tree `BuildTree()` actually
  produces. It compares two app-side numbers to each other — the exact green-while-wrong shape §0
  names, and the **fourth** in this change's history. After F.2b it is not merely vacuous but
  **false**: Play/Stop are now 28px `Draw` plates while the model still bills them as 72px
  `Button`s, so `config.uiHeight = 632` is no longer derived from anything real. Probably still
  correct (dropping 88px likely does not change the row count) but **unprovable by the suite**.
  Deliberately NOT repaired — F.3 deletes the model, the literal and this test. Repairing machinery
  we are about to remove is the workaround pattern again.
  - [ ] F.2a Encoders to single click — `DrawInteractive`'s `doubleClickAction` becomes
    `ControlStyle::action`; `WireDrawNodeActions`/`SetNodeAction` post-`Build()` patching goes.
  - [ ] F.2b Transport back to draw-command icons — `BuildPlayDrawCommands`/`BuildStopDrawCommands`
    were kept unused in the file for exactly this; the `▶️`/`🟥` emoji and their byte-sequence
    assertions retire with them.
  - [ ] F.2c `MarkSelectedBank` post-`Build()` edit → `ControlStyle::selected`.
  - [ ] F.2d `kSceneBlendLabel`/`kBpmLabel` adjacent `Label` nodes → `ControlStyle::caption`.
  - [ ] F.2e Coloured glyphs via `textStyle` — **this deliberately reverses F.1's rewritten
    assertion.** Expected, not churn: F.1 pinned "no `textStyle` carried" to preserve pre-bump
    behaviour; F.2e is the behaviour change that assertion was holding the line against.
- [ ] F.3 **Adopt the portable layout engine, resolution-independently** (operator chose adoption
  over deferral 2026-08-01, and **option 2 — resolution-independent — over keeping the fixed window,
  2026-08-02**). **Upgraded from cleanup to unusable-until-fixed by the 2026-08-03 walkthrough — see
  the finding below.**

  **WALKTHROUGH FINDING (2026-08-03, operator screenshot; verdict "absolute trash", correctly).**
  The app has rendered broken since F.1, and the suite could not see it. The bump replaced the
  layout *paradigm*, not just signatures: at `1940ddcb` the JUCE backend flowed controls itself
  (greedy row-wrap, `DefaultSizeForNode`, 72px floors — the algorithm `FroggersAutoFlowedChromeModel`
  replicates); at `77a3019e` that algorithm is **gone** — `LayoutControls()`
  (`juce/PortableJuceBackend.hpp:976-994`) only applies engine-resolved bounds, and layout is the
  app's job via per-node `LayoutOptions`. We declare none, so every control gets the defaults
  (`PortableUILayout.hpp:53-55`): `main = Intrinsic()`, `cross = Weight(1.0)` — in a vertical root,
  **full window width**, stacked one per row. Symptom map, each traced:
  - *Bank/randomize/scene buttons as full-width stacked bars* — default in-flow children of a
    vertical root. The full-width blue stripe is the selected bank's `ControlStyle::selected`
    inversion (F.2c) at those bounds.
  - *Scene-blend and BPM sliders full-width, cutting across the grid* — same defaults.
  - *Encoder grid and scope overlapping the flowed controls* — grid/scope are out-of-flow
    (explicit bounds); the flow does not reserve space for out-of-flow nodes, so the stack runs
    underneath them. Two coordinate systems, no arbitration.
  - *Scope painted over Play/Stop, top-left* — F.2b's plates are in-flow 28×28
    (`FroggersUiSurface.hpp:764-765`), so they stack at the top of the vertical flow — the same
    corner the scope's explicit bounds claim.
  - *Not ours*: the numeric text boxes on sliders (ask 5, open) and the unlabelled `15.3%` (ask 7).

  **Process finding, recorded for §0:** F.1's "zero behaviour change" was verified against the node
  tree and the test suite only. Nothing in the suite asserts *resolved geometry* — the one guard
  that claims to is the vacuous one Finding 2 documents — so a total layout collapse rode through
  ten green binaries and three green commits. The operator's eyes were the first geometry assertion
  the project ran since the bump. F.3's acceptance criteria below exist so that is never true again.

  **TOPOLOGY DIRECTIVE (operator, 2026-08-03): the surface is ONE grid.** Verbatim intent: *"sheaf
  is the guide for classes but froggers is the guide for topology."* Old Froggers never had this
  problem because everything lived in a single grid — encoders AND buttons — and that grid took
  real effort to get right. F.3 therefore does not build "a grid plus a flowed chrome band": it
  builds **one X×Y grid that fits encoders and buttons logically**, expressed through Sheaf's
  layout classes. Buttons are grid citizens with cell positions, not flow refugees beneath the
  content. Consequences:
  - The layout *classes* (`LayoutOptions`, `Extent`, containers, whatever the idiom trace says)
    come from Sheaf and are used idiomatically — no hand-rolled resolver, no second
    `FroggersAutoFlowedChromeModel`.
  - The *topology* — which control sits in which cell, what X×Y is, where scope/transport/sliders
    fall relative to the encoder blocks — comes from Froggers, with the frozen firmware's own grid
    as the reference for what "logically" means. Sheaf idiom informs HOW cells are declared, never
    WHERE things go.
  - The pending idiom trace's grid question (does the engine offer a native grid/wrap container,
    or do Sheaf's own apps do grids as explicit bounds?) decides the mechanism; the operator has
    already decided the shape.

  **CELL MAP — operator-approved 2026-08-04, with one amendment: the Scene blend label sits BELOW
  its slider.** This supersedes the F.2d caption for scene-blend (a `ControlStyle::caption` can only
  lead, so scene-blend returns to a hand-rolled label, now placed under the slider). B12 is
  *refined*, not reversed: BPM's label still trails to the right of its slider; the two labels
  remain asymmetric with each other and neither leads. Upstream ask 14 (caption placement) still
  stands.

  Six columns (L1 L2 | E1 E2 E3 E4), six rows. Encoder slots keep firmware order: 0-8 named,
  14/15 = CRIS/CRNC, empties render as visibly empty cells (slots never shift between banks):

  | Row | L1 | L2 | E1 | E2 | E3 | E4 |
  |---|---|---|---|---|---|---|
  | 1 | Scope (spans L1-L2, rows 1-2) | ← | Bank tabs ×6 span E1-E4 | ← | ← | ← |
  | 2 | (scope) | ← | slot 0 | slot 1 | slot 2 | slot 3 |
  | 3 | Play | Stop | slot 4 | slot 5 | slot 6 | slot 7 |
  | 4 | Scene 1 | Scene 2 | slot 8 | slot 9 | slot 10 | slot 11 |
  | 5 | Scene blend slider span 2, **label below** | ← | slot 12 | slot 13 | slot 14 CRIS | slot 15 CRNC |
  | 6 | BPM slider span 2, label trailing right (B12) | ← | Randomize page span 2 | ← | Randomize all span 2 | ← |

  **MECHANISM — from the idiom trace (2026-08-03, all citations at pin `77a3019e`):**
  - `Column`-of-`Row`s with `Extent::Weight(1)` cells is the first-party grid idiom (Braid4
    `EmitBraid4CellGrid`, `Braid4UiModel.hpp:74-135`); there is no native X×Y container. Outer
    `Row` = left block `Weight(2)` | right block `Weight(4)`; spans = weighted cells within a row.
  - **`StandardAppLayout` is NOT used** — it is Braid4's topology, not a neutral scaffold (empty
    second visualizer slot does not collapse, `PortableUIStandardLayout.hpp:89-99`), and our
    topology is the operator's. Classes from Sheaf, topology from Froggers.
  - Encoder visualizer underlays via `overlayOf` — deferred resolution copies the target cell's
    final bounds (`PortableUILayout.hpp:672-683, 743-752`); the idiom Braid4 already uses.
  - Explicit bounds are **parent-relative** (`:596`; backend translates to absolute,
    `juce/PortableJuceBackend.hpp:738-756`). Goal state: zero explicit-bounds nodes except at most
    a Braid4-style background paint.
  - Overflow **throws** (`RequireContainerHoldsItsChildren`, `:267-316`) naming container and
    child; fit tests wrap `Build()` in try/catch at multiple sizes — the pattern Braid4's system
    tests already use (`tests/braid4_system_tests.cpp:476-485`).
  - Resize: backend `resized()` only re-applies stale bounds; a real relayout is
    `RefreshFromSurface()` fed a live extent (`juce/PortableJuceBackend.hpp:226-233`;
    host-wiring example `juce/ControllersHarnessApp.cpp:56-70`). Braid4 never wired this —
    **we do, deliberately, in `FroggersMain.cpp`, per option 2.**

  **PREFLIGHT AUDIT (OMNI §14, run 2026-08-04 at operator request — result: REJECTED pending the
  fixes below; implementation may not dispatch until the gate at the bottom passes).**
  1. *§1 violation, resize path asserted not traced.* "Wire live resize in `FroggersMain`" cites a
     harness example, not OUR stack (`FroggersMain → RuntimeSessionOwner → session->Component()`).
     Whether `synth::ui::Surface` exposes `SetContentBounds`, and where `RefreshFromSurface` is
     reachable from our session owner, was never read. → resolved by the enumeration below before
     dispatch.
  2. *§1 violation, deletion set sampled not enumerated.* Every geometry artifact in the surface
     needs a die / survive-as-data verdict with file:line: `ScopeArea`/`GridArea`,
     `kContentAreaHeight`, `RootBounds()`, `RequiredHeight()`, `FroggersEncoderGridLayout`
     (bounds arithmetic dies, slot→cell data survives), every explicit-bounds site in
     `BuildTree()`, and every one of the 19 surface tests that pins current geometry. → same
     enumeration.
  3. *Precision.* `config.uiHeight` is NOT deleted — the window needs an initial size.
     What dies is its load-bearing derivation: the `= RequiredHeight()` claim, the cross-check
     test, and the circular-include workaround. The literal survives demoted to "initial window
     size."
  4. *§8.* Emission LOOPS over the cell map — 6 bank tabs and 16 encoder cells are loops over the
     one data table, never 22 hand-written builder calls. The cell map is a single definition
     site; a second copy in the emitter is the duplication §8 forbids.
  5. *§12.* Fit-test sizes pinned here, not chosen by the implementer: 900×632 (default),
     640×480 (small), 1440×900 (large). Overflow-throw is the only mechanism; no invented guards.

  **Preflight gate — PASSED 2026-08-05, all five findings closed. Implementation may dispatch.**
  - [x] Resize path traced through OUR runtime stack (finding 1) — and it surfaced a blocker plus
        two corrections; see RESIZE TRACE, FINAL below.
  - [x] Deletion/survival table complete: every geometry helper, constant, explicit-bounds site,
        and all 19 surface tests enumerated with individual verdicts (finding 2).
  - [x] uiHeight demotion stated precisely (finding 3); loop-over-map rule stated (finding 4);
        fit sizes pinned at 900×632 / 640×480 / 1440×900 (finding 5).
  - [x] Constraints restated for the brief: no `External/Sheaf` edits, no §A audio safety, no
        parameter-value randomization, no frozen trees; foreground `nice make -j2`; Sonnet;
        sequential.

  **OPERATOR DECISION 2026-08-05: route 2a + file the upstream ask as a GitHub issue.** Build the
  declared grid now against the `Config()`-sized region; file ask 15 with jvictor0 for shell-level
  resolution independence. Window reflow revisits when that lands — and because the internal layout
  is fully declarative, adopting a live extent is then a change to `RootBounds()`'s source, not a
  redesign.

  **TOPOLOGY TRACE RESULT (2026-08-04) — cell map holds, one assertion refined, two brief-changing
  findings.** Verified: `kFroggersSlotsPerBank = 16`, `kFroggersBankCount = 6`
  (`FroggersParameters.hpp:76-78`); 9 named params at slots 0-8 in **all six** banks
  (`:77,145-186,310-340`); slots **9-13 deliberately empty** in every bank
  (`FroggersModulation.hpp:187-191`); banks are Audio=Red, Envelope=Green, Filter=Blue,
  Drive=Orange, Delay=Indigo, Reverb=Cyan (`FroggersParameters.hpp:147-183`). So the 4×4 encoder
  region maps rows 2-5 to slots 0-3 / 4-7 / 8-11 / 12-15 exactly as approved, with 9-13 rendering
  empty.

  - **REFINED — Crunchy is GLOBAL, not per-bank.** CRIS (slot 14, `kFroggersCrispySlot`) is
    per-bank and takes the bank's colour. CRNC (slot 15, `kFroggersCrunchySlot`) is **one shared
    `Parameter` object aliased into all six banks' slot 15**, with its own fixed Yellow
    (`FroggersParameters.hpp:79-80,191,251-256,342-366`) — which is why it renders yellow in the
    operator screenshot. The cell map's positions were right; the sketch's implication that CRNC is
    an ordinary bank-coloured encoder was wrong. **Its cell must keep the distinct global colour**,
    and it is excluded from drill-in/randomize dispatch (`FroggersModulation.hpp:120-126`).
  - **BRIEF-CHANGING (§8) — the emitter must NOT re-derive slot→parameter.** Loop `encoderId`
    0..15 and call `bank.VisibleParameter(encoderId)`, the sanctioned parameter-or-empty accessor
    over the full 16-wide layout (`FroggersModulation.hpp:202-204`, rationale `:187-201`).
    `FroggersBankLayouts()`/`PageParameter()`/`Crispy()`/`Crunchy()` are **construction-time**
    accessors and do not reflect drill-in substitution — using them would create a second
    definition site AND render stale contents.
  - **BRIEF-CHANGING — drill-in reuses the same 16 cells.** At modulation level 1/2 the identical
    physical layout is repopulated with modulator depths rather than page parameters
    (`FroggersModulation.hpp:192-204`); slot **count and positions are unchanged, contents differ
    entirely**. The grid therefore emits one 16-cell region in every mode — no second topology, no
    conditional cell count. This is why the accessor above is load-bearing.

  **TEST ENUMERATION (2026-08-04) — all 19 surface tests classified. 12 unaffected, 6 rewritten,
  1 dies.** This closes preflight finding 2's test half. Every rewrite keeps pinning its ORIGINAL
  property against the new declared layout — §0's rule that a pin is rewritten, never deleted.

  | Test (`FroggersSurfaceTests.cpp`) | Verdict |
  |---|---|
  | `root_and_content_bounds_match_default_config_size` :137 | REWRITE — pins pixel equality against dying `RequiredHeight()`; re-pin as "content inset within root by nonzero margin" |
  | `scope_and_grid_regions_do_not_overlap_at_target_window_size` :148 | REWRITE — invariant survives verbatim (no overlap, both inside content); mechanism moves to resolved cell rects |
  | `scope_area_is_wider_than_tall_and_at_most_a_third_of_its_old_area` :170 | REWRITE — **operator requirement, survives as data**; re-pin against resolved layout |
  | `scope_sits_in_a_left_column_with_the_grid_to_its_right` :194 | REWRITE — **the single most important guard here**; keeps asserting scope-left / grid-right / grid full height |
  | `every_encoder_cell_lies_fully_inside_the_grid_region` :216 | REWRITE — 16 cells, no overlap, fully contained; 4×4 topology is unchanged input |
  | `declared_ui_height_matches_the_derived_required_extent` :241 | **DIES WITH ITS SUBJECT** — `ComputeFlowExtent`/`RequiredHeight` are deleted; nothing left to cross-check. Its replacement is the fit-at-three-sizes overflow test, NOT a rewritten version of itself |
  | `play_and_stop_controls_exist_and_gate_the_transport` :865 | PARTIAL REWRITE — only the two `bounds.width/height == kTransportPlateSize` assertions (:893-894, :906-907) need re-verification against the new resolver; draw-command kind/colour/action and audio-gating assertions are untouched |
  | remaining 12 | UNAFFECTED — node kind / id / action / ordering / audio-state only, no `Bounds` reads |

  Note the asymmetry deliberately: the height cross-check **dies without a rewritten successor**
  because it never tested anything real (Finding 2 above). Its *function* — proving the surface
  fits — is taken over by the pinned-size overflow tests, which is a strictly stronger guarantee.

  **RESIZE-PATH TRACE (2026-08-04) — closes preflight finding 1. Option 2 is ACHIEVABLE and follows
  the Sheaf idiom, but it is NOT free wiring; it needs a named app-side addition.**

  | Fact | Evidence |
  |---|---|
  | `synth::ui::Surface` is a 4-method pure interface — `BuildTree`, `SetActionHandler`, `DispatchAction`, dtor. **No extent setter of any kind.** | `include/synth/PortableUI.hpp:280-291` |
  | Sheaf's own concrete surfaces each declare their OWN `SetContentBounds(ui::Bounds)` — it is a per-surface convention, not a base-interface method | `RuntimePages.hpp:1350,1397,1462`; `ControllersPageUI.hpp:932` |
  | `PortableComponent::RefreshFromSurface()` is **public**, and does the full rebuild: `BuildTree` → `ResolveTree` → `RebuildControls` → `LayoutControls` → `repaint` | `juce/PortableJuceBackend.hpp:214,226-233` |
  | `PortableComponent::resized()` calls **only** `LayoutControls()` — re-applies already-resolved (stale) bounds; it never re-resolves | `juce/PortableJuceBackend.hpp:256-259` |
  | `PortableComponent` is **not** a `juce::Timer`; there is no periodic rebuild inside the backend | class decl `:212-233`, no Timer base, no `startTimer` in file |
  | The established host idiom is: owner's `resized()` → set renderer bounds → `RefreshFromSurface()` | `juce/RuntimePagesJuce.hpp:50-54`; live-extent variant `juce/ControllersHarnessApp.cpp:57-70` (`surface_.SetContentBounds(...)` then `renderer_.RefreshFromSurface()`) |
  | No public component-traversal helper — `CollectPortableComponents` is **test-only** | no non-test definition in `include/`, `juce/`, `src/` |

  **CORRECTION to an earlier claim in this plan's discussion:** it was stated that the app "rebuilds
  at 30 Hz anyway," so resize would come along for free. **That is wrong.** `config.uiFrameHz` is
  consumed only by `Engine` to size the *audio-thread UI-state publish interval*
  (`include/synth/Engine.hpp:278,289-292`); it drives no GUI tree rebuild. Nothing rebuilds the tree
  periodically. A resize therefore requires an explicit refresh — the assumption would have produced
  a design that silently never reflowed.

  **What option 2 consequently requires (all app-side, no Sheaf patch):**
  1. `FroggersUiSurface` gains its own `SetContentBounds(ui::Bounds)` storing the live extent —
     exactly the convention Sheaf's own surfaces use, since the base interface carries none.
  2. `FroggersPageLayout::RootBounds()` returns that stored extent instead of reading fixed
     `context->config->uiWidth/uiHeight`. `config.uiHeight` survives only as the INITIAL size.
  3. Something owning the renderer must, on resize, call (1) then `RefreshFromSurface()`.
     **OPEN SUB-QUESTION (traced separately): which host owns our app's `PortableComponent`, and
     does its `resized()` already call `RefreshFromSurface()`?** Every host in
     `RuntimePagesJuce.hpp` does. If ours does too, step 3 is nearly free and only the extent must
     reach the surface. If not, `FroggersMain` must hand-roll a `juce::Component` child-walk to
     find the renderer, since no public traversal helper exists — ~10 lines of our code reaching
     through Sheaf's shell composite, which is more fragile and should be called out as such.

  **DELETION / SURVIVAL TABLE (closes preflight finding 2's non-test half).** Verdicts are per-item,
  and the recurring split matters: most helpers **die as pixel arithmetic while their intent
  survives as data**. An implementer deleting the whole struct would take the topology with it.

  | `FroggersUiSurface.hpp` | Verdict |
  |---|---|
  | `kControlGap/Margin`, `kDefaultButton*`, `kDefaultSlider*`, `kDefaultLabelHeight` :171-177 | DIES |
  | `ButtonWidth` :193-196, `LabelLikeWidth` :205-208, `ControlWidth/Height` :210-232 | DIES |
  | `FlowedControls()` :263-288 | **SURVIVES AS DATA** — control identity and order |
  | `FlowExtent`/`ComputeFlowExtent` :290-326 | DIES |
  | `kMargin` :341, `kGap` :342 | SURVIVES AS DATA (design tokens); literal arithmetic DIES |
  | `kScopeHeight` :370, `kScopeWidth` :375 | **SURVIVES AS DATA — operator-mandated scope ratio** |
  | `kContentAreaHeight` :401 | SURVIVES AS DATA (content budget) |
  | `kAutoFlowedChromeGap` :410, `RequiredHeight()` :420-423, `ContentArea()` :441-449 | DIES |
  | `RootBounds()` :425-433 | Pixel math DIES; config-driven sizing SURVIVES (now live-extent, see above) |
  | `ScopeArea()` :514-521 | Pixel math DIES; "scope = left column at that ratio" SURVIVES AS DATA |
  | `GridArea()` :530-538 | Pixel math DIES; "grid = right column, full height, gap right of scope" SURVIVES AS DATA |
  | `kColumns=4/kRows=4/kEncoderCount=16` :547-549 | **SURVIVES AS DATA** — slot topology, static_assert-tied to `kFroggersSlotsPerBank` :568-569 |
  | `FroggersEncoderGridLayout::kGap` :550 | SURVIVES AS DATA (token); division DIES |
  | `BoundsForIndex()` :552-565 | Pixel division DIES; **row/col mapping (`ix/kColumns`, `ix%kColumns`) SURVIVES AS DATA** |
  | `kTransportPlateColor` :600 | SURVIVES (colour, not geometry) |
  | `kTransportIconFraction` :601 | DIES — intra-node inset fraction. UNCLEAR whether declared layout reaches inside Draw command factories at all |
  | `kTransportPlateSize` :602 | SURVIVES AS DATA — already expressed as `Extent::Px(28)` at :758-759,764-765 |
  | `BuildPlay/StopDrawCommands` bodies :604-638 | Inset arithmetic DIES; signatures + plate/icon command structure SURVIVE |

  | Explicit-bounds call sites | Verdict |
  |---|---|
  | `RootBounds(context_)` :720, `ContentArea(root)` :721, `builder.Root(kRoot, root)` :724 | DIES |
  | `playStyle/stopStyle .layout.main/cross = Extent::Px(...)` :758-759,764-765; `builder.Draw(kPlay/kStop, factory, style)` :760,766 | **SURVIVE — already LayoutOptions-shaped, no explicit Bounds** |
  | `ScopeArea(content)` :773, `vcoScope.SetBounds(scopeArea)` :776 | DIES |
  | `GridArea(content)` :786, `AppendEncoderGrid(builder, gridArea)` :797 | DIES |
  | `BoundsForIndex(gridArea, ix)` :901, `visualizer->SetBounds(cellBounds)` :908 | DIES — visualizer placement becomes `overlayOf` |
  | `builder.Draw(encoderId, cellBounds, ..., style)` :925 | DIES — explicit-Bounds `Draw` overload replaced by in-flow cell |
  | `builder.Build(root)` :814 | Computed-root argument DIES; the `Build(extent)` entry point survives, now fed the live extent |
  | `Button`/`Slider`/`StatusText`/`Label` calls :857,866,942,963,999,1013,1056,1058 | **UNAFFECTED — none passes explicit Bounds** |

  **RESIZE TRACE, FINAL — OPTION 2 IS NOT ACHIEVABLE AT THIS PIN. Operator decision required.**
  Two corrections and one blocker, all traced 2026-08-04.

  **Correction 1 (reversing this plan's own earlier correction).** The claim "the app rebuilds at
  30 Hz" was marked false above. **It is actually TRUE** — just not via the path first assumed.
  `Runtime<App>::timerCallback()` (`runtime/Runtime.hpp:718-722`) fires `repaintHook_`, wired to
  `ShellComponent::RepaintAll` (`runtime/Shell.hpp:88,60-63`) → `MainPane::RefreshOnTick()` →
  `renderer_.RefreshFromSurface()` (`runtime/MainPane.hpp:60-64`), at
  `startTimerHz(config.uiFrameHz ? : 30)` (`Runtime.hpp:299`). So `uiFrameHz` drives BOTH Engine's
  publish interval AND this UI timer. The tree is rebuilt 30×/sec.

  **Correction 2 — no refresh wiring is needed at all.** `MainPane<App>::resized()`
  (`runtime/MainPane.hpp:66-70`) already does `renderer_.setBounds(getLocalBounds())` then
  `renderer_.RefreshFromSurface()`, and `ShellComponent::resized()` (`Shell.hpp:65`) propagates to
  it. The earlier plan step "wire live resize in `FroggersMain`" is unnecessary — and would have
  been impossible anyway: `RuntimeSessionOwner` exposes only `juce::Component&`
  (`Shell.hpp:110-114,121`), `MainPane` has no public accessor for `renderer_`, and the traversal
  helper is test-only. It would have required a `dynamic_cast` + hand-rolled child walk.

  **THE BLOCKER.** Our surface is **embedded, not top-level**, and the shell composes by
  *node-bounds arithmetic on an already-resolved app tree* — not by resolving our region:
  `RuntimeMainComponent::BuildTree()` (`include/synth/RuntimeMainComponent.hpp:110-140`) calls
  `app_.PortableSurface().BuildTree()` (`:112`, our tree, already resolved by our own
  `Build(root)`), then places the sidebar with
  **`sidebarTree.nodes.front().bounds.x = static_cast<float>(App::Config().uiWidth);`** (`:118`).
  `App::Config()` is a static returning a compiled-in `uiWidth = 900`
  (`app/FroggersAppCore.hpp:185`). **The sidebar is therefore pinned at x=900 forever.** If our
  surface resolved against a live window extent, the app region and the sidebar would desync — the
  sidebar would sit at 900 while our content claimed the full width. This also explains the
  screenshot's large dead area: the renderer takes the whole window while the composed tree stays
  900×632.

  **Consequence: the window-reflow half of option 2 requires an upstream change** (shell composing
  from a live extent rather than `App::Config()`), filed as upstream ask 15. **Everything else in
  F.3 is unaffected and still fixes the actual defect** — the single declared grid, no overlap,
  intrinsic-width buttons, cells flexing within the region.

  **Operator decision (do not choose this for them):**
  - **2a — proceed now, fixed region.** Build the declared grid resolving against the region size
    from `Config()` (as today). Fixes the visible disaster; window stays effectively fixed-size;
    internal layout is fully declarative so a live extent becomes a one-line change the day the
    shell supports it. Fit tests still run at three sizes by feeding `RootBounds` directly.
  - **2b — proceed and pursue upstream.** 2a plus filing ask 15 and revisiting when it lands.
  - **2c — wait.** Leave the app broken until upstream lands. **Not recommended:** the app is
    currently unusable, and 2a's work is required under every outcome.

  **Acceptance criteria, from the screenshot (all operator-confirmable by looking):**
  1. Buttons are intrinsic-width controls in compact rows — nothing stretches to window width by
     default.
  2. Nothing overlaps: scope, transport, grid, chrome, and sliders each occupy distinct regions;
     the flow reserves space such that out-of-flow content never collides with flowed content.
  3. Scope keeps its own region (wider than tall, left column, grid to its right — the standing
     guard, now pinning *position*), with Play/Stop adjacent to it, not under it.
  4. Sliders are bounded-width controls with their labels adjacent (B12 asymmetry preserved), not
     window-spanning strips through the grid.
  5. Resizing the window reflows without overlap at multiple widths — the §14-gate fit assertions
     run at more than one size.

  **The decision, and what it commits us to.** The surface stops declaring a compiled-in size and
  resolves against the actual window instead. That deletes four coupled artifacts, not one:
  1. `FroggersAutoFlowedChromeModel` — the ~200-line replica.
  2. `config.uiHeight = 632` (`FroggersAppCore.hpp:211`), a hand-maintained literal which the new
     `Build(rootExtent)` rationale names directly as *"the compiled-in surface size sru-50
     forbids"*.
  3. `declared_ui_height_matches_the_derived_required_extent`, the test that exists only to
     hand-check 1 against 2.
  4. The circular-include workaround: the literal *cannot* call `RequiredHeight()` because
     `FroggersUiSurface.hpp` includes `FroggersAppCore.hpp` (`FroggersAppCore.hpp:198-201`).

  Keeping the fixed window would have deleted the replica while leaving all three artifacts that
  exist **only** to compensate for it — the workaround-outliving-its-cause pattern this change
  keeps rediscovering.

  **This is a deliberate, operator-approved user-visible change** (§0 requires it be proposed, and
  it was): the window becomes genuinely resizable and the chrome reflows. Both entry points already
  call `setResizable(true, true)` (`FroggersMain.cpp:73`, sheaf-patch `Main.cpp:74`), so the
  affordance is present today and simply does nothing — this makes it real rather than adding it.

  **Guards this must not ship without.** Deleting the height cross-check removes the only current
  assertion about vertical extent, so it is replaced, not dropped: the surface must be asserted to
  fit its root at more than one width, and
  `scope_sits_in_a_left_column_with_the_grid_to_its_right` must be re-proven and must **pin
  position**, per §0's lesson that three tests here were green while wrong.
  `PortableUILayout.hpp:246` treats overflow as an error — use it rather than inventing a check.
  Delete `FroggersAutoFlowedChromeModel` (`FroggersUiSurface.hpp:140-152`+), a ~200-line app-side
  replica of `PortableJuceBackend.hpp`'s sizing and greedy-wrap rules, and express the chrome band
  with `LayoutOptions`/`Extent` resolved by `Build(rootExtent)`.
  **Its stated justification has expired:** the replica exists because the app is portable code with
  no JUCE dependency *"and Sheaf exposes no 'measured auto-flow extent' accessor."*
  `PortableUIMetrics.hpp` now provides exactly that, portable and JUCE-free.
  - Its per-constant citations into `PortableJuceBackend.hpp` line ranges are ~424 commits stale.
    If any part of the replica survives, **those citations get re-verified, not assumed.**
  - Touches the geometry that already shipped one regression, so
    `scope_sits_in_a_left_column_with_the_grid_to_its_right` must be re-proven, and the guard must
    pin position — §0's lesson that three tests here were green while wrong.
  - Unblocks D.6.

- [ ] F.4 **Second pin bump, `77a3019e` → `508d9d68`** (27 commits, operator-reported 2026-08-03 and
  verified the same day — see `/UPSTREAM-SHEAF-ASK.md` SECOND RE-CHECK). All 27 are the upstream
  `synth audio input` change plus a demo app.
  - **Sequenced after F.3 deliberately.** F.3 re-architects the surface against a toolkit we have
    already migrated to and proven green; bumping underneath it would mean any F.3 breakage could
    be either our layout work or an unmigrated API change, with no way to tell. That
    unattributability is precisely what F.1 was created to prevent, and the lesson cost a session
    the last time it was skipped.
  - **Same compile-then-behaviour split as F.1/F.2.** F.4 restores green with zero behaviour change;
    F.5 is the behaviour. Public headers that moved: `AppContext.hpp` (+116), `Engine.hpp`,
    `RuntimeMainComponent.hpp`, `RuntimePages.hpp` (+31), and the browser runtime headers. Expect
    `ValidateRuntimeConfig` to now throw on a negative `numAudioInputs`, and `Engine` to assert
    `block.numRequestedInputChannels == config_.numAudioInputs`.
  - **Enumerate the error surface across EVERY translation unit**, not one un-capped TU — §F-PLAN's
    ERRATA records that mistake being made twice.
- [ ] F.5 **Remove the external-audio workaround — ask 8's cause is finally dead.** Strictly after
  F.4; the API does not exist at our current pin.
  - `constexpr bool kExternalAudioOptedIn = false` (`app/FroggersAppCore.hpp:507`) goes, along with
    the raw `externalInputHasChannel` test at `:508-509` — `block.inputs != nullptr &&
    block.numInputChannels > 0 && block.inputs[0] != nullptr` is exactly the "channel exists"
    question that could never answer "is anything routed". Replace with
    `block.InputView().HasActiveChannel(0)`.
  - `RuntimeConfig::numAudioInputs` must actually be **requested** (it defaults to 0), or
    `ActiveChannelCount()` is 0 forever and the workaround is merely reimplemented.
  - The original comment (`:504-506`) promised this would be "a one-line change rather than a
    rewrite" because the channel plumbing was left intact. **Verify that promise rather than trusting
    it** — it was written at a pin where the replacement API did not exist.
  - **Operator-perceptible**: source #6 becomes a live modulation source that participates in
    randomization. It was disabled because a phantom source steals randomization slots; confirm with
    the operator that it now behaves, and that selecting no input device still leaves it dark.

## §F.6-PLAN — slider widths diverge because two emitters do one job (2026-08-05)

Operator, on the F.3 build: *"the scene slider is too wide and the bpm slider is too narrow. grid
design fail."* Correct on both counts, and the cause is structural rather than a bad constant.

### §1 trace — root cause, cited

Neither slider declares a width. Both pass a bare `synth::ui::ControlStyle{}`, so `layout` is
default-constructed (`AppendSceneBlendGroup`, `AppendBpmControl`, `app/FroggersUiSurface.hpp`).
The divergence comes entirely from **container kind**:

- `AppendSceneBlendGroup` emits `builder.Column(kSceneBlendGroup, ...)` holding `[Slider, Label]`.
  In a Column the horizontal axis is the CROSS axis, so the slider fills the full left-block width.
- `AppendBpmGroup` emits `builder.Row(kBpmGroup, ...)` holding `[Slider, Label]` (via
  `AppendBpmControl`). In a Row the horizontal axis is the MAIN axis, so slider and label **split**
  it — the slider gets roughly half.

So slider width is a side effect of where each label sits, not a declared property. **Both
emitters are structurally identical otherwise** — same `groupLayout` fields (`main =
Weight(rowWeight)`, `cross = Weight(1)`, `padding = 0`, `gap = kGap`), then a container holding a
slider and its label.

**Definition sites, enumerated (OMNI §1 — all, not the first found):** exactly two labelled
sliders exist in the surface — Scene blend and BPM. There is no third. The encoder cells, bank
tabs, transport and scene buttons carry no adjacent label node.

### The OMNI §8 reading, which decides the design

Two structurally near-identical emitters doing one conceptual job is the sequential duplication
§8 forbids: *"if 2+ occurrences → must be looped, abstracted, or vectorized."* And it is the
**direct cause of the defect** — the widths differ precisely because two hand-written emitters can
drift where one parameterized emitter cannot. Any fix that leaves both emitters in place fixes the
symptom and preserves the mechanism.

A shared emitter also clears OMNI §6 comfortably — reused 2+ times (cond. 1), isolates a distinct
transformation stage (cond. 2), prevents repetition of structurally similar code (cond. 4): **3 of
4**, where 2 suffices.

### Options

**F.6-A — one `AppendLabelledSlider(builder, spec)` emitter, placement as a parameter.
RECOMMENDED.** One function emits both rows; a `LabelPlacement` field (`Below` / `Trailing`)
selects the container. **The slider's own `LayoutOptions` are declared once, inside that emitter**,
so both sliders are the same width *by construction* — not by two constants kept in agreement.
Preserves both standing instructions unchanged: scene-blend label below (cell-map amendment), BPM
label trailing (B12). Eliminates the §8 duplication that caused this.

**F.6-B — declare each slider's width separately, keep both emitters.** Give the BPM slider
`main = Weight(1)` and its label `main = Intrinsic()`, then give the scene-blend slider a matching
explicit cross extent. **NOT RECOMMENDED:** the two widths would be equal only while a human keeps
two numbers in agreement — the exact hand-synced-geometry defect F.3 just deleted
(`uiHeight == RequiredHeight()`), reintroduced at smaller scale.

**F.6-C — make both labels sit the same way** (both below, or both trailing). Structurally
symmetric and also kills the duplication, but it reverses one of two recorded operator
instructions — B12's trailing BPM, or the cell-map's below scene-blend. **Not proposed as a
default**; if the operator wants uniform placement, F.6-A implements it by changing one enum value
at one call site, which is the point of making placement a parameter.

### OMNI self-review of this proposal (requested by the operator)

- **§1 data flow / trace.** Root cause traced to container kind with the emitters named; both
  definition sites enumerated and confirmed to be the complete set. PASS.
- **§5 structure depth (2-of-4).** F.6-A nests Column-inside-Row-inside-Row at most. Hidden state
  across levels: no. Duplicated transformation logic across branches: **no — that is what it
  removes.** Loss of input→output traceability: no, one emitter, one spec in. Decomposition not
  matching real stages: no, each level is a real grid region. **0 of 4 → nesting valid.** PASS.
- **§6 helper rule (2-of-4).** 3 of 4 as counted above. PASS.
- **§7 full pipeline, not incremental patch.** F.6-A rewrites the whole labelled-slider path;
  F.6-B would be the incremental patch §7 warns against. PASS for A, FAIL for B.
- **§8 repetition.** The whole point. PASS for A and C; **F.6-B FAILS §8** — it leaves two
  emitters and adds two constants to keep in sync.
- **§10/§11 efficiency.** No loop, no repeated allocation, O(1) either way. N/A.
- **§12 defensive code.** No new guards. The existing `externallyClocked` branch in
  `AppendBpmControl` is real (BPM renders as `StatusText` when slaved) and stays — it is a genuine
  state, not an impossible one. PASS.
- **§14 preflight.** Trace complete and cited; definition sites exhaustive; no unread behaviour
  asserted. The one open item is a **question, not an assumption**: what should the shared width
  actually BE? Recorded below rather than guessed.
- **§16.1.** Verification through a subagent, foreground, counts only.

**Open question for the operator, not to be assumed:** F.6-A makes both sliders equal by
construction, but "equal" still needs a value. Candidates: full left-block width minus the BPM
label gutter (both shrink to the current BPM width), or full width with BPM's label wrapping below
its own slider (both grow to the current scene-blend width, but that is F.6-C). **The screenshot
says scene-blend is too wide AND BPM too narrow, which reads as wanting something between the two
— so this needs the operator's answer, not an implementer's pick.**

- [ ] F.6 Implement the approved option. **Blocked on the width answer above.**

## §G-PLAN — direct launch, no app picker (written 2026-08-01)

Folded in from a standalone `PROPOSAL-direct-launch.md` on 2026-08-02, same reason as §F-PLAN.
Operator request: *"make the build launch just the frogg3rs app, not the rest of the sheaf stuff
that i have to click through to get to it."*

**§1 trace.** The picker is unconditional: `apps/sheaf-patch/Main.cpp:35-51` builds the three-app
vector (MiniApp `:38`, Braid4 `:41`, our out-of-tree app `:44-48`) and always constructs
`LauncherComponent`. **No bypass exists** — no CLI argument, no env var, no single-app-skip branch;
`initialise(const juce::String&)` (`:29`) discards its command-line parameter. Verified by reading
the whole selection path, not inferred. Launching is already one call: `LaunchRegisteredApp<App>`
(`:87-99`) builds the session owner, reads `App::Config()`, and shows it.

**Overridable without patching Sheaf.** `runtime/juce_build.mk` consumes `APP_NAME` (`:25-27`),
`APP_SOURCES` (`:149-150`), `APP_BUILD_DIR` (`:24`), `APP_INFO_PLIST` (`:10`, copied verbatim at
`:154`). sheaf-patch's Makefile sets these with `:=` (`Makefile:3-6`), but **make command-line
overrides beat `:=`** — the same mechanism `build-launcher.sh` already uses for `EXTRA_APP_*`.

**The one correctness constraint:** data paths must not move. Launch goes through
`SheafPatchDataPathsForApp(dataRoot, "frogg3rs")` (`include/synth/AppRegistry.hpp:55`) with the
appId read from `FroggersManifest()` (`app/FroggersRegistration.hpp:23`), so patches under
`~/Library/Sheaf/synth/sheaf-patch/` stay put. Getting this wrong silently orphans operator work.

**Accepted duplication (§8, flagged not hidden).** `app/FroggersMain.cpp` is a ~60-line near-copy of
Sheaf's `Main.cpp`, because `MainWindow` and `LaunchRegisteredApp` are private members of
`SheafPatchApplication` and unreachable from outside it. Structural parity with the upstream file is
the maintenance strategy: if Sheaf exposes a direct-launch entry point, this collapses into it.
`LaunchRegisteredApp` is kept as a single-use template on that basis (§6 exception — it names a
domain concept and does not obscure data flow).

## §G — direct launch

- [x] G.1 **Build opens straight into Frogg3rs.** Committed `0b7899d`. `app/FroggersMain.cpp` +
  four make overrides in `app/build-launcher.sh`. Nothing under `External/Sheaf` edited; the picker
  build still works with no overrides. `APP_BUILD_DIR` also moved our artifacts out of the submodule
  into `app/build-launcher/` (gitignored), serving §0's "Sheaf stays clean" and `juce_build.mk:8`'s
  own "must be distinct per app", which two apps sharing one build dir did not meet.
  - **ERRATA:** the first cut named only three overrides and shipped a bundle whose
    `CFBundleExecutable` said `SheafPatch` while the only binary was `Frogg3rs`. LaunchServices
    resolves through the plist, so **a Finder double-click failed** while the build exited 0, the
    bundle existed, the binary ran directly, and the suite passed — invisible to every automated
    check. Planning defect, not implementation: the implementer built what was written, then
    correctly stopped and reported the inconsistency instead of improvising a fourth override.
    Fixed with `app/Frogg3rs-Info.plist` + `APP_INFO_PLIST`; both files carry a comment naming the
    coupling. **The guard that catches it compares the plist key against the `MacOS/` listing**, not
    two app-side constants against each other — §0's green-while-wrong lesson again.
- [ ] G.2 **Blank window on startup failure.** `LaunchRegisteredApp`'s inner `catch`
  (`app/FroggersMain.cpp:96-98`) logs via `INFO` and returns, leaving an empty window. Harmless
  upstream because the picker is still on screen; here there is no fallback, so a startup failure
  presents as a silent blank window. Inherited structure, not introduced — but it should either quit
  with a return value or surface the error. **Behaviour decision, operator's call.**
- [ ] G.3 **Operator confirms an existing saved patch still loads** from
  `~/Library/Sheaf/synth/sheaf-patch/`. Not self-certifiable and not cheaply testable; it is the
  failure that would actually hurt.

## §H — DEFERRED: wasm/mobile-web UI layer (operator directive, 2026-08-04)

Not part of this change's execution; recorded here so the F.3 design does not foreclose it and the
wasm host-integration work inherits it. Operator intent, near-verbatim:

- The wasm build needs a **separate UI layer for mobile web** — the desktop cell map (§F.3) is not
  the mobile topology.
- On mobile, **the encoder grid takes all horizontal space in a square region of the screen**.
- **Figure out the optimal mobile grid to minimize blank encoder slots.** The desktop 4×4 keeps
  firmware slot positions and shows empties; on a phone, blank cells are wasted glass. E.g. a bank
  with 9 named params + CRIS/CRNC = 11 live encoders — a 3×4 region wastes 1 cell where 4×4 wastes
  5. The mobile layer may re-pack; the desktop layer never does.
- **Dictation gap, unresolved:** the directive contained a truncated clause — "only sharing
  horizontal space with …" — that never named the sharer. Likely the scope or the transport
  controls, but this is a guess; **ask the operator before designing the mobile layer**, do not
  design around the guess.
- **CONSTRAINT FOUND 2026-08-04 — re-packing slots is NOT free.** The mobile goal above
  ("minimize blank encoder slots") assumes cells can be re-ordered. In this app the bank-layout
  slot index **equals the hardware `PhysicalEncoderId`**, because `FroggersParameterModel::Init`
  adds physical encoders in the exact sequence 0..15 (`app/FroggersParameters.hpp:265-267`) and
  `Bank::RegisterParameters(params, offset)` indexes into that layout array rather than into
  encoder-id values. So re-packing the *visual* order without also reordering the
  `AddPhysicalEncoder` calls would change which physical encoder drives which parameter. Note this
  is a consequence of THIS app's construction order, not a Sheaf-wide invariant.
  **Implication:** the mobile layer must either (a) re-pack visually while keeping slot→encoder
  identity intact — i.e. the cell map's *display* order differs from slot order, which is fine
  precisely because the map is data — or (b) leave hardware mapping alone and accept some blanks.
  **UNCLEAR and worth asking before designing:** whether `PhysicalEncoderId` carries further
  hardware meaning (literal wiring position for the Daisy build) beyond what the app-side files
  show. That lives outside the traced scope.

Design consequence for F.3 (why this note lives here): the topology (§F.3 cell map) must stay a
data-level description separate from the Sheaf-idiom emission code, so a second topology can be
emitted for wasm without forking the surface. F.3 should keep the cell map in one declarative
place, not smeared through `BuildTree()`.

Related standing context: the wasm/web V2 host integration is the recorded next task after the
desktop work, and `V2ParamDisplayNames.hpp` was already forked to avoid breaking web.

## §I — DEFERRED: VST implementation (operator directive, 2026-08-04)

Same status as §H: not executed in this change, recorded so nothing designed now forecloses it.

Operator intent: a VST build with **no internal audio/MIDI routing — the DAW handles all of it.**
Consequences worth pinning today, while the adjacent decisions are fresh:

- **Audio input**: source #6's feed is just the plugin's input bus, routed by the DAW. The F.5
  opt-in question ("did the user route anything?") is answered by the host, not by our Audio page
  — F.5's `InputView()` gating should therefore live where a plugin build can swap the answer's
  source, not be entangled with the standalone Audio-page UI.
- **Transport and tempo**: Play/Stop and the BPM slider are standalone-app affordances. Under a
  DAW the host owns transport state and tempo; the §F.3 cell map's row-3/row-6 left-block cells
  are candidates to hide or repurpose in the VST topology. Another reason (with §H) the cell map
  stays data, separate from emission code — VST is a third topology consumer.
- **MIDI**: none of our own routing; whatever note/CC handling exists must accept host-delivered
  events rather than opening devices.
- **Packaging**: unrelated to the §G direct-launch JUCE app; likely intersects D.4 (publish
  pipeline) when that scope opens.

## §D Carried open from the predecessor

- [x] D.1 **S&H dice-roll motion — CONFIRMED ALIVE 2026-07-29.** The 6.1 sweep had already proved
  the wiring intact end to end; the running-app screenshot shows the RNDS cells drawing jagged
  sample-and-hold traces. The original F5 suspicion (a dropped attachment) was refuted, and the
  symptom was most likely the operator never reaching the mod pages while drill-in required a
  double click.
- [x] D.2 **`ScopeWriter` sizing — DECIDED AND RECORDED 2026-07-29.** Open since the predecessor's
  task 1.2. Verdict: **keep the defaults, no change.** 512-sample read window across the 340px
  panel is ~1.5 samples/pixel (oversampled is the safe side — the polyline builder decimates, it
  cannot invent detail); 10.7 ms at 48 kHz shows ~1.2 cycles of the lowest default voice and ~3.5
  of the highest. The default 4096-frame ring is 8× the read window. Braid 4's 6'553'600 frames
  sizes a marker-aligned long-window display it hand-builds and is explicitly **not** a target to
  match. Derivation recorded at the construction site (`app/FroggersAppCore.hpp:122`).
- [ ] D.3 **Voicing judgements — partially closed 2026-07-29.**
  **Closed:** max-Crunchy near-silent chaos is *wanted* ("better than the opposite"); Random S&H
  lane #4's ~5-level quantisation is judged unimportant (one `quantizeLevels` integer, provisional
  since D8a called it "the weakest choice"); randomization overall "basically works fine".
  **Still open:** whether ±0.95 comb feedback and the 2× bump ceiling sound right — those are
  stability numbers, not musical ones. And the randomize-reach question, which E.1 will move.
- [ ] D.4 The whole publish pipeline and acceptance gate (predecessor §5).
- [x] D.5 **Drive Blend — CLOSED 2026-07-29, operator ruled: leave it alone.** Neither the default
  nor the range changes.
  **Why it was raised, and why that framing was wrong.** It was reported as "the authored 20% Drive
  is inaudible — same shape as the pitch bug", inside the silence investigation. The operator
  correctly rejected the premise: `DriveBlendPhase::Process` returns `dry*(1−blend) + phased*blend`
  (`app/dsp/Drive.hpp:352`), so at blend 0 the **dry signal passes at full level**. Blend 0 makes
  the *distortion* inaudible, never the instrument. It cannot cause silence and never could.
  Presenting it alongside the no-audio work implied it was a candidate cause; it was not, and the
  actual silence bugs (20 Hz default pitches; poisoned state never cleared after a blowout) were
  both fixed without touching Blend. **Do not reopen this as a silence lead.**

- [ ] D.6 **Left-column control block — DEFERRED by operator, 2026-07-29.** The requested
  arrangement is Stop/Start, Scene 1/Scene 2 and Scene blend in two columns *beneath* the scope in
  the left column. **Blocked by the toolkit, not by effort:** `Button` nodes take no bounds and are
  auto-flowed full-width below the lowest positioned node, so placing controls requires `Draw`
  nodes, which dispatch only on double-click at pin `1940ddcb`. Positioned controls or single-click
  controls, not both.
  Operator chose to leave it for now rather than surrender single-click. Unblocks the moment
  jvictor0's plain-click lands — at which point this, the custom transport icons, and arbitrary
  glyph colours all become available together. See `UPSTREAM-SHEAF-ASK.md` item 1.

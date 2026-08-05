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

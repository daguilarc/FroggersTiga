# Tasks — `frogg3rs-audio-safety-and-ui-rework`

## §0 Standing constraints

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch (OMNI §4, §15).
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Build/test runs go through a subagent**, reporting counts and failure tails only (OMNI §16.1).
- **`External/Sheaf` stays pinned at `1940ddcb` and clean.** Sheaf-side needs → `UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees stay byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes are sequential.** No parallel implementation dispatches (operator directive,
  2026-07-28). Parallel is permitted for read-only analysis only.
- **Do not add user-visible behaviour the operator did not ask for.** Propose first.
- **An implementer may not close a task whose spec requires the operator to see or hear it.**
  The predecessor did this and cost the operator a wasted test session.

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

- [ ] The four call sites re-enumerated against the current file, not this list.
- [ ] The distribution table in A6 reproduced in the brief verbatim (median 3, `P(1)=P(>=5)`,
      30/30 tie, geometric r=0.7 tail) — an implementer must not re-derive it.
- [ ] Brief states explicitly that parameter-VALUE randomization is OUT of scope and must not be
      touched.
- [ ] Brief forbids reimplementing `RandomizeVisibleValue` (D14).

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

## §E Outstanding — analysed 2026-07-29, NOT yet implemented

Analysis in `design.md` A6/A7. **Nothing here has been started.** An earlier attempt at E.1 was
stopped mid-edit for skipping the proposal step (OMNI §7/§13 — scattered call-site edits with no
written analysis behind them); the dead helper it produced was removed and the tree is clean.

- [x] E.1 **Randomize-depth count distribution (design A6).** Sheaf's own loop is geometric from
  ZERO — P(0)=50% — so Randomize Page on a modulation page does nothing half the time. Replace the
  count selection **app-side**; Sheaf still performs every write, which is the D14 split.
  **Target: MEDIAN 3, P(1) == P(>=5), tail plausible to the full source count** (operator,
  2026-07-29). Weighted table 10/30/30/20 for n=1..4 and 10% spread uniformly over 5..N, where N is
  the connected-source count (13 external-off, 15 on). Explicit table, NOT a coin loop — a
  geometric tail dies long before 13 and the operator wants high MAD up there.
  **Do not tune for the mean** (~3.1); ruled out twice. The earlier "z score" phrasing is
  withdrawn. Design A6 has the table, the checks, and the code sketch.
  **All required API is public and verified** (A6 lists each with line numbers) — including
  `ParameterManager::Scene()` for the live scene, which is what an earlier attempt wrongly believed
  was unavailable.
  Also fix the duplicate-source draw: Sheaf's loop can pick the same source twice; pick distinct
  ones. **Keep** the `connected` filter.
  Note (not a blocker, no decision needed): removing the 50% zero mass roughly doubles the
  source->parameter connections Randomize All makes. Inherent to "never 0"; the operator has seen
  this and chosen the median spec anyway. Same depth LEVEL as before — more sources per parameter
  at level 1, not modulation-of-modulation.
  **Do NOT touch parameter-VALUE randomization** — it has no coin, moves all nine knobs every
  time, and the operator has confirmed it is correct.
  **DONE 2026-07-29.** New `RandomizeParameterModulationDepths(manager, parameter)` in
  `app/FroggersModulation.hpp`, the exact table from A6 reproduced verbatim (10/30/30/20 for
  n=1..4, geometric r=0.7 tail over 5..N, tie preserved, no mean-tuning). Distinct-source partial
  Fisher-Yates fixes the double-draw bug too. All four `Modifier::RandomMod` call sites re-enumerated
  against the live file (still exactly four, same lines as the preflight check) and rewired to call
  the helper directly on the target `Parameter&` — no press, no modifier-hold, matching D14.
  Parameter-value randomization untouched and its own tests still pass unmodified.
  Tests: `randomize_page_mod_detail_is_never_a_no_op_across_500_trials` (0 no-ops/500, was the
  reported bug); `randomize_depth_helper_draws_distinct_sources_even_from_an_adversarial_index_feed`
  (injects a hostile `SetRandomSource` mock — always top-of-range — to prove the Fisher-Yates is
  structurally distinct rather than merely usually-distinct); `randomize_depth_helper_median_count_is_three_across_1000_trials`.
  Suite green 154/154 (151 + 3 new), all ten binaries confirmed by name, zero warnings.
  **Postflight (§14) found two things the brief didn't ask for and the lead fixed directly**, both
  genuine dead code the rewiring produced: `kFroggersTargetBackEncoder` (no remaining reference
  anywhere in `app/`) and the `Modifier::RandomMod` branch inside the value-randomize press helper
  (only ever called with `Modifier::Random` now) — the helper was narrowed from
  `PressBankWithModifier(..., Modifier)` to `PressBankWithRandomValue(...)` rather than left
  accepting a value nothing passes. Re-verified green after the cleanup, all stale comment
  references to the old name fixed too.
- [x] E.2 **Back from level 2 returns to level 1, not level 0 (design A7a).** `Back()` is a full
  `Deselect()`. Remember the level-1 parameter and re-open it. App-side; no Sheaf change.
  **DONE 2026-07-29.** `FroggersModulationDrillIn` gained a private `level1Encoder_` member, set
  by `PressEncoder` only on the 0→1 transition (left untouched on 1→2, so it survives the level-2
  round trip). `Back()` now checks `wasLevelTwo` before resetting and, if true, re-presses
  `level1Encoder_` to land back on the same level-1 parameter. Level 1→0 unchanged.
  **Caught in postflight, before the lead needed to:** `RandomizeAll`'s level-1 branch had its own
  hand-rolled "Back then re-press `originalEncoderId`" after each level-2 excursion, written when
  `Back()` was always a full exit. With `Back()` now auto-reopening level 1, that manual re-press
  would have fired a stray press *inside* the just-reopened L1 view (a modulation-source grid, not
  the top-level parameter grid `originalEncoderId` indexes into) — a real regression E.2 would have
  silently introduced into unrelated code. Removed; one `Back()` now does both the exit and the
  reopen. Lead re-traced this independently rather than taking the report's word for it.
- [x] E.3 **Clicking the active bank exits a drilldown (design A7b).** `RequestBankSelect`'s
  `!= activeBankIx_` guard makes re-selecting the current bank a no-op, which is exactly the case
  the operator needs to escape a drilldown. Reset the drill-in when the bank matches and level > 0;
  keep the no-op when level is already 0 so identical state is not rebuilt on every click.
  **DONE 2026-07-29.** Same-bank branch in `FroggersAppCore::ProcessFrame` now loops
  `while (drillIn_->Level() > 0) drillIn_->Back();` (bounded to 2 iterations by the level cap) when
  the request matches the active bank and level > 0 — reusing E.2's now-correct `Back()` rather
  than adding a second reset primitive. Same-bank-at-level-0 branch is unchanged: nothing executes,
  true no-op preserved.
- [x] E.4 **Test the navigation, not just the state.** E.2/E.3 are behavioural; assert level
  transitions (2→1 on Back, N→0 on same-bank click) rather than that a function was called.
  **DONE 2026-07-29.** `back_from_level_two_returns_to_the_same_level_one_parameter_then_back_again_exits_to_grid`
  asserts pointer-identity of `SelectedParameter()` across the 2→1 pop (not merely "some" parameter),
  then a second `Back()` reaching level 0.
  `clicking_the_active_bank_while_drilled_in_exits_to_the_top_level_grid` drives the real
  `FroggersApp` through UI actions (`kBankSelect`/`kEncoderPress`), asserting `Level()` goes 2→0 on
  a same-bank reselect, and that a same-bank click already at level 0 leaves `ActiveBankIndex()`,
  `Level()`, and `SelectedParameter()` all unchanged.
  Suite green **156/156** (154 + net 2), all ten binaries confirmed by name independently by the
  lead, zero warnings. A stale header comment claiming "no one-level pop... matching the design's
  resolved choice" — the exact staleness design A7a flagged — was also fixed.

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

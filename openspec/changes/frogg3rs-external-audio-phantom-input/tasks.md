# Tasks — `frogg3rs-external-audio-phantom-input`

> **Read `proposal.md` first.** It carries the traced mechanism and the constraints.
> `SUPERSESSION-RECORD.md` is history: read it for what predecessor work carries forward unchanged,
> not for what to do next.

**Goal:** stop opening the operator's microphone unasked, and delete the hardcoded flag that
currently compensates for it. `config.numAudioInputs = 0`; `kExternalAudioOptedIn` and its two
downstream branches are deleted, not flipped.

**Inherited suite state (not re-verified by this document — this change is markdown/comment work;
nothing has been built or run under it yet): 10 binaries, 189 tests, 0 failures, 0 warnings**, per
`../archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/tasks.md`'s last recorded
status. `External/Sheaf` pinned at `77a3019e`. **Any red is a regression**, with the two named
exceptions in T3 below — both MUST change in the same commit as T1, not be chased afterward as a
surprise.

## §0 Standing constraints (binding)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **`External/Sheaf` is pinned and unpatchable.** No task here needs a Sheaf change.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose spec requires operator eyes or ears.**
- **Cite by SYMBOL, not by line.** Every line number in this directory is stale the moment anything
  above it changes.
- **A negative result requires a positive control** (OMNI §9.1): before recording "the source is
  disconnected," print the number proving the setup could have produced a connected reading
  instead (see T2.2).

---

## T1 — `Config()`: request zero audio inputs

- [x] **T1.1** `app/FroggersAppCore.hpp`, `FroggersApp::Config()`: `config.numAudioInputs = 1` →
      `config.numAudioInputs = 0`.
- [x] **T1.2** Rewrite the "Task 2.6" comment above it. It reasoned correctly that the request is
      forwarded verbatim to `juce::AudioDeviceManager::initialiseWithDefaultDevices(
      config.numAudioInputs, ...)` (`Runtime.hpp:237`) but never followed that call through to what
      it actually does with a nonzero count — opens the DEFAULT input device, chosen by nobody.
      Record that finding, and what would justify raising the value again (a real routed-input
      signal from upstream — `UPSTREAM-SHEAF-ASK.md` item 8, asks 1/2 — not a pin bump), so the
      next reader does not re-raise it for the same reason this comment originally gave.
- [x] **T1.3 (DONE 2026-08-09)** Confirm by reading (not by running — this is markdown/comment-adjacent work until an
      implementer picks it up under normal engineering rules) that no other call site depends on
      `numAudioInputs == 1` besides the two named in T3. Search the concept — an input channel
      being requested or assumed present — not just the literal token `numAudioInputs` (OMNI §8's
      enumerate-by-operands method, the same method behind this repo's citation sweeps).
      **Found by this method, not by the literal token:** a SECOND, previously-uncited
      `block.inputs[0]` read at `FroggersAppCore.hpp`'s `externalAudioSample` ternary (`ProcessBlock`,
      per-frame loop) — distinct from the one inside `externalInputHasChannel`'s own definition, and
      not named in `proposal.md` §2's trace. It does not literally "depend on" the value being 1 (the
      ternary short-circuits safely to `0.0f` regardless), so it does not contradict this task's
      claim, but it IS a call site that assumes a channel might be present. T2.2's amendment already
      covers it: "the `block.inputs[0]` read" it names for deletion is THIS one (the one inside
      `externalInputHasChannel` is already implied by deleting that variable's own definition, so the
      amendment naming a second, separate read only makes sense as referring to this site) — deleted
      alongside `externalInputHasChannel`, see T2.2. No other call site found besides the two named in
      T3 and this one.

## T2 — Delete the compensating flag, not just flip it

- [x] **T2.1** `app/FroggersAppCore.hpp`, `ProcessBlock`: delete
      `constexpr bool kExternalAudioOptedIn = false;` and its three-line "flip me when there is a
      real opt-in signal" comment.
- [x] **T2.2 — AMENDED BY PREFLIGHT 2026-08-09 (OMNI §14). Read this, not the original wording.**
      The original said to "fold `externalInputHasChannel` / `externalInputConnected` into whichever
      single expression leaves the fewest names." **Do not do that.** Folding them together lands on
      `externalInputConnected = externalInputHasChannel`, which leaves the invariant TWO-part —
      `numAudioInputs = 0` *and* that derivation — so raising `numAudioInputs` back to 1, which
      `proposal.md` §6 explicitly anticipates a future reader doing, re-creates the phantom sources
      immediately. That moves the landmine from "delete this flag" to "raise this integer" rather
      than removing it.
      **Instead:** `externalInputConnected` becomes a literal `false` carrying the reasoning in a
      comment, and **`externalInputHasChannel` and the `block.inputs[0]` read are DELETED** as
      provably-dead branches (OMNI §12: trace the origin, remove impossible branches — they are
      impossible because this app's own `Config()` never requests the channel). Re-enabling external
      audio then REQUIRES writing a derivation against a real routed signal, which is what
      `proposal.md` §6 says re-enablement means anyway.
      **Positive control (OMNI §9.1), unchanged and still required:** print `block.numInputChannels`
      at startup before T1/T2 (expect `1` — the built-in mic) and again after (expect `0`). A test
      asserting "not connected" proves nothing unless the rig could have read `1`.
      **DONE 2026-08-09, delivered as a standing test instead of a one-off print** (per the executing
      session's own explicit instruction): a before/after `block.numInputChannels` print would need
      two separate builds against two different working-tree states to observe both values, and
      would prove nothing on any later run. Instead:
      `external_audio_sources_stay_registered_and_disconnected_with_zero_input_channels`
      (`app/FroggersHeadlessTests.cpp`) prints the total registered source count (15) and asserts a
      DIFFERENT, unconditionally-connected source (slot 0, Random S&H 1) reads `connected == true` in
      the SAME rig/run as the slots-13/14-disconnected assertions — proving the metadata-reading
      mechanism is live, not uniformly false, every time the suite runs (not just once). See T3.3 for
      the run's actual numbers.
- [x] **T2.3** Grep the concept, not the old expression's shape (OMNI §8): confirm no other call
      site reads `kExternalAudioOptedIn`, `externalInputHasChannel`, or `externalInputConnected` by
      name before deleting any of them. The three known call sites are traced in `proposal.md` §2;
      re-derive the set by reading, don't assume that list is exhaustive.
      **Re-derived by reading, full §8 sweep across `app/` for all six named operands plus two
      concept-search-only operands (`block.inputs`/`block.numInputChannels`, `externalAudioSample`) —
      see the executing session's final report for the complete found-vs-changed table. Summary:**
      `kExternalAudioOptedIn` found 3/changed 3 (all in `FroggersAppCore.hpp`, all deleted).
      `externalInputHasChannel` found 2/changed 2 (both deleted). `externalInputConnected` found
      5/changed 2 (the `FroggersAppCore.hpp` declaration and the `externalAudioSample` ternary
      changed; `FroggersModulation.hpp`'s `Step()` parameter and its two `FroggersAppCore.hpp` call
      sites — the `modulation_.Step(...)` argument — correctly left unchanged, that is the plumbing
      T2 requires stay intact). `kModSlotExternalAudio`/`kModSlotExternalAudioEf` found 9+7/changed
      0+0 (all in `FroggersModulation.hpp` production registration or
      `FroggersModulationTests.cpp`'s bare-fixture tests, both out of this proposal's scope — verified
      by reading, not assumed). No call site depends on the deleted names besides the ones this task
      list already names.

## T3 — The two test consequences (land WITH T1, same commit)

- [x] **T3.1 — `froggers_config_requests_exactly_one_audio_input_channel` must be rewritten, not
      patched.** (`app/FroggersHeadlessTests.cpp:101-114`, verified by reading.) Its own name and
      its `REQUIRE_TRUE(config.numAudioInputs == 1)` assert exactly the premise T1 reverses. Its
      comment (`:101-110`) is the historical argument FOR raising it to 1 in the first place, and
      cites the same `Runtime.hpp:237,260-261` contract `proposal.md` §2 traces to the opposite
      conclusion. Rename the test to match its new assertion (e.g.
      `froggers_config_requests_zero_audio_input_channels`), flip the assertion to `== 0`, and
      replace the comment with the corrected reasoning. Do not leave the old comment's argument
      standing beside a flipped assertion — that is how the next reader gets confused about which
      one is current.
- [x] **T3.2 — `froggers_init_process_block_produces_finite_stereo_output`'s comment goes stale,
      even though its assertions keep passing.** (`app/FroggersHeadlessTests.cpp:72-99`.) Its
      comment (`:77-86`) claims the test "doubles as proof that FroggersApp tolerates an
      actually-present input channel... without producing non-finite output" — true only because
      `SynthRig` currently allocates a real input buffer when `numAudioInputs == 1`
      (`tests/support/SynthRig.hpp:62,72,76,454,456`). Once T1 lands, `SynthRig` allocates zero
      input buffers and this test no longer exercises that property — its NaN/finite/stereo
      assertions still pass, trivially, on a chain that never receives an input sample. **Read
      `SynthRig.hpp` at the cited lines first** to confirm the allocation really is
      `numAudioInputs`-driven before relying on this. Then either (a) correct the comment to stop
      claiming coverage this test no longer has, or (b) — preferred, if `SynthRig` supports a
      per-test config override — construct this test's own rig requesting an explicit input
      channel, so the coverage is retained deliberately rather than lost silently. Decide by
      reading, not by assuming either option is available.
      **DONE 2026-08-09, option (a).** Read `SynthRig.hpp`'s constructor in full: `AudioSettings`
      (its only per-instance override struct) covers sample rate and block size only —
      `numInputChannels_` is unconditionally `App::Config().numAudioInputs` (`:61-62`), and
      `Config()` is a `static` method of the App type `SynthRig<App>` is templated on, not an
      instance field. Option (b) is therefore not available without a second App type overriding
      `Config()`, which is out of this task's scope (and this proposal's — no new App type is
      named anywhere in it). Comment corrected to stop claiming the coverage; states plainly what
      the test still proves (finite stereo output through the real init/process path, now always
      with zero input channels, which is the shape the app runs in).
- [x] **T3.3 (DONE 2026-08-09)** Re-run the full suite after T1+T2+T3.1+T3.2 land together. Report the new
      binary/test counts here, superseding the "inherited" line at the top of this file — do not
      leave stale numbers standing once real ones exist.
      **Result: `nice make -j2 -C app test` — 10 binaries, 191 tests, 0 failures, 0 warnings, exit 0.**
      Net +1 test vs. this file's own "inherited 189" line above — but that line was self-admittedly
      unverified ("nothing has been built or run under it yet"), and turned out to be off by one: a
      direct `git show HEAD:app/FroggersHeadlessTests.cpp` vs. working-tree `TEST_CASE(` count shows
      exactly 4 → 5 in that one file (the only test source this change touches), so the TRUE pre-change
      baseline was 190, not 189, matching the executing session's own dispatch instructions (which
      independently stated 190). 190 + 1 (the new test added below T2's changes) = 191, confirmed.
      Also separately built and ran `froggers_stop_flush_repro` (not part of the `test` target,
      excluded by design): 6/6 PASS, exit 0, post-flush peak exactly `0` in all three S1.2 scenarios
      (seeded, Flip=0 control, Blend=0 control) — unaffected by this change, confirmed still green.
      All five explicitly-named regression guards confirmed PASS by grepping the run's own output:
      `back_exits_to_parameter_grid_from_level_one`,
      `back_from_level_two_returns_to_the_same_level_one_parameter_then_back_again_exits_to_grid`,
      `encoder_cell_never_emits_a_frame_draw_command`,
      `modulation_header_shown_only_while_drilled_in_and_matches_the_level`,
      `modulation_header_sits_below_bank_row_and_above_parameter_cells`. The three pre-existing
      external-audio tests in `FroggersModulationTests.cpp` (bare-fixture, orthogonal to
      `Config()`/`ProcessBlock`) also confirmed PASS, unchanged.

## T4 — `UPSTREAM-SHEAF-ASK.md` item 8 correction

- [x] **T4.1 (DONE 2026-08-09, ahead of the rest of this change)** — item 8's "LANDED at
      `508d9d68`" verdict (both the FULL LEDGER row and the SECOND RE-CHECK row) and the supporting
      "in practice it also delivers the user opt-in we wanted" paragraph were all wrong, and are
      corrected in place, with the specific `HasActiveChannel()` trace that disproves the original
      reading. Done as part of this change's own citation-sweep pass (which also archived its
      predecessor) rather than deferred, because leaving it wrong would have told the next reader
      of that file to make exactly the mistake this change exists to prevent.

## T5 — Spec delta

- [x] **T5.1** `specs/froggers-modulation-slate/spec.md` — tighten "External-audio sources stay
      present but inert when unavailable" so "available" excludes a host-opened default device.
      See that file for the exact wording. Write it to describe the behavior T1/T2 actually
      produce once implemented, not the plan for it — if implementation deviates from
      `proposal.md` §2's trace, the spec follows the code, not this document.
      **Verified 2026-08-09: this delta file already carried the tightened wording** (written ahead
      of the rest of this change, alongside T4.1's citation-sweep pass — checkbox was simply not
      ticked). Re-read against the actual T1/T2 implementation landed in this session: `config.
      numAudioInputs = 0`, `externalInputConnected` a literal `false`, `externalInputHasChannel`
      deleted — the delta's wording ("the app SHALL request zero audio input channels," "disconnected
      by construction rather than by a compensating flag") matches exactly what was built, no
      deviation, no edit needed. This is the delta under this change's own `specs/` directory, not
      yet synced to the main `openspec/specs/froggers-modulation-slate/spec.md` — syncing/archiving
      is a separate step this dispatch was not asked to run.

---

## Carried forward as open scope (from the archived predecessor — unchanged, not re-derived here)

This change does not touch any of the following. Recorded so this directory does not read as
though they were resolved by omission. Full context:
`../archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/tasks.md`.

- **S4.3–S4.8 — operator eyes/ears only, still open.** S4.1 (Stop works) and S4.2 (blowout fixed)
  are PASSED and stay closed. S4.3 (Randomize-All-at-level-1 visual check), S4.4 (badge density),
  S4.5 (drill-level header placement — STEP 1 as of 2026-08-09; do not move it again from this
  entry), S4.6 (sustain floor by ear), S4.7 (saved patches still load), S4.8 (Drive Flip/Hash still
  sound right post-S1.3) are all implementer-unclosable by definition.
- **The fuegoization fixed point.** Zero is a mathematical fixed point of `Fuegoize`'s scramble
  (verified by reading `app/dsp/Fuegoize.hpp` in the predecessor's own audit) — a minimum-position
  parameter is not perturbed by Crispy/Crunchy, while every other position is. This is a verbatim
  port of the firmware's `Parameter.hpp:143` behavior; fixing it would be a deliberate parity
  divergence and is an **operator decision, not taken.** Not this change's scope.
- **`GangedRandomLfoVisualizer`'s unconditional background fill.** Carried forward as open scope per
  this change's own brief. **Not independently verified by this session** — the visualizer is
  Sheaf-owned (`External/Sheaf/.../GangedRandomLfoVisualizer.hpp`, out of scope to read or edit
  under this task's constraints), and despite a repo-wide search, no mention of this specific
  defect was found in the archived predecessor's `tasks.md`, `proposal.md`, or
  `UPSTREAM-SHEAF-ASK.md`. Recorded here as told; an implementer picking this up should verify it
  fresh by reading rather than trust this line. No app-side lever would exist regardless — any fix
  is upstream's, not filed as its own numbered ask yet.
- **S2 (the slew) was DROPPED by the operator** (predecessor `tasks.md` §S2, 2026-08-07: *"oh
  right, audio rate. never mind, we good."*). **Do not revive it.** Nothing in this change should
  be read as reopening that decision.
- **W4.1 — Sheaf pin bump `77a3019e` → `508d9d68`.** Independently deferred, unrelated to this fix
  (`proposal.md` §3). Not this change's scope.

---

## T6 — POSTFLIGHT FINDING (OMNI §14, §8-against-the-diff). Open, NOT executed.

**Recorded 2026-08-09 by the lead's postflight audit of T1–T3.** Implementation matched the amended
proposal exactly — no divergence. This is the separate question §14 requires: *what did writing this
make redundant?*

- [x] **T6.1 — `SetExternalAudioConnected` is dead per-sample work.** It is called once per sample
      from `FroggersModulationSlate::Step`, writing `false` into
      `Metadata(kModSlotExternalAudio).connected` and `Metadata(kModSlotExternalAudioEf).connected`.
      Both are **already `false` at registration** — `RegisterSources()` passes `false` as the
      metadata's last field for both slots, and Sheaf's `ModulatorMetadata::connected` defaults to
      `false` regardless. Its only argument source is now a compile-time `false`. At 48 kHz that is
      ~96,000 stores/second writing a constant over a constant, plus a parameter threaded through
      the per-sample hot path to carry it.
      **Strictly this is not NEWLY redundant** — `kExternalAudioOptedIn` was `constexpr false`, so
      the old expression already folded to false. What T2 removed was the last runtime-*looking*
      derivation, which is what made the dead work visible. Recorded under §16.2 ("pre-existing is
      not a reason to skip investigating a real fix") rather than dropped.
      **Scope note:** this is beyond T1–T3 and was deliberately NOT executed inside this change —
      §4 forbids an executor widening scope, and the fix belongs in its own bounded dispatch.
      **Before removing anything, verify** that nothing else depends on the per-sample write (the
      re-enable path in `proposal.md` §6 will need SOME writer; the question is whether it must run
      per sample or once).

      **DONE 2026-08-09.** Verified all of the above by reading before changing anything (OMNI §1):
      confirmed the per-sample call, both slots' already-`false` registration, and the compile-time
      `false` local at the sole production call site (`FroggersAppCore.hpp`'s `ProcessBlock`).
      **One correction to the finding's own text, found by tracing rather than trusting it** (OMNI
      §1: the plan/finding's own text is a claim, not a fact): "its only argument source is now a
      compile-time `false`" is true of the one PRODUCTION call site, not of `Step()` project-wide. A
      full grep-by-operand sweep (not by the shape of any one call) found 8 more direct `Step()` call
      sites — 2 in `FroggersModulationTests.cpp`, 5 in `FroggersMarblesClockTests.cpp` — plus the
      ~24-site `Fixture::StepOnce` wrapper in the former, one of which
      (`external_audio_cells_present_and_inert_with_no_input`) legitimately drives
      `externalConnected=true` to test the slate's own "flip to connected" mechanism. This changed
      the fix's shape: instead of deleting `SetExternalAudioConnected`'s call and `Step()`'s two
      external-audio parameters outright (which would have silently broken that test's ability to
      express its own scenario), the call was removed from `Step()` and its two parameters removed
      from the signature, with `Fixture::StepOnce` changed to call the (kept, unchanged)
      `SetExternalAudioConnected(bool)` setter directly — same observable effect, zero signature
      changes at any of the ~24+ `StepOnce(...)` call sites (only `StepOnce`'s own body changed).

      **Classification of the three named candidates:**
      1. **The per-sample `SetExternalAudioConnected(...)` call inside `Step()`.** REMOVED. Its
         production argument was a compile-time-`false` local; the call wrote `false` over an
         already-`false` metadata field, ~96,000x/second, for zero effect. The method itself is
         unchanged and KEPT (hard requirement) — now invoked only where a value can actually vary
         (`Fixture::StepOnce`, and a future re-enable derivation).
      2. **`Step()`'s `externalInputConnected` parameter + `ProcessBlock`'s compile-time-`false`
         local.** BOTH REMOVED, but only after tracing that `Step()` is a reused method (§6: 2+ uses
         trivially satisfied, ~9 call sites project-wide) whose parameter had a genuinely live,
         non-constant consumer in the test corpus — removing it required relocating that one live use
         to a direct `SetExternalAudioConnected(...)` call rather than deleting the capability.
      3. **`Step()`'s `externalAudioSample` parameter + the two lines it fed
         (`externalAudioSource_ = NormalizeBipolarToUnit(...)`, `externalAudioEfSource_ =
         externalAudioEf_.Process(...)`).** BOTH REMOVED. Unlike candidate 2: grepped every
         `SourceValue(...)` call across `app/` and found none reading either source's VALUE (only
         `.connected`), so there was no live consumer to preserve. **Proof the two members stay
         defined and finite every sample (hard requirement):** both carry non-static data member
         initializers — `externalAudioSource_ = 0.5f`, `externalAudioEfSource_ = 0.0f` — which C++
         guarantees run at construction, before `RegisterSources()` hands
         `Modulators::UpdateModValues()` a pointer to either; with the per-sample writes gone, both
         stay at these values for the object's whole lifetime (verified by grep: only the two removed
         lines and the two member declarations reference either name in the file). Both are also
         BIT-IDENTICAL to what the removed computation always produced: `NormalizeBipolarToUnit(0.0f)
         == 0.5f` exactly by its own formula, and `dsp::SingleEnvelopeFollower::Process(0.0f)` from a
         zero-initialized `level` holds exactly `0.0f` forever (`target(0.0) > level(0.0)` is false
         from sample one). Not an observable behavior change, only a dead-computation removal.

      **§8 enumeration of what removing these three made redundant in turn (not stopping at the
      first site):**
      - `externalAudioEf_` (the `dsp::SingleEnvelopeFollower` member) and its `SetSampleRate()` call
        in `Prepare()`: found, classified, KEPT — `.Process()` is gone but `SetSampleRate()` does no
        per-sample work (once per `Prepare()`, not the ~96,000/sec pattern this task is about), and
        deleting it would leave a future re-enable to silently rediscover that the struct's own raw
        coefficients are correct only near ~2 kHz, not 48 kHz — the same "keep the re-enable
        infrastructure, remove only the per-sample invocation" logic the hard requirement already
        applies to `SetExternalAudioConnected`.
      - `NormalizeBipolarToUnit`: found still reused for VCO sources 6-8 in the same `Step()`; KEPT,
        unaffected — removing the external-audio call site still leaves it satisfying §6.
      - Comments asserting the removed per-sample mechanism as current fact: found 9 distinct blocks
        across 4 files (`FroggersModulation.hpp` x5, `FroggersAppCore.hpp` x3,
        `FroggersHeadlessTests.cpp` x1, `FroggersModulationTests.cpp` x2) — all rewritten, none left
        stale.
      - Direct `Step()` call sites needing a signature update: found 9 (1 production +
        `Fixture::StepOnce` + 2 direct in `FroggersModulationTests.cpp` + 5 direct in
        `FroggersMarblesClockTests.cpp`) — changed 9/9.

      **Verification.** `nice make -j2 -C app test`: 10 binaries, 191 tests, 0 failures, 0 warnings,
      exit 0 — unchanged from this file's own T3.3 baseline. Positive control (OMNI §9.1) reconfirmed
      firing in the same run as the negative result:
      `external_audio_sources_stay_registered_and_disconnected_with_zero_input_channels` prints slot
      0 ("Random S&H 1") `connected = true` alongside slot 13/14 `connected = false` — the
      metadata-reading mechanism is live, not uniformly false. Separately built and ran
      `froggers_stop_flush_repro` (absolute target path via `-C app`, not part of `make test`): S1.2's
      three scenarios (seeded, Flip=0 control, Blend=0 control) all report post-flush peak exactly
      `0`, 6/6 `[PASS]`, exit 0 — unchanged. Named regression guards reconfirmed PASS by grepping the
      run's own output: `back_exits_to_parameter_grid_from_level_one`,
      `back_from_level_two_returns_to_the_same_level_one_parameter_then_back_again_exits_to_grid`,
      `encoder_cell_never_emits_a_frame_draw_command`,
      `modulation_header_shown_only_while_drilled_in_and_matches_the_level`,
      `modulation_header_sits_below_bank_row_and_above_parameter_cells`.

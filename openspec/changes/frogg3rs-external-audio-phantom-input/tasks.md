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

- [ ] **T1.1** `app/FroggersAppCore.hpp`, `FroggersApp::Config()`: `config.numAudioInputs = 1` →
      `config.numAudioInputs = 0`.
- [ ] **T1.2** Rewrite the "Task 2.6" comment above it. It reasoned correctly that the request is
      forwarded verbatim to `juce::AudioDeviceManager::initialiseWithDefaultDevices(
      config.numAudioInputs, ...)` (`Runtime.hpp:237`) but never followed that call through to what
      it actually does with a nonzero count — opens the DEFAULT input device, chosen by nobody.
      Record that finding, and what would justify raising the value again (a real routed-input
      signal from upstream — `UPSTREAM-SHEAF-ASK.md` item 8, asks 1/2 — not a pin bump), so the
      next reader does not re-raise it for the same reason this comment originally gave.
- [ ] **T1.3** Confirm by reading (not by running — this is markdown/comment-adjacent work until an
      implementer picks it up under normal engineering rules) that no other call site depends on
      `numAudioInputs == 1` besides the two named in T3. Search the concept — an input channel
      being requested or assumed present — not just the literal token `numAudioInputs` (OMNI §8's
      enumerate-by-operands method, the same method behind this repo's citation sweeps).

## T2 — Delete the compensating flag, not just flip it

- [ ] **T2.1** `app/FroggersAppCore.hpp`, `ProcessBlock`: delete
      `constexpr bool kExternalAudioOptedIn = false;` and its three-line "flip me when there is a
      real opt-in signal" comment.
- [ ] **T2.2 — AMENDED BY PREFLIGHT 2026-08-09 (OMNI §14). Read this, not the original wording.**
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
- [ ] **T2.3** Grep the concept, not the old expression's shape (OMNI §8): confirm no other call
      site reads `kExternalAudioOptedIn`, `externalInputHasChannel`, or `externalInputConnected` by
      name before deleting any of them. The three known call sites are traced in `proposal.md` §2;
      re-derive the set by reading, don't assume that list is exhaustive.

## T3 — The two test consequences (land WITH T1, same commit)

- [ ] **T3.1 — `froggers_config_requests_exactly_one_audio_input_channel` must be rewritten, not
      patched.** (`app/FroggersHeadlessTests.cpp:101-114`, verified by reading.) Its own name and
      its `REQUIRE_TRUE(config.numAudioInputs == 1)` assert exactly the premise T1 reverses. Its
      comment (`:101-110`) is the historical argument FOR raising it to 1 in the first place, and
      cites the same `Runtime.hpp:237,260-261` contract `proposal.md` §2 traces to the opposite
      conclusion. Rename the test to match its new assertion (e.g.
      `froggers_config_requests_zero_audio_input_channels`), flip the assertion to `== 0`, and
      replace the comment with the corrected reasoning. Do not leave the old comment's argument
      standing beside a flipped assertion — that is how the next reader gets confused about which
      one is current.
- [ ] **T3.2 — `froggers_init_process_block_produces_finite_stereo_output`'s comment goes stale,
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
- [ ] **T3.3** Re-run the full suite after T1+T2+T3.1+T3.2 land together. Report the new
      binary/test counts here, superseding the "inherited" line at the top of this file — do not
      leave stale numbers standing once real ones exist.

## T4 — `UPSTREAM-SHEAF-ASK.md` item 8 correction

- [x] **T4.1 (DONE 2026-08-09, ahead of the rest of this change)** — item 8's "LANDED at
      `508d9d68`" verdict (both the FULL LEDGER row and the SECOND RE-CHECK row) and the supporting
      "in practice it also delivers the user opt-in we wanted" paragraph were all wrong, and are
      corrected in place, with the specific `HasActiveChannel()` trace that disproves the original
      reading. Done as part of this change's own citation-sweep pass (which also archived its
      predecessor) rather than deferred, because leaving it wrong would have told the next reader
      of that file to make exactly the mistake this change exists to prevent.

## T5 — Spec delta

- [ ] **T5.1** `specs/froggers-modulation-slate/spec.md` — tighten "External-audio sources stay
      present but inert when unavailable" so "available" excludes a host-opened default device.
      See that file for the exact wording. Write it to describe the behavior T1/T2 actually
      produce once implemented, not the plan for it — if implementation deviates from
      `proposal.md` §2's trace, the spec follows the code, not this document.

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

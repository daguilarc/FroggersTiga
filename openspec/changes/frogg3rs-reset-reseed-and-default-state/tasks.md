# Tasks — `frogg3rs-reset-reseed-and-default-state`

Tasks 1-3 are DONE and recorded with their evidence. Tasks 4 onward are open.

## 1. Hygiene sweep (step zero) — DONE, with one correction

`app/FroggersAudioRoutingTests.cpp` swept mechanically: every file-scope
definition enumerated, real call-site counts taken with the definition excluded.
No dead helpers; every one has at least one caller. Dispositions on the hits that
are not simply reused:

- `PressPlay` (1 call) — KEEP. §4's single-use exception: names a domain concept,
  and it is the third member of a `PressFreeze`/`PressStop`/`PressPlay` family
  whose symmetry §5 protects.
- `MeasureEnvelopeArm` (1 call) — KEEP. Isolates a distinct transformation stage;
  inlining ~40 lines into the test body would obscure data flow.
- `ResetPage` (`app/FroggersModulation.hpp:1606`) and `ResetAll` (`:1643`) each
  take an unused `synth::ParameterManager& /*manager*/`, and both call sites pass a manager
  that is discarded. KEEP — deliberately named-out, and it mirrors
  `RandomizeAll`/`RandomizePage`, which do use theirs.

Correction to the record: an earlier pass reported "every helper has 3+ callers."
That was wrong — it covered only the block around `:100-320` and counted
definitions as call sites.

Repeated literals: `kAudibleFundamentalsHz`, `kInaudibleFundamentalsHz` and
`kBandSilenceFloorLinear` were each re-typed in four of the new tests and are now
single-sourced beside `GoertzelPower`. `kExpectedFundamentalsHz`
(`app/FroggersAudioRoutingTests.cpp:320`) is the same triple under another name
and is left alone deliberately: it is a pre-existing test's local, and folding it
in belongs to that test's own change.

`External/Sheaf/projects/synth` — sweep still owed for the files this change
reads, if Task 6 proceeds.

## 2. Land the Grace/Curve mapping change — READY, NOT COMMITTED

`app/dsp/VoiceEnvelope.hpp`, `app/FroggersDspParityTests.cpp`, and the
`openspec/specs/froggers-vco-topology/spec.md` amendment.

Record in the commit that it suppresses Defect A's audible symptom (proposal's
ten-run table), so a future reader is not misled into thinking the audio check
was always this insensitive.

## 3. Fix Defect A — DONE

`if (randomizeRan || resetRan)` in `ProcessFrame()`
(`app/FroggersAppCore.hpp`), one call covering both drains.

Check: `the_two_reset_arms_are_compared_while_the_smoothed_path_is_still_walking`
asserts the two arms are identical at +0 blocks after the reset. Asserted at +0
and not at a settled block because both arms converge by +8 either way — a check
taken after convergence passes with or without the fix.

Shown failing before the fix at 84 differing / worst 0.1879, and the audible
symptom shown gone afterwards at 12/12 -> 0/12 across three runs with Grace/Curve
reverted. Suite: 320 passed, 0 failed.

## 4. Fix Defect B — Sheaf restores the real startup default

Two candidate shapes, both traced:

- **(a) Capture the real default.** Extend `DefaultControlState`
  (`ParameterModulation.hpp:906-914`) to snapshot post-`Init` parameter state —
  centers and materialised depths — and have `RevertAllToDefaults()` restore from
  that snapshot rather than from `config_.defaultValue`. The capture point exists
  and already fires at the right moment (`Engine.hpp:260`, after `app_.Init()` at
  `:231`); it snapshots too little.
- **(b) Add an optional app hook.** A `HasRevertToDefault` alongside
  `HasPrepareToPlay`/`HasProcessFrame` (`AppConcepts.hpp:27-42`), so Frogg3rs
  re-applies `ApplyFroggersDefaultPatch` after a revert.

(a) is preferred: it single-sources the definition of default rather than leaving
two aligned only by each app's diligence. It is also the wider blast radius —
every Sheaf app changes behaviour — so it needs Task 7's drift check before it
ships. Record which was chosen and why.

Check: extend `new_patch_wipes_the_cross_vco_pitch_detents_that_reset_all_restores`
so New leaves all six detents materialised and off neutral. Must fail before.

## 5. Settle Defect C — DONE, refuted

`a_fast_parameter_sweep_with_no_reset_does_not_latch_the_instrument`. Maximal
sweep, restore through `ApplyFroggersDefaultPatch`, no reseed anywhere: decays
to 9.39e-13 against a 1.0e-3 floor.

Carries both controls -- pristine decays (the measurement can report silence)
and a Freeze-latched arm reads 0.509 (it can report a hold). The second was
added after a first version passed with only the first, which is the same
result a rig incapable of showing a hold would produce.

## 6. The Sheaf pull request

Scope: Task 4's fix, its tests, nothing else.

- Sheaf's gate does not build the runtime shell, so a green `projects/synth test`
  does not cover `Runtime.hpp`. Name what the gate did and did not compile.
- The two 96 kHz deadline tests fail deterministically on this machine and are
  pre-existing. State them as carried forward.

## 7. The drift check

Launch, Reset All, and New are three paths to one state, spanning an
app/submodule boundary that no single definition can collapse. The deliverable is
a check that FAILS when any one drifts, proven by breaking each once — not three
tests that happen to agree today.

## 8. Sequence against `frogg3rs-microphone-path-delivery`

That change is in flight and edits `app/FroggersAudioRoutingTests.cpp` at `:1221`
and `:2274,2284`. This change shifts those to `:1223` and `:2276,2286` via two
added includes. Hand off the corrected anchors, or land that change first.

## 9. Postflight

- Re-run §5's enumeration against the DIFF, not only this proposal.
- Name each gate, when it last ran, and whether its inputs moved.
- **Pin build provenance on every recorded measurement.** This investigation
  produced one withdrawn finding and one premature retraction, both traceable to
  runs treated as sharing a binary that did not.
- **Enumerate from the accessor list, not from intuition.** Three separate
  enumerations here were too narrow and each was reported as complete: the
  helper sweep, a 62-of-153 depth walk, and a three-field snapshot of an
  eleven-field observable surface. The fix that worked was reading the list.
- Budget an independent pass with fresh context.

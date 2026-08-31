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

## 4. Fix Defect B — DONE

Sheaf: `HasRestoreStartupState` (`AppConcepts.hpp`) plus
`ApplyPatchMessageAndNotifyApp()` (`Engine.hpp`), one wrapper covering all four
`ApplyPatchMessage` call sites including the arena-exhaustion retry.

Frogg3rs: `FroggersAppCore::RestoreStartupState()` re-invokes
`ApplyFroggersDefaultPatch` and reseeds.

Chose the hook over extending `DefaultControlState` to snapshot post-`Init`
state. The earlier preference for the snapshot was wrong on its own criterion:
a snapshot is a second representation of the default patch and can drift from
the function producing it, whereas the hook makes launch, Reset All and New all
reach the single definition. The snapshot shape would also have had to
re-materialise depth parameters during a revert, on the audio thread.

Check: `new_and_reset_all_both_restore_the_cross_vco_pitch_detents`, renamed
from `new_patch_wipes_...`, which would now misdescribe what it asserts.

Named `RestoreStartupState` rather than `RevertToDefault`: postflight found
`Parameter::RevertToDefault(SceneState)` already carries that name for a
different concept (`ParameterModulation.hpp:495`), and two concepts under one
name are not greppable. Postflight also folded `kExpectedFundamentalsHz`
(`app/FroggersAudioRoutingTests.cpp:330`) into the shared
`kAudibleFundamentalsHz` this change introduced -- it was defensible as a
separate test's local until this change put the same triple in the same file.

## 5. Settle Defect C — DONE, refuted

`a_fast_parameter_sweep_with_no_reset_does_not_latch_the_instrument`. Maximal
sweep, restore through `ApplyFroggersDefaultPatch`, no reseed anywhere: decays
to 9.39e-13 against a 1.0e-3 floor.

Carries both controls -- pristine decays (the measurement can report silence)
and a Freeze-latched arm reads 0.509 (it can report a hold). The second was
added after a first version passed with only the first, which is the same
result a rig incapable of showing a hold would produce.

## 6. The Sheaf pull request — commit landed, PR still owed

`External/Sheaf` commit `9132967e` on `fix-out-of-tree-app-gaps`.

Gate: 918 pass, 2 fail. The two are
`braid4_meets_96000hz_256_frame_deadline_and_continuity` and
`braid4_sparse_modulation_meets_96000hz_256_frame_deadline`, both on the timing
bound `averageSeconds <= blockSeconds * 0.60`
(`tests/braid4_deadline_tests.cpp:231`). Deterministic on this machine,
unrelated to this change, carried forward.

What the gate did NOT compile: the runtime shell. `Runtime.hpp` appears nowhere
in its output, so this change's `Engine.hpp` edit is covered only by the
Frogg3rs app suite instantiating it (321 pass), not by Sheaf's own gate.

## 7. The drift check — DONE, partially proven

`new_and_reset_all_both_restore_the_cross_vco_pitch_detents` asserts the three
states are EQUAL element-wise, not merely that each is materialised and off
neutral. Three paths agreeing a detent exists is not three paths agreeing what
it is, and the weaker form would pass if New restored 0.52 where launch gives
0.51.

Proven to fail for New: the same test recorded `(not materialized) x6` across
several runs before the hook existed. NOT proven for launch or Reset All —
breaking each deliberately is still owed, and until it is done this check is
demonstrated in one direction of three.

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

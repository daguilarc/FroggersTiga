# Tasks — `frogg3rs-drive-tone-floor`

Gates: `cd app && nice make -j2 test` (304/304 before this change);
`app/vst` ctest (3/3). Never above `-j2`, always `nice`. One browser e2e run at
the end as a regression check — nothing here touches the surface or the shell.

One line of code. Everything else is the assertion it does not have.

## 0. Hygiene

- [x] 0.1 Checked rather than assumed: both docs had Tone's DIRECTION right --
      unlike Damping, whose entry read just as plausibly and was backwards --
      and neither gave a range. So this was an addition. Original text: Neither `MANUAL.md` nor `QUICK_DICT.md` gives Tone a range. Both
      describe the direction correctly (unlike Damping, which had it
      backwards), so this is an addition rather than a correction — check
      rather than assume, since the Damping entry read plausibly too.
- [x] 0.2 FOUND vs CHANGED: one definition (`app/dsp/Drive.hpp:456`), one runtime
      caller (`app/FroggersAppCore.hpp:1634`), and two tests calling
      `SetTone(1.0f)` for the bypass default and pinning nothing about the
      floor. Confirmed: NO existing assertion fails on this edit, so the
      positive control had to be written rather than inherited. Original text: Enumerate by NAME before editing, and report found versus changed.
      Expected: one definition (`app/dsp/Drive.hpp:456`), one runtime caller
      (`app/FroggersAppCore.hpp:1634`), and two tests that call `SetTone(1.0f)`
      for the bypass default and pin nothing about the floor. That last part
      is the important one: unlike the damping floor, NO existing assertion
      will fail on this edit, so this change has to supply its own positive
      control rather than inheriting one.

## 1. The floor

- [x] 1.1 Done, 0.02f -> 0.1f, ceiling untouched at 1.0f. Original text: `app/dsp/Drive.hpp:456`, `ExpMapCompute`'s lower bound 0.02f -> 0.1f.
      The upper bound stays 1.0f so the default knob still resolves to an
      exact identity — that is what makes an untouched Tone knob remove
      nothing, and it must survive this change.
- [x] 1.2 `drive_tone_stays_geometric_and_never_reaches_the_inaudible_end`.
      POSITIVE CONTROL, run before the edit: red against 0.02f, and red at the
      floor assertion ALONE -- the geometric-mean, direction and exact-1
      ceiling clauses all passed in the same run, so it was not red for an
      unrelated reason. Original text: A test on the mapping, which is a pure function of the knob:
      the floor and ceiling, the midpoint as the geometric mean of the two
      (the clause that catches a geometric mapping being made linear, which
      moves every value between the ends while leaving both ends right), and
      that the ceiling is still exactly 1.0f.
      POSITIVE CONTROL: run it against the unedited 0.02f first and record
      that it goes red. Nothing existing fails on this edit, so an
      unverified-red test here is a test that proves nothing.
- [x] 1.3 Reported by the test at 48 kHz: knob 0 -> 804.9 Hz, 0.25 -> 1495.8 Hz,
      0.5 -> 2904.0 Hz, 0.75 -> 6312.6 Hz, 1.0 -> bypass. Before: 154.3 /
      417.5 / 1164.8 / 3603.5 Hz. Original text: Report the resulting cutoff in Hz at 48 kHz for knob 0, 0.25, 0.5
      and 0.75, in the test's own output, so the floor can be judged by a
      number as well as by ear. `fc = -fs * ln(1 - alpha) / (2 * pi)`.
- [x] 1.4 `drive_tone_default_knob_passes_its_input_unchanged` -- a block at the
      default knob against one that never calls SetTone, 64 samples, exact. Original text: An identity assertion at the default: `SetTone(1.0f)` then a run of
      samples through `FrogBlock` must come out unchanged by the tone stage.
      The two existing parity tests already depend on this; asserting it here
      states it as a requirement of the range rather than a side effect.
- [ ] 1.5 OPERATOR JUDGEMENT: press Randomize All several times with Drive
      audible and say whether 0.1 is too high (a dark setting worth keeping
      has gone) or still too low. 0.05 is the next value down. Same one-line
      edit either way.

## 2. Docs

- [x] 2.1 Done. Original text: `MANUAL.md`'s Tone entry gains the range in Hz and keeps its
      direction.
- [x] 2.2 Done. Original text: `QUICK_DICT.md`'s one-liner likewise, in that file's own terser form.

## 3. Nothing else moved

- [x] 3.1 306/306 at the floor edit; 308/308 after section 5 added the two
      shared-mapping tests. See 5.7. Original text: App suite green with counts.
- [x] 3.2 3/3. Original text: `app/vst` ctest 3/3.
- [x] 3.3 46/46. Original text: One browser e2e run.

## 4. Carried question

- [x] 4.1 ANSWERED by the operator: same floor, shared function. Executed in
      section 5. Original text:
      OPERATOR: Delay slot 10 "Feedback tone" has the identical mapping
      and the identical numbers, but inside the feedback loop, where
      progressively darker repeats are the effect rather than a defect.
      Raise its floor too, or leave it? Same one-line edit
      (`app/dsp/Delay.hpp:730`).

## 5. One mapping, two callers

- [x] 5.1 FOUND vs CHANGED, by operand (`ExpMapCompute` feeding a one-pole alpha),
      four hits, all classified before any were collapsed:
      `Drive.hpp SetTone (0.1, 1.0, knob)` -> COLLAPSED.
      `Delay.hpp SetFeedbackTone (0.02, 1.0, knob)` -> COLLAPSED, floor raised
      to match.
      `Reverb.hpp DampAlphaFromKnob (0.02, 0.2, 1-knob)` -> KEPT: different
      range and inverted, and folding it in would undo the range the reverb
      change just set.
      `Reverb.hpp DecayFeedbackFromKnob (0.1, 0.98, knob)` -> KEPT, and worth
      the enumeration on its own: it shares the 0.1 literal with Tone but is
      the tank's feedback AMOUNT, not a filter coefficient. Collapsing by
      matching literals rather than by concept would have caught it wrongly.
      Original text: Enumerate by OPERAND before writing the helper, and report found
      versus changed: every site computing a one-pole alpha from a knob, found
      by grepping the operand these share rather than any name they do not.
      Classify each hit before collapsing any — Reverb's damping is
      deliberately a different range AND a different direction, and folding it
      in would destroy the range the reverb change just set.
- [x] 5.2 `dsp::ToneAlphaFromKnob` in `app/dsp/DspMath.hpp`, beside
      `OnePoleLowPass`. The range and the reason for its floor live there
      only; both call sites point at it instead of restating it, and Drive's
      own comment shrank to a pointer. Original text: `ToneAlphaFromKnob` in `app/dsp/DspMath.hpp`, beside
      `OnePoleLowPass` whose coefficient it produces. Both callers already
      include that header. State the range and what each end means in its own
      comment, so neither call site has to.
- [x] 5.3 Both read it. Verified at the call graph, not by the function existing:
      `grep ExpMapCompute(0.1f, 1.0f` returns exactly one hit, inside
      `ToneAlphaFromKnob` itself. Original text: Both callers read it: `FrogBlock::SetTone` (`app/dsp/Drive.hpp`) and
      `StereoDelay::SetFeedbackTone` (`app/dsp/Delay.hpp`). Verify at the CALL
      GRAPH, not by the function existing: grep that no `ExpMapCompute(0.1f,
      1.0f` literal survives anywhere.
- [x] 5.4 `delay_feedback_tone_and_drive_tone_share_one_mapping`, across seven knob
      positions rather than just the ends -- two ranges agreeing at 0 and 1
      could still differ everywhere between, and the middle is where a uniform
      draw lands.
      POSITIVE CONTROL: red before the second call site was repointed, with
      the exact numbers (`0.02 ~= 0.1`). Original text: A test that the two controls agree BY CONSTRUCTION, not by
      coincidence: assert the delay's feedback-tone alpha equals the drive
      block's tone alpha across several knob values. This is the assertion
      that fails if someone later re-inlines one of them.
- [x] 5.5 `delay_feedback_tone_never_reaches_the_inaudible_end`. The existing
      `stereo_delay_feedback_tone_default_knob_is_exact_bypass_alpha` passes
      untouched, so the ceiling assertion was kept rather than replaced.
      POSITIVE CONTROL: red at the floor assertion against 0.02f. Original text: Feedback tone's own floor and identity, asserted the same way Tone's
      are. The existing
      `stereo_delay_feedback_tone_default_knob_is_exact_bypass_alpha` covers
      the ceiling; check it still passes untouched rather than replacing it.
      POSITIVE CONTROL for the floor: show it red against 0.02f first.
- [x] 5.6 Both entries updated, and the manual's says why the loop position
      matters -- the darkening compounds across repeats. Original text: `MANUAL.md` and `QUICK_DICT.md`'s Feedback tone entries gain the
      range, matching what 2.1/2.2 did for Tone.
- [x] 5.7 Re-run after the refactor, not just after the floor edit. Drive tone's
      numbers came out identical to 1.3's: 804.895 / 1495.84 / 2903.98 /
      6312.59 Hz. Feedback tone now reports the same 804.895 / 2903.98.
      308/308, vst 3/3, e2e 46/46. Original text: Re-run the gates after the refactor, not just after the floor edit:
      the shared function touches the drive path too, so Tone's own numbers
      must come out identical to what 1.3 recorded.

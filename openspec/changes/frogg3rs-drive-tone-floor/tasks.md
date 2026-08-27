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

- [x] 3.1 306/306, 0 failures (304 before, plus the two new tests). Original text: App suite green with counts.
- [x] 3.2 3/3. Original text: `app/vst` ctest 3/3.
- [x] 3.3 46/46. Original text: One browser e2e run.

## 4. Carried question

- [ ] 4.1 OPERATOR: Delay slot 10 "Feedback tone" has the identical mapping
      and the identical numbers, but inside the feedback loop, where
      progressively darker repeats are the effect rather than a defect.
      Raise its floor too, or leave it? Same one-line edit
      (`app/dsp/Delay.hpp:730`).

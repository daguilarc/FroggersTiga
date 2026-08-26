# Tasks — `frogg3rs-reverb-wetness-and-damping-floor`

Gates: `cd app && nice make -j2 test` (302/302 before this change);
`app/vst` ctest (3/3). Never above `-j2`, always `nice`. The browser e2e suite
is not affected — nothing here changes the surface or the shell — so it is run
once at the end as a regression check, not as a gate on each step.

Both code edits are one line each. Everything else here is the assertions those
lines currently do not have, and the doc that describes them incorrectly.

## 0. Hygiene

- [ ] 0.1 `MANUAL.md`'s Damping entry (Reverb bank, slot 4) states the
      direction backwards: "Higher knob values mean a brighter, less-damped
      tail; lower values mean a darker, duller tail." The trace says the
      opposite — `DampAlphaFromKnob` maps the knob's TOP to alpha 0.001, and
      `OnePoleLowPass::Process` (`app/dsp/DspMath.hpp:83-87`) darkens as alpha
      falls. Fix the direction, and while there give the entry the actual
      limits in Hz rather than none.
      `QUICK_DICT.md` carries a one-line version of the same parameter —
      check it for the same error rather than assuming it is right.
- [ ] 0.2 Establish whether anything else reads `kMaxReverbWetMix` or
      `DampAlphaFromKnob` before editing either. Grep by NAME, not by the
      literal, and report found-versus-changed. Expected: one definition and
      one use each, but that is the thing to verify, not to assume.

## 1. Wet mix ceiling

- [ ] 1.1 `app/FroggersAppCore.hpp:1827`, `kMaxReverbWetMix` 0.7f -> 0.6f.
- [ ] 1.2 A test that reads the ceiling through the audio path rather than
      reading the constant back. Drive the Reverb wet/dry knob to its maximum
      and assert the dry signal's contribution to the output is at least 40%
      — the property the constant exists to guarantee. A test that asserts
      `kMaxReverbWetMix == 0.6f` restates the edit and would pass against any
      future change that moved the constant and broke the blend.
      POSITIVE CONTROL: it must fail at 0.7f. Run it against the unedited
      constant first and record the number it reports.

## 2. Damping floor

- [ ] 2.1 `app/dsp/Reverb.hpp:384`, `DampAlphaFromKnob`'s lower bound
      0.001f -> 0.02f. The upper bound and the `1.0f - knob01` argument are
      unchanged: the knob keeps its travel and its direction.
- [ ] 2.2 A test on the mapping itself, since it is a pure function: assert
      `DampAlphaFromKnob(1.0f)` is the floor and `DampAlphaFromKnob(0.0f)` the
      ceiling, and that the midpoint is the geometric mean of the two. The
      third clause is what would catch the mapping being changed from
      geometric to linear, which would move every intermediate value while
      leaving both endpoints correct.
- [ ] 2.3 Report the resulting damping cutoff in Hz at 48 kHz for knob 0, 0.5
      and 1.0, in the test's own output, so the operator can judge the floor
      by a number as well as by ear. The formula is
      `fc = -fs * ln(1 - alpha) / (2 * pi)`.
- [ ] 2.4 OPERATOR JUDGEMENT, and the reason this change exists: press
      Randomize All several times with the Reverb bank audible and say whether
      0.02 is too high (a dark setting worth keeping has gone) or still too
      low (mud still wins). 0.01 is the next value down if it is too high.
      Both directions are the same one-line edit.

## 3. Nothing else moved

- [ ] 3.1 App suite green with counts.
- [ ] 3.2 `app/vst` ctest 3/3.
- [ ] 3.3 One browser e2e run as a regression check. Nothing here touches the
      surface or the shell, so a failure would mean this change reached
      further than it was supposed to.

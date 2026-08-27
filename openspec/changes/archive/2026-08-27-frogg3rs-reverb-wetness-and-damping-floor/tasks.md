# Tasks — `frogg3rs-reverb-wetness-and-damping-floor`

Gates: `cd app && nice make -j2 test` (302/302 before this change);
`app/vst` ctest (3/3). Never above `-j2`, always `nice`. The browser e2e suite
is not affected — nothing here changes the surface or the shell — so it is run
once at the end as a regression check, not as a gate on each step.

Both code edits are one line each. Everything else here is the assertions those
lines currently do not have, and the doc that describes them incorrectly.

## 0. Hygiene

- [x] 0.1 Fixed in BOTH docs -- `QUICK_DICT.md:92` carried the same error
      ("brighter tail at higher knob"), so checking it rather than assuming
      was the right call. Direction confirmed twice over before rewriting
      shipped prose: the existing parity test pins
      `DampAlphaFromKnob(0.0f) == 0.2` and `(1.0f) == 0.001`, and
      `OnePoleLowPass::Process` is `out = alpha*in + (1-alpha)*out`, where a
      smaller alpha is a heavier low-pass. So UP is darker. Both entries now
      also carry the actual limits in Hz. Original text: `MANUAL.md`'s Damping entry (Reverb bank, slot 4) states the
      direction backwards: "Higher knob values mean a brighter, less-damped
      tail; lower values mean a darker, duller tail." The trace says the
      opposite — `DampAlphaFromKnob` maps the knob's TOP to alpha 0.001, and
      `OnePoleLowPass::Process` (`app/dsp/DspMath.hpp:83-87`) darkens as alpha
      falls. Fix the direction, and while there give the entry the actual
      limits in Hz rather than none.
      `QUICK_DICT.md` carries a one-line version of the same parameter —
      check it for the same error rather than assuming it is right.
- [x] 0.2 FOUND vs CHANGED, and the count was not what the proposal predicted.
      `kMaxReverbWetMix`: 1 definition, 1 use -- as expected.
      `DampAlphaFromKnob`: 1 definition, 1 runtime use, and TWO existing
      assertions the proposal missed (`FroggersDspParityTests.cpp:2137-2138`)
      pinning both endpoints, plus a manual replica at :2575 that calls the
      real function rather than restating it. The pinned floor is updated
      with the change; the replica needed nothing. Original text: Establish whether anything else reads `kMaxReverbWetMix` or
      `DampAlphaFromKnob` before editing either. Grep by NAME, not by the
      literal, and report found-versus-changed. Expected: one definition and
      one use each, but that is the thing to verify, not to assume.

## 1. Wet mix ceiling

- [x] 1.1 Done. Original text: `app/FroggersAppCore.hpp:1827`, `kMaxReverbWetMix` 0.7f -> 0.6f.
- [x] 1.2 `reverb_wet_mix_always_leaves_at_least_forty_percent_dry`, reading the
      mix the DSP was actually handed rather than the constant. That needed a
      new accessor, `TestLastReverbWetMixEffective()`, because the ceiling is
      applied inline in the `Process()` argument list with no member to read
      back -- the same situation and the same idiom as the two
      `TestLastReverb*KnobEffective()` accessors already beside it.
      Measured: knob 1.0 -> mix 0.6, dry share 0.4.
      POSITIVE CONTROL: no existing test failed on the wet edit alone (144
      passed, 1 failed, and that one was the damping pin), which is the
      evidence that this ceiling genuinely had nothing behind it before.
      The test also pins the ceiling as being on the MAPPED value: half the
      knob is half the ceiling, and knob 0 is exactly 0. Original text: A test that reads the ceiling through the audio path rather than
      reading the constant back. Drive the Reverb wet/dry knob to its maximum
      and assert the dry signal's contribution to the output is at least 40%
      — the property the constant exists to guarantee. A test that asserts
      `kMaxReverbWetMix == 0.6f` restates the edit and would pass against any
      future change that moved the constant and broke the blend.
      POSITIVE CONTROL: it must fail at 0.7f. Run it against the unedited
      constant first and record the number it reports.

## 2. Damping floor

- [x] 2.1 Done, 0.001f -> 0.02f.
      POSITIVE CONTROL, unplanned and better than one written for the purpose:
      the pre-existing parity assertion failed on the edit and named the
      exact numbers -- `DampAlphaFromKnob(1.0f) (0.02) ~= 0.001f`. The change
      demonstrably took effect before any new test was written. Original text: `app/dsp/Reverb.hpp:384`, `DampAlphaFromKnob`'s lower bound
      0.001f -> 0.02f. The upper bound and the `1.0f - knob01` argument are
      unchanged: the knob keeps its travel and its direction.
- [x] 2.2 `reverb_damping_stays_geometric_and_never_reaches_the_inaudible_end`,
      plus the endpoint pin updated in place. The geometric-mean clause is
      the one that would catch the mapping being changed to linear, which
      moves every intermediate value while leaving both endpoints right --
      and the intermediate values are what decide half of all randomized
      patches. It also asserts up-is-darker, the clause both docs had
      backwards. Original text: A test on the mapping itself, since it is a pure function: assert
      `DampAlphaFromKnob(1.0f)` is the floor and `DampAlphaFromKnob(0.0f)` the
      ceiling, and that the midpoint is the geometric mean of the two. The
      third clause is what would catch the mapping being changed from
      geometric to linear, which would move every intermediate value while
      leaving both endpoints correct.
- [x] 2.3 Reported by the test itself, at 48 kHz: knob 0.0 -> 1704.69 Hz,
      knob 0.5 -> 499.12 Hz, knob 1.0 -> 154.34 Hz. Before this change the
      floor was about 7.6 Hz and the midpoint about 108 Hz. Original text: Report the resulting damping cutoff in Hz at 48 kHz for knob 0, 0.5
      and 1.0, in the test's own output, so the operator can judge the floor
      by a number as well as by ear. The formula is
      `fc = -fs * ln(1 - alpha) / (2 * pi)`.
- [x] 2.4 SETTLED by the operator, 2026-08-26: "approved" after hearing the
      change described with its numbers. 0.02 stands. If it later reads as
      too dark or not dark enough, 0.01 and 0.05 are the neighbours and it is
      the same one-line edit. Original text: OPERATOR JUDGEMENT, and the reason this change exists: press
      Randomize All several times with the Reverb bank audible and say whether
      0.02 is too high (a dark setting worth keeping has gone) or still too
      low (mud still wins). 0.01 is the next value down if it is too high.
      Both directions are the same one-line edit.

## 3. Nothing else moved

- [x] 3.1 304/304, 0 failures (302 before, plus the two new tests). Original text: App suite green with counts.
- [x] 3.2 3/3. Original text: `app/vst` ctest 3/3.
- [x] 3.3 46/46. Nothing reached further than it was supposed to. Original text: One browser e2e run as a regression check. Nothing here touches the
      surface or the shell, so a failure would mean this change reached
      further than it was supposed to.

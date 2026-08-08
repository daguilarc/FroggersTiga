# Proposal — `frogg3rs-parametric-slew-and-stop-root-cause`

**Created 2026-08-07. Rewritten 2026-08-07 by an omni-rule audit** of the artifacts that were in
this directory (`SUPERSESSION-RECORD.md`, `tasks.md`) and of the code they cite.

**This document is the change's only executable artifact (OMNI §3).** Before this audit the
directory had no `proposal.md` at all — every other live change directory and the archived
predecessor carry one — so nothing here was legitimately executable. That is fixed by this file.

**This document is self-contained.** Read `SUPERSESSION-RECORD.md` only for the history of how the
diagnosis moved; every binding fact is repeated here.

---

## 0. What the audit changed, and why you should not execute the old plan

The predecessor session measured a real capture, drew a mechanism from it that fit, and wrote that
mechanism into both artifacts as **CONFIRMED**. The mechanism is not necessary for the symptom.

| the artifacts said | the audit found | how |
|---|---|---|
| Root cause: audio-rate modulation of delay Send/Feedback pumps a zeroed loop back to full scale. "The operator's diagnosis is CONFIRMED." | **The symptom reproduces end-to-end with every coefficient held static and no modulation whatsoever.** Modulation is an aggravator, not the cause. | full-chain replica, `chainIn=0` for 500 000 samples, static knobs → pins at 1.0 forever |
| The seed that restarts a zeroed chain is unknown; `digitalReorganizer` is an "UNVERIFIED lead". | **The lead is correct and it is the whole story.** `DigitalReorganizer::Process(0.0f)` is nonzero for any `flip != 0`, and is exactly **−1.0** at `flip == 128`. | read `app/dsp/Drive.hpp:263-282`; exhaustive sweep over all 2304 `flip`×`hashBits` combinations |
| S1a.2 (transport-gate `modulation_.Step()`) is the thing to implement for F3. | Gating **cannot** silence a static DC seed. It remains in scope only because the operator ordered it on its own merits (S1a.1). | the seed is a pure function of frozen knob state, re-emitted identically every sample |
| S2's slew at `RouteAudioSample`'s `knob()` lambda is "one definition site". | `knob()` is called **55×/sample**; `(Filter,8)` is read **twice**, so a per-call slew double-steps it. `vcoDrive` is a second per-sample post-modulation reader outside `knob()`. | call census of `RouteAudioSample` |
| "All 11 path references rewritten. **Verified: zero references to the old live path remain.**" | **Ten stale references remain**, every one line-wrapped across two lines so a single-line grep could not see it. | `grep -B1` on the slug |

The last row is the same failure the record documents about itself one paragraph earlier: grep the
expression's shape and you find only the instances written that way (OMNI §8). It was committed
*after* the rule gained the clause forbidding it.

---

## 1. Objective

**Make silence produce silence.** When the transport stops, every voice reaches `Idle`, and all 14
stateful units are `Reset()`, the audio chain must decay to and remain at zero.

Secondarily, and separately: stop modulation free-running while the transport is stopped (operator
ruling, S1a.1), and remove the parametric-pumping mechanism behind F2's blowout *during play*
(operator design ruling, S2).

## 2. Data flow — the actual mechanism, traced

Per-sample path, `FroggersAppCore::RouteAudioSample()` (`app/FroggersAppCore.hpp:1061`):

```
audioVco1/2/3 --> MixOscVoices --> chainIn --> FrogBlock(drive) --> DriveBlendPhase
    --> FilterFxChain(pureDelay, peak, comb, scoopNotch) --> StereoDelay --> Reverb
    --> SanitizeOutputSample(outputLimiter_ x kMakeUpGain) --> out
```

**Stage 1 — the premise holds.** With the transport stopped and every voice `Idle`,
`VcoAdsrState::apply` returns `input * m_level[i]` and `m_level` is forced to `0.0f` in
`Stage::Idle` (`app/dsp/VoiceEnvelope.hpp:201-203`; `kNumVoices == 3` at `:33`, so all three voices
are gated — none falls through the `voiceIndex >= kNumVoices` guard at `:134`). **`chainIn` is
exactly 0.0f.** Verified by reading.

**Stage 2 — one stage manufactures signal from that zero.**
`DigitalReorganizer::Process(0.0f)` (`app/dsp/Drive.hpp:263-282`) computes
`inputUp = (0+1)*128 = 128` exactly, `inputRemainder = 0` exactly, then `inputInt ^= flip` and a
mask-gated 3-step bit scramble, returning `inputInt_final/128 - 1`:

| `SetFlip` arg | `flip` | `Process(0.0f)`, `hashBits = 0` |
|---|---|---|
| 0.00 | 0 | **0.0** (silent) |
| 0.25 | 63 | 0.4921875 |
| 0.50 | 127 | 0.9921875 |
| ~0.502 | **128** | **−1.0000000** (worst case over all 2304 flip×hash combinations) |
| 0.75 | 191 | −0.5078125 |
| 1.00 | 255 | −0.0078125 |

Also non-silent at `flip == 0` when `hashBits == 8` (Bit-depth knob at max): **0.09375**, because
`mask == 255` finally covers bit 7 and the scramble moves the 128. Verified by reading; the sweep
was measured by a standalone probe (`scratchpad/seedprobe.cpp`, compiled against `app/dsp/*.hpp`
with no JUCE and no `make`).

**Every other stage is provably silent on a zero input with reset state** — `PolynomialDrive`
(every term is `input^n`), both `SampleRateReducer`s, `Oversampler2x`, the fuzz blend, `PureDelay`,
both `ResonantBump` biquads (`b0*0` with zeroed history), `Comb`, `StereoDelay`, `Reverb`,
`OutputLimiter`. Each of those readings carries a nonzero positive control from the same probe, per
OMNI §9.1.

**Stage 3 — the DC is gated by one knob and amplified by three loops.**
`DriveBlendPhase::Process(chainIn, driveWet, blend, phase)` crossfades dry against wet, so
**Drive slot 7 (Blend) at 0 blocks the leak entirely**; above 0 it passes. Downstream:

| loop | max coefficient | in-loop saturator? |
|---|---|---|
| `dsp::Comb` | ±0.95 (`GetFeedback`, a deliberate divergence from the firmware's ±1.1) | **yes** — `PadeSaturator` inside `Process` |
| `dsp::StereoDelay` | `fbk` clamped to 0.98 (`app/dsp/Delay.hpp:332`) | **yes** — added by fix B2, per its own comment |
| `dsp::Reverb` | `fb = decayFb + (1−decayFb)·min(hold, 0.999)`, up to ~0.99998 | **NO** — `aIn = preOut + aFb*fb` is written unsaturated straight into `lineA` (`app/dsp/Reverb.hpp:377-385`); the only limiter sits *after* both tanks |

Verified by reading each loop body.

**Stage 4 — why it reads as exactly 0.999999.** A per-stage limiter's `DesiredMagnitude`
asymptotically approaches but never reaches `kStageCeiling = 0.80` (`app/dsp/Limiter.hpp:73`), and
`SanitizeOutputSample` then multiplies by `kMakeUpGain = 1.0f / kStageCeiling = 1.25`
(`app/FroggersAppCore.hpp:1388`). `0.7999999 × 1.25 = 0.9999999` — the exact value in the capture.

**Stage 5 — why the existing recovery machinery never fires.** `delay_`, `reverb_` and
`outputLimiter_` are tagged `dsp::FiniteOnly` (`app/dsp/RecoveryTier.hpp:47`), reset only on
non-finite state, deliberately exempt from the sustained-magnitude watch so a loud legitimate
reverb tail is not misdiagnosed. The tank grows large but stays finite, so nothing trips.

**This explains every element of the original report** — including "only after a randomize"
(Randomize All is what puts a nonzero value on Drive slot 4), "harsh loud noise" (pinned at the
clamp), "over a minute" (a memoryless function has no decay), and why the harness never reproduced
it (its post-Stop patch leaves Flip or Blend at 0 — probe runs C and D reproduce that silence
exactly, and are the controls proving the replica could have detected non-silence).

## 3. Constraints

- **Sheaf is pinned at `77a3019e` and unpatchable.** Needs go to `/UPSTREAM-SHEAF-ASK.md`.
  Nothing in this proposal needs a Sheaf change.
- **Frozen trees stay byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
  `src/core/PolynomialDrive.hpp` holds the firmware `DigitalReorganizer` this port came from. **It
  is frozen and is not edited**; the fix lands in `app/dsp/Drive.hpp` as a documented divergence.
- **The frozen firmware has the same `f(0) != 0` behaviour and no DC blocker anywhere** (checked:
  no DC-block/highpass stage exists in `src/core/`, `sim/`, or `app/`). On hardware an analog
  output stage would AC-couple this away; this port has no such stage. **So the fix is a deliberate
  parity divergence, in the same class as the operator-approved `±1.1 → ±0.95` comb feedback and
  the `10× → 4× → 2×` peak ceiling** — each of which carries an in-code divergence note. This one
  gets the same treatment.
- **`app/FroggersDspParityTests.cpp` WILL go red at the `DigitalReorganizer` cases with nonzero
  flip/hash. That is the divergence, not a regression.** The pass-through configuration
  (`flip == 0, hashBits == 0`) is provably unaffected, because the correction term is exactly 0
  there — so the documented `Process(1.0f) == 1.0f` property and every parity case at defaults stay
  green. This is the one and only sanctioned exception to "any red is a regression"; it must be
  re-asserted against the new behaviour in the same commit, never deleted.
- **Subagents: Sonnet or Haiku, never Opus**, model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB). Builds emit nothing for ~70 s; run them in your
  own turn, in the background with a progress tick.
- **Code changes sequential.** Parallel dispatch only for read-only analysis (OMNI §4).
- **No AI attribution on commits.**
- **Cite by symbol, not by line** — every line number in this directory goes stale on the next edit.

## 4. Structure plan

**P1 — repair the stale citations** (mechanical). Ten two-line-wrapped references to the
pre-archive path, listed in `tasks.md` S0.1.

**P2 — the F3 fix: make the drive stage map silence to silence.** Single site, `app/dsp/Drive.hpp`.
Factor the existing bit-mangling body into one static helper and return
`Mangle(input) − Mangle(0.0f)`.

- One definition of the mangle math, two call sites — OMNI §6 satisfied (reused, isolates a
  distinct transformation stage) and §8 satisfied (no second copy of the expression).
- **Computed, never cached.** `flip`/`hashBits` are public fields; a cached offset updated only in
  `SetFlip`/`SetHash` would go stale on any direct assignment. The correction is a handful of
  integer ops at 48 kHz — buy the determinism (OMNI §12: protect against the real hazard, not an
  imagined one).
- Removes the DC at its source, so it is removed for **every** consumer: the post-Stop pin, and the
  same DC during play that the comb/delay/reverb have been amplifying all along.
- **It also removes the audio-rate version of the same defect**: when Drive slots 4/5 are
  themselves modulated, `f(0)` varies per sample, so silence becomes a full-scale *signal*, not
  merely an offset. This is the real link to the operator's parametric diagnosis, and one fix kills
  both forms.

**P3 — S1a.2, transport-gate `modulation_.Step()`.** Operator-ordered (S1a.1), kept on its own
merits. Now correctly labelled: it is **not** the F3 fix.

**P4 — reverb tank in-loop saturator.** The delay got one (fix B2); its structural sibling did not.
That asymmetry is a real latent hazard — *any* sustained overdrive from *any* future cause pins the
output near 1.0 by the §2 stage-4 arithmetic. **Gated on operator ears**, because it changes long
reverb tails; not required once P2 lands.

**P5 — S2, narrow slew on recursive-loop coefficients.** Re-scoped by the audit; see `tasks.md` for
the corrected coefficient enumeration and the corrected placement.

## 5. Dependencies

- P2 depends on P1 only for tidiness; they touch overlapping files, so run them in order.
- P2's harness case is the gate for P2 (write the failing case first — the predecessor shipped five
  green per-stage bound tests that moved no symptom).
- P3 is independent of P2 and may follow it.
- P4 and P5 both depend on P2 having landed and been measured, so their effect is read against a
  chain that no longer self-seeds.
- S3 (the `std::array<dsp::Vco,3>` refactor) and S4 (operator verification) are unchanged and stay
  deferred behind operator testing.

## 6. Non-goals

- No new unit is added to the Stop flush. The flush already resets everything and it is not the
  problem — it was never the problem.
- No blanket slew on all parameters. Audio-rate modulation of VCO pitch/shape/PM stays a feature.
- No change to `External/Sheaf`, and no change to any frozen tree.

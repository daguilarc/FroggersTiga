# Proposal — `frogg3rs-stop-isolation-and-legible-labels`

**Created 2026-08-17, at the operator's instruction, after they ruled `frogg3rs-post-expansion-consolidation`'s
outcome a failure on three counts:** Stop still does not silence the instrument after Randomize All; the
encoder labels hide most of every encoder; and labels that the operator had explicitly said needed no
expansion ("A1 D1 S1 R1 etc") were expanded anyway. This change supersedes the failed parts of its
predecessor. Operator, verbatim in substance: *"investigate their causes and supersede the artifacts with a
new proposal to fix what you failed at."*

---

## 1. The Stop investigation — what was actually wrong, with the falsification trail

**Every number below is from `app/FroggersStopSustainRepro.cpp` (built and run 2026-08-17, per the
`*Repro.cpp` convention; not in the test target). The trail is recorded because THREE prior analyses of
"Stop doesn't stop" each blamed a different mechanism, two of them wrongly, and the difference between
this one and those is that this one measured.**

- **Pass A-C (hand-built §7 condition):** drive pre-gains at max, audio-rate depths held by the frozen
  slate. **REFUTED** — all three passes silent by t+2s (pre-stop peak 0.963, live control). The
  predecessor's §7 mechanism — "bounded limit cycle in the drive loops" — does not survive contact with
  the runtime. Its T4.2 was the task that would have caught this and was never run.
- **Pass D (the real trigger, 5x Randomize All then Stop):** **20/20 trials sustain past t+5s**, levels
  0.15-0.38, some GROWING after Stop. Not environmental: fully reproducible headless.
- **Pass D2-D4 (bisection):** zero all depths at Stop / calm the drive+Freeze knobs at Stop / both —
  **5/5 sustain in every arm.** Both live hypotheses falsified. Zeroing depths made it LOUDER (0.88-0.97):
  the frozen depths were net-attenuating. The drives and Freeze shape the TIMBRE of the bug, not the bug.
- **Pass E (per-unit energy timeline):** the verdict. **`AllIdle == 0` at every checkpoint through t+5s.**
  The voices never reach Idle, so the `ForEachStatefulUnit(Reset)` clear — the thing §7a documented as
  what eventually silences the instrument — **never fires**. Output holds ~0.74 flat; the chain is being
  continuously excited by a note that never ends. Every prior analysis asked "why doesn't the tail die?"
  when the question was "why does the note never end?"
- **Pass F (minimal repro, two parameters):** Curve (Envelope slot 12) x Grace (slot 13), everything else
  default. **curve=1.0: output 0.939 at t+10s, flat, AllIdle=0. curve=0.999: 0.933, crawling. curve=0.5
  and 0.0: silent by t+5s, AllIdle=1.** Root cause confirmed with dose-response.

### 1a. The root cause, read from the code after the measurement pointed at it

`dsp::VcoAdsrState::ComputeRampStep` (`app/dsp/VoiceEnvelope.hpp:279-302`) blends a linear step with an
ease-in one-pole step: `blended = linear + curveAmount * (curved - linear)`, where the curved step is
`stepMagnitude^2 / absRemaining`. **At `curveAmount == 1.0` the linear term vanishes and per-sample
progress is `step^2/remaining`** — integrating, a ramp's duration is `remaining0^2 / (2*step^2)` samples:
QUADRATIC in the knob time and PROPORTIONAL to sample rate. A "1-second" attack at 48 kHz takes ~6.7
hours. Near 1.0 the linear rescue term is `(1-curve)*step`, so durations scale as `1/(1-curve)`: at
curve 0.999 the forced ~50 ms Stop release takes ~45 seconds.

**Composed with the Grace ladder it becomes indefinite** (`VoiceEnvelope.hpp:332-370`): with Grace
active, a pending release deliberately leaves a voice in Attack/Decay "progressing toward Hold" — but at
curve~1 it never arrives, so Release never starts, `AllIdle` never turns true, the clear never fires, and
the voice sits at an audible level (ease-in Decay's slow start lingers NEAR PEAK) for hours. The loud
flat 0.94 in pass F is a voice stuck mid-Decay.

**Why only Randomize All triggers it:** Curve and Grace both default to 0 and no hand patch in any prior
investigation raised them together. Randomize randomizes both. **And §1b makes curve == 1.0 exactly
LIKELY rather than measure-zero.**

### 1b. Second defect found on the way: Randomize ratchets commanded values into the clamp

Sheaf's `Parameter::RandomizeVisibleValue` (`External/Sheaf/projects/synth/src/ParameterModulation.cpp:1723`) computes
`delta = target - TargetValue(0)` — **a delta against the RESOLVED value, which includes live
modulation** — then applies it to the commanded value under the [0,1] clamp. With audio-rate modulation
attached, each press's delta is measured against a modulated snapshot; negative-phase snapshots produce
overshooting positive deltas, and repeated presses ratchet the commanded value into the 1.0 ceiling,
where the clamp holds it. Measured: after 5 presses, **Freeze's commanded value was exactly 1.0000 in
20/20 independent trials, and the three drive pre-gains clustered at 0.97/0.69/0.62** — impossible for
uniform draws. This violates the operator's 2026-08-05 ruling that randomize "should never have been
additive" (the A1 fix covered depth randomization; the value path has the same disease through a
different door). It is ALSO why the instrument reliably lands in the drone: the ratchet steers Curve and
the drives toward their ceilings press after press.

### 1c. What §7 and T4.1 got wrong, recorded per OMNI

The predecessor's §7 traced a real bounded-limit-cycle mechanism and mistook it for the reported bug; its
own §7e flagged that only a runtime capture could settle the tail's boundedness, and no one ran one
before this session closed T4.1 as "keep the behaviour." The lead agent then recorded the operator's
"the effect should be the button's" intent as "keep the accident AND add the button" — a misclosure the
operator caught in use. **The accidental behaviour was never the drive limit cycle at all**, so the whole
keep-vs-kill framing was arguing about the wrong mechanism.

## 2. What changes

- **W1 — Bound every envelope ramp (the fix for the root cause).** `ComputeRampStep` SHALL guarantee
  minimum per-sample progress at every Curve setting (recommended: floor the blended step at a fixed
  fraction of the linear step, e.g. `max(blended, step * kCurveMinProgress)` with `kCurveMinProgress`
  ~0.25 — mechanism is the implementer's, the BOUND is the requirement), so every stage completes within
  a small multiple of its knob time at any sample rate. Curve=0 stays bit-identical linear. ⚠ AUDIT-
  CORRECTED 2026-08-17: this document's first draft additionally had grace expiry force Release from ANY
  non-idle stage with the countdown started at pending-mark. That silently rewrote the approved Grace
  requirement (main spec `froggers-sheaf-parameter-model`: Grace is a minimum Hold "so a short gate
  cannot clip a note before its envelope completes Attack and Decay") — with max attack 1.0s and a short
  grace it would clip legitimate notes mid-attack during PLAY, a bounded case, contradicting its own
  "preserves today's behaviour" claim. The grace ladder is NOT changed. With the ramp bound in place the
  existing ladder is already bounded (Attack/Decay complete within the bound → Hold → countdown →
  Release); the transport-stop path gets its own forced release in W2, where it belongs.
- **W2 — Stop isolates the sustained-drive character to the Freeze button (operator ruling, restated).**
  On the running->stopped edge, alongside the existing `kStopFadeReleaseKnob` release override, the three
  drive pre-gains' EFFECTIVE values SHALL be overridden to unity and the Freeze parameter's effective
  value to zero, for as long as the transport is stopped. During play nothing changes; the Freeze latch
  remains the deliberate route to above-unity loop gain. This is the predecessor's §7d option 2 — belt
  over W1's suspenders, so Stop is silent even in the presence of a future stuck-voice-class defect.
  **Additionally (AUDIT-ADDED 2026-08-17, replacing the first draft's play-time grace change): on the
  running->stopped edge every non-idle voice SHALL enter Release immediately, bypassing the Grace
  minimum-hold and any in-progress Attack/Decay.** Grace is a play-time musical guarantee, not a
  transport one; before the Grace packet, `setGate(false)` forced Release synchronously, so Stop already
  HAD this semantic and lost it as a side effect. Restoring it Stop-side keeps play behaviour untouched
  and tightens worst-case Stop-to-silence from ~9s (4x attack + 4x decay + grace under W1 alone, at
  kMaxAttack/Decay/GraceSeconds = 1.0 each) to the ~50 ms forced release plus wet tails. W1 remains
  load-bearing: the forced Release ramp itself is unbounded at curve~1 without it.
- **W3 — Randomize draws land the drawn value (fix the ratchet).** The app's randomize path SHALL
  produce commanded values equal to the drawn uniform value regardless of live modulation. Per the
  `froggers-upstream-uptake` rule the app-side route is checked first: the app already owns the press
  wrapper (`PressBankWithRandomValue`) and MAY replace press-with-RandomHeld by drawing its own value and
  writing it via `HandleSetAbsolute` to both scene poles. The `RandomizeVisibleValue` delta-vs-resolved
  behaviour is ALSO filed upstream (UPSTREAM-SHEAF-ASK.md ask #16 — NOT "#9": the local ledger already
  numbers asks #1-#15 and its header warns duplicate numbering caused a mixup before; audit-corrected
  2026-08-17) since it bites any Sheaf app that randomizes under modulation; this app does not wait for
  it.
- **W4 — Labels: each parameter's NATURAL name, and never over the ring.** Two operator rulings bind
  here, and the second corrected this document's own first draft: (1) labels like "A1 D1 S1 R1 etc" need
  no expansion — the short form IS the name; (2) 2026-08-17, verbatim in substance: "A1 S1 can use short
  names, but it is not acceptable for every parameter" — a truncation like `CmbOff` may not stand alone,
  which was the whole reason the predecessor's label rework existed. So: an operator-approved label list
  assigns every parameter its rendered form — canonical short forms render in the native single-row
  idiom, truncation-class parameters render their readable name — and NEITHER blanket expansion (the
  shipped defect) NOR blanket abbreviation (this document's first draft) satisfies it. Whatever renders
  sits clear of the ring's drawn arc — ⚠ MEASUREMENT CORRECTED 2026-08-17 (P4, measured not assumed): the cell is 136.333x68.0, NOT
  136.3x88.3 — that figure predates the Reset row, which shrank the encoder rows. Ring baseRadius is
  25.8 (not ~34.5). The DEFECT is unchanged in kind: the label plate covered the ring's lower arc — which for legible long names means a real label
  band below the ring (uniform radius trade or row reflow, decided on the mock). The operator's eyes are
  the acceptance gate, on the mock BEFORE building and on screen after, carried from the failed T1.1.

- **W2b — Grit joins the stopped-state override (MEASURED 2026-08-17, packet 2b; the THIRD mechanism).**
  With W1 + W2 in place the pass-D condition still reproduced 20/20. Measured, with an app-free positive
  control: the Stop-edge clear DOES fire (reverb magnitude 0.82 -> exactly 0.000000, 3/3 trials) and the
  reverb's input decays below 1e-7, so nothing re-injects — yet the tank's own state locks flat. Cause:
  `dsp::DigitalReorganizer::Mangle` (`app/dsp/Drive.hpp:363-382`) quantizes to 8-bit buckets and XORs
  the low bits, so an input crossing a bucket boundary flips high bits — LOCAL GAIN IS UNBOUNDED, unlike
  the `PadeSaturator::Saturate` that guards the rest of the loop. Reverb's Grit stage (slot 11,
  `Reverb.hpp:539-543`) sits INSIDE the tank's feedback path ahead of that saturator, so with
  `fb ~= 0.999` (Hold high) the ordinary sub-audible release tail is re-amplified every round trip into
  a saturator-BOUNDED but never-decaying limit cycle. Control: isolated Reverb, one 0.01 seed then exact
  zero for 6.25 s — at the drawn Grit 0.8094 state magnitude locks at 0.306814 forever; at Grit 0 the
  same seed decays to 1.98e-7. So the Grit parameter's effective value SHALL also resolve to zero while
  the transport is stopped, joining W2's list (Grit 0 is an exact bit-identical bypass by construction,
  `Reverb.hpp:534-538`). NOTE the predecessor's §7 guessed "bounded limit cycle" as the mechanism and was
  ruled wrong about the DRIVE loops; this is a real bounded limit cycle in a different unit, and the
  difference is again that this one was measured.
- **W5 — Max attack halves (operator ruling 2026-08-17, delivered mid-audit, verbatim: "that max attack
  is also way too long. half a second at most").** `kMaxAttackSeconds` (`app/dsp/VoiceEnvelope.hpp:63`)
  goes 1.0f -> 0.5f, extending the constant's own recorded ruling history (2.5s -> 1.0s, operator
  2026-07-29). Scope is ATTACK ONLY — the operator named attack alone. `kMaxDecaySeconds` (1.0f) keeps
  its value but its stated rationale ("Mirrors kMaxAttackSeconds (1.0f)") is now false and must be
  rewritten as decay's own standing judgment; `kMaxGraceSeconds` untouched. The ITEM 4 parity test
  (`FroggersDspParityTests.cpp:234-254`) keys its assertions to the constant; its prose citing 1.0s is
  updated to match.

## 3. Non-goals

- No new parameters, no slot changes, no Sheaf fork or local patch (Sheaf#9 is an ask, not a dependency).
- The Freeze button, Reverse Blend, Diffusion, Reset and Record work from the predecessor stands — the
  operator's failure ruling covered Stop, labels, and the label-scope override, not the Delay slate.
- W1's `kCurveMinProgress` and any audible change to high-Curve envelope FEEL is by-ear tunable; the
  REQUIREMENT is bounded completion, not a particular curve shape.
- **Known open defect, recorded not fixed (T2.6, packet 2b/2c 2026-08-17).** `dsp::DigitalReorganizer::
  Mangle` (`app/dsp/Drive.hpp:363-382`) quantizes its input to 8-bit buckets and XORs the low bits, so an
  input crossing a bucket boundary flips high bits -- LOCAL GAIN IS UNBOUNDED there, unlike the
  `PadeSaturator::Saturate` that bounds the rest of the reverb tank's feedback loop. This is reachable
  DURING PLAY anywhere Grit (Reverb slot 11, the only knob-reachable user of `Mangle`) sits inside a
  feedback loop with high `fb` -- packet 2b's control measured it directly: isolated `dsp::Reverb`, one
  0.01 seed then exact zero for 6.25s, locks at a constant 0.306814 at the drawn Grit 0.8094 versus
  decaying to 1.98e-7 at Grit 0. W2b's stopped-state Grit override (T2.5) only prevents this from
  mattering while the transport is STOPPED; it does nothing for the same mechanism during play. Bounding
  `Mangle`'s local gain is a TONE change to an audible parameter and requires the operator's own ears to
  judge, not a mechanical bug fix -- deliberately NOT done in this change. Filed as a follow-up.

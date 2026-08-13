# Proposal — `frogg3rs-validation-and-upstream-uptake`

**Created 2026-08-12.** Supersedes the outstanding work of `frogg3rs-bank-expansion`, which is built and
archived. That change grew every bank from nine parameters to fourteen — thirty new parameters — and left
exactly two things no implementer could close, plus a standing dependency on upstream Sheaf. This change
carries both, and nothing else.

**This change is scoped to what a machine cannot finish.** Everything in `frogg3rs-bank-expansion` that
could be verified by a test IS verified by a test: 211 tests, 0 failures, 0 warnings, with a positive
control run against every measurement. What remains needs either a human's ears and eyes, or a dependency
this project does not control.

---

## Why

`frogg3rs-bank-expansion` is built, green and archived — but "green" covers only what a test can see. Two of
its tasks were written from the start as un-closable by an implementer because they need a human's eyes and
ears, four shipped ranges were chosen by an implementer against no specified value, two of its measurements
were reported but never pinned as regression tests, and six upstream Sheaf gaps remain outstanding against a
pinned dependency this project deliberately never forks. Leaving those inside an archived change would
record them as done. They are not.

## What Changes

- Adds a requirement that a bank parameter be **audibly effective** across its range and in the direction its
  name implies — a property registration, bounding and default-parity tests do not reach.
- Adds a requirement that a **measured bound be pinned by a regression test**, not left as prose.
- Adds a new capability, `froggers-upstream-uptake`, requiring that an upstream gap be **proven
  app-unreachable before it is treated as blocking**, and that the pinned dependency is never forked or
  locally patched.
- Schedules the by-ear and visual validation the predecessor could not close, and parks the six upstream
  items behind an explicit re-check gate.

## 1. Objective

Two capabilities, deliberately kept separate because one is reachable now and the other is not:

1. **Hands-on validation of the thirty new parameters.** Automated tests prove each knob is wired, bounded
   and default-neutral. They do NOT prove any of it sounds like the thing its name promises, and two tasks
   were explicitly written as un-closable by an implementer for that reason.
2. **Uptake of upstream Sheaf fixes**, when and if they land. `External/Sheaf` is pinned at `77a3019e` and
   is deliberately not forked — a fork was tried on 2026-07-27 and reverted the same day because the
   gitlink was unresolvable from any other checkout (`UPSTREAM-SHEAF-ASK.md`). So every upstream gap is an
   ask, and this change is where the app-side uptake work lives once an ask is answered.

## 2. What `frogg3rs-bank-expansion` actually left open

Verified against that change's own `§EXECUTION` record, not assumed:

- **T6.2 — the over-length label rework's acceptance criterion is VISUAL.** The rendering is verified
  programmatically (six character slots emitted for `CmbOff` where four were before; badge-chip clearance
  checked algebraically against `AppendBadge`'s own formula), but **nobody has looked at it.** The
  predecessor change records a prior UI change in this project taking four attempts precisely by asserting
  a weaker property than "the operator can see it."
- **T8.4 — Ring Mod's low-frequency end is a by-ear taste call.** It gates nothing: "off" is the shared
  zero taper at the bottom of the knob, not a frequency. The carrier range currently ships at 20 Hz - 5 kHz,
  an implementer's judgement call with no spec value behind it.

**Beyond those two, this change adds by-ear validation the predecessor never scheduled at all** — see §3.

## 3. Why by-ear validation is its own scope, not a formality

The predecessor's tests establish, for each of the thirty parameters, that it is registered, reachable,
bounded, and that its default reproduces the previous behaviour. **Three classes of defect survive all of
that**, which is why this is real work rather than a sign-off:

- **A knob wired to the wrong end of its own range** passes every bound and default test and sounds
  backwards. Nothing automated distinguishes "brighter as it rises" from "darker as it rises."
- **A range that is technically safe and musically useless.** Four parameters ship with ranges chosen by an
  implementer against no spec value: Ring Mod's carrier (20 Hz - 5 kHz), `kMaxDecaySeconds` (1.0 s), the
  Grace maximum (1.0 s), and Curve's shape family. Each is defensible and none is validated by ear.
- **A measured bound that is correct and still wrong for playing.** Drive's Bias is bounded to +-0.02
  because peak swing rises with any nonzero bias on an unbounded 5th-order polynomial — measured, +6.1% at
  the bound. Whether that range is musically worth having at all is a question no measurement answers.

**Also carried here, recorded rather than left implicit:** two of the predecessor's measurements
(Reverb Tilt, Reverb Tuned) were performed and reported but NOT pinned as regression tests — they ran in
standalone harnesses. They are correct today and nothing guards them tomorrow.

## 4. Upstream: what is actually blocked, and what is not

**Corrected relative to a claim made and withdrawn during the predecessor change, because getting this
wrong once is the reason it is stated carefully here.** Sheaf issues 1-6 are open upstream. Issue 7 —
`EncoderDraw`'s 4-character label cap — **was filed by this project and then withdrawn and closed by this
project as not-planned**, because the premise was wrong: `BuildFourteenSegmentCommands` is public with
`numChars` as an ordinary parameter, and the app owns the `std::vector<DrawCommand>` that
`BuildEncoderDrawCommands` returns, so the app composes its own label block. That is how T6.1 shipped, with
`External/Sheaf` untouched.

**The lesson is recorded because it generalizes to every item below:** "no app-side lever exists" is a
claim about a whole surface and needs the whole surface read. A missing configuration field is a different,
much smaller fact. **Before any task here is treated as upstream-blocked, that check is repeated.**

Consequently this change carries **no** work for issue 7, and treats the remaining six as genuinely
upstream-gated but re-checkable.

## 5. Non-goals

- **No new parameters.** All six banks are at fourteen and the slate is closed.
- **No Sheaf fork and no local Sheaf patch.** The pinned-upstream property is worth more than the features
  (`UPSTREAM-SHEAF-ASK.md`), and this constraint is what makes §4's app-side-first check load-bearing
  rather than optional.
- **The design doc's open question 8** — the ASR envelopes cannot modulate anything, and the fifteen-source
  slate is full — remains open and out of scope. It is a modulation-slate question, not a bank-slot one,
  and it may outrank everything here.
- **Whether `kPmLfoDepth = 0.15` is the right PM depth ceiling** stays an open by-ear tuning item, fixed by
  changing the constant if it proves too shallow, not by adding a knob.

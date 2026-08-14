# Tasks — `frogg3rs-validation-and-upstream-uptake`

> **Read `proposal.md` first.** It carries the scope and why each item is here rather than in the archived
> `frogg3rs-bank-expansion`.

**Inherited state, VERIFIED not assumed** (archived change's `§EXECUTION`, re-run at the end of session 8):
10 binaries, **211 tests, 0 failures, 0 warnings**. `External/Sheaf` pinned at `77a3019e`.

## §0 Standing constraints

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB).
- **`External/Sheaf` is pinned and unpatchable, and is never forked.** See T2.0 before recording anything
  as upstream-blocked.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose resolution requires the operator's ears or eyes.** T1.1 and
  T1.2 are exactly that, and they are the reason this change exists.

---

## T1 — Hands-on validation of the thirty new parameters

- [ ] **T1.1 — VISUAL, operator only. Carried from the archived change's T6.2.** Look at the over-length
      label rendering on a real run. The app-side treatment emits the full label (six character slots for
      `CmbOff` where four rendered before) in a slightly larger plate. Confirm on screen: (a) every label's
      full text legible without truncation, (b) the modulation badge chips still render correctly in the
      same cells, (c) the plate applied consistently across all six banks, not just the worst-case label.
      **A test asserting a weaker property than "the operator can see it and confirms it" does not close
      this** — a prior UI change here took four attempts for exactly that reason.
- [ ] **T1.2 — BY EAR, operator only. Carried from the archived change's T8.4.** Settle Ring Mod's
      low-frequency end. Ships at 20 Hz - 5 kHz, an implementer's choice against no specified value. "Off"
      is the shared zero taper at the bottom of the knob, so nothing depends on the low end except taste.
- [ ] **T1.3 — BY EAR: sense and usefulness of all thirty, bank by bank.** For each new parameter confirm
      it is audible across its range and moves in the direction its name implies. A knob wired to the wrong
      end of its range passes every automated test this project has and sounds backwards. Work bank by
      bank; record any parameter that is inaudible, inverted, or musically dead, with the bank and slot.
- [ ] **T1.4 — BY EAR: the four ranges chosen without a specification.** Ring Mod's carrier span,
      `kMaxDecaySeconds` (1.0 s), the Grace maximum (1.0 s), and Curve's shape family (an ease-in one-pole
      blend, linear and bit-identical at its default). Each is defensible and none is confirmed. Report
      keep-or-change per item.
- [ ] **T1.5 — BY EAR, then decide: is Drive's Bias worth its bounded range?** It is bounded to `+-0.02`
      because peak swing rises with ANY nonzero bias on an unbounded 5th-order polynomial (measured: 363.4
      at bias 0, 385.7 at the bound, +6.1%). The measurement cannot say whether so narrow a range is worth a
      slot. If it is not, say so — removing it is cheaper now than after it ships.
- [ ] **T1.6 — Pin the two unguarded measurements as regression tests.** Reverb Tilt (post-limiter peak
      0.8000 at centre vs 0.7973 at brightest) and Reverb Tuned (peak exactly 0.8000 across every sweep rate
      and room size) were measured in standalone harnesses and reported, but NOT pinned. Add regression
      tests, and per OMNI §9.1 confirm each FAILS when the bound is deliberately broken before trusting it.
- [ ] **T1.7 — By-ear tuning item, carried forward and still not a parameter.** Is `kPmLfoDepth = 0.15` the
      right maximum PM depth? If too shallow, change the constant — do NOT add a knob to rescale it. This
      was rejected once already as a meta-control.
- [ ] **T1.8** Re-run the full suite and report counts; any red is a regression against 211/0/0.

## T2 — Upstream Sheaf uptake (issues 1-6), currently blocked

- [ ] **T2.0 — GATE ON EVERY ITEM BELOW, and it is not a formality.** Before treating any item as
      upstream-blocked, prove no app-side route exists by reading the WHOLE relevant surface — the public
      functions and the values the library returns to us — not just the absence of a configuration field.
      **This project got that wrong once**: it concluded `EncoderDraw`'s 4-character label cap was
      unreachable because `EncoderDrawState` had no field for it, wrote a blocking gate, and filed an
      upstream issue; the app could in fact compose its own label block, and the issue was withdrawn and
      closed. Repeat this check per item, and record the result either way.
- [ ] **T2.1** Re-read Sheaf issues 1-6 against the CURRENT pin and record, per issue: still open upstream?
      still affects this app? and — per T2.0 — is it actually app-unreachable? Issues as filed:
      1 embedded surfaces can't resolve against a live extent; 2 unlabelled sidebar CPU percentage and
      out-of-tree apps copying `Main.cpp`; 3 verbatim-copied `Info.plist` breaking Finder launch, and
      `ControlStyle::caption` placement; 4 an app can't distinguish "device presented an input channel"
      from "user routed an input"; 5 `GangedRandomLfoVisualizer`'s unconditional opaque background;
      6 `check_ui_boundary.sh` aborting on macOS bash 3.2.
- [ ] **T2.2** For each item T2.1 finds app-reachable: implement it app-side and close it out here, rather
      than waiting on upstream. Sequential code changes, own packet each.
- [ ] **T2.3 — BLOCKED, not schedulable.** For each item that is genuinely upstream-only: when the pin
      moves and the fix is present, take it up app-side and REMOVE the corresponding app-side workaround if
      one exists. Nothing here is actionable until `External/Sheaf`'s pin changes.
- [ ] **T2.4 — Issue 7 carries NO work and is recorded closed.** It was this project's own filing, withdrawn
      and closed as not-planned because the premise was wrong. Recorded so it is not re-opened as an
      upstream dependency.

## T3 — Delay's three vestigial slots: Detune, Color, Halo (`proposal.md` §6)

**Operator-raised 2026-08-13. Detune is ruled OUT; what replaces it is open.** Color and Halo have no
destination of their own — each is averaged into a neighbouring knob's value — so three of Delay's nine
original slots hold one-and-a-half controls between them.

- [ ] **T3.1 — OPERATOR DECISION, not closable by an implementer.** Decide the fate of each of the three
      slots. The recoverable candidates, both already tiered in the archived design doc and both left
      unbuilt when the predecessor shipped Width balance and Crush into their slots: **Diffusion** (Tier 3,
      allpass smear, genuinely new, flagged for gain leak if the allpass chain is badly tuned) and
      **Freeze** (structurally new; the deliberate form of the accidental sustain in §7). A third option
      needs no new parameter at all: retire Detune and give Color and Halo real destinations.
- [ ] **T3.2** Whatever T3.1 picks, the fold itself comes out: `params.ddet = 0.5*(ddet + Color)` and
      `params.dmod = 0.5*(dmod + Halo)` (`app/dsp/Delay.hpp`'s row-mapping) must stop averaging two knobs
      into one value, so each slot owns exactly one job.
- [ ] **T3.3** Re-check the survivors against the selection rule before building: if the modulation matrix
      can already reach the effect by routing one of the fifteen sources onto an existing parameter, the
      parameter is rejected. Diffusion and Freeze both pass today; re-confirm rather than inherit it.
- [ ] **T3.4** The Delay research files this analysis would ideally cite are GONE — they lived in a session
      scratchpad outside the repo. If a fresh candidate sweep is wanted, it starts from the archived
      `BANK-EXPANSION-DESIGN.md` table, not from those files. Recorded so nobody hunts for them.

## T4 — Stop does not silence the instrument (`proposal.md` §7)

**The operator reported this, likes the sound, and has NOT asked for a fix. Nothing here changes DSP
without an explicit decision.** What is scheduled is the part that is not a matter of taste: three
parameters can place a feedback loop's near-origin gain near 4, which makes a non-decaying limit cycle
reachable during ordinary play, not only after Stop.

- [ ] **T4.1 — OPERATOR DECISION.** Whether to keep the behaviour, and if so whether to keep it as an
      accident or promote it to a real control. **Freeze (`proposal.md` §6.3) is the deliberate form of
      exactly this effect** and is already a tiered, unbuilt candidate — so "keep it" and "make it a knob"
      are not opposed. The five options are listed at §7d; option 1 (clamp the loop-gain PRODUCT below 1)
      is the only one that addresses the instability rather than the symptom.
- [ ] **T4.2 — Runtime capture, the one thing reading could not settle.** Determine whether the tail is
      strictly bounded by the ~1.05 s Grace-plus-fade window before `Reset()` lands, or can run
      indefinitely because something re-seeds the loop independently of the transport gate. Static analysis
      cannot answer this; capture the output after Stop with the drives high and audio-rate modulation
      active, and report the decay envelope. Per OMNI §9.1 confirm the capture would SHOW a decaying tail
      if one existed, before reporting that none does.
- [ ] **T4.3 — Correct the archived rule where it is cited, whatever T4.1 decides.** `frogg3rs-bank-
      expansion` §7a established that a pre-gain on an in-loop saturator's argument cannot raise the loop's
      per-sample BOUND. True, and it was read throughout that change as clearing those parameters
      generally. It does not: `Saturate` has unit slope at the origin, so it is a ceiling, not a
      contraction. The rule needs its companion stated wherever it is cited — **a bound is not a decay** —
      including at the withdrawal of Comb Drive's headroom flag, which was the one flag that would have
      caught this.
- [ ] **T4.4 — Comb Drive specifically.** It is the third instance and was outside the investigation's
      scope; the comb sits in the always-on filter chain rather than behind a send, and `FilterFx.hpp`'s own
      header already records that a self-oscillating comb at ±0.95 sits near instability before Comb Drive
      multiplies it by up to four. Confirm by measurement whether the comb reaches a non-decaying state at
      reachable knob positions, with a positive control.

## Recorded, not scheduled — no task closes these

- **The design doc's open question 8** — the ASR envelopes cannot modulate anything and the fifteen-source
  modulation slate is full, with three VCO-EF slots partially duplicating what a true envelope source would
  do better. A modulation-slate question, not a bank-slot question, and it may outrank everything above.
  Untouched since it was first raised.
- **All six banks are closed at fourteen parameters.** No further bank-fill candidate is in scope here. The
  selection rule that governed the expansion still stands for any future proposal: if the modulation matrix
  can already produce the effect by routing one of the fifteen sources onto an existing parameter, the
  parameter is rejected.

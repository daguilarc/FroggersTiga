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

**Operator-raised and DECIDED 2026-08-13 (T3.1).** Color and Halo had no destination of their own — each was
averaged into a neighbouring knob's value — and Detune overlapped Stereo width at about a twelfth its
strength, so three of Delay's nine original slots held one-and-a-half controls between them. All three are
replaced: **Detune -> Freeze, Color -> Reverse Blend, Halo -> Diffusion.** What remains open below is HOW
each is built, not whether — and T3.1a is a genuine blocker on Freeze.

- [x] **T3.1 — DECIDED by the operator, 2026-08-13.** All three vestigial slots are replaced:
      **Detune -> Freeze, Color -> Reverse Blend, Halo -> Diffusion** (`proposal.md` §6.4). **Color is NOT
      made into a real tone control** — the shipped Feedback tone already damps the repeats and a second
      wet-output tone was judged not to earn a slot. Ducking stays cut, as the research's own first-cut
      recommendation. Slot assignment is a recommendation only: Halo -> Diffusion is name-adjacent and the
      same idea, the other two are interchangeable.
- [x] **T3.1a — DECIDED, operator 2026-08-13: Freeze clamps.** Full Freeze is deliberate loop gain = 1
      while `fbDrive` reaches 4.0, so the product would reach ~4 and full Freeze would GROW instead of hold.
      **Freeze clamps the loop-gain product to 1.** Two consequences the implementer must not miss:
      - **The clamp is CONTINUOUS, not applied once at freeze-on** (operator, same ruling). Un-toggling
        Freeze restores sub-unity loop gain and the tail decays normally — the control can never leave a
        runaway loop behind it. Implement it as a property of the freeze mapping evaluated every sample,
        not as a latched state change.
      - This is the same clamp `proposal.md` §7d option 1 proposes for the accidental case. **Building
        Freeze therefore fixes, or half-fixes, the Stop-sustain bug as a side effect** — T4.1 must be
        settled knowing that, since the operator likes the accidental sound.
- [ ] **T3.1b — Freeze SHALL be built as a crossfade, not a write-enable toggle.** `write = inSignal *
      (1 - freeze)` with `fb_eff = lerp(fbk, 1.0, freeze)`. Built as a toggle it fails the operator's own
      continuous-range rule — the rule that cut Cycle and Hard Sync — and the whole reason it passes is
      that its midpoint (new input bleeding in over a slowly-decaying loop) is a real playable state.
- [ ] **T3.1c — Diffusion's allpass coefficient SHALL be bounded by the knob mapping**, using the same
      `-0.98f` margin `dsp::DriveBlendPhase` and `dsp::Comb` already carry. Allpass sections are unity-gain
      by construction only while the coefficient stays strictly inside the unit circle. **Do not assert
      this in a comment** — §7 records what this codebase's one loop-gain-above-unity defect cost, and it
      came from proving a bound and assuming a contraction.
- [x] **T3.1d — Reverse Blend: continuity accepted, buffer smoothing DECIDED, operator 2026-08-13.**
      Its continuity is by construction rather than by precedent — every shipped reference makes reverse a
      discrete MODE, and the continuous forward/reverse crossfade is the research's own extrapolation. It
      passes the continuous-range rule (the midpoint is a real mixed texture). **The known edge-of-buffer
      click hazard at the crossfade is answered by buffer smoothing**, which the operator has accepted as
      part of the cost. The smoothing is not optional garnish: it is what makes the control shippable, so
      it is specified with the parameter rather than left to implementation taste.
- [ ] **T3.2** Delete the fold: `params.ddet = 0.5*(ddet + Color)` and `params.dmod = 0.5*(dmod + Halo)`
      come out of `MapRowsToDelayParams` (`app/dsp/Delay.hpp`), so each slot owns exactly one job. Note it
      changes Detune's own reachable range, which is moot once Detune is retired but matters if the two
      land in separate commits.
- [ ] **T3.3** Re-check each survivor against the selection rule before building: if the modulation matrix
      can already reach the effect by routing one of the fifteen sources onto an existing parameter, it is
      rejected. All three pass today; re-confirm rather than inherit it.
- [x] **T3.4 — DONE: the research is checked in** at `research/`, beside this proposal, rather than in a
      temp scratchpad one cleanup away from deletion. Two earlier versions of this task were wrong in
      opposite directions — the first said the files were gone, the second said to copy them only if
      relied on.

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

## T5 — Four new controls: Record, Freeze, Reset Page, Reset All (`proposal.md` §8)

**Operator-requested 2026-08-13 and deliberately kept in this change — these and the Delay slate are one
user story.** T5.1-T5.2 are buildable now; T5.3 needs an operator answer first.

- [ ] **T5.1 — Reset Page / Reset All.** New row appended after `Randomize` in
      `FroggersCellMap::kRightRows`, two `Button` nodes each `Extent::Weight(2.0f)` — the same two-halves
      weighting `AppendRandomizeRow` already uses, which is what "same size" means concretely.
      **Mirror `RandomizePage`/`RandomizeAll`'s own enumeration rather than re-deriving it**:
      `drillIn.BankRef()` for the page, `for (bankIx...) model.BankAt(bankId)` for all, INCLUDING their
      `drillIn.Level()` branching — a Reset that ignores drill level means the wrong thing while drilled in.
      Parameter minimum is uniformly `0.0` (`ClampToRange` ignores `RangeKind` and there are no
      per-parameter min/max fields), so no per-parameter table is needed.
- [ ] **T5.1a — ⚠ Depths reset to NEUTRAL (0.5), never to 0.0.** Depth parameters are bipolar and their off
      position is `kNeutralModulationDepthCenter`. **Writing literal 0.0 is FULL NEGATIVE depth** — a reset
      that took "set depths to 0" literally would produce a maximally-modulated patch while appearing to
      clear it, and would look correct in any test that only asserted "the value is 0". Reuse the existing
      `ZeroExistingModulationDepths(Parameter&)`, which already writes the neutral centre to both scene
      poles and deliberately skips unmaterialized depths.
- [ ] **T5.1b — Test that the trap is not re-introduced.** Assert after a reset that each touched depth
      reads its NEUTRAL value and that the parameter is audibly unmodulated — not merely that some number
      changed. Per OMNI §9.1, confirm the test FAILS if the reset is changed to write 0.0.
- [ ] **T5.2 — Freeze button, beside Stop, as a `Draw` node.** Append a third child to
      `AppendTransportRow`'s row, matching Play/Stop's existing 28 px plate idiom. **It must be a Draw node,
      not a `Button` with `selected` and not `NodeKind::Toggle`:** the library renders selected state as
      `brighter(0.14f)` on the background and `TextColourForNode` has no `selected` branch at all, so text
      colour never changes — a genuine inversion is not available from the library's state handling
      (`UPSTREAM-SHEAF-ASK.md` item 3, landed only partially). A Draw node emits its own commands, so the
      inversion is free and needs no upstream change. Latches on one click, releases on the next, driving
      the Delay Freeze parameter to maximum while latched.
- [ ] **T5.2a** Depends on Freeze existing as a parameter (T3.1 / T3.1a-b). Do not build the button first.
- [ ] **T5.3 — OPERATOR DECISION before any Record work.** Two things asked for are not available and one
      is, so the request cannot be built as stated (`proposal.md` §8.3):
      - **Sheaf provides NO recording capability** — no writer, no output tap, no API.
      - **The audio config page cannot host the configuration.** It is built by Sheaf's internal
        `BuildAudioPageTree` from a closed snapshot with no app extension point, and this app never wires
        the page at all. Checked before being called blocked, per this change's own rule: unlike the
        encoder label, there is nothing app-side to compose over. **Making it possible is an upstream ask.**
      - **The app core cannot use JUCE** — `check_no_juce.cpp` compiles the core WITH JUCE on the include
        path and fails if any header resolves into it. v1's recorder is entirely JUCE.
      **The decision:** capture in the core and export in `FroggersMain.cpp` (the JUCE host, outside the
      gate) is the shape that fits the app's existing split — but **which formats** (WAV alone needs no
      dependency at all; MP3/FLAC/OGG pull JUCE into the export layer, which is allowed there; v1 shipped
      all four) and **where the configuration lives** are operator choices, not implementation details.
- [ ] **T5.4** Rebuild and re-run the full suite; report counts. Baseline 213/0/0.

## Recorded, not scheduled — no task closes these

- **The design doc's open question 8** — the ASR envelopes cannot modulate anything and the fifteen-source
  modulation slate is full, with three VCO-EF slots partially duplicating what a true envelope source would
  do better. A modulation-slate question, not a bank-slot question, and it may outrank everything above.
  Untouched since it was first raised.
- **All six banks are closed at fourteen parameters.** No further bank-fill candidate is in scope here. The
  selection rule that governed the expansion still stands for any future proposal: if the modulation matrix
  can already produce the effect by routing one of the fifteen sources onto an existing parameter, the
  parameter is rejected.

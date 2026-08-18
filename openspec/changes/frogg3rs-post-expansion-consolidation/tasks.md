# Tasks — `frogg3rs-post-expansion-consolidation`

> **Read `proposal.md` first.** It carries the scope and why each item is here rather than in the archived
> `frogg3rs-bank-expansion`.

**Current state, VERIFIED not assumed:** 10 binaries, **213 tests, 0 failures, 0 warnings** — the archived
change's `§EXECUTION` recorded 211, and the encoder-label rework added two more. `External/Sheaf`
pinned at `77a3019e`. **Re-run 2026-08-13 during the audit, not inherited: 213 passed / 0 failed across
the ten test binaries** (the 0-warnings half is carried from the predecessor's build, not re-measured).
**Superseded by execution as of packet P1: the suite now stands at 211/0/0** — see T1.8 for why, and do not
treat 213 as the number to match.

**AUDITED against the code 2026-08-13 — `proposal.md` §9 is the record.** Five artifact claims were wrong
and are corrected in place at T1.1, T3.1c, T5.2, `proposal.md` §7a, and a missing `MODIFIED` spec delta
(T3.5). Two stale records were found while auditing T5.1's own anchors: the code comment is **folded into
T5.1's packet** (T5.1d, operator 2026-08-13), the live-spec drift stays recorded (T5.1c) because fixing
it would widen this change's delta set. Everything else held up.

**Read the blocks in dependency order, not numeric order.** T3 (Delay slate) gates T5.2 (the Freeze button),
and T3.1a's clamp decision is coupled to T4.1's — see `proposal.md` §6.4b-i.

## §0 Standing constraints

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB).
- **`External/Sheaf` is pinned and unpatchable, and is never forked.** See T2.0 before recording anything
  as upstream-blocked.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose resolution requires the operator's ears or eyes.** T1.1 and
  T1.2 are exactly that. They were this change's original reason for existing; it has since grown four
  more blocks, so they are no longer the whole of it.

---

## T1 — Hands-on validation of the thirty new parameters

- [x] **T1.1 — EXERCISED BY THE OPERATOR 2026-08-17 AND FAILED.** Two counts: the two-row plate hides
      ~95% of every ring's lower semicircle, and the long-name expansion was applied to ALL cells when
      the operator had ruled short names (`A1 D1 S1 R1`) need none — the predecessor's own enumeration
      says Envelope was "already clean" and out of scope. Superseded by
      `frogg3rs-stop-isolation-and-legible-labels` W4/T4, where the operator gate now runs BEFORE the
      build, on a mock. Original task text follows.
      **T1.1 — VISUAL, operator only. Carried from the archived change's T6.2.** Look at the over-length
      label rendering on a real run. **Description corrected 2026-08-13 against
      `app/FroggersUiSurface.hpp:1218-1366` — see `proposal.md` §9.** The earlier wording here ("six
      character slots for `CmbOff` where four rendered before, in a slightly larger plate") was inherited
      from the predecessor's `§EXECUTION` record and does not describe what ships. What ships: the app
      strips Sheaf's own trailing four-character label block (:1232-1239), draws **its own** plate below
      `centerY` (:1293-1305), and renders the parameter's **long** name (:1342 — "Comb offset", not the
      short name `CmbOff`) split across **two rows of TEN 14-segment character slots** (`kGridColumns =
      10`, :1348/:1364), each row centred by leading-space padding. Confirm on screen: (a) every label's
      full text legible without truncation **on both rows**, (b) the modulation badge chips still render
      correctly in the same cells, (c) the two-row plate applied consistently across all six banks, not
      just the worst-case label, (d) one-line names render on row 0 with row 1 blank rather than
      re-centred. **A test asserting a weaker property than "the operator can see it and confirms it"
      does not close this** — a prior UI change here took four attempts for exactly that reason.
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
- [x] **T1.6 — DONE 2026-08-14 (lead agent; §9.1 proof by subagent packet P8v). Suite 245/0 pending final
      count.** Two pins added to `FroggersDspParityTests.cpp`: Tilt swept 0-1 (11 positions) and Tuned
      swept as the archived grid (4 sizes x 3 step periods), each asserting the wet path never exceeds
      `dsp::kStageCeiling` with a positive control proving the rig sits AT the ceiling (archived numbers
      0.8000/0.7973 reproduced as 0.8 at every point).
      **P8v's finding, recorded because it changes what these pins guard:** breaking `kStageCeiling`
      itself is SELF-REFERENTIAL — measured peak and asserted bound move together (0.899999 vs 0.9, still
      green), so a deliberate ceiling retune tracks rather than fails. What the pins genuinely catch is
      the wet limiter being loosened or bypassed: `kReverbWetLimiterCeiling` -> 1.2f drove peaks to
      1.19562 (tilt) / 1.16545 (tuned) against the 0.8 bound — 114/117, restored byte-identical to
      117/117. That is the regression class the spec delta names, and the archived sabotage records for
      both measurements were limiter bypasses too, so the guard matches its history.
      **Original task text:** Pin the two unguarded measurements as regression tests. Reverb Tilt (post-limiter peak
      0.8000 at centre vs 0.7973 at brightest) and Reverb Tuned (peak exactly 0.8000 across every sweep rate
      and room size) were measured in standalone harnesses and reported, but NOT pinned. Add regression
      tests, and per OMNI §9.1 confirm each FAILS when the bound is deliberately broken before trusting it.
- [ ] **T1.7 — By-ear tuning item, carried forward and still not a parameter.** Is `kPmLfoDepth = 0.15` the
      right maximum PM depth? If too shallow, change the constant — do NOT add a knob to rescale it. This
      was rejected once already as a meta-control.
- [ ] **T1.8** Re-run the full suite and report counts; any red is a regression against the CURRENT baseline, which moved to **211/0/0** in packet P1 (two tests deleted with the Detune DSP and the Color/Halo fold they pinned) and rises again as each later packet adds its own. Historically **213/0/0** (the
      archived change ended at 211; the encoder-label rework added two).

## T2 — Upstream Sheaf uptake (issues 1-6 and 8), currently blocked

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
- [x] **T2.5 — Issue 8 is OURS, filed and then downgraded, and carries no work here.** *"Out-of-tree apps
      can't add settings to the audio page, or add a sidebar page of their own"* — filed 2026-08-13 after
      confirming both the audio page's closed snapshot AND that `MainPane::Page` is a closed enum, so there
      is no app-side seam. **WAV-only recording (T5.3) means nothing needs configuring, so it blocks
      nothing being built**, and it is recorded upstream as the least important of our open issues. It
      becomes real again only if MP3/FLAC/OGG are added (T5.3d).
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
      **⚠ SUPERSEDED 2026-08-14 — see T3.1e and `proposal.md` §6.4b-iii.** This task ruled "Freeze clamps
      the loop-gain PRODUCT to 1", which can only be written with `fbDrive` inside Freeze's own mapping and
      makes the knob NON-MONOTONIC at high Feedback Drive (0.98 -> 0.25, so turning Freeze up takes loop
      gain 3.92 -> 1.00). The operator's intent — full Freeze holds, continuously, never leaving a runaway
      behind — is preserved by clamping Freeze's own coefficient at unity instead. Original text follows
      for the record.
      **Freeze clamps the loop-gain product to 1.** Two consequences the implementer must not miss:
      - **The clamp is CONTINUOUS, not applied once at freeze-on** (operator, same ruling). Un-toggling
        Freeze restores sub-unity loop gain and the tail decays normally — the control can never leave a
        runaway loop behind it. Implement it as a property of the freeze mapping evaluated every sample,
        not as a latched state change.
      - **The clamp binds the Freeze PARAMETER only, not the Freeze BUTTON** (operator, 2026-08-13 —
        ruled earlier but never written down until this audit; see T5.2 and `proposal.md` §6.4b-ii).
        The clamp is applied where the freeze mapping resolves the encoder's value, so the transport
        latch can bypass it. **It therefore does NOT alter the un-frozen product** `fbk * fbDrive`:
        `proposal.md` §7d option 1 clamps that product at every knob position, this does not, and the
        accidental Stop-sustain behaviour is untouched by T3.1b.
- [x] **T3.1b — BUILT, packet P4 (2026-08-14). Suite 231/0, 8 tests added.** Shipped as
      `fbEff = fbk + (1.0f/fbDrive - fbk) * freeze` with `write = inSignal * (1 - freezeEff)`, plus
      `bool dfrzLatched` on `DelayParams` for P5's button. **The form is load-bearing and must not be
      "simplified":** at `freeze == 0` it is `fbk + x*0.0f`, bit-exactly `fbk`, so the default and the
      accidental Stop-sustain are untouched; the algebraically identical
      `lerp(fbk*fbDrive, 1, freeze)/fbDrive` is NOT bit-exact at zero and breaks the default-reproduction
      invariant. Measured products at `fbDrive` max: freeze 0 → **3.91998** (unchanged), freeze 1 unlatched
      → **0.99999** (holds), latched → **3.99998** (grows).
      **Consequence worth an operator ear (not a defect):** at high Feedback Drive the Freeze ENCODER
      lowers the product from ~3.92 toward 1 as it rises, because "hold" means unity — so the knob and the
      button move in opposite directions at the top of the Drive range. Cheap to change if it plays wrong.
- [x] **T3.1e — DONE 2026-08-14, by the lead agent directly (three subagent attempts were killed by the
      watchdog before writing anything). Suite 242/0/0.** Shipped as
      `dsp::StereoDelay::FreezeFeedback(fbk, freeze, latched)` — extracted so `Process()` and the tests
      exercise the SAME arithmetic instead of a test re-deriving the formula against itself. Knob runs
      `fbk` -> 1.00 monotonically (smallest measured step +0.0002, never negative, across fbk 0.00-0.98 x
      freeze 0.00-1.00); latch returns `kFreezeLatchOverdrive = 1.05f`. **`fbDrive` does not appear in the
      mapping.** At `freeze == 0` the result is bit-exactly `fbk`, so §7's accidental sustain is untouched.
      **Why the mapping had to be extracted, and it is not a style choice:** the monotonicity property
      cannot be measured through round-trip peaks — at high Feedback Drive the loop saturates within two
      round trips, so peak ratios cannot distinguish "grew a lot" from "grew a bit" and would give a false
      pass. The guard has to assert at the level the property lives.
      **Tests: one deleted, two added, two retuned.** Deleted
      `..._full_unlatched_holds_loop_gain_at_unity_across_full_feedback_drive_sweep` — it pinned the
      retired product clamp. Added the monotonicity guard and a latch-exceeds-encoder gate that pins the
      RELATION rather than 1.05, since that constant is by-ear. **Both `> 2x` thresholds removed** (dsp
      parity and the surface end-to-end): that multiple was an artifact of `fbDrive` living inside Freeze's
      mapping (4.0 vs 1.0), and keeping it would have asserted the removed coupling back into existence.
      The real ratio is `kFreezeLatchOverdrive / 1.0` at every Drive setting; measured 1.04999983.
      **⚠ Weakness recorded, not hidden:** the surface end-to-end test's margin is now thin — latched
      0.798707 vs unlatched 0.791369, ratio 1.00927 — because the wet limiter compresses both toward its
      0.8 ceiling. It is deterministic and passes, but it is a far weaker signal than the 2.94x P5
      reported, and the DSP-level tests now carry the real proof. Worth re-rigging below the limiter if
      that test ever needs to be trusted on its own.
- [ ] **T3.1e-spec — the requirement this closed.** Repoint Freeze's
      mapping so it references nothing outside Freeze (`proposal.md` §6.4b-iii):
      - `fbEff = fbk + (1.0f - fbk) * freeze` for the knob — monotonically UP at every Feedback Drive
        setting, clamped at unity (lossless recirculation). **`fbDrive` must not appear in this mapping.**
      - `fbEff = kFreezeLatchOverdrive` (strictly above unity) when `dfrzLatched` — the gate survives, the
        knob still cannot reach what the button reaches.
      - `write = inSignal * (1 - freezeEff)` unchanged.
      - **`kFreezeLatchOverdrive` is a BY-EAR constant.** Pick a starting value, name it, and flag it for
        the operator — do not present it as derived or measured.
      - **Bit-exactness at `freeze == 0` is preserved by construction** (`fbk + x * 0.0f`), so the default
        and §7's accidental Stop-sustain stay untouched. Re-assert it, do not assume it.
      - **Retune P4's eight tests and P5's latched-grows test** — every measured number in them was
        `fbDrive`-derived and is now obsolete. Re-measure; do not adjust expectations to fit.
      - **Add the test the original mapping would have failed:** sweep `freeze` 0 -> 1 at several
        Feedback Drive settings including maximum, and assert `fbEff` is monotonically NON-DECREASING at
        every one. This is the defect class `proposal.md` §3 calls out — a knob wired to the wrong end of
        its range passes every bound and default test and sounds backwards — and nothing in the suite
        caught it.
- [ ] **T3.1b-spec — the requirement this closed.** Freeze SHALL be built as a crossfade, not a
      write-enable toggle. `write = inSignal *
      (1 - freeze)` with `fb_eff = lerp(fbk, 1.0, freeze)`. Built as a toggle it fails the operator's own
      continuous-range rule — the rule that cut Cycle and Hard Sync — and the whole reason it passes is
      that its midpoint (new input bleeding in over a slowly-decaying loop) is a real playable state.
- [x] **T3.1c — BUILT, packet P2 (2026-08-14). Suite 217/0.** `DelayDiffuser`: three cascaded Schroeder
      allpass sections per channel (4.7 / 12.3 / 21.1 ms, deliberately non-harmonic), applied to `dL`/`dR`
      on the wet tap BEFORE `wetLimiterL/R`, so it applies once per repeat and the existing limiters still
      bound what escapes. Coefficient `a = ddif * kDiffusionCoeffScale` (0.7f) — **bounded by the mapping,
      as this task required, not by a comment.** Six tests added.
      **Design note the proposal got wrong, decided by the lead agent, not the implementer:** §6.3 and
      §6.4a call `DriveBlendPhase`'s allpass "the identical building block". Its recurrence is, but its
      memory is ONE SAMPLE, which rotates phase rather than smearing time — a cascade of those is a quiet
      phaser, not diffusion. The recurrence form and the coefficient-margin discipline were reused; the
      one-sample state was not. §6.4a's own "genuinely new plumbing rather than a drop-in" is the accurate
      half of that passage.
      **⚠ Recovery note, and the reason this task carries one:** packet P2's subagent was killed by a
      watchdog mid-way through this task's own §9.1 break-and-restore, leaving TWO deliberate breakages in
      the tree — a commented-out pin plus a stray `std::cerr` in `FroggersDspParityTests.cpp`, and
      **`kDiffusionCoeffScale = 1.5f` in the shipping header**, outside the unit circle, directly beneath
      the comment block explaining why the bound held by construction. Both were found by grepping for the
      `TEMP-BREAK` marker after the stall and restored. **A green build would not have caught the constant,
      because the test that pins it was the other half of the sabotage.** Future packets must perform
      break-and-restore as a single chained command so a stall cannot leave the break behind.
      **The requirement this closed, kept for the record:** the coefficient SHALL be bounded by the knob
      mapping, using the same `0.98` margin `dsp::DriveBlendPhase` carries — its authored mapping is
      `0.98f * (2*knob - 1)`, i.e. the open interval (-0.98, 0.98), resetting to `-0.98f`
      (`app/dsp/Drive.hpp:676`, `:689`, `:711`). **Corrected 2026-08-13 (`proposal.md` §9): this task
      previously credited that margin to `dsp::Comb` as well, and `Comb` does not have it** — its margin
      is `kMaxFeedbackMagnitude = 0.95f` (`app/dsp/FilterFx.hpp:537`), the same ±0.95 §7b's table lists,
      and `FilterFx.hpp` holds no 0.98 stability constant at all. Allpass sections are unity-gain by
      construction only while the coefficient stays strictly inside the unit circle. **Not asserted in a
      comment** — §7 records what this codebase's one loop-gain-above-unity defect cost, and it came from
      proving a bound and assuming a contraction. Shipped at 0.7, comfortably inside both bounds.
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
- [x] **T3.5 — DONE 2026-08-13: the spec delta this decision needed was missing, and is now written.**
      T3.1 is DECIDED, and `openspec/specs/froggers-sheaf-parameter-model/spec.md:110` requires "slots 0-8
      are unchanged from the Delay bank's existing nine parameters" — Detune, Color and Halo are three of
      those nine. This change carried only `ADDED` deltas, so nothing in it contradicted the requirement
      it contradicts. Added to `specs/froggers-sheaf-parameter-model/spec.md`: a `MODIFIED Requirements`
      section restating **One sixteen-slot bank per Froggers page** in full (the same form the predecessor
      used at `archive/2026-08-13-frogg3rs-bank-expansion/specs/froggers-sheaf-parameter-model/spec.md:55`)
      with the Delay scenario rewritten, plus an `ADDED` requirement pinning what T3.1a-d decided —
      Freeze's continuously-evaluated unity clamp, Diffusion's mapping-enforced coefficient bound, Reverse
      Blend's buffer smoothing, and a playable midpoint for all three. **Slot indices are deliberately NOT
      pinned**, because T3.1 rules the assignment a recommendation; the requirement names the three
      controls and leaves which vacated index each takes to the implementer.
      **`openspec validate --strict` passed before this task as well as after** — it validates delta
      structure, never whether a change specified what it decided, so it was never evidence here.

## T4 — Stop does not silence the instrument (`proposal.md` §7)

**The operator reported this, likes the sound, and has NOT asked for a fix. Nothing here changes DSP
without an explicit decision.** What is scheduled is the part that is not a matter of taste: three
parameters can place a feedback loop's near-origin gain near 4, which makes a non-decaying limit cycle
reachable during ordinary play, not only after Stop.

- [x] **T4.1 — ⚠ SUPERSEDED 2026-08-17 by `frogg3rs-stop-isolation-and-legible-labels`, and the closure
      below was a MISCLOSURE twice over.** (1) The operator's intent was ISOLATION — the effect lives in
      the button and Stop actually stops — not "keep the accident and add the button"; the operator
      caught this in use. (2) §7's mechanism itself was wrong: runtime capture (which T4.2 called for and
      nobody ran before closing this) shows the sustain is a VOICE THAT NEVER ENDS — Curve~1 makes
      `ComputeRampStep` asymptotic and the Grace ladder strands the pending release, so AllIdle never
      fires the clear. The drive loops only coloured it. Full trail: the new change's proposal §1.
      Original closure text follows for the record.
      **T4.1 — DECIDED, operator 2026-08-13: KEEP the behaviour, and PROMOTE it — via the Freeze button,
      not a knob.** This task asked whether to keep the sound and, if so, whether to leave it an accident
      or make it a control. The answer is both: **none of §7d's five options is taken, so no DSP changes
      and the accidental behaviour is untouched at all three sites** (Delay `FbDr`, Reverb `TkDv`, Filter
      `CDrv`); and the deliberate form arrives as the Freeze BUTTON overriding the Freeze parameter's
      clamp (T5.2, `proposal.md` §6.4b-ii), which reaches loop gain `1.0 * fbDrive` while latched.
      **Note what this does NOT do:** the override lives in the Delay's own freeze mapping, so the Reverb
      tank and the Filter comb keep their own near-4 loop gains and remain reachable by ordinary play, as
      §7b describes. T4.4's measurement is therefore still worth having, and §7d option 1 remains on the
      table as a separate future decision if the comb ever proves a problem.
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

- [x] **T5.1 — DONE 2026-08-14, two packets: P6a (model, subagent) + P6b (UI/wiring, lead agent). Suite
      244/0 at P6b, 243/0 after T5.2b's deliberate -1.** Model: `ResetPage`/`ResetAll` beside their
      Randomize counterparts, mirroring their enumeration INCLUDING `drillIn.Level()` branching and the
      Crispy asymmetry (page includes, all excludes — matching RandomizeAll's own `includeCrispy=false`);
      values -> 0.0 via `HandleSetAbsolute` on both scene poles; returns void (Reset never allocates, so
      no partial outcome exists — P6a's reasoned fill-in). UI: `RightKind::Reset`, `kRightRows` extent
      7 -> 8, `AppendResetRow` as Randomize's visual twin, constants in both blocks, atomics + drain
      beside Randomize's. End-to-end test: max |value| 0.994712 before reset -> 0 after, through the real
      action path. **Original task text:** New row appended after `Randomize` in
      `FroggersCellMap::kRightRows`, two `Button` nodes each `Extent::Weight(2.0f)` — the same two-halves
      weighting `AppendRandomizeRow` already uses, which is what "same size" means concretely.
      **`kRightRows` is `std::array<RightRow, 7>` (`app/FroggersUiSurface.hpp:362`) — its extent goes to
      8**; "appended" is not just one more initializer.
      **Mirror `RandomizePage`/`RandomizeAll`'s own enumeration rather than re-deriving it**:
      `drillIn.BankRef()` for the page, `for (bankIx...) model.BankAt(bankId)` for all, INCLUDING their
      `drillIn.Level()` branching — a Reset that ignores drill level means the wrong thing while drilled in.
      **Read `RandomizeBankValues` itself, not the comment above `RandomizePage`** — see T5.1d, which
      is part of this same packet.
      Parameter minimum is uniformly `0.0` (`ClampToRange` ignores `RangeKind` and there are no
      per-parameter min/max fields), so no per-parameter table is needed.
- [x] **T5.1a — DONE (P6a), and the trap was avoided as specified:** depths go through the existing
      `ZeroExistingModulationDepths` (neutral 0.5 to both poles), never a hand-rolled loop.
      **Original task text:** ⚠ Depths reset to NEUTRAL (0.5), never to 0.0. Depth parameters are bipolar and their off
      position is `kNeutralModulationDepthCenter`. **Writing literal 0.0 is FULL NEGATIVE depth** — a reset
      that took "set depths to 0" literally would produce a maximally-modulated patch while appearing to
      clear it, and would look correct in any test that only asserted "the value is 0". Reuse the existing
      `ZeroExistingModulationDepths(Parameter&)`, which already writes the neutral centre to both scene
      poles and deliberately skips unmaterialized depths.
- [x] **T5.1b — DONE (P6a wrote it; the lead agent ran the §9.1 proof independently after P6a's first
      report went missing):** broken-to-0.0 reads `SceneCenter(0) (=0) not within 1e-06 of kNeutral
      (=0.5)`, 27/34 -> restored 34/34 byte-identical. **Original task text:** Test that the trap is not
      re-introduced. Assert after a reset that each touched depth
      reads its NEUTRAL value and that the parameter is audibly unmodulated — not merely that some number
      changed. Per OMNI §9.1, confirm the test FAILS if the reset is changed to write 0.0.
- [ ] **T5.1e — RECORDED: a regression ceiling derived from a pre-expansion count, found in packet P6a's
      §8 sweep.** `app/FroggersModulation.hpp:887` states Randomize All presses "61" parameters for the
      parameter-page case, and `app/FroggersModulationTests.cpp:454/474/479` derive a `61 * 13 = 793`
      ceiling from it — including in the test's own NAME
      (`randomize_all_on_parameter_page_stays_within_793_ceiling_...`). **61 is the pre-expansion figure**
      (9 params x 6 banks + 6 Crispy + 1 Crunchy); with `kFroggersParamsPerBank = 14` the parameter set is
      far larger. The test passes because the actual materialized count sits well below 793, **not because
      the bound is correct** — a bound that holds by accident is exactly what OMNI §9.1 warns about, and
      it would not catch the growth it was written to catch. Re-derive the figure from
      `kFroggersParamsPerBank` rather than patching the literal, and rename the test, which hardcodes 793.
      **Note this is the predecessor change's staleness, not this change's** — `frogg3rs-bank-expansion`
      raised the count and left the ceiling behind. Recorded here because this change is where it surfaced.
      **It also shows the limit of a word-based sweep:** T5.1d's instruction searched "9 values"/"nine
      parameters" and could never have found "61". §8's operand rule — search the CONCEPT's operands, not
      one syntactic form — is what found it.
- [ ] **T5.1c — RECORDED, still unscheduled: the live surface spec disagrees with the code about where
      the Randomize controls are.** `openspec/specs/froggers-app-surface-layout/spec.md:16` places
      Randomize All in the global chrome band and Randomize Page in the per-page/bank header; the code has
      had both together in `kRightRows`' last row since task 10.2, and `AppendRandomizeRow`'s own comment
      says so. Pre-existing drift, found while auditing T5.1's anchors (`proposal.md` §9). **Correcting it
      means adding `froggers-app-surface-layout` to this change's delta set, which is a scope decision the
      operator has not taken** — so it stays recorded, not folded. It is load-bearing for T5.1 all the
      same: that task appends two new controls to the row the spec says does not hold both, so whoever
      builds T5.1 is working against a spec sentence known to be false and must not quietly inherit it a
      third time.
- [x] **T5.1d — DONE (P6a's sweep): found 4, changed 3** (`:1199`, `:1223`, `:1266` — all now say 14);
      the fourth (the `FullPhysicalLayout` "11 = 9+Crispy+Crunchy" block) left deliberately, because a
      naive 9->14 also forces 11->16 and makes the adjacent `>= numModulators+1 = 16` sentence
      self-contradicting — needs a Sheaf-internal behavioural check outside Reset's scope. Flagged, not
      half-fixed. The sweep also surfaced the stale "61" ceiling — see T5.1e.
      **Original task text (operator, 2026-08-13):** FOLDED INTO T5.1's PACKET, not filed for later. Correct the
      stale comment at `app/FroggersModulation.hpp:1266`, which describes Randomize Page at level 0 as
      "this bank's 9 values + Crispy". `RandomizeBankValues` (:1213-1221) loops `kFroggersParamsPerBank`,
      which is **14** (`app/FroggersParameters.hpp:77`) — pre-expansion staleness sitting on the exact
      function T5.1 tells the implementer to mirror. One line, mechanical, and it lands in the same commit
      as T5.1 rather than becoming a someday-chip (OMNI §16.2). **Sweep the same function's neighbours
      while there** — any other "9 values"/"nine parameters" wording in `FroggersModulation.hpp`'s
      Randomize comments is the same staleness and is corrected in the same pass; report count found vs
      count changed, per OMNI §8.
- [x] **T5.2 — BUILT, packet P5 (2026-08-14). Suite 236/0, 5 tests added, no existing test modified.**
      `kFreeze` action + node id, `BuildFreezeDrawCommands(bounds, latched)` drawing a diamond glyph with
      plate/glyph colours SWAPPED when latched, appended as `AppendTransportRow`'s third `Draw` child via
      a capturing lambda that re-reads the latch every rebuild. Latch is `std::atomic<bool>
      freezeLatched_` on `FroggersAppCore` (release/acquire, modelled on `SetDesiredTransportRunning`),
      toggled in `HandleAction`, and copied to `delayParams.dfrzLatched` right after
      `MapRowsToDelayParams`. End-to-end measured at `fbDrive` 4.0 with the Freeze encoder at max:
      **latched 0.797584 vs unlatched 0.270953** (~2.94x), both primed from an identical 0.626565.
- [x] **T5.2b — DONE 2026-08-14 (lead agent): mirror member, accessor, per-sample write and the
      redundant test all removed; 0 references remain; surface 35/35; suite 243/0/0 (the -1 is the
      deleted test, accounted). **Original task text:** CLEANUP, found in postflight review of P5's own
      diff, not yet done. P5 added
      `delayParamsDfrzLatchedForTest_` (`app/FroggersAppCore.hpp:1809`) plus a
      `TestDelayParamsDfrzLatched()` accessor, written every sample in `RouteAudioSample()` and read by no
      production path — a mirror of `FreezeLatched()` one line above it. It exists only so
      `freeze_latch_reaches_delay_params_directly` can observe the wire. **P5's own §9.1 proof showed that
      test is redundant:** breaking the wire failed BOTH it and the end-to-end latched-grows test, so the
      end-to-end test already covers the failure mode, more strongly and without production state.
      Remove the member, the accessor and that one test; keep the end-to-end test. Cost today is one bool
      store per sample — negligible, which is why this did not block the testable build.
- [ ] **T5.2-spec — the requirement this closed.** Freeze button, beside Stop, as a `Draw` node. Append a third child to
      `AppendTransportRow`'s row (`app/FroggersUiSurface.hpp:724-744`), matching Play/Stop's existing 28 px
      plate idiom. Latches on one click, releases on the next.
      **⚠ THE BUTTON OVERRIDES T3.1a's CLAMP — operator ruling, 2026-08-13, verbatim in substance:**
      *"when toggled, the freeze button should override the freeze encoder parameter and take it to the
      maximum above the clamp — period. when not toggled, the clamped encoder parameter prevails."*
      This is the whole point of the control and the reason it is not merely a shortcut for turning the
      Freeze encoder up:
      - **Latched:** the Freeze value the DSP sees is the un-clamped maximum, so the loop gain becomes
        `1.0 * fbDrive` — up to 4.0 — and the loop GROWS. This is the deliberate form of the accidental
        Stop-sustain behaviour analysed at `proposal.md` §7, which the operator likes and is keeping.
        Its depth is therefore set by the Feedback Drive knob (slot 9): below that knob's centre
        `fbDrive < 1` and the latch holds rather than grows, which is expected, not a defect.
      - **Unlatched:** T3.1a's clamped encoder value prevails, exactly as before, and the tail decays.
      - **Implement the override at the point where the freeze mapping resolves the encoder's value**, so
        the clamp is a property of the PARAMETER path and the latch substitutes for it. Do NOT implement
        it by writing 1.0 into the Freeze parameter and then clamping — that path cannot exceed the
        clamp by construction, which is the bug this ruling exists to prevent.
      - **The transport-spec invariant still holds:** releasing the latch restores sub-unity loop gain, so
        the runaway exists only while the operator is holding it and can never be left behind.
      **What is ruled out, and what is not — corrected 2026-08-13 (`proposal.md` §9).** Ruled out:
      `NodeKind::Toggle` (present in Sheaf, used nowhere in `app/`) and a `Button` relying on
      `ControlStyle::selected`, because `StateColourFor` renders selection as `brighter(0.14f)` on the
      background and `TextColourForNode` (`PortableJuceBackend.hpp:1036-1042`) branches on `enabled`
      only — text colour never changes on selection (`UPSTREAM-SHEAF-ASK.md` item 3, landed partially).
      **NOT ruled out, and the earlier version of this task wrongly implied it was:** a `Button` whose
      `color` and `textStyle.color` the APP swaps from its own latch state. `ControlStyle` carries both
      fields (`PortableUIBuilders.hpp:20-33`), `GlyphColourForNode` (`:1046`) honours a supplied
      `textStyle`, and the app already branches a style on its own state (`FroggersUiSurface.hpp:957`).
      **Build the `Draw` node** — it is free, needs no upstream change, and keeps Freeze visually
      consistent with the two controls it sits beside — but build it as the preferred option, not as the
      only one. If it proves awkward, the `Button` route above is live and equally satisfies the spec's
      "SHALL NOT depend on the control library's selected-state rendering".
- [x] **T5.2a — SATISFIED.** The dependency was honoured: Freeze landed as a parameter in packet P4
      (T3.1b) and the button in P5 (T5.2), in that order.
- [ ] **T5.2c — Testing pitfall found in P5, recorded so the next Delay test does not rediscover it.**
      Measuring a Delay unit's recirculating level via `dsp::StereoDelay::StateMagnitude()` gives FALSE
      NEGATIVES at short delay times: at `dtim = 0` the active round trip touches only ~48 of the
      buffer's 96000 samples, so loud stale content elsewhere in the line masks the real growth or decay.
      Use peak-tracked `GetLastWet()` instead. This is an instrument-is-dead-but-the-number-looks-right
      case in the sense of OMNI §9.1, and it was caught only by running the measurement, not by reading it.
- [x] **T5.3 — DECIDED, operator 2026-08-13: WAV only, for now.** WAV needs no format choice, so there is
      **nothing to configure and therefore nothing to place** — which dissolves the whole placement problem
      rather than working around it. Consequences, each recorded because they change other items:
      - **Sheaf#8 is downgraded to the least important open issue and says so upstream.** It blocks a
        design we would have liked and blocks nothing we are building. It only becomes real if MP3/FLAC/OGG
        are added later (v1 shipped all four; this app does not need them).
      - **The export layer needs no JUCE audio-format dependency at all.** A WAV file is a 44-byte header
        plus interleaved PCM — writable by hand. So the core/host split is about the file dialog only, not
        about encoding.
      - **Record is unblocked and buildable now** (T5.3a-c below).
- [x] **T5.3a — DONE 2026-08-16 (P7a). Hook at `FroggersAppCore.hpp:988`, post-limiter mono, exactly what
      the operator hears; bounded 30 min, truncation flagged and capture stops (§9.1 proven: flag neutered
      -> red 37/1, restored -> green 38/0, boundary exact at capacity=256); liveness max |sample| 0.936
      over 2048 frames. Allocation happens at arm time on the UI thread, never on the audio thread.**
      Original: Capture, in the app core.** Accumulate output samples into a bounded buffer. No JUCE, no
      Sheaf facility — this is a buffer and nothing more, so it sits inside the `check_no_juce` gate
      cleanly. **Bound the capacity and report truncation** rather than growing without limit; v1 capped at
      roughly thirty minutes and warned the operator, which is the precedent to follow.
- [x] **T5.3b — DONE 2026-08-16 (P7a core, P7b alert). `ArmRecording()` refuses with exactly "Press Play
      before recording." — STORED at refusal time, not recomputed from transport state, since arming can
      succeed and the transport stop afterward (P7a's own correction to the brief). Surfaced as a real
      `juce::AlertWindow` in the host; the refusal-callback test asserts the exact string.**
      Original: refuse to record when audio is not running**, with an explanation rather than a silent
      no-op. v1's own wording was *"Press Play before recording"*. A silent refusal is the failure mode
      worth designing against here.
- [x] **T5.3c — DONE 2026-08-16 (P7b). Chosen split, and why: the WAV ENCODING lives in the CORE
      (`EncodeWavPcm16Mono`, pure std::, inside the no-JUCE gate) because no test binary compiles
      `FroggersMain.cpp` (verified against the whole Makefile), so core placement is what makes it
      headlessly testable — §9.1 proven (sample-rate field corrupted -> red 43/1; restored -> 48000 /
      dataChunk 128 / round-trip within 1 LSB, green 44/0). The host is dialog-plus-byte-streaming:
      `juce::FileChooser` + `FileOutputStream`, truncation noted v1-style. **`make all` builds only the
      stub `Main.cpp`** — the host compile was verified with `./app/build-launcher.sh`, which produced a
      fresh 37.9MB `Frogg3rs.app`. Original: Export, in `app/FroggersMain.cpp`** (the JUCE host, outside the no-JUCE gate): the save
      dialog, and a plain WAV writer. **Only the dialog needs the host layer** — the WAV encoding itself
      could live either side, so put it wherever it reads better and say which was chosen.
- [ ] **T5.3d — Recorded, not scheduled: adding formats later re-opens Sheaf#8.** If MP3/FLAC/OGG are ever
      wanted, a format control appears and needs a home, and the only home before #8 lands is the app's own
      instrument panel. Noted so the connection is not rediscovered.
- [x] **T5.4 — DONE 2026-08-16: 254 passed / 0 failed / 0 warnings across the 10 binaries** (pre-refactor
      run; the postflight §8 row-builder refactor is re-verified by one further run recorded in the chat).
      **Every test accounted for, 213 -> 254:** P1 -2 (fold + Detune tests retired with their subjects);
      P2 +6 (Diffusion); P3 +6 (Reverse Blend); P4 +8 (Freeze); P5 +5 (Freeze button); P6a +5 (Reset
      model); T3.1e net +1 (product-clamp test deleted, monotonicity + latch-gate added); P6b +2 (Reset
      UI); T5.2b -1 (redundant latch-wire test removed with its production mirror); T1.6 +2 (Reverb pins);
      P7a +3 (capture); P7b +6 (Record UI + WAV). 213-2+6+6+8+5+5+1+2-1+2+3+6 = 254.

## Recorded, not scheduled — no task closes these

- **The design doc's open question 8** — the ASR envelopes cannot modulate anything and the fifteen-source
  modulation slate is full, with three VCO-EF slots partially duplicating what a true envelope source would
  do better. A modulation-slate question, not a bank-slot question, and it may outrank everything above.
  Untouched since it was first raised.
- **All six banks are closed at fourteen parameters.** No further bank-fill candidate is in scope here. The
  selection rule that governed the expansion still stands for any future proposal: if the modulation matrix
  can already produce the effect by routing one of the fifteen sources onto an existing parameter, the
  parameter is rejected.

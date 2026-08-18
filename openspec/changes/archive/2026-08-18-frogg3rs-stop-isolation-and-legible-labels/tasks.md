# Tasks — `frogg3rs-stop-isolation-and-legible-labels`

> Read `proposal.md` first — §1 is the investigation record and every task below cites it.

**Baseline, VERIFIED 2026-08-16: 254 tests / 0 failures / 0 warnings across 10 binaries** (the
predecessor's final suite). `External/Sheaf` was pinned at `77a3019e` for this change's own verification; **the pin moved to
`508d9d68` mid-session (commit 211f1df, upstream main) — every test result recorded below, including
T5.1's 266/0/0, was measured against `77a3019e`, not the new pin.** The app builds clean against
`508d9d68` and the operator confirmed it runs; the suite has NOT been re-measured against it. Repro binary:
`app/FroggersStopSustainRepro.cpp` (NOT in the test target, `*Repro.cpp` convention).

## §0 Standing constraints (carried, plus this change's own lessons)

- **Subagents: Sonnet or Haiku, never Opus; model set explicitly on every dispatch.**
- **`nice make -j2`, never higher** (8-core/16 GB).
- **Subagent briefs MUST forbid `make test`** (the full suite exceeds the 600s no-progress watchdog —
  it killed four packets last change). Targeted binaries with ABSOLUTE paths; `rm -f` the binary before
  each verification rebuild (same-second mtime collisions fake green); the lead agent runs the full
  suite in a backgrounded shell.
- **§9.1 break-and-restore as ONE chained command** with the `TEMP-BREAK` token; grep must return 0
  before any report.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`. No AI attribution.
- **Code changes sequential; parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose resolution requires the operator's ears or eyes** (T4).

## T1 — Bound every envelope ramp (root cause, proposal §1a / W1)

- [x] **T1.1 — `ComputeRampStep` progress floor.** Guarantee per-sample progress >= a fixed fraction of
      the linear step at every `curveAmount` in [0,1]; `curve == 0` stays bit-identical to the linear
      path (the existing early return). Name the fraction (`kCurveMinProgress`, suggest 0.25f) as a
      BY-EAR-TUNABLE constant with a comment saying so. The requirement is the BOUND, not the shape.
- [x] **T1.2 — Play-time grace semantics preserved and bounded (TEST-ONLY; no ladder change).**
      ⚠ AUDIT-CORRECTED 2026-08-17: the first draft changed the grace ladder (countdown from
      pending-mark, force Release from any stage at expiry). That contradicts the approved main-spec
      Grace requirement — minimum Hold "so a short gate cannot clip a note before its envelope completes
      Attack and Decay" — and its own "preserves today's behaviour" claim (max attack 1.0s + short grace
      would clip legitimate notes mid-attack during play). The ladder stays UNTOUCHED; the transport-stop
      path is T2.1. This task adds the tests that pin both properties: (a) with T1.1's ramp bound in, a
      pending release under active grace reaches Release within (bounded stage completion + grace) at
      every curve including 1.0 exactly; (b) the minimum-hold guarantee holds — a short gate with a long
      attack and active grace still completes Attack and Decay before Release begins.
- [x] **T1.3 — Tests.** ✅ done; kCurveMinProgress landed at 0.4f (measured: 0.25f misses T1.3b's own
      2.0s/2.5s bounds at 2.65s worst; table in the constant's comment), worst duration multiple 2.5369. (a) Duration-bound sweep: every stage completes within a small multiple of its
      knob time across curve x knob grids INCLUDING curve == 1.0 exactly, at 48k and 96k; report the
      worst multiple observed. (b) The pass-F minimal repro as a REAL suite test: curve=1.0, grace=0.5,
      decay mid, sustain audible -> Stop -> assert silence (< 1e-4 peak) within 2.5s and AllIdle within
      2.0s, with the pass-F pre-fix numbers (0.939 at t+10s, AllIdle=0) recorded in the comment as what
      this guards against. (c) Bit-identity at curve == 0. Per OMNI §9.1 prove (a) and (b) each fail
      against the un-fixed ramp (single chained command; the un-fix is one-line reversible).
- [x] **T1.4 — Re-run the randomized-trigger condition** (pass D, 20 trials) against the fixed build and
      report reproductions (expect 0/20; any nonzero is a second mechanism and STOPS this change).
      ⚠ ORDERING (audit, 2026-08-17): run AFTER T2.1 — under W1 alone a randomized long-attack/grace
      patch may legitimately take up to ~9s to silence, which pass D's t+5s criterion would miscount as
      a reproduction.

- [x] **T1.5 — Max attack 0.5s (proposal §2 W5, operator ruling 2026-08-17: "half a second at most").**
      `kMaxAttackSeconds` (`app/dsp/VoiceEnvelope.hpp:63`) 1.0f -> 0.5f, with the ruling appended to the
      constant's own history comment. ATTACK ONLY: `kMaxDecaySeconds` keeps 1.0f but its "Mirrors
      kMaxAttackSeconds (1.0f)" rationale is rewritten as decay's own judgment (the mirror is broken by
      this ruling, deliberately); `kMaxGraceSeconds` untouched. Update the ITEM 4 parity-test prose
      (`FroggersDspParityTests.cpp:234-254`) that cites 1.0s; assertions stay keyed to the constant.
      T1.3's duration-bound sweep runs against the NEW ceiling.

- [x] **T1.6 — Grace countdown sentinel bug (FOUND DURING P1, 2026-08-17; second voices-never-idle
      mechanism, pulled in-change per OMNI "see something, say something").** In the Hold-stage grace
      block (`VoiceEnvelope.hpp:350-363`), the init guard `if (m_graceRemaining < 0.0f) { re-init }`
      runs BEFORE the expiry check, so a countdown that decrements past zero without landing on exactly
      0.0f (any `graceSeconds * sampleRate` that is not float-exact) goes negative and RE-ARMS to the
      full grace on the next sample — an infinite minimum-hold during ordinary play. Fix so expiry fires
      for any countdown reaching <= 0 while the -1.0f "not started" sentinel still initializes exactly
      once per pending release (mechanism implementer's). Tests: (a) grace values chosen to be
      float-inexact (e.g. knob 0.3) expire within grace + 1 sample; (b) widen T1.2(a)'s curve grid from
      its graceKnob=1.0 workaround to multiple grace values (the workaround existed only because of this
      bug). Also fix the now-half-false `kMaxGraceSeconds` comment ("matches kMaxAttackSeconds/
      kMaxDecaySeconds's own scale" — attack is 0.5f since T1.5; review finding, plan-mandated). §9.1
      both tests.

## T2 — Stop-edge silencing and drive/Freeze isolation (proposal §2 W2, operator ruling)

- [x] **T2.1 — Stop-edge forced release (AUDIT-ADDED 2026-08-17, replacing the first draft's play-time
      grace change).** On the running->stopped edge, every non-idle voice enters Release immediately,
      bypassing the Grace minimum-hold and any in-progress Attack/Decay (mechanism the implementer's —
      e.g. a `ForceReleaseAll()` on the envelope called at the edge beside the existing
      `delayReverbClearPending_` logic, `FroggersAppCore.hpp:878-919`). This restores the pre-Grace-packet
      `setGate(false)` synchronous-force semantic for Stop only; play-time gating is untouched.
      `kStopFadeReleaseKnob` then shortens the ramp exactly as today.
- [x] **T2.2 — Effective-value override on the stopped transport**, beside `kStopFadeReleaseKnob`'s
      release override and in its exact idiom: while stopped, the three drive pre-gains resolve to unity
      and Freeze to zero, WITHOUT writing to the parameter model (commanded values untouched, exactly as
      the release override never writes the Release knob). Play resumes bit-identical. Per OMNI §8: the
      stopped-state overrides (release knob + three drives + Freeze) render through ONE idiom instance,
      not five copies.
- [x] **T2.3 — Tests.** (a) While stopped, with commanded drives at max and Freeze at 1.0: effective
      values read unity/zero; resuming play restores the commanded mapping bit-exactly. (b) Stop with
      grace = 1.0, curve = 0, voice mid-Attack: AllIdle within the fade time (~0.1s), NOT after
      stage-completion + grace (~2s+) — pins T2.1. §9.1 both: break by skipping the override / the forced
      release, confirm each test reds, restore.
- [x] **T2.4 — The Freeze BUTTON while stopped is a no-op on the audio** (loop gain 1.05 over zeroed,
      un-fed lines) — pin that as a test so nobody "fixes" the button into a stopped-state noisemaker.

- [x] **T2.5 — Grit joins the stopped-state override (proposal §2 W2b; MEASURED third mechanism,
      packet 2b 2026-08-17).** Add the reverb's Grit parameter (Reverb slot 11) to T2.2's stopped-state
      effective-value override, resolving to 0.0f (its exact bit-identical bypass, `Reverb.hpp:534-538`)
      — same idiom instance, NOT a sixth scattered ternary. Tests: (a) stopped with commanded Grit at
      max, effective Grit reads 0 and play restores it bit-exactly (extends T2.3(a)'s existing
      assertions, same test); (b) the app-free tank control as a suite test — seed the reverb once at
      0.01, then feed exact zero, assert state magnitude decays below 1e-4 within the bound at the
      measured drawn state (pre-fix numbers in the comment: locks at 0.306814 forever at Grit 0.8094;
      decays to 1.98e-7 at Grit 0). §9.1 both. Then **re-run T1.4's pass-D 20 trials** — expect 0/20;
      any nonzero is a FOURTH mechanism and STOPS this change.
- [x] **T2.6 — Record the underlying `Mangle` defect (no fix here).** `dsp::DigitalReorganizer::Mangle`
      (`app/dsp/Drive.hpp:363-382`) has unbounded local gain across quantization-bucket boundaries; the
      stopped-state override only prevents it from mattering while stopped. It remains reachable DURING
      PLAY anywhere Grit sits inside a feedback loop. Record this in the change's proposal §3 non-goals
      as a known open defect with the packet-2b measurement cited, and file it as a follow-up. Do NOT
      change `Mangle` in this change — a gain bound there is a tone change requiring the operator's ears.

## T3 — Randomize lands the drawn value (proposal §1b / W3)

- [x] **T3.1 — App-side draw.** Replace the press-with-RandomHeld value path in the app's own wrapper
      with: draw uniform, write via `HandleSetAbsolute` to BOTH scene poles (`kScenePoles` idiom).
      Depth randomization (A1's non-additive zero-then-draw) is already correct and untouched.
- [x] **T3.2 — Tests.** Under full-positive audio-rate modulation on a parameter, 50 randomize presses:
      the commanded value's empirical distribution stays roughly uniform (no monotone drift; mean in
      [0.35, 0.65]; fraction at the exact 1.0 clamp < 5%). Record the PRE-FIX measurement in the test
      comment: commanded == 1.0000 in 20/20 trials after 5 presses (proposal §1b). §9.1: prove it fails
      against the old press path.
- [x] **T3.3 — File the upstream ask as `UPSTREAM-SHEAF-ASK.md` ask #16** (audit-corrected 2026-08-17:
      the first draft said "Sheaf#9 candidate", but the ledger already numbers local asks #1-#15 — #9 is
      the parameter-timing ask — and its header warns duplicate numbering caused a mixup before):
      `RandomizeVisibleValue` deltas against the modulated resolved value; include the ratchet
      measurement. Follow `UPSTREAM-SHEAF-ASK.md` conventions; the app does NOT wait on it (T3.1 is the
      fix).

## T4 — Labels: short names, expansion only where needed, ring never obscured (proposal §2 W4)

- [x] **T4.1 — Natural-name rendering, per an operator-approved label list.** ⚠ CORRECTED 2026-08-17:
      the first draft of this task mandated short names for everything that fits, and the operator
      rejected it — "A1 S1 can use short names, but it is not acceptable for every parameter." The rule:
      each cell renders the parameter's NATURAL label — the short form where it IS the name (`A1`-`R3`
      and kin), the readable full name where the short form is a truncation (`CmbOff` never stands
      alone). The split is not the implementer's judgment: T4.4's mock carries a proposed label for
      EVERY parameter of all six banks, and the operator's redline of that list is the binding input —
      **the approved list now exists: `labels.md`, beside this file (operator, 2026-08-17).**
      Single-row native idiom for the short/verbatim-fitting labels; the expanded treatment survives
      only for names the list says must render long.
- [x] **T4.2 — Geometry: labels below the ring, never over it.** Whatever renders sits in vertical space
      that does not intersect the ring's DRAWN ARC (⚠ CELL SIZE CORRECTED 2026-08-17 by
      P4's own measurement: 136.333x68.0, baseRadius 25.8 — the 136.3x88.3 figure predates the Reset
      row. Defect unchanged in kind: the label plate covered the ring's lower arc). Because T4.1's corrected rule keeps
      genuinely long labels, the geometry makes room for them by ADDING cell height below the ring —
      **operator ruling 2026-08-17, verbatim in substance: "do not shrink the encoder ring"**, which
      matches their original words ("space you should have ADDED below each encoder"). The ring's
      radius and position are byte-identical to today; each encoder cell grows ~20 px; the window height
      (the app's own `Config()`) grows accordingly (~80 px over four encoder rows), and the
      target-window-size integrity requirement is re-verified at the new size. A first draft of this
      task offered a ring-radius reduction as an option; that option is DEAD.
- [x] **T4.3 — Tests.** (a) Envelope bank cells emit exactly the single-row short-name block; a
      truncation-class cell (e.g. Filter slot 0) emits its approved long label in full (assert command
      counts/geometry against the approved list, not pixels). (b) No label draw command's bounds
      intersect the ring's drawn arc in ANY cell, all six banks — computable from the command list.
      (c) Every rendered label matches the operator-approved list VERBATIM, so a later rename cannot
      silently reintroduce a truncation. §9.1 all three.
- [x] **T4.4 — OPERATOR GATE, carried from the failed T1.1 and now FIRST, not last.** ✅ **PRE-BUILD
      HALF DONE 2026-08-17:** the mock (ring untouched, label band in ~20 px of ADDED cell height, all
      86 labels) was approved — "these labels look great. as long as you're not shrinking the encoder
      ring itself, i approve." The approved list is checked in beside this file as `labels.md` and is
      the binding input for T4.1/T4.3(c). **POST-BUILD HALF CLOSED 2026-08-17** — operator, on the
      built-and-launched `Frogg3rs.app`, verbatim: "everything works and sounds and looks great!"
      That closes T4.4 and the change's last open task. (Original wording: on-screen confirmation of
      the built result by the operator.) A test asserting a weaker property than "the operator can see
      it and confirms it" does not close this — the prior change closed its build with 254/0/0 and
      still failed the operator's eyes.

## T6 — The Freeze latch holds the drone across Stop (REOPENED 2026-08-17, operator in use)

**Why this exists.** The operator exercised the built app and reported the Freeze button does not
reproduce the effect it was created for: *"the whole point is to reproduce that error... it's not just
about setting freeze above the encoder maximum, it also has to stop everything else to get that
effect"*, and *"i can still manipulate every encoder and get parameter changes just like if it was still
playing."* W2's premise — that the sustained-drive character stays reachable **during play** through the
latch — was wrong: the effect only ever existed with the transport STOPPED, so T2.1/T2.2/T2.4 together
made it unreachable by any route and T2.4 pinned that as correct. Third recorded misreading of this same
operator intent (see proposal §1c for the first two). Spec delta amended to match before any code.

- [x] **T6.1 — Gate the whole stop-edge teardown on the latch.** (landed 8b26ea8: `TransportTeardownActive()`, `FroggersAppCore.hpp`, single-sources the forced release, the `stoppedKnob` overrides, and both `ForEachStatefulUnit(Reset)` clears.) While `FreezeLatched()` is true, the
      running->stopped edge SHALL NOT force release, SHALL NOT apply the stopped-state effective-value
      overrides (the `stoppedKnob` idiom's six sites: release knob, three drive pre-gains, Freeze knob,
      Grit), and SHALL NOT run the `ForEachStatefulUnit(Reset)` clear — including the deferred clear that
      fires later when `AllIdle()` first turns true. The instrument holds its sounding state. ONE gate
      read by every site, not six independent checks (OMNI §8); `stoppedKnob` already single-sources
      five of them.
- [x] **T6.2 — Releasing the latch while stopped silences.** (landed 8b26ea8: `latchReleasedWhileStopped` edge + shared `runStopTeardown()` lambda, `FroggersAppCore.hpp`; test `freeze_latch_release_while_stopped_silences_within_the_bound`.) Latch release while stopped is a second
      "stop edge": it runs the teardown that T6.1 suppressed, silencing within the same bound an
      unlatched Stop guarantees. This is the escape hatch — without it the only way out of the drone is
      Play, which the operator did not ask for and which would make the button a trap.
- [x] **T6.3 — Parameter edits stay live while frozen** (landed 8b26ea8: test `encoder_edit_while_frozen_changes_the_output_measurably`, `FroggersAudioRoutingTests.cpp`.) (operator ruling 2026-08-17, choosing faithful
      reproduction over a full state lock): the drone responds to encoder changes exactly as the
      original accidental state did. This is the DEFAULT behaviour of `RouteAudioSample` (it runs every
      sample regardless of transport) — so this task is a TEST that pins it, not a code change. Verify
      that claim by reading before writing the test.
- [x] **T6.4 — Tests.** (landed 8b26ea8: (a) `freeze_alone_holds_the_ring_above_an_audible_floor_and_stops_the_transport`; (b) `freeze_latch_release_while_stopped_silences_within_the_bound`; (c) `encoder_edit_while_frozen_changes_the_output_measurably`; (d) T2.4 explicitly superseded/replaced in `FroggersAudioRoutingTests.cpp`, comment cites T6; (e) covered by T7.4 below.) (a) Stop with the latch engaged, on a patch with drives up: output stays above
      an audible floor past the bound an unlatched Stop must meet — the inverse of T2.3(b). (b) Release
      the latch while stopped: silence within the bound (T6.2). (c) An encoder edit while frozen changes
      the output measurably (T6.3), with a positive control proving the drone was live before the edit.
      (d) **T2.4 is SUPERSEDED and must be replaced, not left standing** — it asserts the latch is a
      no-op while stopped, which is now exactly backwards. (e) Unlatched Stop still silences: T2.3, T1.4
      (pass-D 0/20) and T1.3(b) MUST still pass unchanged — the bound only ever relaxes with the latch
      engaged. §9.1 for (a), (b), (c).
- [x] **T6.5 — OPERATOR GATE.** (operator confirmed in built app, 2026-08-18.) The operator confirms in the built app that pressing Freeze and then
      Stop reproduces the drone, and that releasing Freeze silences it. An implementer may not close
      this (§0). `kFreezeLatchOverdrive` (1.05f, `dsp/Delay.hpp:505`, flagged in its own comment as a
      by-ear constant that nothing measures or justifies) is the operator's to retune once the effect is
      actually reachable and audible.

## T7 — Freeze is self-contained; Stop is unconditional (operator, 2026-08-17, after testing T6)

**Why.** T6 made the drone reachable only as Freeze+Stop, and left the latch armed through Stop — so a
button labelled Stop conditionally meant "sustain". Operator ruling: **Freeze stops the transport
itself** (the effect needs no Stop press), and **Stop stops everything and resets the Freeze button.**
This restores Stop's unconditional silence guarantee, which T6 had weakened. Spec delta re-amended.

- [x] **T7.1 — Engaging Freeze stops the transport.** (landed 8b26ea8: `kFreeze` handler now pushes `MessageIn::Stop` + `SetDesiredTransportRunning(false)` on engage, `FroggersUiSurface.hpp`; comment rewritten.) The `kFreeze` handler
      (`FroggersUiSurface.hpp:1705-1712`) currently only flips the latch, with a comment stating it
      "pushes no MessageIn and never touches SetDesiredTransportRunning" — that comment is now wrong and
      must be rewritten, not left. On ENGAGE it SHALL also push `MessageIn::Stop` and
      `SetDesiredTransportRunning(false)`, exactly as the `kStop` handler does (:1700-1704) — including
      `SetDesiredTransportRunning`, whose D17 purpose is to stop a later audio-device renegotiation from
      re-asserting Start behind the app's back. On RELEASE it SHALL NOT start the transport (the
      operator resumes with Play).
- [x] **T7.2 — Stop disarms the latch.** (landed 8b26ea8: `kStop` handler calls `SetFreezeLatched(false)` before the Stop push, `FroggersUiSurface.hpp`, with happens-before ordering documented.) The `kStop` handler SHALL `SetFreezeLatched(false)` alongside
      its existing Stop push, so the teardown gate (`TransportTeardownActive`) evaluates true and the
      instrument silences on the same edge. Order matters: the latch must be clear before the audio
      thread next evaluates the gate — trace the message/flag ordering rather than assuming.
- [x] **T7.3 — Tests.** (landed 8b26ea8: (a) `freeze_alone_holds_the_ring_above_an_audible_floor_and_stops_the_transport`; (b) `stop_disarms_the_latch_and_silences_the_held_drone_within_the_bound`; (c) `no_freeze_stop_press_sequence_leaves_the_instrument_sounding_after_stop`; (d) `releasing_freeze_does_not_restart_the_transport`; Play's own latch-disarm pinned separately in a065ebc's `play_disarms_the_freeze_latch_and_returns_the_voice_gate_to_the_transport`.) (a) Freeze pressed while playing, with no Stop press: transport reads stopped
      AND output sustains above an audible floor past the bound (this is T6.4(a) restated without the
      Stop press — update that test rather than duplicating it). (b) Freeze engaged and sustaining, then
      Stop: latch reads disarmed and output silences within the bound. (c) No sequence of Freeze/Stop
      presses leaves the instrument sounding after a Stop — at minimum Freeze->Stop, Freeze->Freeze->Stop,
      Stop->Freeze->Stop. (d) Releasing Freeze does NOT restart the transport. §9.1 for (a), (b), (c).
- [x] **T7.4 — Re-verify the T6 regression set** (landed 8b26ea8: `stopped_transport_overrides_drive_and_freeze_to_unity_zero_and_resumes_bit_exact`, `stop_forces_release_from_mid_attack_bypassing_grace_and_stage_completion`, `stop_silences_curve_one_grace_active_voice_within_bound_pass_f_repro_as_suite_test` all present; suite 266/0/0 per T5.1.) unchanged: the unlatched-Stop tests
      (`stopped_transport_overrides_...bit_exact`, `stop_forces_release_from_mid_attack_...`,
      `stop_silences_curve_one_grace_active_voice_...`) and pass-D 0/20 all still hold.
- [x] **T7.5 — OPERATOR GATE** (operator confirmed in built app, 2026-08-18) (supersedes T6.5): the operator confirms in the built app that Freeze
      alone produces the drone, that Stop always kills it and clears the button, and that the button's
      lit/unlit state matches reality. `kFreezeLatchOverdrive` (1.05f) remains theirs to retune by ear.

## T5 — Suite

- [x] **T5.1** Full rebuild + suite; baseline 254/0/0; account for every test added or changed. The
      pass-D randomized stop condition and the pass-F minimal repro MUST be in the suite (T1.3b, T1.4),
      not only in the repro binary.
      ✅ **CLOSED 2026-08-17.** Clean rebuild (`rm -rf app/build`) + full `make test`: **exit 0, 266
      passed, 0 failed, 0 warnings**, all four packets' tests in ONE run. Per binary: dsp_parity 123,
      surface 45, modulation 35, audio_routing 27, parameter_model 12, marbles_clock 8, headless 6,
      mono_validation 4, visualizer 4, scope_advance_index 2.
      **Accounting vs the 254 baseline (enumerated from the diff, not inferred): 16 TEST_CASEs added,
      4 removed, net +12 — exact match.** The 4 removals are superseded, not lost:
      `max_attack_knob_reaches_sustain_within_the_new_one_second_ceiling` (renamed by T1.5 to
      `..._current_half_second_ceiling`), and three that asserted the two-row label treatment the
      operator rejected (`encoder_cell_renders_the_full_label_not_truncated_to_four_chars`,
      `every_parameter_label_fits_the_two_line_grid`,
      `one_word_label_uses_only_row_one_two_word_label_uses_both_rows`).
      T1.3(b)'s pass-F repro is in the suite (`FroggersHeadlessTests.cpp`); T1.4's pass-D re-run is
      recorded at 0/20 (run after T2.1 per the ordering correction) and T2.5's reverb-tank guard is in
      `FroggersDspParityTests.cpp`.

## Superseded in the predecessor (recorded there, pointed at from here)

- `frogg3rs-post-expansion-consolidation` T4.1 (closed as "keep the accident" — misclosure), T4.3/T4.4
  (drafted against §7's superseded mechanism), T1.1 (operator exercised the gate and FAILED it), and its
  §7 analysis (mechanism superseded by this change's §1a).

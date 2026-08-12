# Tasks — `frogg3rs-bank-expansion`

> **Read `proposal.md` first.** It carries the re-verified evidence, the operator's rulings with their
> verbatim quotes, and this session's own new findings (§4). This file does not repeat that reasoning.

**Goal of THIS change:** record decisions and correct two of the design doc's claims, as markdown only.
No source file is touched, nothing is built. The implementation tasks below (T1) are written so a later
session can pick them up, and are explicitly **not closable yet** — T1.0 is a hard gate.

**Session 2 (2026-08-11) adds, still markdown only, nothing built:** the central selection rule
(`proposal.md` §0a) and a further round of REJECTED candidates it produced (§3 rulings 6-7); DECIDED,
implementation-ready-but-not-yet-implemented task blocks for Filter (T2), Drive (T3), Delay (T4), and a
now-COMPLETE Reverb (T5); and the over-length short-label rework the operator folded into this change
(T6). Every PENDING RESEARCH slot named in `proposal.md` §9 stays unfilled at the end of session 2 — no
task in that session invents a candidate for one.

**Session 3 (2026-08-11, this update) adds, still markdown only, nothing built:** round-2 research fills
every remaining PENDING RESEARCH slot — Filter T2 gains its slot-13 task, Drive T3 gains slots 10-13,
Delay T4 gains slots 10/12/13, and a new Audio task block (T7) covers slots 12-13 (PM Rate, VCO Balance).
Filter, Drive, and Delay task blocks now cover their banks' full fourteen-parameter slates, matching T5's
Reverb. A new rejection (PM Depth Max, `proposal.md` §3 ruling 8) is recorded but has no task, same as
every other cut candidate. Ring Mod (Audio slots 9-11) was, at the end of this session, the only bank-slot
item in this entire change with no implementation task written at all — see "Recorded, not scheduled"
below at the time. **Session 5 corrects this: that framing was itself wrong — see the session-5 note
below.** §0 and §VERIFY are both re-run/re-affirmed at the bottom of this session's own edits.

**Session 4 (2026-08-11, this update) adds, still markdown only, nothing built:** a single new operator
ruling — a binding 10%-floor/80%-cap on VCO Balance's crossfade weights (`proposal.md` §4.4, §9.5). T7.2
is rewritten from a headroom-measurement task into an invariant-assertion task, split into T7.2 (the
requirement itself), T7.2a (the test asserting it), and T7.2b (the OMNI §9.1 positive control proving the
test rig can observe a violation). The headroom-flagged list (`proposal.md` §9.6) drops from three items
to two — **Comb Drive and Bias only**; VCO Balance is discharged by construction, not measurement. No
bank slot, slot count, or other ruling changes this session.

**Session 5 (2026-08-12, this update) corrects Ring Mod's design entirely, still markdown only, nothing
built.** The design recorded through session 4 — an open carrier choice, a pre-gate/post-gate
sub-question, and a claimed collision with `froggers-vco-topology`'s "No hardcoded cross-VCO coupling"
requirement — was WRONG (`proposal.md` §1's session-5 note, §0a addendum, §3 ruling 1, §4.2, §9.5, all
rewritten this session, not merely restated). Ring Mod is an ordinary parameter: each VCO has its own
ring modulator with an internal carrier, and the Ring Mod knob's range is that carrier's frequency, mapped
across audio rate the same way `Vco::PitchToPhaseIncrement` maps pitch. There is no carrier decision, no
cross-VCO coupling, and no collision — that collision was never real. **A new task block, T8, is added
for Ring Mod (Audio slots 9-11), the same pattern as T2-T7; it moves out of "Recorded, not scheduled" for
the first time.** The headroom-flagged list (`proposal.md` §9.6) stays at two items — Ring Mod is checked
and found unconditionally bounded (a product of two `[-1, 1]`-bounded signals), not flagged. No other bank
slot, slot count, or ruling changes this session.

**Inherited suite state (not verified by this session — markdown-only work, nothing built or run):** per
the most recent archived change's own final count
(`../archive/2026-08-10-frogg3rs-external-audio-phantom-input/tasks.md` T3.3), 10 binaries, 191 tests, 0
failures, 0 warnings, `External/Sheaf` pinned at `77a3019e`. Not re-verified here; a future implementing
session should re-run the suite before trusting this number.

## §0 Standing constraints (binding on any future implementing session)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB).
- **`External/Sheaf` is pinned and unpatchable.** No task here needs a Sheaf change.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose resolution requires an operator decision.** **Session 2
  correction: this line originally read "T1.0 and T2 are written to this rule explicitly," but no T2
  section existed in session 1 — a stale forward-reference, found and fixed here, not merely restated.**
  T1.0 is the one task block this rule actually gates today; the "Recorded, not scheduled" section below
  is where items with no written task at all live for the same underlying reason (an operator answer is
  missing), which is a different mechanism than a gated-but-written task. Session 2's own T2 (Filter) is
  explicitly NOT gated by this rule — see T2's own header, it needs no operator decision, only headroom
  re-derivation (T2.4).
- **Cite by SYMBOL, not by line** (`proposal.md` §0).
- **A negative result requires a positive control** (OMNI §9.1) — applies to T1.4's headroom
  re-measurement below.

---

## T0 — This change's own deliverable (markdown only, completable now)

- [x] **T0.1** Read `BANK-EXPANSION-DESIGN.md` in full and re-verify every claim this change carries
      forward, by symbol, against the current tree. Done — see `proposal.md` §2.
- [x] **T0.2** Record the operator's rulings with verbatim quotes, as DECIDED. Done — `proposal.md` §3.
- [x] **T0.3** Find and record anything stale or wrong beyond what the task brief already named. Done —
      `proposal.md` §4: patch persistence is name-keyed not slot-keyed (§4.1, still correct), and Decay is
      a two-part change, not a one-part insertion (§4.3, still correct). **Session 5 correction: this
      task's own original wording also claimed Ring Mod had "a second open sub-question plus a real
      tension with `froggers-vco-topology`'s ... coupling requirement" — that claim was WRONG and is
      removed here, not merely restated. `proposal.md` §4.2 now records the corrected design in full: an
      internal per-VCO carrier, no cross-VCO coupling, no tension with that requirement.**
- [x] **T0.4** Write the spec delta for the one capability this change's decided scope actually touches —
      `specs/froggers-sheaf-parameter-model/spec.md`. Done: one MODIFIED requirement (Envelope's target
      14-slot layout) and one ADDED requirement (bank-slate growth is safe for existing patches by
      construction, generalizing §4.1's finding for every future bank-fill proposal, not only this one).
- [x] **T0.5** `openspec validate --all --strict` — run 2026-08-11, after all three files were written.
      First attempt caught the exact soft-wrap trap this file's own §VERIFY warning names: the ADDED
      requirement's opening sentence had its `SHALL` on the second wrapped source line, not the first,
      and the validator (which only scans a requirement's first source line) flagged it. Fixed by
      rewriting the opening sentence so `SHALL` appears on its first line. Second attempt: **`Change
      'frogg3rs-bank-expansion' is valid`**, and `openspec validate --all --strict`: **66 passed, 0
      failed** (65 pre-existing specs + this change; no change-count regression against the pre-change
      baseline of 65 specs, 0 live changes).

**Session 2 (2026-08-11) additions to T0, same rules — markdown only:**

- [x] **T0.6** Read the three round-1 research files (`RESEARCH-audio-filter.md`, `RESEARCH-drive-delay.md`,
      `RESEARCH-reverb.md`, outside this repo) in full; record the central selection rule they and the
      operator's own words converge on, and re-verify its worked example and the Random S&H slew finding
      live against `app/dsp/Vco.hpp` and `app/dsp/RandomShLane.hpp`. Done — `proposal.md` §0a.
- [x] **T0.7** Apply the selection rule to record a further round of REJECTED candidates (self-FM, Glide,
      VCO Spread, Sub-Oscillator, and the promotion of Peak Slope from recommendation to ruling), each
      with a reason so none is re-proposed. Done — `proposal.md` §3 rulings 6-7.
- [x] **T0.8** Write the DECIDED slate for Filter (slots 9-12), Drive (slot 9), Delay (slots 9, 11), and
      Reverb (all of 9-13, now COMPLETE), citing the three research files as candidate provenance and
      re-verifying every Tier-1/reuse claim live against the current tree. Mark every remaining slot
      PENDING RESEARCH rather than inventing a fill, since the round-2 research covering them was still
      being written as of this session. Done — `proposal.md` §9.
- [x] **T0.9** Extend the spec delta (`specs/froggers-sheaf-parameter-model/spec.md`) with MODIFIED
      requirement scenarios for Filter/Drive/Delay/Reverb's new decided slot counts, replacing the
      now-inaccurate "five banks hold nine parameters, unchanged" scenario for those four banks. Done.
- [x] **T0.10** Record the over-length short-label rework as new scope in this change, per the operator's
      own instruction, re-counting the affected parameters directly from `FroggersBankLayouts()` rather
      than trusting the task brief's count verbatim (it undercounted Drive by one). Done — `proposal.md`
      §10, task T6 below.
- [x] **T0.11** `openspec validate --all --strict` — re-run after all session-2 edits. Result recorded at
      the bottom of this file, §VERIFY.

**Session 3 (2026-08-11) additions to T0, same rules — markdown only:**

- [x] **T0.12** Locate and read the two round-2 research files in full (`RESEARCH2-audio-filter.md`,
      `RESEARCH2-drive-delay.md`, outside this repo, at the scratchpad path this session's own task brief
      named), confirming each re-reads round 1's rejected-candidates list and the operator's central rule
      before proposing anything, per their own opening sections. Done — `proposal.md` §9's provenance
      note.
- [x] **T0.13** Apply the selection rule and the operator's own verbatim rejection to record PM Depth Max
      as CUT, including a correction of an earlier mischaracterization of what the candidate actually
      does (raises `kPmLfoDepth`'s ceiling above 0.15, not an interpolation within it), and carry the
      underlying by-ear tuning question forward separately, not as a parameter. Done — `proposal.md` §3
      ruling 8, §8.
- [x] **T0.14** Write the DECIDED slate for Filter slot 13 (Scoop Depth), Drive slots 10-13 (Link, Fold,
      Tone, Bias), Delay slots 10/12/13 (Feedback Tone, Width Balance, Crush), and Audio slots 12-13 (PM
      Rate, VCO Balance), re-verifying every hardcoded literal live against the current tree (`Vco.hpp`,
      `Drive.hpp`, `Delay.hpp`, `VoiceEnvelope.hpp`, `FroggersAppCore.hpp`, `FilterFx.hpp`) rather than
      trusting either round of research verbatim. Done — `proposal.md` §9.1-9.3, §9.5.
- [x] **T0.15** Trace and record the mechanism behind VCO Balance's headroom flag — the predecessor
      change's own §K per-stage table ruling Audio "No limiter needed," and why that verdict depends on
      `MixOscVoices`'s constant-total equal-thirds average specifically. Done — `proposal.md` §4.4.
- [x] **T0.16** Extend the spec delta (`specs/froggers-sheaf-parameter-model/spec.md`) with MODIFIED
      requirement scenarios completing Filter/Drive/Delay at fourteen parameters each and growing Audio
      from nine to eleven (PM Rate, VCO Balance; Ring Mod's three slots deliberately left empty). Done.
- [x] **T0.17** Write the session-3 totals section — the thirty-parameter target slate, the twenty-seven
      now decided, the Tier-1-style-unlock vs. genuinely-new-code tally, and the complete three-item
      headroom-flagged list, verified against every artifact above rather than assumed. Done —
      `proposal.md` §9.6.
- [x] **T0.18** Extend Filter (T2), Drive (T3), and Delay (T4) task blocks below with implementation tasks
      for every newly decided slot, and add a new Audio task block (T7) for PM Rate/VCO Balance — Ring Mod
      still gets no implementation task, per §0's standing rule. Done — see T2-T4, T7 below.
- [x] **T0.19** `openspec validate --all --strict` — re-run after all session-3 edits. Result recorded at
      the bottom of this file, §VERIFY.

**Session 4 (2026-08-11) additions to T0, same rules — markdown only:**

- [x] **T0.20** Record the operator's VCO Balance floor/cap ruling verbatim, derive the 10%-floor →
      80%-cap arithmetic explicitly, and trace why a convex weight triple bounded by `max(|v1|,|v2|,|v3|)`
      discharges the headroom question by construction rather than by measurement. Cross-reference the
      deliberate parallel to `dsp::VcoAdsrState::kMinSustainLevel`, verifying its 0.05 → 0.10 history by
      reading the archived `2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/tasks.md` (`S4.6`)
      against the current `VoiceEnvelope.hpp`. Done — `proposal.md` §4.4 (rewritten), §9.5 (VCO Balance
      entry rewritten).
- [x] **T0.21** Update the headroom-flagged list (`proposal.md` §9.6) from three items to two, recording
      why VCO Balance came off rather than silently dropping it. Update every other place that counted
      three flagged items (§11) to match. Done.
- [x] **T0.22** Add the floor/cap as a binding spec requirement, not only proposal narrative — a new
      scenario under the same MODIFIED requirement the other per-bank scenarios extend. Done —
      `specs/froggers-sheaf-parameter-model/spec.md`, "VCO Balance keeps every VCO in the mix."
- [x] **T0.23** Rewrite T7.2 from a headroom-measurement task into an invariant-assertion task (T7.2), add
      the test task (T7.2a) and its OMNI §9.1 positive-control task (T7.2b). Done — see T7 below.
- [x] **T0.24** `openspec validate --all --strict` — re-run after all session-4 edits. Result recorded at
      the bottom of this file, §VERIFY.

## T1 — Envelope bank: implementation-ready, BLOCKED on T1.0

**Do not execute T1.1-T1.6 until T1.0 is closed by the operator.** They are written now so the next
implementing session has a concrete plan, not so they can be executed immediately.

- [ ] **T1.0 — BLOCKING GATE, operator decision required, not closable by an implementer.** Whether the
      Envelope renumbering migrates existing saved patches, ships alongside an explicit "patches reset"
      notice, or is judged unnecessary given `proposal.md` §4.1's finding (no silent value-corruption risk
      via the patch-file mechanism specifically — but that finding does not cover the ergonomics/muscle-
      memory consequence of the physical reshuffle, nor any other stateful surface `proposal.md` §4.1
      itself flags as not audited, e.g. MIDI-learn mappings, which ARE slot-indexed per
      `MidiController.cpp`'s `MessageIn` JSON and were not investigated further under this change's
      scope). **Present `proposal.md` §4.1 to the operator as evidence, do not present it as the
      decision.**
- [ ] **T1.1** `app/dsp/VoiceEnvelope.hpp`, `VcoAdsrState`: add `Stage::Decay` to the `enum class Stage`
      between `Attack` and `Hold`. Per `proposal.md` §4.3, this is a two-part change: (a) Attack's target
      ceiling in `stepVoice` changes from `sustainLevel` to an independent peak (recommend `1.0f`, the
      same ceiling `Vco`'s own output already has — do not invent a new named constant without checking
      whether one is warranted per OMNI §6's 2-of-4 test), and (b) the new Decay stage ramps from that
      peak down to `MapSustain(sustainKnob)`, reusing the same divide-by-mapped-time idiom
      `attackStep`/`releaseStep` already establish. `VcoAdsrState::apply`'s signature grows a `decayKnob`
      parameter; update its one call site in `dsp::MixOscVoices` and that function's own call site in
      `FroggersAppCore.hpp`'s `RouteAudioSample`.
- [ ] **T1.2** `app/FroggersParameters.hpp`, `FroggersBankLayouts()`: rewrite the Envelope bank's
      `params` array to the interleaved order in the spec delta's "The Envelope bank holds fourteen
      parameters" scenario (`specs/froggers-sheaf-parameter-model/spec.md`). `kFroggersParamsPerBank`
      is bank-uniform (`= 9`) today — Envelope needs its own per-bank parameter count or an equivalent
      structural change; do not force the other five banks' array size to grow to accommodate Envelope's
      14. Read `FroggersBankLayout`'s own `params` field type (`std::array<FroggersParamSpec,
      kFroggersParamsPerBank>`) before assuming this is a one-line edit — it is sized per the
      bank-uniform constant, and Envelope needs to escape that uniformity, which the other five banks
      still rely on to varying degrees (spec delta's per-bank scenarios: Audio and, at slots 0-8, every
      other bank still hold their original nine; Filter/Drive/Delay/Reverb now escape it too, per session
      2 — see T2-T5 below). Preserve the Sustain parameters' existing `1.0f` non-neutral `defaultValue`
      at their new slot indices (2, 6, 10).
- [ ] **T1.3** Short names for Curve (slot 12) and Grace (slot 13): propose 4-char abbreviations (e.g.
      `Curv`, `Grac`). **Session 2 correction, per `proposal.md` §10: this task's own original wording was
      WRONG and is corrected here, not merely restated.** It claimed `CmbOff`/`PkFreq` and the other
      longer-form Filter/Drive/Delay/Reverb labels "are rendered through a different, non-14-segment
      path" — re-verified against `External/Sheaf/projects/synth/include/synth/EncoderDraw.hpp` this
      session and found false: `UpperShortLabel`'s `maxChars = 4` default applies uniformly, and
      `CmbOff`/`PkFreq`/etc. are silently truncated on-screen today, exactly like any other over-length
      `shortName` would be. The 4-char cap is universal on this rendering path, not page-specific to
      Envelope. Curve and Grace's short names must stay within it for the same reason every label T6
      below fixes must — pick reasonable abbreviations and move on, matching how every compliant
      `shortName` in this file was chosen (do not use `CmbOff`/`PkFreq` as the model; they are two of the
      23 T6 exists to fix).
- [ ] **T1.4** Grace's mechanism needs its own design pass at implementation time — this proposal does
      not fully specify it. It needs a per-voice timer that defers the `Attack/Decay/Hold -> Release`
      transition `setGate(false)` currently triggers immediately, until at minimum Attack (and per the
      design doc's own framing, arguably Decay) has completed. Read `VcoAdsrState::setGate` and
      `stepVoice`'s `Stage::Hold` case in full before starting; do not assume the existing per-sample step
      idiom trivially extends to a state-machine edge case it wasn't written to handle.
- [ ] **T1.5** Headroom re-check (OMNI §9.1 positive control required): per `proposal.md` §4.3, confirm by
      measurement, not by re-assumption, that the raised Attack ceiling does not push `chainIn` (the
      average of the three gated voices) above what it already reaches today at Sustain=1.0 — the
      per-voice bound is unchanged (still ≤1.0) but this must be measured on the actual three-voice
      average, not asserted from the per-voice argument alone, per the standing rule (`proposal.md` §7).
- [ ] **T1.6** Rebuild and re-run the full suite; report new binary/test counts, superseding this file's
      "inherited" line at the top. Any red beyond an intentionally-updated Envelope-shape test is a
      regression.

## T2 — Filter bank: all fourteen slots implementation-ready, COMPLETE (`proposal.md` §9.1)

No blocking gate — unlike Envelope, these are new parameters filling previously-empty slots, not a
reorder of existing occupied ones, so `proposal.md` §4.1's ADDED requirement covers patch safety by
construction (spec delta's "A newly added parameter loads at its ordinary default from an older patch"
scenario). **Session 3: slot 13 (Scoop Depth) added — this bank's task list is now complete, no PENDING
RESEARCH slot remains.**

- [ ] **T2.1** `FroggersAppCore.hpp`'s `RouteAudioSample`: replace the hardcoded `/*useParallel=*/true`
      argument to `FilterFxChain::Process` with a knob read (Filter bank slot 9, short name `Topo`).
- [ ] **T2.2** `FroggersAppCore.hpp`'s `RouteAudioSample`: stop feeding `filterChain_.scoopNotch.SetFreq`/
      `SetWidth` the same `bumpFreq`/`bumpWidth` locals `peak` uses; feed them independent values read
      from Filter bank slots 10 (`ScFq`) and 11 (`ScWd`).
- [ ] **T2.3** `app/dsp/FilterFx.hpp`'s comb branch: add a pre-gain into the existing
      `feedback * PadeSaturator::Saturate(filter.Process(tapped))` call (Filter bank slot 12, `CDrv`).
- [ ] **T2.4 — headroom re-derivation required (OMNI §9.1 positive control), per `proposal.md` §9.1's
      flag.** `rawCombTrim = 1.0f / (1.0f + std::fabs(comb.feedback))` was measured against `|comb| <= A +
      |fb|` at unity input gain. T2.3's pre-gain invalidates that bound. Re-derive the worst case with the
      pre-gain included, and re-measure `combTrimSmoother`'s adequacy against it — do not assume the
      existing trim still covers the new worst case.
- [ ] **T2.5 — session 3.** `FroggersAppCore.hpp`'s `RouteAudioSample`: stop feeding
      `knob(FroggersBankId::Filter, 8)` to both `filterChain_.scoopNotch.SetHeight` and
      `FilterFxChain::Process`'s `scoopMix` argument; feed `SetHeight` its existing slot-8 read unchanged
      and feed `scoopMix` an independent value read from Filter bank slot 13 (`ScDp`). No headroom
      re-derivation task follows — `proposal.md` §9.1 confirms `SetHeight`'s dip and `scoopMix`'s convex
      blend are each independently bounded regardless of how the two are decoupled.
- [ ] **T2.6** `app/FroggersParameters.hpp`: register the five new Filter parameters at slots 9-13 with
      the short names above, following T1.2's pattern for escaping `kFroggersParamsPerBank`'s uniformity.
- [ ] **T2.7** Rebuild and re-run the full suite; report new binary/test counts.

## T3 — Drive bank: all fourteen slots implementation-ready, COMPLETE (`proposal.md` §9.2)

**Session 3: slots 10-13 (Link, Fold, Tone, Bias) added — this bank's task list is now complete, no
PENDING RESEARCH slot remains.**

- [ ] **T3.1** `app/dsp/Drive.hpp`'s `Oversampler2x`: replace the hardcoded `antiAlias.SetAlphaFromNatFreq
      (0.4f)` constructor call with a knob-driven `SetAlphaFromNatFreq` call (Drive bank slot 9, short
      name `ABrt`). No headroom flag — a lowpass cutoff change cannot raise peak amplitude.
- [ ] **T3.2 — session 3.** `app/dsp/Drive.hpp`'s `PolynomialDrive::SetCoefs`: replace the hardcoded
      `0.25f` weight in both `coefs[1]`'s and `coefs[3]`'s `0.25f * (computedGain - 1.0f)` term with a
      knob-driven scalar (Drive bank slot 10, short name `Link`). No headroom re-derivation task follows —
      `proposal.md` §9.2 confirms `PolynomialDrive::Process(0) == 0` regardless of the coefficients, and
      `coefs[1]`/`coefs[3]` stay bounded to `[-10, 10]` regardless of the coupling weight.
- [ ] **T3.3 — session 3.** `app/dsp/Drive.hpp`'s `FrogBlock::Process`: replace the hardcoded `sinIn = out
      / 4.0f` divisor with a knob-mapped value (Drive bank slot 11, short name `Fold`). No headroom
      re-derivation task follows — `proposal.md` §9.2 confirms `Sine01`'s output stays bounded to `[-1, 1]`
      for any divisor and `Process(0) == 0` at every Fold setting.
- [ ] **T3.4 — session 3.** `app/dsp/Drive.hpp`: add a post-chain `dsp::OnePoleLowPass` stage after
      `FrogBlock`'s existing output (Drive bank slot 12, short name `Tone`). No headroom flag — a lowpass
      only removes energy.
- [ ] **T3.5 — session 3, headroom re-derivation required (OMNI §9.1 positive control), per
      `proposal.md` §9.2's flag.** `app/dsp/Drive.hpp`'s `PolynomialDrive`: apply a knob-driven DC offset
      before `Process` and subtract `Process(bias)` after — `PolynomialDrive::Process(input + bias) -
      PolynomialDrive::Process(bias)` — the same subtract-after-shift construction
      `DigitalReorganizer::Process` already uses, generalized to `PolynomialDrive` (Drive bank slot 13,
      short name `Bias`). `f(0) = 0` is closed by construction; MEASURE the peak-swing residual across the
      whole Bias sweep before shipping — `PolynomialDrive::Process` is an unbounded 5th-order polynomial,
      so the DC-cancellation alone does not bound peak swing identically at every setting.
- [ ] **T3.6** `app/FroggersParameters.hpp`: register the five new Drive parameters at slots 9-13.
- [ ] **T3.7** Rebuild and re-run the full suite; report new binary/test counts.

## T4 — Delay bank: all fourteen slots implementation-ready, COMPLETE (`proposal.md` §9.3)

**Session 3: slots 10, 12, 13 (Feedback Tone, Width Balance, Crush) added — this bank's task list is now
complete, no PENDING RESEARCH slot remains.**

- [ ] **T4.1** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: add a pre-gain into the existing
      `WriteSample(inSignal + fbk * PadeSaturator::Saturate(fbL), lineL)` / `fbR`/`lineR` lines (Delay
      bank slot 9, short name `FbDr`). **No headroom re-derivation task follows this one** — `proposal.md`
      §9.3 records why the round-1 flag on this exact candidate was withdrawn as wrong: `Saturate` clamps
      to ±1 unconditionally before the addition, so a pre-gain into it cannot raise the per-sample bound
      the existing comment already proves. Do not re-open this as a headroom item without new evidence.
- [ ] **T4.1a — session 3.** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: insert a `dsp::OnePoleLowPass`
      into the feedback tap ahead of the existing `PadeSaturator::Saturate` call, damping `fbL`/`fbR`
      before `WriteSample` (Delay bank slot 10, short name `FbTn`). No headroom flag — a lowpass inside a
      feedback loop only removes recirculating energy.
- [ ] **T4.2** `app/dsp/Delay.hpp`: replace the hardcoded `lfoInc = 2.0f * 3.14159265f * 0.25f /
      sampleRate` with a knob-driven rate (Delay bank slot 11, short name `MdRt`).
- [ ] **T4.2a — session 3.** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: replace the two hardcoded
      weights `const float widthSpread = p.dwid * baseSeconds * 0.35f;` and `const float cross = p.dwid *
      0.5f;` with a complementary pair of knob-derived weights, both still driven by `p.dwid` (Delay bank
      slot 12, short name `WBal`). No headroom re-derivation task follows — `proposal.md` §9.3 confirms
      `fbL`/`fbR` stay a convex combination bounded by `max(|dL|,|dR|)` at any split, and `widthSpread`
      only offsets a `ReadAt` tap position, never adds gain.
- [ ] **T4.2b — session 3.** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: route the feedback tap's
      repeats through `dsp::SampleRateReducer` and/or `dsp::DigitalReorganizer` (`app/dsp/Drive.hpp`),
      reused as-is — both already ship the `f(0) = 0` DC-block fix, so no hand-rolled copy is needed
      (Delay bank slot 13, short name `Crsh`). No headroom flag — these structs' outputs never exceed
      their input's magnitude.
- [ ] **T4.3** `app/FroggersParameters.hpp`: register the five new Delay parameters at slots 9-13.
- [ ] **T4.4** Rebuild and re-run the full suite; report new binary/test counts.

## T5 — Reverb bank: all of slots 9-13, COMPLETE, session 2 (`proposal.md` §9.4)

- [ ] **T5.1** `app/dsp/Reverb.hpp`: replace the hardcoded `kModLfoHz = 0.35f` feeding `modLfoPhase`'s
      increment with a knob-driven rate (Reverb bank slot 9, short name `MdRt`).
- [ ] **T5.2** `app/dsp/Reverb.hpp`: add a pre-gain into the existing `aIn = preOut + fb *
      PadeSaturator::Saturate(aFb)` / `bIn` lines (Reverb bank slot 10, `TkDv`).
- [ ] **T5.3** `app/dsp/Reverb.hpp`: route the tank feedback (`aFb`/`bFb`) through
      `dsp::DigitalReorganizer::Process` ahead of the existing `PadeSaturator::Saturate` call (Reverb bank
      slot 11, `Grit`). Reuse `DigitalReorganizer::Process`/`Mangle` as-is (`app/dsp/Drive.hpp`) — do not
      hand-roll a copy of the bit-scramble formula, which would reintroduce the `f(0) != 0` defect that
      formula's own DC-blocked construction already fixed once.
- [ ] **T5.4** `app/dsp/Reverb.hpp`: add a bipolar tone-shave stage (two `dsp::OnePoleLowPass` instances,
      one direct lowpass tap and one complementary highpass via `input - lowpass(input)`, crossfaded
      around the knob's center) applied to `wet`/`mixedOut` before the existing `wetLimiter.Process()`
      call (Reverb bank slot 12, `Tilt`).
- [ ] **T5.5 — headroom re-derivation required (OMNI §9.1 positive control), per `proposal.md` §9.4's own
      carried-forward flag.** `kReverbWetLimiterThreshold` (0.72) and its attack/release were measured
      against the un-tilted spectral content. Re-sweep with Tilt at its brightest setting before shipping;
      do not assume the existing limiter margin covers it unmeasured.
- [ ] **T5.6** `app/dsp/Reverb.hpp`: drive `dA`/`dB` (currently `180 + sizeNorm*1300` / `260 +
      sizeNorm*1800`, modified by `applyMod`) directly from this parameter's own resolved value, per §3
      ruling 5's re-scoping — no pitch tracker (Reverb bank slot 13, `Tund`).
- [ ] **T5.7 — MEASURE, do not assume safe, per `proposal.md` §9.4's own carried-forward caution.**
      Modulating Tuned at audio rate sweeps a delay length inside a feedback loop. Confirm by measurement
      that this stays bounded under the existing in-loop saturator and wet limiter before shipping audio-
      rate modulation depth on this target; the existing Mod-depth precedent on the same `dA`/`dB` pair is
      not itself a substitute for measuring this new use.
- [ ] **T5.8** `app/FroggersParameters.hpp`: register the five new Reverb parameters at slots 9-13,
      completing the bank.
- [ ] **T5.9** Rebuild and re-run the full suite; report new binary/test counts.

## T6 — Over-length short-label rework, new scope added by the operator (`proposal.md` §10)

**23 of the 54 current page parameters (not the task brief's "22 of 53" — `proposal.md` §10 records the
correction) have a `shortName` longer than the 14-segment display's 4-character cap
(`UpperShortLabel`'s `maxChars = 4` default, `EncoderDraw.hpp`).** Enumerated by bank, full name and
current `shortName`:

- **Filter (8 of 9):** Comb offset (`CmbOff`), Peak freq (`PkFreq`), Peak gain (`PkGain`), Comb delay
  (`CmbDly`), Comb feedback (`CmbFb`), Comb LP (`CmbLP`), Comb/Peak (`Cmb/Pk`), Scoop (`Scoop`). Clean:
  Peak Q (`PkQ`).
- **Drive (5 of 9 — corrected from the task brief's "4 of 9"):** Drive (`Drive`), Shape (`Shape`), Bit
  depth (`BitDp`), Blend (`Blend`), Phase (`Phase`). Clean: SRR 1 (`SRR1`), SRR 2 (`SRR2`), XOR (`XOR`),
  Fuzz (`Fuzz`).
- **Delay (6 of 9):** Delay time (`DlyTm`), Stereo width (`Width`), Detune (`Detune`), Mod depth
  (`ModDp`), Wet mix (`WetMx`), Color (`Color`). Clean: Send (`Send`), Feedback (`Fb`), Halo (`Halo`).
- **Reverb (4 of 9):** Decay (`Decay`), Pre-delay (`PreDly`), Stereo width (`Width`), Mod depth
  (`ModDp`). Clean: Wet/dry (`Wet`), Room size (`Room`), Damping (`Damp`), Diffusion (`Diff`), Hold
  (`Hold`).
- **Audio and Envelope: 0 of 9 each, already clean.** Confirmed, not merely restated.

- [ ] **T6.1** Design the larger, "slightly larger colored boxes with smaller text" cell treatment for the
      23 over-length labels — the operator's own words, verbatim: *"they can just be slightly larger
      colored boxes with smaller text."* This cell also renders the modulation badge chips
      (`AppendBadge`/`drawBadges`, `EncoderDraw.hpp`, sharing the same `centerX`/`centerY`/`baseRadius`
      geometry the label uses) — any box resize or font-size change must be checked against chip layout
      too, not designed against the label in isolation.
- [ ] **T6.2 — VISUAL acceptance criterion, not a code-shape one.** A screenshot (or live capture) of at
      least one full bank containing over-length labels (e.g. Filter), at the resolution the hardware
      display actually renders at, showing: (a) every label's full text legible without truncation, (b)
      the modulation badge chips still rendered correctly in the same cells, (c) the box-size change
      applied consistently across the bank, not just to the single worst-case label. **A test asserting a
      weaker property than "the operator can see it and confirms it" does not close this task** — a prior
      UI change in this project took four attempts for exactly that reason (`proposal.md` §10).
- [ ] **T6.3** Apply the same cell treatment to the Envelope and Audio banks' cells too (0 over-length
      labels today, but T1/T2/T3/T4/T5/T7 above add new parameters to Envelope, Filter, Drive, Delay,
      Reverb, and Audio — confirm no newly-added `shortName` from those tasks exceeds 4 characters, since
      they are chosen to comply from the start, but the *rendering* change T6.1 makes should be uniform
      across all six banks' cells, not just the four with pre-existing violations).
- [ ] **T6.4** Rebuild and re-run the full suite; a rendering-only change should not change any DSP
      test's outcome — any red is a regression, not an expected side effect.

## T7 — Audio bank: slots 12-13 implementation-ready, session 3, T7.2 tightened session 4 (`proposal.md` §9.5)

**Ring Mod (Audio slots 9-11) is covered by its own task block, T8, below — not this one** (PM Rate and
VCO Balance are a separate pair of slots from Ring Mod's three). Through session 4 this header claimed
Ring Mod was "deliberately NOT covered by any task in this block or anywhere else in this file," blocked
on two open sub-questions plus a `froggers-vco-topology` spec tension — **that framing was WRONG and is
corrected at T8 and `proposal.md` §4.2, session 5; it is no longer true.** No blocking gate on the tasks
below — new parameters filling previously empty slots, same patch-safety-by-construction reasoning as T2's
header. **Session 4: T7.2 is rewritten from a headroom-measurement task into an invariant-assertion task
(T7.2, T7.2a, T7.2b), per the operator's new floor/cap ruling on VCO Balance — see below.**

- [ ] **T7.1** `app/dsp/Vco.hpp`'s `StepPmLfo`: decouple the rate calculation
      (`ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmKnob01)`) from `pmKnob01`, feeding it a second,
      knob-driven rate value shared across all three VCOs' calls instead (Audio bank slot 12, short name
      `PMrt`). Per `proposal.md` §9.5, this ships as ONE knob shared across VCO1-3's `StepPmLfo` calls, not
      three independent per-VCO rates — only one slot is available. No headroom flag — phase-domain only,
      `EvalWaveMorph` always evaluates a wrapped phase to `[-1, 1]` regardless of what drives the offset.
- [ ] **T7.2 — session 4: invariant, not a measured headroom item. `proposal.md` §4.4/§9.5/§9.6.**
      `app/dsp/VoiceEnvelope.hpp`'s `MixOscVoices`: replace the fixed `(v1 + v2 + v3) * (1.0f / 3.0f)`
      return with a normalized 3-point crossfade driven by one knob (Audio bank slot 13, short name
      `VBal`). **Binding requirement, operator ruling:** the three weights SHALL sum to exactly 1 at every
      knob position, AND each weight SHALL stay in `[0.10, 0.80]` — no knob position may reduce any VCO to
      0% or raise any VCO to 100% of the mix; three VCOs with a 10% floor each cap any single VCO at 80%
      (`1 - 0.10 - 0.10`). This is the same class of constraint as `dsp::VcoAdsrState::kMinSustainLevel`
      (§4.3/§4.4) — a knob must not be able to gate a signal path to zero.
- [ ] **T7.2a — session 4, test SHALL assert the invariant, not measure output level.** Write a test that
      sweeps the VCO Balance knob across its full `[0, 1]` range and asserts, at each sampled position:
      (a) the three weights sum to exactly 1 (within float tolerance), and (b) each weight stays within
      `[0.10, 0.80]`. Do NOT write this as a `chainIn`-level measurement — the floor-and-cap invariant
      makes the bound provable from the weight mapping alone (`proposal.md` §4.4 derives the convexity
      argument in full); measuring output samples to indirectly infer the weights would be strictly weaker
      evidence than asserting the weights themselves.
- [ ] **T7.2b — session 4, positive control required (OMNI §9.1).** Before trusting T7.2a's test, prove
      the rig can actually observe a violation: temporarily break the invariant (e.g. let one weight reach
      0 or 1, or let the sum drift off 1) and confirm the test in T7.2a fails. A test that asserts `sum(w)
      == 1` but never varies the knob, or never exercises the extremes, would pass vacuously — this task
      exists so that risk is checked once, not assumed away.
- [ ] **T7.3** `app/FroggersParameters.hpp`: register the two new Audio parameters at slots 12-13.
- [ ] **T7.4** Rebuild and re-run the full suite; report new binary/test counts.

## T8 — Audio bank: Ring Mod, slots 9-11, session 5 (`proposal.md` §3 ruling 1, §4.2, §9.5)

**Corrects the prior framing in full.** Through session 4, this bank-slot item was wrongly recorded as
blocked on an open carrier choice, a pre-gate/post-gate sub-question, and a `froggers-vco-topology`
collision — none of that was real (`proposal.md` §4.2). Ring Mod is an ordinary parameter: each VCO has
its own ring modulator with an internal carrier, and the Ring Mod knob's range is that carrier's
frequency, mapped across audio rate the same way `Vco::PitchToPhaseIncrement` maps pitch. No blocking gate
on the tasks below — new parameters filling previously empty slots, same patch-safety-by-construction
reasoning as T2's header.

- [ ] **T8.1** `app/dsp/Vco.hpp` (or a new small per-VCO ring-modulator helper alongside it): add an
      internal carrier oscillator per VCO, phase-stepped the same way `Vco::Process` already steps its own
      carrier (`carrierPhase = WrapPhase(carrierPhase + phaseIncrement)`), with its own frequency driven by
      the Ring Mod knob through the same exponential map `PitchToPhaseIncrement` already uses
      (`ExpMapCompute(min, max, ringModKnob01)`). Do not reuse `PitchToPhaseIncrement`'s own `20.0f`/
      `20000.0f` literals without checking whether they are the right bounds for Ring Mod's own range —
      the low bound is the one implementation detail `proposal.md` §8 leaves open (sub-audio floor vs.
      audio-rate-only).
- [ ] **T8.2** `FroggersAppCore.hpp`'s `RouteAudioSample` (or wherever the three VCOs' audio is combined):
      multiply each VCO's own signal by its own internal carrier's output before combining, one multiply
      per VCO (Audio bank slots 9-11, short names `RM1`/`RM2`/`RM3`). Whether the multiply reads the
      pre-gate (`vcoOut[i]`) or post-gate (`gatedVoices.v1/v2/v3`) signal is an ordinary implementation
      choice, not a design decision this change makes — either satisfies the spec, since both are the SAME
      VCO's own signal either way (`proposal.md` §4.2). No headroom re-derivation task follows — §9.5/§9.6
      confirm the product of two `[-1, 1]`-bounded signals is itself bounded to `[-1, 1]`.
- [ ] **T8.3** `app/FroggersParameters.hpp`: register the three new Ring Mod parameters at Audio bank
      slots 9-11, following T7.3's pattern.
- [ ] **T8.4** By-ear settle the one open implementation detail from `proposal.md` §8: whether the knob's
      low end reaches sub-audio (an effectively-clean position) or stays audio-rate across the whole
      sweep. Not a blocking gate — either choice satisfies `proposal.md` §3 ruling 3's continuous-range
      rule.
- [ ] **T8.5** Rebuild and re-run the full suite; report new binary/test counts.

## Recorded, not scheduled — no task closes these

- **Cross XOR, Cycle, Hard Sync, Peak Slope, self-FM, Glide/portamento/slew, VCO Spread, Sub-Oscillator,
  PM Depth Max — all cut.** `proposal.md` §3 rulings 2-3, 6-8. Nothing was ever built for any of them;
  nothing to remove. PM Depth Max's own underlying by-ear tuning question (is `kPmLfoDepth = 0.15` the
  right ceiling?) is carried forward separately below, as a tuning item, not a parameter. Peak Slope moved
  here from a "recommendation, not ruling" framing in session 1 — the
  session-2 task brief lists it among the operator's REJECTED candidates, closing that gap.
- **Reverb Tuned is no longer merely "cost corrected" — it is now specified and scheduled, at T5.6-T5.7.**
  `proposal.md` §3 ruling 5's correction is what made this possible; the correction itself does not need
  restating here since the item it unblocked now has real tasks.
- **Ring Mod (Audio 9-11) is no longer on this list — session 5 (`proposal.md` §3 ruling 1, §4.2, §9.5).**
  It moved to a real task block, T8, above. Through session 4 it was wrongly recorded here as "the only
  remaining blocker in this entire change"; that framing was itself wrong and is removed, not resolved by
  an operator decision.

## Carried forward as open scope — restated from `proposal.md` §8 (session 5), do NOT fill in

**Updated 2026-08-12 (session 5).** Every PENDING RESEARCH bank slot session 2 left open — Audio 12-13,
Filter 13, Drive 10-13, Delay 10/12/13 — is DECIDED and scheduled (T2, T3, T4, T7 above); Ring Mod (Audio
9-11), the one item that remained through session 4, is now DECIDED and scheduled too (T8, above) — its
"blocker" framing was wrong, not resolved. **Zero bank-slot items remain on this list.** What remains here
is open for reasons other than "the research hadn't been written" or "the framing was wrong" — **do not
invent a fill or a decision for any of these:**

- **Whether the Ring Mod knob's range reaches sub-audio at its low end — session 5, an implementation
  detail, NOT a bank-slot blocker** (`proposal.md` §4.2, §8, §9.5; T8.4 above). Either choice satisfies
  the continuous-range rule (§3 ruling 3); settle by ear at build time.
- **Whether `kPmLfoDepth = 0.15` is the right maximum PM depth — session 3, an open by-ear tuning item,
  NOT a parameter** (`proposal.md` §3 ruling 8, §8). If too shallow by ear, fix the constant directly; no
  task in this file schedules that tuning pass.
- The design doc's open question 8 — ASR envelopes cannot modulate anything; a modulation-slate question,
  not a bank-slot question, that may outrank everything in this document. Untouched by sessions 2-5.
- Whether the Envelope renumbering migrates or resets saved patches (T1.0 above). Untouched by sessions
  2-5.

## §VERIFY — `openspec validate --all --strict`, session 2

Run 2026-08-11 after all session-2 edits to `proposal.md`, `tasks.md`, and
`specs/froggers-sheaf-parameter-model/spec.md`. Result: **`change/frogg3rs-bank-expansion` valid**,
**66 passed, 0 failed (66 items)** — the same total item count as session 1's own final run (65
pre-existing specs + this one change), confirming no regression and no accidental new spec/change file
introduced by this update. No soft-wrap trap this time; the new requirement scenarios added in this
session use the same `THEN`-per-line style the validated Envelope scenario already established, so none
of their opening lines needed a rewrite.

## §VERIFY — `openspec validate --all --strict`, session 3

Run 2026-08-11 after all session-3 edits to `proposal.md`, `tasks.md`, and
`specs/froggers-sheaf-parameter-model/spec.md`. Result: **`✓ change/frogg3rs-bank-expansion`**,
**Totals: 66 passed, 0 failed (66 items)** — the identical total item count as session 2's own final run,
confirming no regression and no accidental new spec/change file introduced by this session either (every
edit this session extended existing scenarios in the same one change and its one spec delta, never added
a new file). No soft-wrap trap this session: every new/extended requirement scenario continues the same
`THEN`-per-line style already established, and the one MODIFIED requirement's own opening sentence (`One
sixteen-slot bank per Froggers page`) was untouched this session, so no re-check of its SHALL placement
was needed.

## §VERIFY — `openspec validate --all --strict`, session 4

Run 2026-08-11 after all session-4 edits to `proposal.md`, `tasks.md`, and
`specs/froggers-sheaf-parameter-model/spec.md`. Result: **`✓ change/frogg3rs-bank-expansion`**,
**Totals: 66 passed, 0 failed (66 items)** — the identical total item count as sessions 2 and 3's own
final runs, confirming no regression and no accidental new spec/change file introduced this session
either (the one new scenario, "VCO Balance keeps every VCO in the mix," was added under the same existing
MODIFIED requirement, not as a new requirement or a new file). No soft-wrap trap: the new scenario's own
first `THEN` line carries its SHALL, matching the established per-scenario style.

## §VERIFY — `openspec validate --all --strict`, session 5

Run 2026-08-12 after all session-5 edits to `proposal.md`, `tasks.md`, and
`specs/froggers-sheaf-parameter-model/spec.md`. Result: **`✓ change/frogg3rs-bank-expansion`**,
**Totals: 66 passed, 0 failed (66 items)** — the identical total item count as sessions 2-4's own final
runs, confirming no regression and no accidental new spec/change file introduced this session either (the
Audio bank scenario was rewritten in place from "eleven parameters, slots 9-11 pending" to "fourteen
parameters, complete," under the same existing MODIFIED requirement, not as a new requirement or a new
file). No soft-wrap trap: the rewritten scenario's own opening `WHEN`/`THEN` lines are unchanged in
placement from the version that already validated in session 3, only their content changed. Confirmed no
other spec in the tree (`external-ring-mod-mix`, an unrelated existing spec about external-audio input
mixing, not this change's Ring Mod knobs) references or is affected by this change.

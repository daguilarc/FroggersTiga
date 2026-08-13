# Tasks — `frogg3rs-bank-expansion`

> **Read `proposal.md` first.** It carries the re-verified evidence, the operator's rulings with their
> verbatim quotes, and this session's own new findings (§4). This file does not repeat that reasoning.

**Goal of THIS change:** record decisions and correct two of the design doc's claims, as markdown only.
No source file is touched, nothing is built. The implementation tasks below (T1) are written so a later
session can pick them up. **Session 6: T1.0, the hard gate that made them un-closable from session 1
through session 5, is MOOT — there are no saved patches and no MIDI-learn mappings to migrate or reset
(`proposal.md` §3a ruling 11). T1.1-T1.6 are executable.**

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
to two — **Comb Drive and Bias only** as of this session; session 6 later changed the composition again,
to **Bias and Topology** — VCO Balance is discharged by construction, not measurement. No
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

**Session 6 (2026-08-12, this update) is the OMNI §14 PREFLIGHT audit of sessions 1-5, still markdown only,
nothing built.** It re-verified every hardcoded-literal claim in this change directly against the tree (all
held, including §10's 23-of-54 label count), re-derived the two claims session 5 leaned hardest on (Ring Mod
does NOT collide with `froggers-vco-topology`'s coupling requirement; VCO Balance's convexity argument is
valid) — and found five things that did not hold (`proposal.md` §1a). Consequences in this file: **three
questions for the operator — T7.0 (PM Rate relaxes a live `froggers-vco-topology` requirement), T8.0 (Ring
Mod has no off position or depth control), and default parity for all twelve unlocked literals** — plus
tightened T1.1, T3.2, T3.3, T4.2a, and T6. A second spec delta is added,
`specs/froggers-vco-topology/spec.md`. **All three were put to the operator as questions, not standing
gates, and all three are answered (`proposal.md` §3a rulings 9-11): Ring Mod gains a true zero position
implemented as ONE function shared with PM (T8.0-T8.0c, `proposal.md` §4.5); PM Rate keeps Audio slot 12
with `froggers-vco-topology` relaxed to match (T7.0); and patch compatibility is a non-goal — verified zero
saved patches and zero MIDI-learn mappings on disk — which makes T1.0 MOOT and unblocks the Envelope block
T1.1-T1.6 for the first time since session 1. That last ruling also DELETED the T9 block the audit had
written for default parity: with no patches to preserve, choosing a default is one `defaultValue` line in
each registration task (T2.6/T3.6/T4.3/T5.8/T7.3/T8.3), not a work package.** A fourth question the operator
raised while reviewing — is Filter Topology even a continuous parameter? — is a real finding and is
**answered by the operator in the same session (ruling 12): a continuous morph, with the dead series branch
deleted (T2.1, T2.1a).** A fifth,
non-blocking item follows from the operator's own "isn't this already part of the parameter class?" question:
Sheaf already ships six per-parameter-type mapping functions this app calls zero times, while
`RouteAudioSample` open-codes six of them. **Written up as a task block, then checked against §8's actual
test and REJECTED as a non-finding** — see "Recorded, not scheduled" at the bottom of this file.

**Session 7 (2026-08-12, this update) is an OMNI audit of the artifacts themselves, after rulings 9-12
landed, still markdown only, nothing built.** No decision, slot, ruling or count changes. What changed is
every place that had not caught up with those rulings or with `proposal.md` §7a: `T9` survived as a live
pointer in nine places after ruling 11 deleted it (now the `defaultValue` line in each registration task);
`proposal.md` §11/§1a still said two audit questions were open while §3a and this file said all four were
answered; §11a still named the ⚠ list as "Comb Drive, Bias" instead of Bias and Topology; §0's
positive-control line pointed at T1.4 instead of T1.5; T8.1 still cited a withdrawn sub-audio design
question; and the carried-forward list still carried the Envelope migrate-or-reset question its own intro
declares closed. Full finding list at `proposal.md` §1b. **One finding is not bookkeeping, and this session
got it wrong before getting it right — both recorded:** chasing a wording question (do `Crispy`/`Crnchy`
get shortened text, and to what?) established that `EncoderDrawState` has no label config field, and a
first pass concluded from that alone that T6 needed a Sheaf unpin, wrote a blocking gate T6.0, and filed an
upstream issue. **Reading further showed `BuildFourteenSegmentCommands` is public with `numChars` as a
parameter and the app owns the returned command vector, so T6.1 is ordinary app-side work.** The gate is
deleted, §0's "no Sheaf change" restored — now checked against T6 rather than assumed — the mechanism is
written into T6.1 so it is not re-derived, and the upstream issue was withdrawn and closed. **No gate
exists anywhere in this change.**

**Inherited suite state (not verified by this session — markdown-only work, nothing built or run):** per
the most recent archived change's own final count
(`../archive/2026-08-10-frogg3rs-external-audio-phantom-input/tasks.md` T3.3), 10 binaries, 191 tests, 0
failures, 0 warnings, `External/Sheaf` pinned at `77a3019e`. Not re-verified here; a future implementing
session should re-run the suite before trusting this number.

## §0 Standing constraints (binding on any future implementing session)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB).
- **`External/Sheaf` is pinned and unpatchable.** No task here needs a Sheaf change. **Session 7 checked
  this against T6 specifically, which no prior session had: it holds. T6.1 composes its own label block
  from the public `BuildFourteenSegmentCommands` over the command vector Sheaf returns — app-side, Sheaf
  untouched (T6.1, `proposal.md` §10). A pass earlier this same session claimed the opposite and was
  wrong.**
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **An implementer may not close a task whose resolution requires an operator decision.** **Session 2
  correction: this line originally read "T1.0 and T2 are written to this rule explicitly," but no T2
  section existed in session 1 — a stale forward-reference, found and fixed here, not merely restated.**
  T1.0 was the one task block this rule actually gated; the "Recorded, not scheduled" section below
  is where items with no written task at all live for the same underlying reason (an operator answer is
  missing), which is a different mechanism than a gated-but-written task. **Session 6: T1.0 is moot (ruling
  11) and T7.0/T8.0 are answered (rulings 10 and 9). Session 7: this rule now gates NOTHING** — T6, briefly
  gated earlier that same session on a wrong reading of what the app can draw, is ordinary app-side work
  (T6.1). Every remaining `[ ]` task is an implementation or measurement task an implementer can close on
  evidence. Session 2's own T2 (Filter)
  was never gated by this rule; the headroom re-derivation its header once cited, T2.4, is itself WITHDRAWN
  (the flag was wrong — `proposal.md` §7a). What Filter carries instead is T2.1a, a measurement, not a gate.
- **Cite by SYMBOL, not by line** (`proposal.md` §0).
- **A negative result requires a positive control** (OMNI §9.1) — applies to every measurement task in
  this file, and the corrected list is: **T1.5** (the Envelope headroom re-check; this line said "T1.4"
  through session 6, which is Grace's design pass and measures nothing), **T2.1a** (Topology's peak-stage
  sweep), **T3.5** (Bias's peak swing), **T5.5** (Tilt against the wet limiter), **T5.7** (Tuned's
  audio-rate delay-length sweep), **T7.2b** and **T8.0c** (each already written as its own positive
  control).

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

## T1 — Envelope bank: implementation-ready, UNBLOCKED (session 6)

**T1.0 was this change's oldest gate and it is now moot — see below. T1.1-T1.6 are executable.** They were
written in session 1 so a later session had a concrete plan; that session can now run them.

- [x] **T1.0 — MOOT, session 6, closed on evidence rather than on a decision (`proposal.md` §3a ruling 11).**
      The operator: *"i haven't saved any old patches?"* — verified, not taken on trust:
      `~/Library/Sheaf/synth/sheaf-patch/patches/frogg3rs/` contains zero patch files, and the runtime
      config's MIDI-instrument block is `"controllers":[]`, so the MIDI-learn surface this task called out as
      slot-indexed holds nothing either. **There is nothing to migrate and nothing to reset.** What survives
      is not a decision: Sustain and Release land under different physical encoders, which the operator will
      meet at the hardware. The original text is kept below for the record.
      ~~**BLOCKING GATE, operator decision required, not closable by an implementer.** Whether the
      Envelope renumbering migrates existing saved patches, ships alongside an explicit "patches reset"
      notice, or is judged unnecessary given `proposal.md` §4.1's finding (no silent value-corruption risk
      via the patch-file mechanism specifically — but that finding does not cover the ergonomics/muscle-
      memory consequence of the physical reshuffle, nor any other stateful surface `proposal.md` §4.1
      itself flags as not audited, e.g. MIDI-learn mappings, which ARE slot-indexed per
      `MidiController.cpp`'s `MessageIn` JSON and were not investigated further under this change's
      scope). **Present `proposal.md` §4.1 to the operator as evidence, do not present it as the
      decision.**~~
- [x] **T1.1** `app/dsp/VoiceEnvelope.hpp`, `VcoAdsrState`: add `Stage::Decay` to the `enum class Stage`
      between `Attack` and `Hold`. Per `proposal.md` §4.3, this is a two-part change: (a) Attack's target
      ceiling in `stepVoice` changes from `sustainLevel` to an independent peak (recommend `1.0f`, the
      same ceiling `Vco`'s own output already has — do not invent a new named constant without checking
      whether one is warranted per OMNI §6's 2-of-4 test), and (b) the new Decay stage ramps from that
      peak down to `MapSustain(sustainKnob)`, reusing the same divide-by-mapped-time idiom
      `attackStep`/`releaseStep` already establish. **Session 6 audit adds part (c), without which (a)
      silently changes every existing patch's Attack timing:** `attackStep` is `sustainLevel /
      std::max(mapAttack(attackKnob) * m_sampleRate, 1.0f)` — its numerator is the old ceiling. Change the
      numerator to the new peak along with the ceiling, or the ramp climbs at the old slope toward a higher
      target and the realized attack time becomes `attackTime / sustainLevel`, up to 10x longer at
      `kMinSustainLevel = 0.10f` (`proposal.md` §4.3). `VcoAdsrState::apply`'s signature grows a `decayKnob`
      parameter; update its one call site in `dsp::MixOscVoices` and that function's own call site in
      `FroggersAppCore.hpp`'s `RouteAudioSample`.
- [x] **T1.2** `app/FroggersParameters.hpp`, `FroggersBankLayouts()`: rewrite the Envelope bank's
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
- [x] **T1.3** Short names for Curve (slot 12) and Grace (slot 13): propose 4-char abbreviations (e.g.
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
- [x] **T1.4** Grace's mechanism needs its own design pass at implementation time — this proposal does
      not fully specify it. It needs a per-voice timer that defers the `Attack/Decay/Hold -> Release`
      transition `setGate(false)` currently triggers immediately, until at minimum Attack (and per the
      design doc's own framing, arguably Decay) has completed. Read `VcoAdsrState::setGate` and
      `stepVoice`'s `Stage::Hold` case in full before starting; do not assume the existing per-sample step
      idiom trivially extends to a state-machine edge case it wasn't written to handle.
- [x] **T1.5** Headroom re-check (OMNI §9.1 positive control required): per `proposal.md` §4.3, confirm by
      measurement, not by re-assumption, that the raised Attack ceiling does not push `chainIn` (the
      average of the three gated voices) above what it already reaches today at Sustain=1.0 — the
      per-voice bound is unchanged (still ≤1.0) but this must be measured on the actual three-voice
      average, not asserted from the per-voice argument alone, per the standing rule (`proposal.md` §7).
- [x] **T1.6** Rebuild and re-run the full suite; report new binary/test counts, superseding this file's
      "inherited" line at the top. Any red beyond an intentionally-updated Envelope-shape test is a
      regression.

## T2 — Filter bank: all fourteen slots implementation-ready, COMPLETE (`proposal.md` §9.1)

No blocking gate — unlike Envelope, these are new parameters filling previously-empty slots, not a
reorder of existing occupied ones, so `proposal.md` §4.1's ADDED requirement covers patch safety by
construction (spec delta's "A newly added parameter loads at its ordinary default from an older patch"
scenario). **Session 3: slot 13 (Scoop Depth) added — this bank's task list is now complete, no PENDING
RESEARCH slot remains.**

- [x] **T2.1 — DECIDED as a continuous morph, session 6 (`proposal.md` §3a ruling 12, §9.1).** The operator
      asked whether Topology is a continuous parameter. It was not — `useParallel` is a `bool`, failing §3
      ruling 3, the rule that cut Cycle and Hard Sync and that arrived a session after Topology was tagged
      "Tier 1, already covered." Reading the branch it switched into made it worse: the series path is
      `pureDelay → comb → peak` and nothing else — no `combTrim`, no `peakTrim`, no `peakLimiter`, and
      `combPeakBlend`/`scoopMix` unused, so it would also have disabled Filter slot 7 and all four Scoop
      parameters. **Ruling: build it as a continuous morph — and NOT by running both branches.**
      `Comb::Process` advances a delay line, `ResonantBump::Process` is a `BiquadDf1` with four state terms,
      and `pureDelay`/`combTrimSmoother`/`peakTrimSmoother`/`peakLimiter` are stateful too, so a two-branch
      crossfade needs a duplicate stateful chain — two combs ringing independently, a different instrument.
      **Implement instead by morphing what the peak stage READS**, inside the existing parallel path:
      ```
      peakIn = input * (1 - topo) + combPath * topo     // the one new line
      ```
      with `combPath`, `peakPath`, the Comb/Peak blend and the Scoop blend all unchanged around it (Filter
      bank slot 9, short name `Topo`). `topo = 0` is bit-identical to today; `topo = 1` is series. **Delete
      the `useParallel` argument and the dead series branch** — do not keep them beside the morph. Every unit
      is still processed exactly once per sample.
- [x] **T2.1a — headroom measurement required (OMNI §9.1 positive control), per `proposal.md` §9.1's flag.**
      At high Topology the peak biquad's input is the comb's output rather than the raw input — a new
      operating point for the stage whose ceiling this codebase has already lowered twice (10x → 4x → 2x).
      `peakTrim` and `peakLimiter` remain in force, so this is a narrower question than the deleted branch
      posed, but it is not zero. **Sweep the whole Topology range, not just its ends** — the midpoint mixes
      raw input and comb output into the biquad and is its own case — and report the level actually reached
      alongside the verdict, not just "no clipping." **The re-fit, if one is needed, is bounded: re-sweep
      `peakLimiter`'s threshold and attack/release against the new input distribution. The `1/height` trim is
      algebraic and does not move.**
- [x] **T2.2** `FroggersAppCore.hpp`'s `RouteAudioSample`: stop feeding `filterChain_.scoopNotch.SetFreq`/
      `SetWidth` the same `bumpFreq`/`bumpWidth` locals `peak` uses; feed them independent values read
      from Filter bank slots 10 (`ScFq`) and 11 (`ScWd`).
- [x] **T2.3** `app/dsp/FilterFx.hpp`'s comb branch: add a pre-gain **on the ARGUMENT of** the existing
      `feedback * PadeSaturator::Saturate(filter.Process(tapped))` call — i.e.
      `Saturate(combDrive * filter.Process(tapped))`, not `combDrive * feedback * Saturate(...)` (Filter bank
      slot 12, `CDrv`). **The placement is load-bearing, not stylistic** (`proposal.md` §7a): inside the
      clamp the loop's per-sample bound `|input| + |fb|` is unchanged at any drive setting and
      `rawCombTrim` stays exactly right; outside it the bound becomes `|input| + g*|fb|` and the trim must
      become `1/(1 + g*|fb|)`. Default: unity gain (T2.6).
- [x] **T2.4 — WITHDRAWN, session 6: this task existed to re-derive a bound that a pre-gain does not move.**
      It read: *"`rawCombTrim = 1.0f / (1.0f + std::fabs(comb.feedback))` was measured against `|comb| <= A +
      |fb|` at unity input gain. T2.3's pre-gain invalidates that bound."* **It does not.**
      `PadeSaturator::Saturate` ends in `std::max(-1.0f, std::min(1.0f, output))`, so the fed-back term is
      clamped to ±1 BEFORE the multiply by `feedback`, and `|output| <= |input| + |fb|` holds at any drive
      (`proposal.md` §7a, §9.1). This is the same argument T4.1 already used to withdraw the identical flag on
      Delay Feedback Drive over the identical construct — **keeping both was a contradiction, and it survived
      three sessions because the argument was restated per site instead of stated once.** §7a now states it
      once, for all three saturator pre-gains. What remains is not a headroom item: heavier drive pins the
      saturator nearer ±1 more often, raising typical level toward a worst case that does not move — audible,
      not unsafe.
- [x] **T2.5 — session 3.** `FroggersAppCore.hpp`'s `RouteAudioSample`: stop feeding
      `knob(FroggersBankId::Filter, 8)` to both `filterChain_.scoopNotch.SetHeight` and
      `FilterFxChain::Process`'s `scoopMix` argument; feed `SetHeight` its existing slot-8 read unchanged
      and feed `scoopMix` an independent value read from Filter bank slot 13 (`ScDp`). No headroom
      re-derivation task follows — `proposal.md` §9.1 confirms `SetHeight`'s dip and `scoopMix`'s convex
      blend are each independently bounded regardless of how the two are decoupled.
- [x] **T2.6** `app/FroggersParameters.hpp`: register the five new Filter parameters at slots 9-13 with
      the short names above, following T1.2's pattern for escaping `kFroggersParamsPerBank`'s uniformity.
      **Defaults, so the instrument's own startup voice is unchanged** (`proposal.md` §3a ruling 11): Topology
      → 0 (parallel, bit-identical to today); Comb Drive → unity pre-gain; Scoop Depth → the same default
      slot 8 carries. Scoop Freq/Width have no literal to match — they track `bumpFreq`/`bumpWidth` at
      runtime today — so default them to what the Peak knobs' own defaults produce.
- [x] **T2.7** Rebuild and re-run the full suite; report new binary/test counts.

## T3 — Drive bank: all fourteen slots implementation-ready, COMPLETE (`proposal.md` §9.2)

**Session 3: slots 10-13 (Link, Fold, Tone, Bias) added — this bank's task list is now complete, no
PENDING RESEARCH slot remains.**

- [x] **T3.1** `app/dsp/Drive.hpp`'s `Oversampler2x`: replace the hardcoded `antiAlias.SetAlphaFromNatFreq
      (0.4f)` constructor call with a knob-driven `SetAlphaFromNatFreq` call (Drive bank slot 9, short
      name `ABrt`). No headroom flag — a lowpass cutoff change cannot raise peak amplitude.
- [x] **T3.2 — session 3.** `app/dsp/Drive.hpp`'s `PolynomialDrive::SetCoefs`: replace the hardcoded
      `0.25f` weight in both `coefs[1]`'s and `coefs[3]`'s `0.25f * (computedGain - 1.0f)` term with a
      knob-driven scalar (Drive bank slot 10, short name `Link`). **Session 6 audit rewrote why no headroom
      task follows, because the original reason was invalid** (bounded coefficients do not bound an unbounded
      5th-order polynomial — this change's own Bias entry says so): Link unlocks the gain/coefficient-phase
      pairing that `0.25f * (computedGain - 1.0f)` locks today, so maximum gain can meet coefficient phases
      that today occur only at intermediate gains. **Measured, not argued:** worst-case `|Process|` over
      `|input| <= 1` is 200.5 today vs 212.1 with the pairing free, +0.5 dB (`proposal.md` §9.2). That number,
      not the coefficient bound, is why this slot is unflagged. `Process(0) == 0` at every Link setting still
      holds — every term carries a positive power of `input`. **Default parity: the knob's default SHALL
      resolve to `0.25` — one `defaultValue` line, recorded at T3.6.**
- [x] **T3.3 — session 3.** `app/dsp/Drive.hpp`'s `FrogBlock::Process`: replace the hardcoded `sinIn = out
      / 4.0f` divisor with a knob-mapped value (Drive bank slot 11, short name `Fold`). No headroom
      re-derivation task follows — `proposal.md` §9.2 confirms `Sine01`'s output stays bounded to `[-1, 1]`
      for any divisor and `Process(0) == 0` at every Fold setting. **Session 6 audit — the mapping SHALL keep
      the divisor strictly positive.** At divisor `0`, `out / 0` is `±inf`, `Sine01`'s own `phase -
      std::floor(phase)` turns that into `NaN`, and this codebase has already been silenced permanently once
      by a `NaN` reaching `SanitizeOutputSample` (`FroggersAppCore.hpp`'s `scoopNotch` comment). An
      `ExpMapCompute(min, max, knob)` range does this by construction; a linear map through zero does not.
      **Default parity: the knob's default SHALL resolve to `4.0` — one `defaultValue` line, recorded at
      T3.6.**
- [x] **T3.4 — session 3.** `app/dsp/Drive.hpp`: add a post-chain `dsp::OnePoleLowPass` stage after
      `FrogBlock`'s existing output (Drive bank slot 12, short name `Tone`). No headroom flag — a lowpass
      only removes energy.
- [x] **T3.5 — session 3, headroom re-derivation required (OMNI §9.1 positive control), per
      `proposal.md` §9.2's flag.** `app/dsp/Drive.hpp`'s `PolynomialDrive`: apply a knob-driven DC offset
      before `Process` and subtract `Process(bias)` after — `PolynomialDrive::Process(input + bias) -
      PolynomialDrive::Process(bias)` — the same subtract-after-shift construction
      `DigitalReorganizer::Process` already uses, generalized to `PolynomialDrive` (Drive bank slot 13,
      short name `Bias`). `f(0) = 0` is closed by construction; MEASURE the peak-swing residual across the
      whole Bias sweep before shipping — `PolynomialDrive::Process` is an unbounded 5th-order polynomial,
      so the DC-cancellation alone does not bound peak swing identically at every setting. **Two re-fits are
      acceptable outcomes, recorded so this reads as scheduled work rather than open-ended risk
      (`proposal.md` §9.6): either measure max `|Process|` across the Bias sweep and apply a compensating
      output trim, the way the comb branch already carries one, or bound the knob's range so peak swing stays
      inside today's maximum.** The second needs no new state; prefer it if the measurement supports it.
- [x] **T3.6** `app/FroggersParameters.hpp`: register the five new Drive parameters at slots 9-13.
      **Defaults reproducing today's literals:** Anti-Alias Brightness → `0.4f` natFreq; Link → `0.25f`;
      Fold → divisor `4.0f`; Tone → fully open (a lowpass that removes nothing); Bias → 0.
- [x] **T3.7** Rebuild and re-run the full suite; report new binary/test counts.

## T4 — Delay bank: all fourteen slots implementation-ready, COMPLETE (`proposal.md` §9.3)

**Session 3: slots 10, 12, 13 (Feedback Tone, Width Balance, Crush) added — this bank's task list is now
complete, no PENDING RESEARCH slot remains.**

- [x] **T4.1** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: add a pre-gain **on the ARGUMENT of** the
      existing `WriteSample(inSignal + fbk * PadeSaturator::Saturate(fbL), lineL)` / `fbR`/`lineR` lines —
      `Saturate(fbDrive * fbL)` (Delay bank slot 9, short name `FbDr`). Placement per `proposal.md` §7a, the
      same rule T2.3 and T5.2 follow. **No headroom re-derivation task follows this one** — `proposal.md`
      §9.3 records why the round-1 flag on this exact candidate was withdrawn as wrong: `Saturate` clamps
      to ±1 unconditionally before the addition, so a pre-gain into it cannot raise the per-sample bound
      the existing comment already proves. Do not re-open this as a headroom item without new evidence.
- [x] **T4.1a — session 3.** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: insert a `dsp::OnePoleLowPass`
      into the feedback tap ahead of the existing `PadeSaturator::Saturate` call, damping `fbL`/`fbR`
      before `WriteSample` (Delay bank slot 10, short name `FbTn`). No headroom flag — a lowpass inside a
      feedback loop only removes recirculating energy.
- [x] **T4.2** `app/dsp/Delay.hpp`: replace the hardcoded `lfoInc = 2.0f * 3.14159265f * 0.25f /
      sampleRate` with a knob-driven rate (Delay bank slot 11, short name `MdRt`).
- [x] **T4.2a — session 3.** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: replace the two hardcoded
      weights `const float widthSpread = p.dwid * baseSeconds * 0.35f;` and `const float cross = p.dwid *
      0.5f;` with a complementary pair of knob-derived weights, both still driven by `p.dwid` (Delay bank
      slot 12, short name `WBal`). No headroom re-derivation task follows — `proposal.md` §9.3 confirms
      `fbL`/`fbR` stay a convex combination bounded by `max(|dL|,|dR|)` at any split, and `widthSpread`
      only offsets a `ReadAt` tap position, never adds gain. **Session 6 audit — two bounds are now binding
      spec lines, not prose, because the code that guarantees them today is the code being replaced:** (a) the
      cross-feed weight SHALL stay in `[0, 1]` (today `p.dwid * 0.5f` cannot exceed 0.5, which is the only
      reason the convexity holds), and (b) the spread SHALL NOT lengthen a read tap past `capacity`. On (b),
      note while the file is open (OMNI §16.2) that `timeR = max(0.001, baseSeconds + modSeconds +
      widthSpread)` can ALREADY exceed `capacity` today at maximum time and width (2.0 s + 0.7 s vs a 2.0 s
      buffer, `Delay.hpp:276,309,316`), after which `ReadAt` wraps modulo capacity — **pre-existing, not this
      change's to fix, but do not widen its reach.** **Default parity: the knob's default SHALL reproduce
      today's `0.35 : 0.5` ratio — one `defaultValue` line, recorded at T4.3.**
- [x] **T4.2b — session 3.** `app/dsp/Delay.hpp`'s `StereoDelay::Process`: route the feedback tap's
      repeats through `dsp::SampleRateReducer` and/or `dsp::DigitalReorganizer` (`app/dsp/Drive.hpp`),
      reused as-is — both already ship the `f(0) = 0` DC-block fix, so no hand-rolled copy is needed
      (Delay bank slot 13, short name `Crsh`). No headroom flag — these structs' outputs never exceed
      their input's magnitude.
- [x] **T4.3** `app/FroggersParameters.hpp`: register the five new Delay parameters at slots 9-13.
      **Defaults reproducing today's literals:** Feedback Drive → unity pre-gain; Feedback Tone → fully open;
      Mod Rate → 0.25 Hz; Width Balance → today's `0.35 : 0.5` ratio; Crush → off.
- [x] **T4.4** Rebuild and re-run the full suite; report new binary/test counts.

## T5 — Reverb bank: all of slots 9-13, COMPLETE, session 2 (`proposal.md` §9.4)

- [x] **T5.1** `app/dsp/Reverb.hpp`: replace the hardcoded `kModLfoHz = 0.35f` feeding `modLfoPhase`'s
      increment with a knob-driven rate (Reverb bank slot 9, short name `MdRt`).
- [x] **T5.2** `app/dsp/Reverb.hpp`: add a pre-gain **on the ARGUMENT of** the existing `aIn = preOut + fb *
      PadeSaturator::Saturate(aFb)` / `bIn` lines — `Saturate(tankDrive * aFb)` (Reverb bank slot 10, `TkDv`).
      Placement per `proposal.md` §7a; bounded by the same clamp argument, no re-derivation task follows.
- [x] **T5.3** `app/dsp/Reverb.hpp`: route the tank feedback (`aFb`/`bFb`) through
      `dsp::DigitalReorganizer::Process` ahead of the existing `PadeSaturator::Saturate` call (Reverb bank
      slot 11, `Grit`). Reuse `DigitalReorganizer::Process`/`Mangle` as-is (`app/dsp/Drive.hpp`) — do not
      hand-roll a copy of the bit-scramble formula, which would reintroduce the `f(0) != 0` defect that
      formula's own DC-blocked construction already fixed once.
- [x] **T5.4** `app/dsp/Reverb.hpp`: add a bipolar tone-shave stage (two `dsp::OnePoleLowPass` instances,
      one direct lowpass tap and one complementary highpass via `input - lowpass(input)`, crossfaded
      around the knob's center) applied to `wet`/`mixedOut` before the existing `wetLimiter.Process()`
      call (Reverb bank slot 12, `Tilt`).
- [x] **T5.5 — headroom re-derivation required (OMNI §9.1 positive control), per `proposal.md` §9.4's own
      carried-forward flag.** `kReverbWetLimiterThreshold` (0.72) and its attack/release were measured
      against the un-tilted spectral content. Re-sweep with Tilt at its brightest setting before shipping;
      do not assume the existing limiter margin covers it unmeasured.
- [x] **T5.6** `app/dsp/Reverb.hpp`: drive `dA`/`dB` (currently `180 + sizeNorm*1300` / `260 +
      sizeNorm*1800`, modified by `applyMod`) directly from this parameter's own resolved value, per §3
      ruling 5's re-scoping — no pitch tracker (Reverb bank slot 13, `Tund`).
- [x] **T5.7 — MEASURE, do not assume safe, per `proposal.md` §9.4's own carried-forward caution.**
      Modulating Tuned at audio rate sweeps a delay length inside a feedback loop. Confirm by measurement
      that this stays bounded under the existing in-loop saturator and wet limiter before shipping audio-
      rate modulation depth on this target; the existing Mod-depth precedent on the same `dA`/`dB` pair is
      not itself a substitute for measuring this new use.
- [x] **T5.8** `app/FroggersParameters.hpp`: register the five new Reverb parameters at slots 9-13,
      completing the bank. **Defaults reproducing today's literals:** Mod Rate → `kModLfoHz = 0.35f`; Tank
      Drive → unity pre-gain; Grit → off; Tilt → centre (no shave); Tuned → whatever `dA`/`dB` produce at
      today's Room-size behaviour.
- [x] **T5.9** Rebuild and re-run the full suite; report new binary/test counts.

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
- **Session 6 audit — two more over-length labels exist on the same rendering path and are outside this
  count's scope, not missed by it: `Crispy` and `Crnchy` (6 characters each), registered at slots 14 and 15
  of every bank (`FroggersParameters.hpp`).** They go through the same `UpperShortLabel` `maxChars = 4` path,
  so they are truncated today too. The label-site total is 25, not 23. T6.1's rendering change covers them
  automatically if it is applied per-cell (T6.3 already requires uniformity); what needs a decision is
  whether their TEXT is also shortened. Recorded so "23" is not read as the total.
  **Session 7 audit — nothing here needs an operator decision, and the labels keep their words.** T6.1's
  cell treatment is buildable app-side (see T6.1's own mechanism note), so it covers `Crispy`/`Crnchy`
  along with the other 23 and no `shortName` string has to be shortened to fit. An earlier pass this
  session wrote this up as a blocking gate on the belief that T6 needed a Sheaf change; that was wrong
  (`proposal.md` §10, §1b finding 9) and the gate is deleted rather than left standing.

- [x] **T6.1** Design the larger, "slightly larger colored boxes with smaller text" cell treatment for the
      23 over-length labels — the operator's own words, verbatim: *"they can just be slightly larger
      colored boxes with smaller text."* **Mechanism, traced session 7 so this is not re-derived
      (`proposal.md` §10) — it is app-side and `External/Sheaf` stays untouched:** Sheaf's
      `BuildEncoderDrawCommands` hard-codes `UpperShortLabel(state.shortLabel)` at `maxChars = 4` and sizes
      the display from `baseRadius`, and `EncoderDrawState` has no field to change either. But it returns
      its `std::vector<DrawCommand>` **by value** into this app's own draw lambda
      (`FroggersUiSurface.hpp`'s single `BuildEncoderDrawCommands(state, extent)` call site), the label
      block is the LAST thing it appends, and **`BuildFourteenSegmentCommands` is public and inline with
      `numChars` as an ordinary parameter** (`numChars <= 0` auto-sizes to the text length). So: bind the
      returned vector, drop or cover the trailing 4-character block, and append an opaque
      `DrawCommand::Kind::FillRoundedRect` plus
      `BuildFourteenSegmentCommands(fullLabel, largerBounds, onColor, offColor, /*numChars=*/0)`.
      **Two things to expect:** the replacement re-derives its own display bounds instead of reading
      Sheaf's, and covering the original depends on it being emitted last — not part of Sheaf's contract,
      so T6.2's visual check is the guard if that ever changes. This cell also renders the modulation badge chips
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
- [x] **T6.3** Apply the same cell treatment to the Envelope and Audio banks' cells too (0 over-length
      labels today, but T1/T2/T3/T4/T5/T7 above add new parameters to Envelope, Filter, Drive, Delay,
      Reverb, and Audio — confirm no newly-added `shortName` from those tasks exceeds 4 characters, since
      they are chosen to comply from the start, but the *rendering* change T6.1 makes should be uniform
      across all six banks' cells, not just the four with pre-existing violations).
- [x] **T6.4** Rebuild and re-run the full suite; a rendering-only change should not change any DSP
      test's outcome — any red is a regression, not an expected side effect.

## T7 — Audio bank: slots 12-13 implementation-ready, session 3, T7.2 tightened session 4 (`proposal.md` §9.5)

**Ring Mod (Audio slots 9-11) is covered by its own task block, T8, below — not this one** (PM Rate and
VCO Balance are a separate pair of slots from Ring Mod's three). Through session 4 this header claimed
Ring Mod was "deliberately NOT covered by any task in this block or anywhere else in this file," blocked
on two open sub-questions plus a `froggers-vco-topology` spec tension — **that framing was WRONG and is
corrected at T8 and `proposal.md` §4.2, session 5; it is no longer true.** **Session 6 audit: T7.1 (PM Rate)
IS now gated, by T7.0 — a real collision with `froggers-vco-topology`, found by reading the requirements
sessions 3-5 never opened. T7.2-T7.4 (VCO Balance) remain ungated. That gate is since ANSWERED (ruling 10):
T7.1 implements against the relaxed delta, and nothing in this block is gated any more.** There are no saved
patches to preserve (ruling 11); PM Rate's own default still has to reproduce today's rate so the instrument's
fresh-launch voice is unchanged, which is T7.3's `defaultValue` line. **Session 4: T7.2 is rewritten from a headroom-measurement task into an invariant-assertion task
(T7.2, T7.2a, T7.2b), per the operator's new floor/cap ruling on VCO Balance — see below.**

- [x] **T7.0 — ANSWERED by the operator, session 6 (*"A:1"*), `proposal.md` §3a ruling 10.** PM Rate
      collides with two in-force clauses of `openspec/specs/froggers-vco-topology/spec.md` (the PM LFO's
      frequency being "an exponential function of the PM knob"; "Phase modulation is self-contained", which a
      single rate knob across three VCOs breaks). **Ruling: PM Rate keeps Audio slot 12 and the spec is
      relaxed to match** — the delta at `specs/froggers-vco-topology/spec.md` stands as written, and T7.1
      below implements against that delta, not against the pre-change spec. The accepted trade, recorded so a
      later session does not read it as an oversight: one PM Rate knob moves all three VCOs' LFO rates
      together.
- [x] **T7.1** `app/dsp/Vco.hpp`'s `StepPmLfo`: decouple the rate calculation
      (`ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmKnob01)`) from `pmKnob01`, feeding it a second,
      knob-driven rate value shared across all three VCOs' calls instead (Audio bank slot 12, short name
      `PMrt`). Per `proposal.md` §9.5, this ships as ONE knob shared across VCO1-3's `StepPmLfo` calls, not
      three independent per-VCO rates — only one slot is available. No headroom flag — phase-domain only,
      `EvalWaveMorph` always evaluates a wrapped phase to `[-1, 1]` regardless of what drives the offset.
- [x] **T7.2 — session 4: invariant, not a measured headroom item. `proposal.md` §4.4/§9.5/§9.6.**
      `app/dsp/VoiceEnvelope.hpp`'s `MixOscVoices`: replace the fixed `(v1 + v2 + v3) * (1.0f / 3.0f)`
      return with a normalized 3-point crossfade driven by one knob (Audio bank slot 13, short name
      `VBal`). **Binding requirement, operator ruling:** the three weights SHALL sum to exactly 1 at every
      knob position, AND each weight SHALL stay in `[0.10, 0.80]` — no knob position may reduce any VCO to
      0% or raise any VCO to 100% of the mix; three VCOs with a 10% floor each cap any single VCO at 80%
      (`1 - 0.10 - 0.10`). This is the same class of constraint as `dsp::VcoAdsrState::kMinSustainLevel`
      (§4.3/§4.4) — a knob must not be able to gate a signal path to zero.
- [x] **T7.2a — session 4, test SHALL assert the invariant, not measure output level.** Write a test that
      sweeps the VCO Balance knob across its full `[0, 1]` range and asserts, at each sampled position:
      (a) the three weights sum to exactly 1 (within float tolerance), and (b) each weight stays within
      `[0.10, 0.80]`. Do NOT write this as a `chainIn`-level measurement — the floor-and-cap invariant
      makes the bound provable from the weight mapping alone (`proposal.md` §4.4 derives the convexity
      argument in full); measuring output samples to indirectly infer the weights would be strictly weaker
      evidence than asserting the weights themselves.
- [x] **T7.2b — session 4, positive control required (OMNI §9.1).** Before trusting T7.2a's test, prove
      the rig can actually observe a violation: temporarily break the invariant (e.g. let one weight reach
      0 or 1, or let the sum drift off 1) and confirm the test in T7.2a fails. A test that asserts `sum(w)
      == 1` but never varies the knob, or never exercises the extremes, would pass vacuously — this task
      exists so that risk is checked once, not assumed away.
- [x] **T7.3** `app/FroggersParameters.hpp`: register the two new Audio parameters at slots 12-13.
      **Defaults:** PM Rate → the rate today's `ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmKnob01)` produces at
      the PM knob's own default; VCO Balance → centre, i.e. the equal-thirds mix it replaces.
- [x] **T7.4** Rebuild and re-run the full suite; report new binary/test counts.

## T8 — Audio bank: Ring Mod, slots 9-11, session 5 (`proposal.md` §3 ruling 1, §4.2, §9.5)

**Corrects the prior framing in full.** Through session 4, this bank-slot item was wrongly recorded as
blocked on an open carrier choice, a pre-gate/post-gate sub-question, and a `froggers-vco-topology`
collision — none of that was real (`proposal.md` §4.2). Ring Mod is an ordinary parameter: each VCO has
its own ring modulator with an internal carrier, and the Ring Mod knob's range is that carrier's
frequency, mapped across audio rate the same way `Vco::PitchToPhaseIncrement` maps pitch. **Session 6 audit:
that correction stands — but "no blocking gate on the tasks below," as this header read through session 5, is
now false. T8.0 gates T8.1-T8.3: the corrected design has no off position and no depth control, which is a
design question for the operator, not an implementation detail** (`proposal.md` §4.2, §9.5). **That question
is since ANSWERED (ruling 9) — a true zero position shared with PM as one function — so T8.1-T8.3 are
ungated, with T8.0a-T8.0c ahead of them as ordinary tasks.** There are no saved patches for shipping this to
disturb (ruling 11); what the zero position buys is that the instrument at its own defaults still sounds
unmodulated, which is T8.3's `defaultValue` line.

- [x] **T8.0 — ANSWERED by the operator, session 6; was posed as a question, not a standing gate.** As
      specified through session 5, Ring Mod had no knob position at any frequency where a VCO passed through
      unchanged (a product has no unity position; at sub-audio it is a slow full-depth tremolo through zero,
      not a clean signal — which is also why session 5's "sub-audio low end" item, formerly T8.4, is
      withdrawn as false-premised). **Ruling: a true zero position at the bottom of Ring Mod's own knob,
      implemented as ONE function shared with PM** (`proposal.md` §3a ruling 9, §4.5). Costs no slots, keeps
      the knob continuous under `proposal.md` §3 ruling 3, and gives default parity for free. T8.0a-T8.0c
      below implement it; they are ordinary tasks, not gates.
- [x] **T8.0a — the shared function, written ONCE, before T8.1 uses it.** Add the knob→depth ramp to
      `app/dsp/DspMath.hpp`, beside `ExpMapCompute`/`ZeroedExpCompute`/`WrapPhase`/`Sine01` (this app's
      existing home for shared scalar primitives — no new file, no new concept location). Signature takes the
      knob value, the control's own floor, and its own ramp width; body is `Vco::PmDepthScale`'s existing
      floor / smoothstep `t*t*(3-2t)` / ceiling shape, moved rather than re-derived (`app/dsp/Vco.hpp:137-150`).
      **§6 2-of-4 is satisfied and recorded at `proposal.md` §4.5 — do not re-litigate it, and do not add a
      second copy for Ring Mod.**
- [x] **T8.0b — `Vco::PmDepthScale` becomes a call into it, passing `kPmLfoFloor`/`kPmLfoRampWidth`.** Keep
      the existing public static as the named domain concept (its call sites and its parity tests refer to
      it); only its body changes. **PM's behaviour must be bit-identical after this** — the existing parity
      tests at `app/FroggersDspParityTests.cpp:96-100` already assert the floor, the ramp top, and the
      midpoint, so run them as the regression, and add the midpoint-equality assertion against the shared
      function directly if they do not already cover it.
- [x] **T8.0c — positive control (OMNI §9.1) for the shared function itself.** A test that only checks
      `f(floor) == 0` and `f(floor + width) == 1` passes for a function that returns a step. Sweep the knob
      across `[0, 1]` and assert the ramp is monotonic and strictly between 0 and 1 strictly inside the ramp
      window, then perturb the smoothstep to a step and confirm the test fails. Without that, "PM is
      unchanged" is asserted by an instrument that cannot see the change.
- [x] **T8.1** `app/dsp/Vco.hpp` (or a new small per-VCO ring-modulator helper alongside it): add an
      internal carrier oscillator per VCO, phase-stepped the same way `Vco::Process` already steps its own
      carrier (`carrierPhase = WrapPhase(carrierPhase + phaseIncrement)`), with its own frequency driven by
      the Ring Mod knob through the same exponential map `PitchToPhaseIncrement` already uses
      (`ExpMapCompute(min, max, ringModKnob01)`). Do not reuse `PitchToPhaseIncrement`'s own `20.0f`/
      `20000.0f` literals without checking whether they are the right bounds for Ring Mod's own range.
      **The low bound is now a by-ear taste question with nothing riding on it (T8.4), NOT the open
      "sub-audio floor vs. audio-rate-only" design question this task cited through session 6** — that
      framing rested on a sub-audio carrier being an effectively-clean position, which is false, and "off"
      is now the shared zero gate (T8.0a), not a frequency (`proposal.md` §4.2, §8, §9.5).
- [x] **T8.2** `FroggersAppCore.hpp`'s `RouteAudioSample` (or wherever the three VCOs' audio is combined):
      blend each VCO's own signal toward that signal multiplied by its own internal carrier, by the amount
      T8.0a's shared ramp returns for that VCO's Ring Mod knob — so at or below the floor the VCO passes
      through untouched and the multiply's effect ramps in smoothly above it (Audio bank slots 9-11, short
      names `RM1`/`RM2`/`RM3`). The blend keeps the `[-1, 1]` bound: it is a convex mix of two signals each
      already bounded to `[-1, 1]`. Whether the multiply reads the
      pre-gate (`vcoOut[i]`) or post-gate (`gatedVoices.v1/v2/v3`) signal is an ordinary implementation
      choice, not a design decision this change makes — either satisfies the spec, since both are the SAME
      VCO's own signal either way (`proposal.md` §4.2). No headroom re-derivation task follows — §9.5/§9.6
      confirm the product of two `[-1, 1]`-bounded signals is itself bounded to `[-1, 1]`.
- [x] **T8.3** `app/FroggersParameters.hpp`: register the three new Ring Mod parameters at Audio bank
      slots 9-11, following T7.3's pattern. **Defaults: at or below the zero floor** (T8.0), so the
      instrument starts unmodulated exactly as it does today.
- [ ] **T8.4** By-ear settle the knob's low end, **downstream of T8.0's answer, not independent of it —
      session 6 audit rewrote this task.** Its original wording ("sub-audio, an effectively-clean position")
      was wrong: a sub-audio carrier is a slow full-depth tremolo, not a clean position (`proposal.md` §4.2).
      Once T8.0 settles what makes Ring Mod inaudible, pick the frequency range by ear against that.
- [x] **T8.5** Rebuild and re-run the full suite; report new binary/test counts.

## Recorded, not scheduled — no task closes these

- **Adopting Sheaf's `ParameterManager::Get*` mapping family in place of the app's own `ExpMapCompute` calls
  — CONSIDERED AND REJECTED, session 6. Recorded so it is not re-proposed as an §8 finding, because it was
  first written up as one (a task block T10) and that was wrong.** The observation is true: Sheaf exports six
  per-parameter-type mapping functions (`GetLinear`, `GetExponential`, `GetZeroBasedExponential` and bipolar
  variants, `ParameterModulation.hpp:805-811`) that this app calls zero times, and `ExponentialMap` is
  `minValue * std::pow(maxValue / minValue, normalized)` — the same line as `dsp::ExpMapCompute`
  (`DspMath.hpp:43-46`). **It fails §8's own test anyway.** §8 asks whether the same logic is WRITTEN more
  than once; in this app it is written once and called six times (all in `RouteAudioSample`'s Filter
  routing), which is the abstraction §8 asks for. §1's stricter test — reuse is only real when the runtime
  call graph is single-sourced — also passes. The duplicate copy lives in a **pinned, unpatchable dependency**
  (§0), so it is not this app's duplication to eliminate, and eliminating it from the consumer's side would
  put an out-of-line call that does a `ParameterById` lookup and **throws `std::invalid_argument` on a
  non-positive endpoint** into a per-sample audio path (`FilterFx.hpp`'s own comment: refreshed "from
  `RouteAudioSample()` once per sample") — a worse structure bought for the deletion of a four-line helper.
  **OMNI §1's corollary applies to the lead's own findings too: when honouring a rule forces a worse
  structure, the reading of the rule is suspect, not the structure.**

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
"blocker" framing was wrong, not resolved. **Zero bank-slot items remain on this list. Session 6's audit
raised three questions that were not slot decisions, and the operator raised a fourth; all four were
answered in the same session (T7.0, T8.0, patch compatibility, and Filter Topology — `proposal.md` §3a
rulings 9-12), so they are recorded as closed below, not carried.** What remains here
is open for reasons other than "the research hadn't been written" or "the framing was wrong" — **do not
invent a fill or a decision for any of these:**

- **~~How Ring Mod is turned off.~~ ANSWERED — `proposal.md` §3a ruling 9, task T8.0.** Recorded as closed
  rather than deleted so a later session does not re-open it. Session 5's "whether the Ring Mod knob's range
  reaches sub-audio" stays withdrawn — false premise, superseded by ruling 9.
- **~~Whether `froggers-vco-topology` may be relaxed for PM Rate; whether default parity is required.~~ Both
  ANSWERED, session 6 — `proposal.md` §3a rulings 10-11 (T7.0).** Ruling 11 closed T1.0 as moot and deleted
  the default-parity task block that had been written for it: choosing each new parameter's `defaultValue`
  is one line in the registration tasks (T2.6/T3.6/T4.3/T5.8/T7.3/T8.3), not a work package.
- **~~Is Filter Topology a continuous parameter?~~ ANSWERED — `proposal.md` §3a ruling 12: it was not, and it
  is now built as a continuous morph with the dead series branch deleted (T2.1, T2.1a).**
- **~~The three behaviour changes default parity cannot absorb~~ (Scoop Freq/Width stop tracking the Peak
  knobs; Scoop Depth narrows slot 8) — NOT open, and no longer riding with T1.0, which is itself moot.**
  With zero saved patches (ruling 11) each is just a number to pick, recorded at T2.6's defaults.
- **~~How the over-length short labels get fixed; whether `Crispy`/`Crnchy` get shortened text.~~ NOT
  OPEN — session 7.** T6.1 is buildable app-side with Sheaf untouched, so every label keeps its words and
  renders in full; nothing needs deciding (T6.1's mechanism note, `proposal.md` §10). **A pass earlier in
  the same session put a blocking gate here on a wrong reading and it is deleted, not softened** — kept as
  a struck line so it is not re-raised (`proposal.md` §1b finding 9).
- **Whether `kPmLfoDepth = 0.15` is the right maximum PM depth — session 3, an open by-ear tuning item,
  NOT a parameter** (`proposal.md` §3 ruling 8, §8). If too shallow by ear, fix the constant directly; no
  task in this file schedules that tuning pass.
- The design doc's open question 8 — ASR envelopes cannot modulate anything; a modulation-slate question,
  not a bank-slot question, that may outrank everything in this document. Untouched by sessions 2-6. **This
  is the only genuinely open item left on this list.**
- ~~Whether the Envelope renumbering migrates or resets saved patches (T1.0 above).~~ **MOOT — ruling 11,
  verified against an empty patch directory and an empty MIDI-controller list. Kept struck rather than
  deleted so a later session does not re-open it; this bullet contradicted the section's own intro from
  session 6 until it was corrected here.**

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

## §VERIFY — `openspec validate --all --strict`, session 6 (the §14 preflight audit)

Run 2026-08-12 after all session-6 edits to `proposal.md`, `tasks.md`,
`specs/froggers-sheaf-parameter-model/spec.md`, and the **new** `specs/froggers-vco-topology/spec.md`.
Results: `openspec validate frogg3rs-bank-expansion --strict` → **`Change 'frogg3rs-bank-expansion' is
valid`**; `openspec validate --all --strict` → **Totals: 66 passed, 0 failed (66 items)**.

**Why the total is still 66 even though a file was added, stated explicitly rather than left to look like a
no-op:** the item count is per spec and per change, and the new file is a second spec DELTA inside this one
existing change, not a new spec and not a new change — so it adds zero items while changing what this change
validates against. The delta's own two MODIFIED requirement headers were checked to match the live spec's
headers verbatim (`Froggers oscillator topology is preserved`, `No hardcoded cross-VCO coupling`), since a
MODIFIED requirement whose header does not match is how a delta silently becomes an addition. No soft-wrap
trap: both new requirement bodies, and the parameter-model delta's new `A newly exposed hardcoded value
defaults to the value it replaces` requirement, carry their `SHALL` on the requirement's first source line.

**Not verified by this session, unchanged from every prior session: nothing was built or run.** The suite
counts at the top of this file remain inherited, and the one measurement session 6 performed (Link's
worst-case `|Process|`, `proposal.md` §9.2) was a standalone numeric evaluation of the polynomial's own
formula, not a build of this tree.

**Final re-run, session 6 — after all four operator rulings (9-12), after items 10-11 were corrected from
phantom rulings back to questions and then answered for real, and after the saturator pre-gain fix (§7a;
T2.3/T2.4/T4.1/T5.2; the three spec `THEN` lines that now pin the gain to the saturator's INPUT):**
`openspec validate frogg3rs-bank-expansion --strict` → **valid**; `--all --strict` → **66 passed, 0 failed
(66 items)**, item count unchanged because this session's one new file is a second spec DELTA inside the same
change, not a new spec or a new change.

**Earlier re-run this session, kept for the record — after the first three rulings landed and the Filter Topology finding was
recorded** (the same three files plus
`specs/froggers-vco-topology/spec.md`, which gained a third MODIFIED requirement): `openspec validate
frogg3rs-bank-expansion --strict` → **valid**; `--all --strict` → **66 passed, 0 failed (66 items)**, count
unchanged for the same reason as above. **Ruling 9's spec home was chosen to avoid an orphan:** it broadens
the live requirement `Phase modulation has a true zero position` rather than adding a differently-named one,
since a renamed requirement in a delta becomes an ADDITION and would have left the original standing beside
it saying something narrower.

## §VERIFY — `openspec validate --all --strict`, session 7 (the artifact audit)

Run 2026-08-12 after all session-7 edits to `proposal.md`, `tasks.md`, and
`specs/froggers-vco-topology/spec.md` (`specs/froggers-sheaf-parameter-model/spec.md` was not touched this
session — no requirement, scenario or `THEN` line changed in it). Results:
`openspec validate frogg3rs-bank-expansion --strict` → **`Change 'frogg3rs-bank-expansion' is valid`**;
`openspec validate --all --strict` → **Totals: 66 passed, 0 failed (66 items)** — unchanged from sessions
2-6, as expected: this session added no file, no requirement and no scenario. Every edit was to narrative
prose or to a task's own body, never to a `### Requirement:` header or a `#### Scenario:` block, so no
soft-wrap `SHALL`-placement re-check was needed.

**Two claims this session re-verified against the tree rather than carrying forward** (OMNI §1 — a repeated
claim is the repeater's claim), both recorded at `proposal.md` §1b: `PadeSaturator::Saturate`'s terminal
`std::max(-1.0f, std::min(1.0f, output))` plus all three of §7a's in-loop call sites, and Link's worst-case
`|Process|` measurement, recomputed independently from `SetCoefs`/`Process` — 200.45 vs 212.06, +0.49 dB,
matching §9.2's recorded 200.5 / 212.1 / +0.5 dB. **Nothing was built or run;** the suite counts at the top
of this file remain inherited, unchanged since the archived predecessor change.

**Later passes this session, both re-validated unchanged — valid, 66 passed, 0 failed (66 items).** The
first added a blocking gate T6.0 on the reading that T6 needed a Sheaf change; the second **deleted it
again** after reading `BuildFourteenSegmentCommands`' public `numChars` parameter and the by-value
`std::vector<DrawCommand>` return, which together make T6.1 app-side. Net effect on the validator: none in
either direction — T6.0 was a task, never a spec item, and the §10/§1b text around it is proposal
narrative. No spec delta was touched in any pass this session.

## §EXECUTION — session 8 (2026-08-12): the proposal is built

**Executed via bounded subagents, all on Sonnet (never Opus), code changes strictly sequential, every build
`nice make -j2`.** Verified baseline before touching anything: 10 binaries, **191 tests, 0 failures, 0
warnings** — which confirms this file's previously *inherited* (never re-run) count rather than trusting it.

**Final state: 10 binaries, 211 tests, 0 failures, 0 warnings.** Twenty new tests were added along the way.

Packets, in the order they ran, each left green before the next started:

1. **Registration foundation** — `kFroggersParamsPerBank` 9 -> 14 (a single uniform bump, since all six
   banks reach fourteen; T1.2's "escape the uniformity" framing was session-1's, when only Envelope was
   growing), all 30 new specs, Envelope reordered to interleaved ADSR, `ParameterCount()` 61 -> 91.
2. **Fallout the first packet exposed, which the brief had wrongly ruled out** — `RouteAudioSample` indexes
   Envelope knobs by fixed slot, so the reorder moved Sustain under Decay and silenced the synth; the
   depth-storage pool (1200) was smaller than 91 x 15 = 1365; and Drive's new long name "Bias" tripped the
   design-D8 guard forbidding Random S&H control names in any bank. Fixed by rewiring to 4*vco+{A,D,S,R},
   deriving the pool as `kMaxParameters * kNumModulators`, renaming the long name to "Waveshaper offset"
   (short name stays `Bias`), and making that guard case-insensitive — strengthened, not weakened.
3. **T8.0a-c** the shared `dsp::TrueZeroDepthTaper`, with `Vco::PmDepthScale` now a call into it.
4. **T2.x** Filter. 5. **T3.x/T4.x** Drive and Delay. 6. **T5.x** Reverb. 7. **T1.x/T7.x/T8.x** Envelope and
   Audio. 8. **T6.1/T6.3/T6.4** the label rendering.

**Measurements actually taken (OMNI §9.1 — each ran its positive control and each moved):**
- **Topology (T2.1a)** peak output across the whole sweep: 0.7950 at topo 0, 0.7945 at 0.25, 0.7939 at 0.5,
  0.7943 at 1.0 — all under the 0.80 limiter ceiling, and the sweep's own shape shows the midpoint is not
  an extrapolation of the ends. Sabotage (bypassing `peakTrim`) pinned it at 0.8000.
- **Bias (T3.5)** worst-case `|Process|` 363.4 at bias 0; peak swing rises with ANY nonzero bias, so the
  knob's range is bounded to +-0.02, giving 385.7 (+6.1%). Recorded plainly: zero increase is unreachable
  for any nonzero bias on an unbounded 5th-order polynomial.
- **Tilt (T5.5)** post-limiter peak 0.8000 at centre vs 0.7973 at brightest — no new overshoot; sabotage
  (tilt depth x50) moved the pre-limiter peak to ~50 while the limiter still held.
- **Tuned (T5.7)** swept 0<->1 at step periods of 1, 4 and 32 samples and via a 440 Hz LFO, across four room
  sizes: peak exactly 0.8000 in every configuration. No range or rate narrowing needed.
- **Envelope headroom (T1.5)** the three-voice mix peak at the `kMinSustainLevel = 0.10` floor rose from
  **0.100000 to 1.000000** — exactly the transient §4.3 predicted. Per-voice bound still <= 1.0. Audible,
  not unsafe, as recorded.
- **VCO Balance (T7.2b)** raising the cap 0.80 -> 0.95 broke the sum invariant at 1.15 and the test caught
  it — **and it was invisible at the default knob position**, which is why the full sweep, not a spot check,
  is the guard.
- **Shared taper (T8.0c)** replacing the smoothstep with a step failed both the existing midpoint assertion
  and the new strict-interior one; reverting restored green.

**Still open, and neither is closable by an implementer:**
- **T6.2** — the label rework's acceptance criterion is VISUAL. The rendering is verified programmatically
  (six character slots emitted for `CmbOff` instead of four, badge-chip clearance checked algebraically),
  but nobody has looked at it. A prior UI change here took four attempts by asserting a weaker property
  than "the operator can see it."
- **T8.4** — Ring Mod's low-frequency end is a by-ear taste call. It gates nothing; "off" is the zero
  gate, not a frequency.

**Implementer judgement calls, recorded rather than buried** (no spec value existed for any of these):
`kMaxDecaySeconds` and the Grace maximum (1.0 s each); Curve's shape family (an ease-in one-pole blend,
linear and bit-identical at its default); Ring Mod's carrier range (20 Hz - 5 kHz); and Grace's semantics
where two sources conflicted — the whole deferral is conditional on the knob being above zero, because
"the default is a strict no-op" is the testable requirement and was treated as binding.

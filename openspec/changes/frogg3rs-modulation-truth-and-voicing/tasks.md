# Tasks — `frogg3rs-modulation-truth-and-voicing`

## §0 Standing constraints (carried verbatim from the predecessor; still binding)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch (OMNI §4, §15).
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Build/test runs go through a subagent**, foreground inside the brief, counts + failure tails
  only (OMNI §16.1). Count that all ten binaries ran — `make test` has no `-k`.
- **`External/Sheaf` pinned and clean** (`77a3019e` until W4.1 lands `508d9d68`). We do not patch
  Sheaf; needs → `/UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential; parallel dispatch only for read-only analysis.**
- **No unrequested user-visible behaviour. Propose first.**
- **An implementer may not close a task whose spec requires operator eyes or ears.**
- **A pin is rewritten, never deleted; a guard asserts the property that broke, not two app-side
  numbers against each other.** Six green-while-wrong guards to date; the list is long enough.
- **An instruction's rationale is part of the instruction** (F.6 lesson): when the rationale dies,
  re-derive rather than mechanically preserve — and when honouring an instruction requires adding
  a branch, suspect a literal reading.
- **Enumeration is complete only when every TU/site is compiled or swept** (F-PLAN ERRATA, twice).
- **Systematic debugging is binding for W1/W2:** no fix before the recorded root cause.
- **The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.**

### Execution order — this file is the single proposal layer

1. **W1** modulation truth — *blocked on investigation report (dispatched 2026-08-05)*
2. **W2** filter maxima — *blocked on investigation report (dispatched 2026-08-05)*; fixes are
   per-offender operator decisions
3. **W3** scene-2 opposite defaults — value sign-off then implement; independent of W1/W2 code
   paths until proven otherwise by the W1 trace (defaults touch `FroggersParameters.hpp`, W1
   likely touches `FroggersModulation.hpp` — re-check overlap when W1's fix is scoped)
4. **W4.1** pin bump `508d9d68` → **W4.2** external-audio unblock → **W4.3** G.2 decision
5. **W5** operator walkthrough: C.2 + G.3 + criteria 1–4 — last, never implementer-closed

W1 before W2's implementation (both touch the audible result; one variable at a time when the
operator is listening). W4 after W1/W2 so bump breakage stays attributable (F.1's lesson).

---

## W1 — Modulation truth (badges vs depths vs sound)

Symptoms (operator, 2026-08-05, F.6 build): S1 Randomize All badges nearly every parameter;
S2 drill-in depth knobs all read 12 o'clock; S3 Randomize All inside level-1 shows no icons.

- [ ] W1.0 **Root-cause investigation** — dispatched 2026-08-05, read-only, tracing: what a badge
  renders FROM (existence vs nonzero); what `EnsureModulationDepth` + `RandomizeVisibleValue`
  actually write and to WHICH scene; what a level-1 knob reads and what 12-o'clock means for the
  depth's RangeKind (bipolar zero vs unipolar mid); whether ensures accumulate without removal;
  what the E.1 tests actually assert; the level-1 Randomize All branch; any publish-path
  overwrite. **Findings land here as the §1 trace before any fix is proposed.**
- [ ] W1.1 Fix per root cause — *unwritten until W1.0 lands (Iron Law).*
- [ ] W1.2 Guard test pinning the RESULTING visible depth in the scene the UI reads, across a
  randomize call — not helper call counts. Rewrite the E.1 tests' pins if W1.0 shows they assert
  the wrong property; record the green-while-wrong instance if so.
- [ ] W1.3 Operator confirms: badges, drill-in knobs, and audible modulation agree (criterion 1);
  S3 either fixed or explicitly accepted as designed (criterion 2).

## W2 — Filter maxima, evidence-first

- [ ] W2.0 **Gain-path audit** — dispatched 2026-08-05, read-only: all 9 Filter params +
  Crispy/Crunchy, mapped ranges with math, worst-case at each maximum, limiter placement
  (pumping vs tone-only), what the storm/blowup tests do and do not catch. **Findings land here.**
- [ ] W2.1 Operator triage of the ranked offenders — which are defects vs character. Per-offender
  decision: range trim / stage headroom / per-stage limiting / leave. **By ear, one at a time.**
- [ ] W2.2 Implement the agreed treatments, sequentially, each with a guard that pins the property
  that was wrong (e.g. sustained-tone limiter engagement at max, not merely peak ceiling).
- [ ] W2.3 Extend the storm test if W2.0 shows it cannot catch the operator's symptom (per-param
  endpoint dwell + limiter-engagement assertion, not just >1.0 samples).

## W3 — Scene-2 opposite VCO defaults + shared light cross-coupling

- [ ] W3.0 Trace how per-scene defaults work today: `FroggersParamSpec.defaultValue` is
  single-valued (`FroggersParameters.hpp:145-186`) — establish where scene values initialize and
  whether a per-scene default exists in Sheaf's `ParameterConfig` or must be set post-init by the
  app. File:line before design.
- [ ] W3.1 Operator sign-off on exact values: which "shape" params invert (Shp1-3? PM1-3 too?),
  what "opposite" means numerically (1.0 − default? range-flipped?), and the cross-coupling
  matrix (which sources → which targets at what light depth, both scenes identical).
- [ ] W3.2 Implement + tests (fresh-patch scene 1 vs scene 2 defaults differ as signed off;
  blend midpoint audible check is W5's).
- [ ] W3.3 **Existing-patch impact stated before landing:** defaults apply to fresh patches; the
  operator's saved patches must not be rewritten. Verify load path leaves stored values alone.

## W4 — Carried mechanics (from predecessor F.4/F.5/G.2)

- [ ] W4.1 Pin bump `77a3019e` → `508d9d68` (27 commits, all audio-input + a demo app).
  Compile-first, zero behaviour change, full-TU error enumeration, suite green before W4.2.
- [ ] W4.2 Remove `kExternalAudioOptedIn` (`FroggersAppCore.hpp:507` area):
  `RuntimeConfig::numAudioInputs` requested nonzero; gate on
  `block.InputView().HasActiveChannel(0)`. The old comment promised a one-line change — verify,
  don't trust; written when the API didn't exist. Operator confirms source #6 behaves and no-device
  leaves it dark (recorded residual: device-selected-but-unpatched yields an active silent channel).
- [ ] W4.3 G.2: startup failure currently logs and leaves a blank window (`FroggersMain.cpp`
  inner catch). Operator decision: quit-with-code vs error surface. Then implement.

## W5 — Operator verification (closes the change)

- [ ] C.2 walkthrough on the final build; G.3 saved-patch load from
  `~/Library/Sheaf/synth/sheaf-patch/`; criteria 1–4 from `proposal.md`. **None of this is
  implementer-closable.**

## Deferred (carried)

- §H mobile-web UI layer (incl. slot-repack constraint: slot index == PhysicalEncoderId here;
  display-order repack is the viable route; hardware meaning UNCLEAR — ask first).
- §I VST layer (DAW owns audio/MIDI routing; W4.2's gating must stay swappable for it).
- D.4 publish pipeline.

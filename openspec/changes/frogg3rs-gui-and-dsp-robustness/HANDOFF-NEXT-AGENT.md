# Handoff — next agent, 2026-07-28

Read `/Users/diegoaguilar-canabal/Desktop/omni-rule.md` FIRST. It is binding. The two failures
that defined the last session were omni-rule failures, not coding failures — see "How the last
session failed" below before writing any code.

---

## AUDIT PASS, 2026-07-28 — read this before the rest of the file

An OMNI §14 preflight audit re-verified every claim in the proposal, design, tasks and the four
delta specs against the tree. Findings are folded into those documents; this section records what
changed so the corrections are not mistaken for the original text.

**Resolved for you — do not redo:**

- **The upstream question is answered: plain-click did NOT land.** `origin/main` moved
  `1940ddcb → 1dd4d275`, and `RetainedDrawComponent` still dispatches only from
  `mouseDoubleClick`. Task 6.4 takes its "not landed" branch (Play/Stop → labelled Buttons) and
  needs **no pin bump and no operator approval**. Selected-text inversion and Slider-label
  rendering have not landed either, so 6.3 stays background-only and 6.6 stays necessary.
  Full trace in design **E7**. Item 1 of the suggested first hour is done.
- **tasks.md checkboxes were stale and are now correct.** §1 and all of §3 had landed while every
  box read `[ ]`. §6 has been moved to the top of tasks.md, because it is the current work and was
  buried between §3 and §4.

**Corrections you would otherwise have been misled by:**

- **The envelope followers are in the wrong file.** This handoff and design E6-F4 cited
  `FroggersAppCore.hpp:593` / `:389-390`. They are in **`FroggersModulation.hpp`** at those lines —
  private to `FroggersModulationSlate`, and following that slate's *own separate* VCO instances,
  not the audio-path VCOs (`FroggersAppCore.hpp:221-226`). See design E6-F4.
- **Every line citation into `FroggersAppCore.hpp` and `FroggersUiSurface.hpp` had drifted**, some
  to code that no longer exists (`kBankLabels` is deleted). Corrected in place and marked
  `(re-verified 2026-07-28)`. `app/dsp/**` and `External/Sheaf/**` citations were accurate.
- **Task 6.7's remedy is withdrawn and replaced.** "Abandon the generic ScopeVisualizer and
  hand-build Draw commands like Braid 4" contradicted this change's own `froggers-vco-topology`
  delta ("no bespoke waveform rasterization exists in the app") — two requirements in one change
  demanding opposite implementations. All four operator requirements are reachable *through* the
  generic visualizer, with less code. Design **E6-F4-bis** has the mechanism table.
- **6.7's bottom band is a tap-point fix, not an open question (task 6.7a).** The existing
  envelope followers are tapped **pre-gate**, on an oscillator whose generator has no amplitude
  term (`dsp::Vco::Process` returns a pure function of phase), so they hold a fixed level and the
  band would have rendered three motionless lines — the same defect the top band has. Fix: feed
  the *display* followers the post-gate per-voice values 6.7c already extracts. **Do not re-tap
  the existing followers** — they are registered modulation sources, and changing their input is a
  parity break for a display change; use separate `SingleEnvelopeFollower` instances.
- **Encoders have the same double-click disease** and cannot be fixed app-side (they need bounds,
  which `Button` nodes lack). Not a task — a **disclosure**, new task 6.10.
- **6.5 depends on 2.2, but §6 precedes §2.** Carve-out added: pull only the `StereoDelay`/`Reverb`
  half of 2.2 forward. Note `StereoDelay` already has `ClearBuffers()`.
- **6.6 can invalidate 3.7.** Two new Label nodes join the auto-flowed chrome band whose height
  3.7 derived; re-run that derivation as part of 6.6.
- **One spec requirement was unmeetable** and has been rewritten: "no raw floating-point blend
  value is shown" — JUCE's slider text box is unconditional upstream, so no app change can satisfy
  it. It is now scoped to what the app controls, with the readout recorded as an upstream item.
- **A spec rationale cited the shadow-copy numbers as "measurement"** while its sibling requirement
  forbids measured thresholds. Reworded: the argument rests on "large but finite", not on 8.7.
- **Two spec gaps filled:** single-click operation of the *transport* (the spec only covered banks)
  and the S&H visualizer attachment (6.8 had no requirement behind it).

## Canonical documents (there is no other handoff — a stale root HANDOFF.md was deleted for cause)

- `openspec/changes/frogg3rs-gui-and-dsp-robustness/` — proposal.md, design.md (E1–E6), tasks.md.
  **design.md §E6 and tasks.md §6 are the current work.** §6 takes priority over §2–§5.
- `openspec/changes/archive/2026-07-28-froggers-sheaf-app/ARCHIVE-RECORD.md` — history, traps,
  what the predecessor change actually shipped vs claimed.
- `UPSTREAM-SHEAF-ASK.md` + `sheaf-audioconfig-labels.patch` — items for jvictor0 (upstream Sheaf
  maintainer). ~~Check whether plain-click landed before task 6.4~~ — **checked 2026-07-28, it has
  not** (design E7). All three asks are still open. Two additions belong in that file: the
  Slider-label gap (6.6), and the fact that plain-click blocks the **encoder grid**, not just the
  transport.

## State of the tree

- Repo `daguilarc/frogg3rs`, branch `froggerstiga-desktop-v2`, baseline `b409106`. NOTHING in the
  parent repo is committed; `app/` is untracked. `git diff b409106 --name-status` must show ONLY
  `M External/Sheaf` (the sanctioned dafa54b6→1940ddcb pin bump — expected, do not investigate).
- `External/Sheaf` = upstream pin `1940ddcb`, working tree clean. **Never modify it.** The audit
  ran `git fetch origin` inside it to answer the plain-click question: that added remote refs only
  (`origin/main` now shows `1dd4d275`); HEAD, the checked-out tree and the parent's gitlink are
  unchanged. Nothing to undo. A local
  fork was tried 2026-07-27 and reverted; the reverted commits sit on local branch `froggers-fork`
  for upstreaming. Consequence: Draw-node actions are DOUBLE-CLICK at this pin.
- Frozen trees stay byte-identical: `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- Tests: 127/127 across 11 binaries (`nice make -j2 test` in `app/`). Build the launcher ONLY with
  `./app/build-launcher.sh` (globs all headers; hand-written lists silently untrack `app/dsp/`).
- Machine: 8-core/16 GB — `-j2` + `nice`, never more. Subagents: Sonnet/Haiku only, model set
  explicitly. No AI attribution in commits.
- The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/` (logs + persisted
  patch state, keyed by parameter NAME). For default-patch tests use a FRESH data root.

## What actually works (operator-verified 2026-07-28)

Scopes draw (wrongly — see below), page order is signal-path, ASR labels keep their digit, bank
highlight is inverted (but double-click — being reversed), title text gone, Play/Stop icons look
decent, window no longer clips. The scoopNotch permanent-silence bug is genuinely fixed
(`app/FroggersAppCore.hpp:668-685`, re-verified 2026-07-28).

## What is broken (operator-verified) — all traced in design.md E6, tasks in §6

1. **Default patch inaudible** (F1): pitch knobs default 0.0 → 20 Hz (`app/dsp/Vco.hpp:118-121`);
   `ApplyFroggersDefaultPatch` (`app/FroggersModulation.hpp:916`) never sets them. Fix = task 6.2.
2. **Slider labels never render** (F2): `PortableJuceBackend.hpp:1231` uses `setName` — draws
   nothing. "BPM"/"Scene blend" invisible. Fix = adjacent Label nodes, task 6.6.
3. **Stop fails** (F3): single-click is a no-op on Draw nodes; AND no tail-kill exists (delay/
   reverb feedback rings through a closed gate; no unit has Reset()). Tasks 6.4, 6.5, 2.2.
   The single-click half also hits **the encoder grid**, which cannot be fixed app-side — task
   6.10 is the disclosure.
4. **Bank buttons don't switch on single click** (F3.1): reverse part of 3.1 — Button nodes +
   post-Build `node.selected` patch. Task 6.3.
5. **Scope band wrong in kind** (F4): taps pre-gate free-running VCOs (moves while silent);
   `drawMarkers=true` but marker recording never ported (`app/dsp/Vco.hpp:47-52`). Operator wants:
   top = post-gate audio (silence ⇒ flat), bottom = a control-rate band, colours
   blue/yellow/magenta. Task 6.7. **Corrected by the audit:** the EFs are `vcoEnvelopeFollowers_`
   in **`FroggersModulation.hpp:593`** (not `FroggersAppCore.hpp`), private, and following the
   slate's own separate VCOs; the Braid-4 hand-built-Draw remedy is **withdrawn** in favour of the
   generic visualizer (design E6-F4-bis); and the bottom band's source is an open operator question
   (6.7a). The ONE-node-vs-TWO-panels report now has two named code-side candidates in E6-F4 —
   check those rather than looking open-endedly.
6. **S&H dice-roll motion missing on mod pages** (F5): `FroggersRandomShVisualizer.hpp` exists;
   cell attachment unverified. Task 6.8.

## Ruled out — do not re-litigate

- Divisor clamps in the filter (refuted twice; knob values cannot leave [0,1]:
  `ClampToRange` = unconditional `std::clamp`, `ParameterModulation.cpp:449-452`).
- Persisted-state / Crunchy theories for the default silence (operator: Crunchy untouched).
- Sustain defaults as the silence cause (defaults flow: `FroggersParameters.hpp:327`).
- Bank reorder corrupting saved patches (persistence is name-keyed, traced to
  `ParameterValuesToJSON`).
- Output clamp: operator chose HARD clamp at 1.0 (task 2.8) — no saturator, do not re-offer one.
- The scene float readout cannot be hidden app-side (slider text box is unconditional upstream).

## The defect CLASS (most important thing in this file)

Four independent bugs — scoopNotch setters, scope AdvanceIndex, EF scope feed, (suspected) S&H
cell attachment — are ONE defect: the port copies structures and formulas, then drops required
call sites. Unit tests are structurally blind to it (they configure objects themselves). Task
1.4/6.1 — the systematic call-site sweep — was planned twice and never run. **Run it before any
feature work.** Every hour spent on it likely saves a GUI-test round-trip.

## How the last session failed (so you don't repeat it)

1. **Closed a GUI task without seeing pixels.** The verifying agent said the screen was locked and
   no visual confirmation happened; the lead accepted anyway and told the operator it was ready.
   OMNI §14 postflight was skipped. Rule now in tasks.md §6: nothing GUI-marked closes without
   the pixels being seen; 6.9 is an operator walkthrough.
2. **Verified fields, not renders.** "The BPM label exists in the node" was accepted as "the BPM
   label is visible." It never renders. When a spec says visible, verify VISIBLE.
3. **Traded function for cosmetics** (Draw-node bank buttons: pretty inversion, dead single-click).
   The operator's priority is explicit: function first.
4. The predecessor's trap #1 ("tests pass ≠ app works") was quoted repeatedly and still recommitted.
   127 green tests coexisted with an instrument that starts silent, can't switch pages by click,
   and can't stop.

## Suggested first hour (revised by the audit)

1. ~~Check upstream~~ — **done, see the audit section above.** Plain-click did not land; 6.4 takes
   the Button branch, no pin bump.
2. Dispatch the 6.1 call-site sweep (parallel analysis subagents allowed, Sonnet/Haiku). It is the
   only task that addresses the defect *class*, it was planned twice and never run, and F1/F4/F5
   are all suspected members of it.
3. Then 6.2 (pitch defaults) — smallest change with the biggest operator-facing payoff.
4. Launch the app and screenshot, and settle the two-panels-vs-one-node question while you are
   looking at it (design E6-F4 names the two code-side candidates, so this is a diagnosis and not
   a stare). It gates 6.7c.

The only thing needing the operator up front is 6.10 — the encoder double-click limitation. Raise
it before the work, not after.

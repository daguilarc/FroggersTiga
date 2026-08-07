# Proposal — `frogg3rs-modulation-truth-and-voicing`

> # ⚠ STOP — THIS PLAN IS SUPERSEDED AND ITS COMPLETED ITEMS ARE NOT VERIFIED WORKING
>
> **Read `FAILURE-REPORT-AND-HANDOFF.md` in this directory BEFORE anything else in this file.**
>
> The operator ran the final consolidated build on 2026-08-05 and reports **four failures still
> present**, every one of which this plan records as fixed and green:
> 1. Randomize All still produces far more than the intended count of modulation badges.
> 2. Filter Crispy at max still blows out.
> 3. **Stop does not stop** — audio continues over a minute after the transport is stopped.
> 4. Randomize All inside a level-1 drilldown ejects the operator to the main page.
>
> **176 tests pass. Four reported symptoms remain.** Every checkbox and every "measured green"
> claim below describes an isolated stage under synthetic input, never the real signal path.
> Treat completion marks in this file as *"code was written and a narrow test passed"*, NOT as
> *"the instrument works"*.
>
> Superseded by: **`frogg3rs-blowout-and-drilldown-repair`** (scope F1-F5, carried from the
> failure report).


**Created 2026-08-05. Supersedes `frogg3rs-audio-safety-and-ui-rework`** — see
`SUPERSESSION-RECORD.md` for exactly what carried over, what stays open, and why supersession
rather than extension.

## Why this change exists

The operator ran the predecessor's final build (F.6, all green, layout approved as "perfect") and
found three things, two of which contradict work the predecessor recorded as done:

1. **The modulation UI is telling two contradictory stories.** Randomize All on the main page
   paints modulation badges on nearly every parameter — implying far more than the designed
   median-3 nonzero depths. Drilling into any parameter shows every depth knob at 12 o'clock —
   implying zero modulation everywhere. At most one of those displays is telling the truth, and
   the operator cannot see the actual modulation state. Additionally, Randomize All while inside a
   level-1 drilldown shows no source icons at all. §E was recorded done with green tests; if the
   depths are real but invisible, or visible but unreal, those tests pinned the wrong property —
   the sixth green-while-wrong instance this project has hit.

2. **Filter parameter maxima still sound wrong.** Operator: "some of the filter parameter maxima
   still sound like shit... this was supposed to have been fixed but i don't think the fix
   worked." §A capped comb feedback and the resonant bump and added a master limiter — the
   operator accepted the blowout fix by ear at the time. What remains is per-parameter: specific
   maxima are objectionable, plausibly (a) sustained near-clipping driving the master limiter into
   audible pumping, or (b) spectra a limiter cannot help with. The ask is explicit: identify WHICH
   parameters are the offenders, and consider limiting at the parameter/stage level, not only at
   the output.

3. **New voicing scope.** Scene 2's default VCO shapes should be the OPPOSITE of scene 1's, with
   the same light cross-coupling modulation defaults — so a fresh patch has two genuinely
   different-sounding scenes to blend between instead of two copies of one.

## Method constraint (binding)

Items 1 and 2 are bugs, so they run under systematic debugging: **no fix is proposed or
implemented until the root-cause investigation lands and is recorded in `tasks.md`.** Two
read-only investigations were dispatched 2026-08-05 (modulation badge/depth truth-tracing; filter
maxima gain-path audit). Their findings become the W1/W2 §1 traces; fixes derive from those
traces, not from the symptoms.

## What this change delivers

- **W1 — modulation truth.** One display of modulation state that is actually true: badges and
  drill-in knobs agree with each other and with the audible result. Whatever the root cause turns
  out to be (badge-on-existence vs badge-on-nonzero, scene-write mismatch, unipolar-midpoint
  misread, cumulative ensures, publish overwrite), the fix lands with a guard test that pins the
  RESULTING VISIBLE DEPTH in the scene the UI reads — not call counts.
- **W2 — filter taming, evidence-first.** A ranked, file:line-cited table of which filter maxima
  misbehave and how (limiter-pumping vs tone), then per-offender treatment (range trim, stage
  headroom/limiting, or gain compensation) — decided WITH the operator per offender, since §A
  history shows these calls are made by ear. The §A constants are not touched until the evidence
  says which ones move.
- **W3 — scene-2 opposite defaults.** Scene 1 and scene 2 ship different: VCO shape defaults
  inverted in scene 2, identical light cross-coupling modulation defaults in both. Exact values
  are operator sign-off items.
- **W4 — carried mechanics.** The second Sheaf bump (`77a3019e` → `508d9d68`), external-audio
  workaround removal (ask 8 landed), and the G.2 startup-failure decision.
- **W5 — operator verification.** Nothing visual or audible closes at an implementer.

## Out of scope

- `External/Sheaf` modifications — pinned; needs go to `/UPSTREAM-SHEAF-ASK.md` (15 filed to
  date; 7+12 going to a GitHub issue 2026-08-05).
- Frozen trees stay byte-identical: `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- §H mobile-web layer and §I VST layer — deferred, carried from the predecessor.
- Parameter-VALUE randomization (no coin flip, every knob every time) — operator-confirmed
  correct, untouchable.

## Success criteria (falsifiable, with who checks)

1. After Randomize All, the set of badged sources equals the set of nonzero-depth sources, and
   drill-in knobs display those same depths. Test-pinned on resulting values; operator confirms
   visually.
2. Randomize All inside a level-1 drilldown produces visible evidence of what changed (or the
   plan records explicitly why level-1 cells carry no badge, with the operator accepting that).
3. Every filter parameter can sit at its maximum without the output limiter audibly pumping on a
   sustained tone — or the specific exceptions are recorded as operator-accepted character.
   Operator's ears decide.
4. A fresh patch's scene 2 sounds distinct from scene 1 (inverted VCO shapes), blend sweeps
   between them, and both carry the same light cross-coupling. Operator confirms by ear.
5. Suite green across all ten binaries at every landing; `External/Sheaf` clean at its pin.
6. No task whose spec requires operator eyes/ears is closed by an implementer.

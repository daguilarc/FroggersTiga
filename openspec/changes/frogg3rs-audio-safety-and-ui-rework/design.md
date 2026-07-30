# Design / analysis — `frogg3rs-audio-safety-and-ui-rework`

Every claim cites the `file:line` where it was verified (OMNI §1, TRACE DON'T ASSERT). Line numbers
were re-read on 2026-07-28 after the predecessor's §2 work landed and shifted them.

---

## A1. The signal chain, end to end (OMNI §1 data-flow trace — §14 preflight requires this)

`FroggersApp::ProcessBlock` → per sample → `RouteAudioSample()` (`app/FroggersAppCore.hpp:750`):

| # | Stage | Where | Bound on its output |
|---|---|---|---|
| 1 | 3 × `dsp::Vco::Process` | `app/dsp/Vco.hpp:149` | `EvalWaveMorph` of phase — **no amplitude term at all**, so ≈±1 by construction |
| 2 | `dsp::MixOscVoices` (ASR gate + 3-voice mean) | `app/FroggersAppCore.hpp:775`, impl `app/dsp/VoiceEnvelope.hpp:150-168` | ≤ stage 1; mean of three gated voices |
| 3 | `FrogBlock` drive | `app/FroggersAppCore.hpp:787+` | `SetGain = ExpMapCompute(1,5)` → up to **5×**; internally saturated |
| 4 | `DriveBlendPhase` | `app/dsp/Drive.hpp:345-352` | allpass, coefficient now `0.98*(2p−1)` so \|a\|<1 (fixed by predecessor) |
| 5 | `ResonantBump` peak | `app/FroggersAppCore.hpp:825` | `height = ExpMapCompute(1,10)` → **10× (+20 dB)** ← **primary gain offender** |
| 6 | `Comb` | `app/FroggersAppCore.hpp:849`, impl `app/dsp/FilterFx.hpp:292-298` | `out = in + fb*Saturate(lp(delayed))`, `fb` from `GetFeedback` ∈ **±1.1** ← **primary sustain offender** |
| 7 | `scoopNotch` | `app/FroggersAppCore.hpp:826-840` | a **dip**, height ∈[0.05,1] — adds no gain |
| 8 | `StereoDelay` | `app/FroggersAppCore.hpp:872` | feedback hard-clamped to **0.98** (`app/dsp/Delay.hpp:170`) — stable, but ~35 s to −60 dB |
| 9 | `Reverb` | `app/FroggersAppCore.hpp:879` | `(1−mix)*dry + mix*wet` |
| 10 | `SanitizeOutputSample` | `app/FroggersAppCore.hpp:912-920` | hard clamp at 1.0 (predecessor's 2.8) ← **replaced by the limiter** |

**Where the blowout comes from:** stage 6 pins itself at its saturator limit whenever \|fb\|>1,
stage 5 multiplies that by up to 10, and stage 10 receives ~20× full scale. Every value is finite
the whole way, which is why finiteness guards never fired and `sawNaN` stayed 0.

### A1a. Correction to the predecessor's analysis

The predecessor (and my own earlier statement to the operator) claimed the comb **diverges
exponentially**, growing ~10% per delay period. **That is false.** The `PadeSaturator` sits inside
the feedback path (`app/dsp/FilterFx.hpp:294`), so the fed-back term is bounded by
\|fb\| × 1.0 = 1.1 and the comb output by \|input\| + 1.1. \|fb\|>1 produces **sustained
self-oscillation at the saturator limit**, never decaying — loud and permanent, not divergent.

This distinction changes the fix. A divergence needs a magnitude tripwire; a sustained
self-oscillation needs the loop gain brought below unity. Both are being done, but only the second
addresses the cause.

---

## A2. Decisions (operator-approved 2026-07-28)

| Decision | From | To | Reasoning |
|---|---|---|---|
| Comb feedback max | ±1.1 | **±0.95** | \|fb\|<1 is the definition of a decaying loop. 0.95 still rings a long time and stays musical |
| `ResonantBump` max height | 10× (+20 dB) | **2× (+6 dB)** | An audible resonant peak without a 20 dB multiplier on a pinned comb; revised down from an initial 4× target — see amendment below |
| Output stage | hard clamp 1.0 | **limiter, threshold 0.9** | Gain reduction, not clipping |
| Attack max | 2.5 s | **1.0 s** | Operator judgement; covers pad swells |

**Amended 2026-07-29.** 4× (+12 dB) was the first target and was implemented, but the operator heard
it modulated and judged it "still too harsh ... very close to blowout territory". The comb feeding
this stage is bounded near `|in| + 0.95` (~2 at full scale), so 4× handed the output stage ~8 — about
9× over the limiter's 0.9 threshold, meaning the limiter rode hard and continuously and the sustained
gain reduction was itself the harshness. Revised to **2× (+6 dB)**, which roughly halves how hard the
limiter works while staying an audible resonant peak. Recorded at the definition site,
`app/dsp/FilterFx.hpp:109-122`, and at the call site, `app/FroggersAppCore.hpp:970-982`. If it still
reads as harsh, the next lever is the comb feedback (0.95) that feeds it, not this ceiling — past a
point, lowering this further just makes the resonance inaudible.

**Why the range and not the randomizer.** An earlier draft proposed leaving the ranges alone and
narrowing only what Randomize All draws. Rejected by the operator, correctly: a knob region that
reliably destroys the audio is a defect regardless of who selects it, narrowing only the draws
leaves it reachable by hand, and it creates two contradictory definitions of "the range". The range
*is* the set of possible randomization outcomes.

**Parity consequence, accepted deliberately.** `app/FroggersDspParityTests.cpp:528-529` pins
`GetFeedback`'s ±1.1 literally, and the bump's 10× appears in the same suite. These changes **will**
fail those tests. The pinned expectations get rewritten with a recorded divergence note — the same
treatment design D6 already gives the fuegoize UB divergence. A parity test failing here is the
intended outcome, not a regression.

### A2a. Limiter design

**Role: safety net.** With A2's range changes landed it should never engage on musical material.
That argues for a conservative threshold and against anything that colours the sound.

- Threshold **0.9**; above it, apply gain reduction so the output asymptotes to 1.0.
- **Gain reduction, not per-sample waveshaping.** A saturator distorts every sample it touches; a
  limiter computes one gain coefficient and applies it. In-range material must pass through
  bit-identical — that is the acceptance test.
- Fast attack, release on the order of 100 ms, so it does not pump.
- **Preserve** the existing non-finite → `0.0f` branch and the denormal flush
  (`app/FroggersAppCore.hpp:912-920`). Only the magnitude path changes.
- The limiter is **not** a substitute for Tier 1/Tier 2 recovery (already landed). Those clear
  poisoned state; the limiter only bounds what escapes meanwhile.
- **Required comment at the limiter's definition (operator, 2026-07-28):** record that this is
  another feature that does **not** need to be internal once a VST/plugin build exists — a plugin
  host owns final gain staging and typically has its own limiting, so in that context this stage is
  redundant and a candidate to compile out or bypass. Writing it down at the definition means
  whoever builds the VST finds it there rather than rediscovering the argument. This joins the
  existing set of standalone-only concerns rather than being a new category.

---

## A3. UI rework (operator, after running the build)

**A3a. Scope band geometry — the operator's strongest complaint.** `kScopeWidth = 340.0f` against a
column that takes the **full remaining content height**, which after the predecessor's dead-strip
removal grew by another 54 px. The result is a panel far taller than wide. Requirement: **at most
one third of its current area, and wider than tall.**

**⚠️ POSITION IS NOT IN SCOPE. Only the height changes.** An earlier revision of this section added
"the reclaimed space goes to the encoder grid", which the operator never asked for; an implementer
reasonably read that as licence to restructure, moved the scope from its left-hand column to a
full-width band across the top, and gave the grid the whole width beneath. Operator: *"WHEN DID I
ASK FOR YOU TO CHANGE THE LOCATION OF IT? i said just the height should change."*

The scope stays in the **upper-left column**, grid to its right, at **340 × 64** (12% of the
original 340 × 528, 5.3× wider than tall). The space freed **below** it in the left column is left
**empty** — the operator intends transport and scene controls there (A3g / tasks D.6) and it is not
the grid's to take.

Guarded by `scope_sits_in_a_left_column_with_the_grid_to_its_right`. That guard exists because
nothing pinned the scope's *location*, so the regression shipped with every existing assertion
green — the same failure shape as the window-height guard that compared two app-side numbers to
each other.

**A3b. Trace colours.** Currently Red / Orange / Yellow (`app/FroggersAppCore.hpp:128-130`) — three
hues that collapse together under protanopia/deuteranopia. New: **Cyan / Pink / Yellow**.
`synth::Color` has `Cyan` and `Yellow` but **no** `Pink` or `Magenta` (verified by grep of
`External/Sheaf/projects/synth/include/synth/Color.hpp`), so pink is `Color::Rgb(255, 105, 180)` —
bright against the panel's `Rgb(12,14,16)` background and separated from cyan and yellow in both
blue channel and luminance.

**A3c. One panel, not two.** The predecessor left "one visualizer node vs two visible panels" as an
open question. **Operator resolved it by looking: there is ONE panel.** No reconciliation work is
needed, and the EF second band is dropped — if a sensible second band does not present itself, the
single audio-rate scope stands alone.

**A3d. Post-gate tap.** The scope still animates before Play is ever pressed, because
`dsp::Vco::Process` writes to the scope (`app/dsp/Vco.hpp:164-167`) **before** `MixOscVoices`
applies the ASR gate. Move the write to the post-gate per-voice values. `adsr.apply` is public
(`app/dsp/VoiceEnvelope.hpp:164-166`) — extend `MixOscVoices` with an out-parameter rather than
re-applying the envelope at a second call site (OMNI §8).

**A3e. Transport controls.** The predecessor removed the Play/Stop icons to regain single-click,
because `Draw` nodes dispatch only on double-click at this pin
(`External/Sheaf/projects/synth/juce/PortableJuceBackend.hpp:549-555`). The operator liked the
icons. **Resolution (operator-approved): `Button` nodes with `▶` and `■` as the label text.** Button
labels render, Buttons dispatch on single click, and the glyphs read as icons — both properties, no
upstream dependency. `BuildPlayDrawCommands`/`BuildStopDrawCommands` stay in the file for the day
plain-click lands.

**A3f. BPM annotation removed.** The label switching to "BPM (no effect while stopped)" was never
requested, and it changes the width of a flowed control, shifting the alignment of its neighbours
every time the transport starts or stops. Revert to a constant `"BPM"`.

**Process note.** That annotation was invented by an agent to "improve discoverability" and nobody
asked for it. Standing rule for this change: **do not add user-visible behaviour the operator did
not request.** Propose it first.

---

## A4. Carried from the predecessor — verified in the tree, do NOT redo

Landed and verified: scope `AdvanceIndex` + regression test; call-site sweep (found
`ConfigureProcessingTiming` never called, fixed); signal-path bank order; ASR short labels; canvas
title removed; scene controls; window-height derivation (632, computed from a model of the
backend's flow); audible pitch defaults; single-click bank buttons; visible BPM/Scene-blend `Label`
nodes; Stop clearing delay/reverb tails including the long-release case; per-unit `Reset()` on eight
DSP units; Tier 1 finiteness and Tier 2 magnitude recovery with the derived 100.0 ceiling; the
`DriveBlendPhase` marginal-stability fix; the full-range endpoint sweep.

**Superseded:** task 2.8's hard clamp at 1.0 (→ limiter), and the `frogg3rs-dsp-recovery` spec
requirement forbidding soft-knee limiting.

**Still open and inherited:** the S&H dice-roll observation (its wiring was proved intact, so it may
have been a symptom of double-click drill-in), `ScopeWriter` sizing, §4 voicing judgements, and the
whole §5 publish pipeline.

---

## A6. Randomize-depth count distribution (analysis, 2026-07-29)

**Symptom.** Randomize Page on a modulation detail page does nothing roughly half the time.

**Cause, traced.** `Bank::RandomizeModulationDepths`
(`External/Sheaf/projects/synth/src/ParameterModulation.cpp:2881-2911`) chooses how many sources to
touch with `while (manager_->NextRandomCoin() < 0.5f)` — a geometric distribution starting at
**zero**:

| n sources | 0 | 1 | 2 | 3 | 4 |
|---|---|---|---|---|---|
| P | **50%** | 25% | 12.5% | 6.25% | 3.1% |

Mean 1. On Randomize All this hides inside 54 calls (~27 no-ops among ~61 real changes, which is
why All feels fine). Randomize Page at drill-in level 1/2 makes **exactly one** call
(`app/FroggersModulation.hpp`, `RandomizePage`), so the button is a coin flip.

**Not a Sheaf bug to report.** The distribution may suit other apps; the operator's 1-3 bias is our
taste. Fix belongs app-side.

**This is distinct from parameter-VALUE randomization, which is fine and must not be changed.**
`RandomizeBankValues` loops all nine slots unconditionally and each press lands in
`case Modifier::Random: parameter.RandomizeVisibleValue(...)` (`:2872-2874`) with no guard. Every
knob moves, every time. The coin only ever governed *how many modulation sources* get depths — a
different question, since attaching all 15 would be absurd whereas moving all 9 knobs is what
"randomize page" means.

**Target distribution — SPECIFIED BY THE OPERATOR, 2026-07-29. Supersedes two earlier drafts;
the "z score" phrasing from the first is withdrawn, do not reason from it.**

Three constraints, all from the operator:

1. **Median 3.** Not 2 — the instrument is Frogg3rs. Deliberate, not an off-by-one to "correct".
2. **Above 4 and below 2 are equally rare:** `P(n = 1) == P(n >= 5)`.
3. **The tail stays plausible all the way up.** Counts above 4 run to the connected-source count
   (13 with external audio off, 15 with it on) and should have **high MAD** — a wide spread that
   occasionally produces a genuinely dense patch, not a tail that dies at 5.

| n | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | ... | 13 |
|---|---|---|---|---|---|---|---|---|---|---|
| P | 10% | 30% | 30% | 20% | 3.1% | 2.2% | 1.5% | 1.1% | ... | 0.18% |

- `P(n<=2) = 0.40 < 0.5 <= P(n<=3) = 0.70` -> **median 3**.
- `P(1) = 0.10 = P(n>=5)` -> equally rare in both directions.
- Tail is **geometric at r = 0.7**, not uniform: the operator asked for exponential falloff above 5
  while keeping the far end plausible. r=0.7 puts P(13) at ~1 in 555 -- rare but genuinely seen.
  r=0.5 would make it ~1 in 5000, which is indistinguishable from never and defeats constraint 3.
- **MAD ~1.10** against a median of 3.

**3 is CO-MODAL with 2, not the sole peak** -- the 30/30 tie is deliberate (operator: "your p2/3
tied was a good idea"), and supersedes an intermediate 25/35 version that made 3 uniquely modal.
Median 3 is unaffected either way. If a later revision wants 3 as the unique mode it needs
`P(3) > P(2)`; do not "fix" the tie into one accidentally.

Implementation -- weighted table for 1..4, then a coin loop for the geometric tail. NOT a single
geometric from 1, which cannot produce this shape:

    const float u = manager.NextRandomCoin();
    std::size_t count;
    if (u < 0.10f)      { count = 1; }
    else if (u < 0.40f) { count = 2; }
    else if (u < 0.70f) { count = 3; }
    else if (u < 0.90f) { count = 4; }
    else {
        count = 5;
        while (count < eligible.size() && manager.NextRandomCoin() < 0.7f) { ++count; }
    }
    count = std::min(count, eligible.size());

Guard `eligible.size() < 5` before entering the tail branch.

**Mean is ~3.1 and is NOT a target** — the operator has ruled it out twice. Recorded only so the
aggregate is not a surprise: Randomize All makes 54 of these calls, so it goes from ~54
source->parameter connections to ~165. Same depth LEVEL as before (more sources per parameter at
level 1, never modulation-of-modulation). If that reads as too dense by ear, the lever is this
table, not the level.

**On skew and the mean, recorded so nobody re-opens it:** median 2 constrains neither skew
direction, and the support has a hard floor at 1 with room above, so any spread is necessarily
right-tailed — there is no long left tail available. Mass can be piled on 1 to pull the mean below
2, or on the tail to push it above. An earlier draft of this section spent effort choosing among
those to control Randomize All's aggregate connection count. **The operator has ruled that out:
the median is the spec.** The mean of the shape above happens to land near 2.03; that is a
consequence, not a target.

**Consequence, noted but NOT a blocker:** eliminating the 50% zero mass roughly doubles the number
of source->parameter connections Randomize All makes (54 calls, mean ~1 -> ~2). That is inherent to
"never 0" and cannot be avoided by reshaping within 1-3. Mentioned so the change is not surprising
by ear; it does not need a decision before implementing.

**Feasibility — the blocker I hit was wrong.** Mid-implementation I stopped believing the app could
not obtain the live `SceneState`, because `Bank::HandlePress` takes no scene and resolves it
internally. That was a failure to look far enough:
**`ParameterManager::Scene()` is public** (`ParameterModulation.hpp:816-817`) and returns the live
scene. Every other piece is public too and already used by this app:
`Parameter::EnsureModulationDepth` (`:499`, used by `ApplyFroggersDefaultPatch`),
`Parameter::RandomizeVisibleValue` (`:494`), `ParameterManager::NextRandomValue/Coin/Index`
(`:864-866`), `ParameterGroup::GetModulators().Metadata()` (`:335`, `:290`).

So the app can choose the count and the source set while **Sheaf still performs every mutation** —
which is precisely the split design D14 already prescribes ("the app chooses the target set, Sheaf
does every write"). This is not a D14 violation; reimplementing `RandomizeVisibleValue` would be.

**Two properties Sheaf's loop has that the replacement should not inherit:**
1. It can draw the **same source twice** (independent draws, no exclusion), which wastes a draw and
   makes the effective count lower than the nominal one.
2. It counts only `connected` sources — that part is correct and must be kept. It is also why the
   external-audio mis-reporting (B11) let randomize reach a source carrying nothing.

## A7. Modulation drill-in navigation (analysis, 2026-07-29)

Two separate defects, both app-side, both reported by the operator.

**A7a. Back from level 2 exits all the way to level 0.** `FroggersModulationDrillIn::Back()`
(`app/FroggersModulation.hpp`) is `bank_->Deselect(); level_ = 0;`. Sheaf's `Bank` has no level
concept at all — one `Parameter* selected_` and a bool derived from it — and `Deselect()` is a full
exit with no one-level pop. The app's own header comment records this as "matching the design's
resolved choice", i.e. it was a deliberate simplification, not an oversight. The operator has now
overruled it: from level 2, Back should return to level 1.

**Fix is app-side and does not need Sheaf.** The drill-in already tracks `level_`; it can also
remember the level-1 parameter and, on Back from level 2, `Deselect()` and then re-open that
parameter to land back at level 1. No Sheaf change, no new Sheaf API.

**A7b. Clicking the bank you are already on does nothing.** `RequestBankSelect` is handled at
`app/FroggersAppCore.hpp:413-428`, guarded by:

```cpp
if (bankRequest >= 0 && bankRequest < kFroggersBankCount && bankRequest != activeBankIx_)
```

The `!= activeBankIx_` term makes re-selecting the active bank a no-op — and that is exactly the
case the operator needs, because when drilled into a modulation page the active bank is still the
page you want to get back to. Operator: *"clicking on the page bank for the page we are on is the
way the user should always be able to get to that page, even when they are in a modulation
drilldown for a parameter on that page."*

**Fix:** when the requested bank equals the active bank and the drill-in level is above 0, reset
the drill-in (full `Deselect()`, level 0) instead of returning early. When it equals the active bank
*and* level is already 0, keep the current no-op — rebuilding identical state on every click would
be wasted work. Note the existing early-return also protects `drillIn_.emplace(...)` from
needlessly reconstructing on a same-bank click, so the guard is not simply deleted.

## A5. Open question NOT resolved here

**Drive Blend defaults to 0** (`app/FroggersParameters.hpp`, Drive slot 7), and
`DriveBlendPhase::Process` returns `dry*(1−blend) + phased*blend` (`app/dsp/Drive.hpp:352`), so the
default patch's authored 20% Drive is crossfaded out entirely and is inaudible. This is authored
intent that never reaches the audio — the same shape as the pitch-default bug — but the correct
value is a voicing decision. **Operator has not ruled. Do not guess.**

# Design — `frogg3rs-gui-and-dsp-robustness`

Every claim below cites the file:line where it was verified (OMNI §1, TRACE DON'T ASSERT).
Claims that were **not** verified are marked `[UNVERIFIED]` and must be verified during
implementation, not assumed.

---

## E0. Audit status — 2026-07-28 (OMNI §14 preflight, re-run against the tree)

Line citations in E1–E5 were written before dispatches 1–4 edited `app/FroggersAppCore.hpp` and
`app/FroggersUiSurface.hpp`. **Every citation into those two files had drifted**; the ones that
matter have been corrected in place below and are marked `(re-verified 2026-07-28)`. Citations
into `app/dsp/*.hpp` and `External/Sheaf/**` were spot-checked and are accurate.

Landed vs open, verified in the tree (not from the checkbox state, which was stale):

| Section | Status |
|---|---|
| E1 oscilloscope `AdvanceIndex()` | **LANDED** — `app/FroggersAppCore.hpp:480` |
| E2 DSP recovery, output clamp | **OPEN** — no `Reset()` exists anywhere in `app/dsp/`; `kMaxOutputMagnitude` still `8.0f` (`app/FroggersAppCore.hpp:750`) |
| E3a bank inversion | **LANDED, THEN SUPERSEDED** by E6-F3.1 (Draw nodes cost single-click) |
| E3b page order | **LANDED** — enum `app/FroggersParameters.hpp:67-74`; `kBankLabels` deleted, labels now derived from `FroggersBankLayouts()` |
| E3c ASR labels | **LANDED** — `app/FroggersParameters.hpp:153-155` (`Atk1`/`Sus1`/`Rel1`…) |
| E3d scene controls | **LANDED** — `app/FroggersUiSurface.hpp:801`, handler `:870+` |
| E3e BPM | **LANDED, THEN SUPERSEDED** by E6-F2 (the label is a Slider label — never rendered) |
| E3f title text | **LANDED** — removed, note at `app/FroggersUiSurface.hpp:479` |
| E3g window sizing | **LANDED** — `config.uiHeight = 596` (`app/FroggersAppCore.hpp:177`) |
| E4 Crunchy voicing | **OPEN** — operator judgement |
| E6 (F1–F6) | **OPEN — this is the current work** |

**E7 (new, below) records the upstream answer task 6.4 was waiting on.**

---

## E1. Oscilloscope revival

**Traced.** Sheaf's `ScopeWriter` needs two feeds per sample: `Write()` to store the value, and
`AdvanceIndex()` to move the ring-buffer cursor (`AdvanceIndex` is literally `index_ += amount`,
`External/Sheaf/projects/synth/include/synth/DspScope.hpp:126-128`).

- Froggers calls `Write()` — `app/dsp/Vco.hpp:166`.
- Froggers calls `Publish()` per block — `app/FroggersAppCore.hpp:448`.
- Froggers **never** calls `AdvanceIndex()` — zero matches under `app/`, verified by grep.
- Braid 4 calls it once per sample — `External/Sheaf/projects/synth/apps/braid-4/Braid4Core.hpp:487`.

**Failure mechanism.** `index_` stays 0, so every `Write()` overwrites slot 0 and `Publish()`
always republishes `publishedIndex_ = 0`. `ScopeReader`'s no-marker fallback then computes
`endIndex_ == startIndex_ == 0` (`DspScope.hpp:266-267`), making `empty_` permanently true
(`:270,298`). `BuildScopePolylines` returns early on `reader.Empty()`
(`PortableUIBuilders.hpp:56-58`), so `BuildScopeWaveformCommands` emits only the background
`Fill` and midline `Line` (`:171-181`) and never a `Polyline`.

**Decision.** Call `vcoScopeWriter_.AdvanceIndex()` once per sample in `ProcessBlock()`'s
per-frame loop, mirroring Braid 4's placement.
**LANDED** — `app/FroggersAppCore.hpp:480` (re-verified 2026-07-28). The cursor now advances; E6-F4
is about what the advancing cursor *shows*, which is a separate defect.

**Secondary, deliberately separated.** Froggers constructs `ScopeWriter` with defaults
(`maxChannels=16, maxFrames=4096`, `DspScope.hpp:80`) at `app/FroggersAppCore.hpp:777`, while
Braid 4 passes explicit `{kOscillatorCount*2, kScopeFrames}` with `kScopeFrames = 6'553'600`
(`Braid4Core.hpp:35,707`). This is **not** causal — the index is stuck regardless of buffer size.
Do not bundle it into the fix; it is its own task (1.2) so that if the scope still misbehaves we
know which change did what.

---

## E2. DSP recovery architecture — the load-bearing decision

### E2a. The enumeration (verified)

| Unit | file:line | Reset path today | Reachable-unstable |
|---|---|---|---|
| `dsp::Vco` ×3 phases | `app/dsp/Vco.hpp:110-111` | none | no (WrapPhase/Sine01 bound it) |
| `VcoAdsrState` | `app/dsp/VoiceEnvelope.hpp:139-141` | `init()`, **sample-rate renegotiation only** | no |
| `Oversampler2x`, `SampleRateReducer`×2 | `app/dsp/Drive.hpp:119-121,153-155` | none | no |
| `DriveBlendPhase` allpass | `app/dsp/Drive.hpp:286-287` | none | no |
| `ResonantBump` ×2 biquad | `app/dsp/FilterFx.hpp:131`, recursion `app/dsp/DspMath.hpp:86-99` | **none** | **yes, empirically** |
| `Comb` delayLine/lowpass, fb ±1.1 | `app/dsp/FilterFx.hpp:244-249,299-307` | **none** | likely co-culprit |
| `PureDelay` | `app/dsp/FilterFx.hpp:315-317` | none | no (pass-through) |
| `StereoDelay` | `app/dsp/Delay.hpp:224-231` | `ClearBuffers()` via `SetSampleRate()`, **renegotiation only** | no (fb ≤0.98) |
| `Reverb` tank/damp/mod | `app/dsp/Reverb.hpp:77-94` | **no reset method exists at all** | no via legal params |

**Verified conclusion: there is no recovery mechanism in this instrument by any route.**
Re-verified 2026-07-28: `grep "void Reset()" app/dsp/` still returns nothing.
`PrepareToPlay()` (`app/FroggersAppCore.hpp:218-285`) touches only `audioAdsr_` and `delay_`;
`filterChain_`, `reverb_`, `drive_`, `driveBlendPhase_` and the VCOs get no reset even there —
and that path fires only on sample-rate renegotiation, never on user action.

### E2b. Why the obvious fix is wrong

The predecessor's handoff proposed guarding on `isfinite()`. **That would not have caught the
bug we actually reproduced.** The Randomize-All-then-Crunchy-max excursion pushed the Filter
stage past the ±8.0 output clamp (`app/FroggersAppCore.hpp:750`, re-verified 2026-07-28) to ≈8.7 at t≈7.35s, with
repeated multi-second clipping bursts over 60s — and `sawNaN` stayed **0 the entire time**. The
state was large, sustained, and audibly broken, but finite the whole way.

A guard keyed on non-finite values passes review, ships, and leaves this exact bug in place.

### E2c. Decision — two-tier recovery, magnitude and finiteness

**Tier 1 — finiteness (correctness).** Every unit in the table gains a `Reset()` that zeros its
recursive state. After each block, if any unit's state is non-finite, reset **that unit only**
and continue. This is a genuine recovery, unlike `SanitizeOutputSample`, which masks the symptom
at the output and lets the poisoned state keep producing.

**Tier 2 — magnitude (the reproduced bug).** Units whose state magnitude exceeds a sane ceiling
for a sustained period are also reset. This is the tier that catches the finite blowup, and it is
the one that needs care: the threshold must not fire on legitimately loud output.

**Thresholds are DERIVED, not measured (revised 2026-07-28, operator's point).** An earlier draft
proposed measuring the distribution of normal-operation state magnitudes and picking a threshold
from it. That is weak: a measured threshold is only as representative as the patches that happened
to be tested, and it cannot be shown correct. Digital audio has settled conventions here; use them.

**The signal bounds are fully determined by the code:**

- Input to the filter chain is bounded to **±1.0** by `PadeSaturator::Saturate`
  (`app/dsp/FilterFx.hpp:94-99` — a Pade rational saturation with an explicit
  `max(-1, min(1, ...))`).
- `ResonantBump` is an RBJ peaking EQ. Gain at centre frequency is `A²` where `A = sqrt(height)`
  (`app/dsp/FilterFx.hpp:164`), so gain **= height**, and height is
  `ExpMapCompute(1.0f, 10.0f, knob)` (`app/FroggersAppCore.hpp:665`, re-verified 2026-07-28) →
  **maximum 10× (+20 dB)**.
- `scoopNotch`'s height is `max(0.05f, 1.0f - 0.95f * scoop)` ∈ [0.05, 1] — a **dip**, so it
  attenuates and adds no gain.
- Therefore legitimate filter-stage magnitude is **≈10**, with headroom for resonant ringing say
  ≈30. Nothing musical approaches 100.

**Decision — per-unit state ceiling = 100.0** (40 dB above full scale). That is 10× above any
legitimate value, and 3.4e36 below float overflow. Divergence is **exponential**, so an unstable
filter crosses 100 within milliseconds: the guard fires effectively immediately on a real fault
and never on music. No measurement required, and the margin is auditable by anyone reading the
derivation above.

### E2c-bis. The ±8.0 output clamp is itself a defect

`SanitizeOutputSample` clamps to `kMaxOutputMagnitude = 8.0f`
(`app/FroggersAppCore.hpp:743-751`, still `8.0f` — re-verified 2026-07-28, task 2.8 has not run).

In float audio **1.0 is full scale (0 dBFS)** — the VST/AU/CLAP convention and what the audio
device expects. **8.0 is +18 dBFS.** The app therefore hands the device eight times full scale,
and the device hard-clips it to 1.0. Hard-clipping a signal at one eighth of its amplitude
produces a square wave. **This is a substantial part of the harsh "buzz" the operator reported**:
the clamp does not protect anything audible, it manufactures the worst-sounding possible failure
and passes it on.

On listener/speaker safety: actual SPL depends on the operator's volume control and hardware and
is not knowable from here. What *is* controllable is never exceeding 0 dBFS and avoiding
full-scale transients — which is precisely what a master limiter is for, and is standard in every
soft synth.

**Decision (operator, 2026-07-28): hard clamp at 1.0.** Change `kMaxOutputMagnitude` from `8.0f`
to `1.0f` and keep the existing `std::clamp`. Do **not** introduce a saturator on the output.

A soft limit via `PadeSaturator::Saturate` was offered and **declined**. The reasoning for the
choice, recorded so it is not re-litigated: a saturator would impose a tonal character the frozen
DSP never had, on every loud patch, muddying the parity story this port exists to preserve. A hard
clamp adds no tone — it only stops handing the device values it cannot represent. The device was
already hard-clipping these samples; the only change is that the clip now happens at the correct
threshold instead of eight times too high.

**What this does and does not fix.** It removes the 8× overshoot that turned any overload into a
square wave, which is the part of the reported "buzz" the output stage was responsible for. It
does **not** make overload pleasant — a signal driven past full scale still clips, because clipping
is what a hard limit does. The fix for *reaching* that state is the Tier 2 magnitude recovery
(E2c), not the output stage. These are complementary: Tier 2 stops the divergence, the clamp
bounds what escapes in the meantime.

**Preserve unchanged:** the non-finite → `0.0f` branch and the denormal flush. Only the constant
changes.

**Explicitly rejected.** Clamping divisors at `GetDelaySamples` / the bump coefficient update, as
the predecessor's handoff proposed. Audited and refuted 2026-07-27: all three divisors are
exponentially mapped between strictly positive endpoints
(`app/FroggersAppCore.hpp:653-685` via `ExpMapCompute`, `app/dsp/DspMath.hpp:43-46`,
`min * pow(max/min, v)` with `min > 0`), and knob values cannot leave [0,1] because `GetRaw`
routes through `ClampToRange`, which is an unconditional `std::clamp(v, 0, 1)`
(`External/Sheaf/projects/synth/src/ParameterModulation.cpp:449-452, 1207-1216`). Empirically
confirmed: `q ∈ [0.100, 1.000]`, `height ∈ [1.000, 2.749]`, `combFreq ∈ [4.17e-4, 5.76e-4]` —
all inside designed bounds throughout the failing run. **Those clamps would be dead branches**
(OMNI §12: verify real possibility, remove impossible branches).

### E2d. Reset granularity

Per-unit, not global. A global "reset all DSP" on any anomaly would audibly cut the reverb tail
and delay repeats every time one filter misbehaved. Resetting only the offending unit keeps the
rest of the signal intact.

---

## E3. UI decisions

### E3a. Bank selection — inverted colours, app-side  ⚠️ LANDED, THEN SUPERSEDED BY E6-F3.1

**Do not implement this section.** It shipped, and the Draw-node choice below cost single-click
dispatch, which the operator ranks above the cosmetics. Task 6.3 reverts it. The trace is kept
because 6.3 reuses it.

**Traced.** Bank labels were built as `label + " *"` when selected (former
`app/FroggersUiSurface.hpp:428-435`; that `kBankLabels` array no longer exists — labels now come
from `FroggersBankLayouts()`, and the selection predicate is `BankSelected()` at
`app/FroggersUiSurface.hpp:839-845`), then rendered via plain `builder.Button(...)`. This is a
**Sheaf-wide convention, not a Froggers quirk** — Braid 4 does the same
(`Braid4UiModel.hpp:388-399`).

Sheaf's `Node` already has a `selected` field, and `ButtonColourForNode`
(`PortableJuceBackend.hpp:1130-1148`) already inverts the *background* for it. But
`Builder::Button` (`PortableUIBuilders.hpp:300-306`) has no parameter to set `selected`, and
`TextColourForNode` (`:1109-1127`) never branches on it — so text never inverts even when
background does.

**Decision.** Render bank buttons as `Draw` nodes with `FillRoundedRect` + `Text`, choosing both
colours in app code. Precedent already exists in this file: Play/Stop are `Draw` nodes with a
post-`Build()` field patch (`SetNodeActionAndLabel`). Zero Sheaf changes.

The Builder gap is reported upstream (`UPSTREAM-SHEAF-ASK.md` item 3) so other apps can drop the
asterisk convention too — but we do not wait for it.

### E3b. Page order  ✅ LANDED

**Traced.** The order *was* Audio, Reverb, Filter, Drive, Delay, Envelope, encoded in **three
independent places**:
1. enum `FroggersBankId` — `app/FroggersParameters.hpp:67-74`
2. `FroggersBankLayouts()` array — `app/FroggersParameters.hpp:145-176`
3. `kBankLabels` string array — former `app/FroggersUiSurface.hpp:428-430` (hardcoded, **not
   derived** from 1 or 2 — this duplication was itself a defect)

**Outcome (re-verified 2026-07-28):** the enum now reads Audio, Envelope, Filter, Drive, Delay,
Reverb; `kBankLabels` is **deleted** — `grep kBankLabels app/` returns nothing, and
`WireDrawNodeActions` reads `FroggersBankLayouts()[bankIx].name`
(`app/FroggersUiSurface.hpp:741-746`). The third copy is gone, as OMNI §8 required.

Target order (operator): **Audio, Envelope, Filter, Drive, Delay, Reverb** — signal path.

**Risk, traced.** Every consumer references banks symbolically via `FroggersBankId::X`, never by
numeric literal (checked across `app/FroggersAppCore.hpp:554-679` and all `*Tests.cpp`), so
renumbering is internally consistent.

**~~`[UNVERIFIED]`~~ — RESOLVED before 3.2 executed:** persistence is name-keyed
(`ParameterValuesToJSON`), so bank index is not encoded numerically and the reorder is safe. The
reorder shipped; the operator's saved patches survived it.

**Decision.** Reorder all three sites together, and **derive `kBankLabels` from
`FroggersBankLayouts()`** rather than leaving a third parallel copy (OMNI §8: 2+ occurrences of
the same conceptual list must be single-sourced).

### E3c. ASR labels — a truncation bug, not a naming problem  ✅ LANDED

**Traced.** The names were *already* VCO-numbered: `"Attack VCO1"` / short `"AtkV1"` (now
`"Atk1"`, `app/FroggersParameters.hpp:153-155`). The defect is that `EncoderDraw` renders
`UpperShortLabel(state.shortLabel)` (`EncoderDraw.hpp:790`) with a hard 4-char truncation
(`:571-582`, default `maxChars=4`). So `AtkV1 → ATKV`, `SusV1 → SUSV` — **the VCO digit, the only
distinguishing character, is exactly what gets cut.**

**Decision.** Short names become `"Atk1"/"Sus1"/"Rel1"` etc. — exactly 4 chars, digit survives.
No Sheaf change. Safe: only `name` carries the global-uniqueness constraint
(`ParameterModulation.cpp:3069-3071`); `shortName` has none.

### E3d. Scene controls  ✅ LANDED (`app/FroggersUiSurface.hpp:801`, handler `:870+`)

**Traced.** S1/S2 dispatch `kSceneSelect` → `MessageIn::SceneSelect` →
`ParameterManager::SetLessSelectedScene` (`ParameterModulation.cpp:3336-3341`), which is
`if (blend<=0.5) SetSceneEndpoints(leftScene, sceneIx) else SetSceneEndpoints(sceneIx, rightScene)`.
That reassigns **which stored scene occupies the less-weighted endpoint** and never moves the
blend. At extreme blend positions, clicking therefore does nothing audible — precisely the
operator's "inconsistent" report.

**Decision.** S1/S2 become a toggle between blend extremes: push `SetSceneBlend(0.0f)` /
`SetSceneBlend(1.0f)`. Relabel to "Scene 1"/"Scene 2", label the slider "Scene blend", drop the
float readout. Sheaf's `SetSceneBlend` already exists and is currently unused for this purpose —
no Sheaf change.

**Note for the implementer:** this deliberately diverges from Braid 4's inherited convention
(`app/FroggersParameters.hpp:189-191`). That is an intentional product decision, not an
oversight — do not "restore" it.

### E3e. BPM — wired correctly; a discoverability problem  ⚠️ PARTLY SUPERSEDED BY E6-F2

The wiring analysis below stands and is still the reason **no wiring change** is wanted. What is
superseded is the `[UNVERIFIED]` label conflict at the end of this section: E6-F2 settles it from
the backend code — the label is a *Slider* label, and Slider labels are never drawn. The operator
was right; the code reading was right about the field and wrong about the pixels. Task 6.6 owns
the fix, and task 3.6 was closed on a field assertion it should not have been closed on.

**Traced, and it works.** `RequestTempoBpm` → `app/FroggersAppCore.hpp:353-354` →
`MasterClock::SetTempoBpm` → `pendingQuarterNotesPerSample_ = bpm/(60*sampleRate)`
(`MasterClock.cpp:963-969`) → `TransportQuarterNotesAt` (`app/FroggersAppCore.hpp:394-401`) →
gates `audioAdsr_.setGate()`. **BPM does change the D17 ASR gate rate.** It no-ops only when
`syncConfig_.receiveClock` is set, i.e. when an external clock is expected.

**Answering the operator's question directly: no, BPM is not irrelevant without a MIDI clock.**
It sets the internal tempo. But it has **zero audible effect while the transport is stopped**
(the gate is closed outright then — comment at `app/FroggersAppCore.hpp:392`), which is the
likely source of "doesn't seem to do anything."

**~~`[UNVERIFIED]` conflict~~ — RESOLVED, and not in the code's favour.** The control carries a
label string (`app/FroggersUiSurface.hpp:834-836`, `"BPM"` / `"BPM (no effect while stopped)"`)
that the backend **never draws**: `NodeKind::Slider` routes `node.label` to
`juce::Slider::setName()` (`External/Sheaf/projects/synth/juce/PortableJuceBackend.hpp:1231`,
re-verified 2026-07-28 — still true at upstream `origin/main` too, see E7). The operator's report
was correct. See E6-F2.

**Decision.** No wiring change. Improve discoverability — annotate or grey the BPM control while
the transport is stopped, **as a rendered `Label` node** (task 6.6), not as a Slider label.

### E3f. Title text  ✅ LANDED

**Traced.** On-canvas label `"Frogg3rs Synth"` at former `app/FroggersUiSurface.hpp:347` (removed;
the note explaining the removal is at `:479`). Distinct from
`config.appName` (`app/FroggersAppCore.hpp:135`) and `FroggersManifest().displayName`
(`app/FroggersRegistration.hpp:22`), which are launcher/window-title metadata and are **not**
the on-canvas text.

**Decision.** Remove the canvas label only. Leave the metadata alone — the launcher entry still
needs a name. Logo deferred (see proposal, Out of scope).

### E3g. Window sizing  ✅ LANDED (`uiHeight` now 596, `app/FroggersAppCore.hpp:177`)

**⚠️ Re-opens if task 6.6 lands.** 6.6 adds two `Label` nodes to the same auto-flowed chrome band
whose height this section computes. Labels consume flow width and can push the band onto an extra
row. **6.6 must re-run 3.7's height derivation and its `uiHeight >= computed extent` test**, not
assume 596 still holds.

**Traced.** `Config()` set `uiWidth=900, uiHeight=560` (now `:158` and `:177`).
`RuntimeShellSession` (`Shell.hpp:86-88`) calls `setSize` from `MainPane::IntrinsicBounds()`
(`MainPane.hpp:72-74`) → `RuntimeMainComponent::IntrinsicBounds()`
(`RuntimeMainComponent.hpp:204-210`) = `{uiWidth + Layout::kSidebarWidth, uiHeight}`.

So the window **does** derive from `Config()` — a sidebar width is added horizontally, and height
is `uiHeight` with **zero slack**. The app's chrome band (bank row, scene/BPM controls) is
auto-flowed by generic runtime chrome outside `FroggersPageLayout`'s content area
(`app/FroggersUiSurface.hpp:33-34`), so its height is not counted — exactly the observed
bottom-row clipping.

**Decision.** Prefer computing the required height from the layout rather than hardcoding a
larger magic number, with a test asserting `uiHeight >= computed extent`. If the chrome band's
height genuinely cannot be queried, raise the constant and document the measurement — but attempt
the derived version first.

---

## E4. Crunchy voicing — not a defect

**Traced.** Crunchy swept to max drops RMS 0.29 → ~0.01 and holds. Never non-finite, never
exceeds the clamp. Real signal, drastically attenuated by Fuegoize's chaotic scramble at
`fuegKnob=1`.

This is **distinct from both** the blowup and from legitimate silence (e.g. all three ASR
sustains at 0 gives rms≈0.005 from sample 0, with no preceding loud stretch — a different
signature).

**Decision.** Do not "fix" it as a bug. It is a by-ear voicing judgement in the same class as
inherited tasks 8.6 and 12.6: is max Crunchy *supposed* to be near-silent chaos? The operator
decides. Scoped as task 4.1, and it may legitimately resolve as "working as intended."

---

## E6. 2026-07-28 operator GUI test — SUPERSEDING FINDINGS

The operator ran the build after dispatches 1–4. Result: cosmetics landed; core functionality is
worse than the archived change's own smoke report. These findings supersede E1's "fixed" status
(partially), E3e's BPM conclusion (wrongly closed), and parts of E3a's tradeoff. Every claim cited.

### F1. Default patch is inaudible on the operator's hardware — root cause of "Play emits silence"
`Vco::PitchToPhaseIncrement(0, sr) = ExpMapCompute(20/sr, 20000/sr, 0)` = **20 Hz**
(`app/dsp/Vco.hpp:118-121`, re-verified 2026-07-28). `ApplyFroggersDefaultPatch`
(`app/FroggersModulation.hpp:916-964`, called once from `Init()` at `app/FroggersAppCore.hpp:203`)
sets shapes (Audio slots 3-5), cross-VCO pitch-mod detents, Drive 20% — and **never touches the
pitch knobs (Audio slots 0-2)**, which therefore sit at their 0.0f registration default (the
`{"VCO1","VCO1"}` specs at `app/FroggersParameters.hpp:147-149` declare no `defaultValue`, unlike
the Sustain specs at `:153-155` which declare `1.0f`). Three
20 Hz oscillators through laptop speakers (operator's device: MacBook Air speakers, per the
session log) ≈ silence. Randomize moves the pitch knobs → audio appears. Matches the operator's
repro exactly. Sustain is NOT the cause: `.defaultValue = spec.defaultValue` flows at
`app/FroggersParameters.hpp:327` and the 1.0f sustains moved correctly with the bank reorder.
**Decision:** default pitches must land in laptop-audible range. knob = ln(f/20)/ln(1000);
e.g. 110 Hz → 0.2468, 220 Hz → 0.3471, 330 Hz → 0.4058. Operator directive (2026-07-28): audit
EVERY default and range for audibility — sustain, filter, delay/reverb wet semantics included —
not just pitch. The acceptance proxy: a fresh-state headless run of the default patch must show
energy above ~150 Hz, not merely nonzero RMS (nonzero RMS at 20 Hz is precisely the bug).

### F2. Slider labels are NEVER RENDERED by the backend — reopens 3.6, invalidates its closure
`PortableJuceBackend.hpp:1224-1232`: a Slider node's label goes to `juce::Slider::setName()`,
which **draws nothing**; no Label component is attached. So "BPM", "Scene blend", and 3.6's
"(no effect while stopped)" annotation are all invisible. The operator reported this correctly
and was told the code was right; the agent verified the node FIELD, not the RENDER. Every
label-bearing check from here on must verify pixels, not fields.
**Decision:** render labels app-side as adjacent `Label` nodes — Label nodes demonstrably render
(the removed canvas title did).

### F3. Stop: three stacked problems
1. Single-click on Play/Stop dispatches nothing (Draw nodes; `doubleClickAction` only at this
   pin). Same for bank buttons — the operator cannot switch pages by clicking. The E3a tradeoff
   (full colour inversion via Draw nodes, at the cost of double-click) is REVERSED for banks:
   **function over cosmetics.** Bank buttons return to `Button` nodes with a post-`Build()`
   `node.selected = true` patch — `ButtonColourForNode` already inverts the background for
   `selected` (`PortableJuceBackend.hpp:1130-1148`); text inversion waits for upstream. Play/Stop
   stay Draw (icons) ONLY if upstream plain-click lands immediately; otherwise they too revert to
   labelled Buttons until it does.
2. Even a dispatched Stop (`FroggersUiSurface.hpp:864-868`) only closes the ASR gate. **No tail
   kill exists**: randomized Delay feedback (≤0.98) / Reverb Hold sustain indefinitely and no unit
   has a Reset() (E2a's table — that gap is now user-facing, not just fault-recovery).
   **Decision:** Stop closes the gate AND flushes delay/reverb tails via the per-unit Reset()
   from task 2.2. If the operator later wants ring-out, that becomes an option, not the default.
3. A dispatch-path bug on top cannot be excluded without GUI verification — open.

### F4. The scope implementation is wrong in kind, not merely unpolished

**Revised by the 2026-07-28 audit.** The defect list below is confirmed. The *remedy* originally
written here — "abandon the generic ScopeVisualizer and hand-build Draw commands like Braid 4" —
is **withdrawn**: it was not required by any of the operator's four requirements, it contradicts
this change's own `froggers-vco-topology` delta ("no bespoke waveform rasterization exists in the
app"), and it is more work than the alternative. See F4-bis for the replacement decision.

**The four defects, all confirmed:**

- **Tap point.** `dsp::Vco::Process` writes the scope BEFORE the ASR gate is applied downstream
  (write at `app/dsp/Vco.hpp:164-167`; gating at `dsp::MixOscVoices`,
  `app/dsp/VoiceEnvelope.hpp:164-167`, called from `RouteAudioSample`,
  `app/FroggersAppCore.hpp:615-619`), so scopes show free-running oscillators during silence.
  Operator is right: lines moving while inaudible is a broken visualization.
- **No cycle alignment.** The visualizer is constructed `drawMarkers=true`
  (`app/FroggersAppCore.hpp:120-122`) but `RecordStart`/`RecordEnd` marker recording was never
  ported (`app/dsp/Vco.hpp:47-52`, explicitly skipped) — unaligned 512-sample windows at 30 fps
  read as chaos. The E1 fix made the cursor advance; it did not make the display meaningful.
- **One band, not two.** `app/FroggersAppCore.hpp:120-122` builds a single 3-layer
  `ScopeVisualizer` over the VCO UI states; there is no envelope-follower band at all.
- **Braid 4 comparison, traced:** Braid 4 does NOT use the generic ScopeVisualizer for its band —
  it hand-builds Draw commands from its own scope state, with SEPARATE VCO and LFO stacks
  (`apps/braid-4/Braid4UI.hpp:41-53`). That is *how Braid 4 did it*, not a constraint on us.

**Operator requirement (2026-07-28), unchanged:** TOP band = audio-rate signal, post-gate
(silence ⇒ flat line); BOTTOM band = the three LFO-rate envelope followers; trace colours
**blue / yellow / magenta**.

**Citation correction (audit).** The design previously located the envelope followers at
`app/FroggersAppCore.hpp:593`, computed at `:389-390`. **Wrong file.** They are
`vcoEnvelopeFollowers_` at **`app/FroggersModulation.hpp:593`**, stepped at
**`app/FroggersModulation.hpp:389-390`** into `vco1EfSource_`/`vco2EfSource_`/`vco3EfSource_`
(`:391-393`, declared `:642-644`). Two consequences the implementer must not discover the hard way:

1. They are **private members of `FroggersModulationSlate`**, reachable only through
   `FroggersAppCore::Modulation()` (`app/FroggersAppCore.hpp:530`). Wiring them to a scope needs
   either an accessor on the slate or the write to happen inside `Step()`. Pick one deliberately.
2. They follow the slate's **own private VCOs** (`vco1_`/`vco2_`/`vco3_`), which are a *separate
   set of instances* from the audio-path VCOs — stated explicitly at
   `app/FroggersAppCore.hpp:221-226`. They are driven from the same Audio-bank knobs
   (`app/FroggersAppCore.hpp:437-443`), so they track the audio VCOs, but they are **not gated**
   and they are not the samples the operator hears.

**⚠️ The existing EF taps are the wrong feed for a scope — same defect as the top band.**
(Revised after operator challenge, 2026-07-28: the first version of this note blamed the follower
and asked which signal the bottom band should carry. That was the wrong diagnosis. The follower is
fine; it is tapped in the wrong place, exactly like the top band.)

`VcoEnvelopeFollowers::Process` (`app/dsp/EnvelopeFollowers.hpp:45-55`) is a standard asymmetric
rectify-and-smooth: `target = clamp(|v|,0,1)`, attack coeff when rising, release when falling,
attack 0.01 s / release 0.05 s (`:34-39`). Nothing wrong with it. The problem is what it is fed:

1. **Its input has no amplitude term.** `dsp::Vco::Process` returns
   `EvalWaveMorph(modulatedPhase, morphKnob01)` (`app/dsp/Vco.hpp:149-159`) — a pure function of
   phase. Pitch sets the increment, PM offsets the phase; **neither scales the output.** A VCO
   here has constant amplitude by construction.
2. **It is tapped pre-gate.** `vcoEnvelopeFollowers_.Process(vco1Raw, vco2Raw, vco3Raw, …)`
   (`app/FroggersModulation.hpp:389-390`) runs on the raw oscillator, before `MixOscVoices` applies
   the ASR (`app/dsp/VoiceEnvelope.hpp:164-167`). The note envelope — the only thing that would
   give these a contour — is not in the input.

Constant amplitude in, constant level out. The smoothing then removes the waveform too: release
τ = 50 ms against a 9.1 ms period at 110 Hz, so the level droops only `exp(-4.55/50)` ≈ **8.7%**
between rectified peaks (≈3% at 330 Hz). Displayed: a line at a fixed height with a few percent
wobble. It is not frozen — Shape moves it (sine `|mean|` = 0.637·A vs square = A, ≈1.6×), as does
PM depth and anything modulating those knobs — but on a sustained patch with static knobs it is
flat.

**Decision: feed the display followers the post-gate per-voice signal** — the same three values
6.7c already extracts for the top band. The bottom band then shows the actual ASR contours, and
the two bands acquire an honest relationship: one signal, at audio rate on top and as its envelope
below. No operator decision is needed for this; it is the same tap-point fix applied twice.

**Constraint — do NOT re-tap the existing followers.** `vco1EfSource_`/`vco2EfSource_`/
`vco3EfSource_` (`app/FroggersModulation.hpp:391-393`) are registered **modulation sources**
(design D5 slots 9-11). Changing their input changes what the modulation slate hears — a DSP
parity break in service of a display change. The scope band needs its **own** followers.
`dsp::SingleEnvelopeFollower` (`app/dsp/EnvelopeFollowers.hpp:66`) is already the one-channel
form of the identical ported formula, so this is three instances of existing code, not new DSP.

**The TWO-identical-panels report — two code-side candidates, check these before speculating.**
The surface builds exactly ONE `Visualizer` node for the scope (`app/FroggersUiSurface.hpp:526`),
and `ScopeVisualizer::DrawVisible` overlays all three layers inside one bounds
(`External/Sheaf/projects/synth/include/synth/PortableUIBuilders.hpp:241-259`), so one node
cannot legitimately paint two panels. Candidates, in order of likelihood:
  1. **Coordinate-space mis-classification.** `RetainedDrawComponent::SetNode` guesses whether
     commands are node-local or surface-absolute via `DrawCommandsLookLocal`
     (`PortableJuceBackend.hpp:122`, called at `:503`). A wrong guess paints the panel's
     background `Fill` (`PortableUIBuilders.hpp:170`) offset from the component, which reads as a
     second dark rectangle beside the real one.
  2. **Encoder-cell visualizer underlays** (`app/FroggersUiSurface.hpp:662-668`) — the
     transfer-function/modulation underlays are also dark rectangles and sit in the grid.
Look at the GUI to decide between them; the point of listing them is that the look is now
diagnostic rather than open-ended.

### F4-bis. Decision — keep the generic visualizer; fix the feeds (audit, 2026-07-28)

All four operator requirements are reachable **without any app-side rasterization**, which keeps
the `froggers-vco-topology` requirement intact and is strictly less code than the Braid 4 pattern:

| Requirement | Mechanism | Evidence it exists at pin `1940ddcb` |
|---|---|---|
| Top band post-gate | Move the scope `Write()` off the raw VCO output onto the post-ASR per-voice value | `VcoAdsrState::apply` is public and already computes the three gated values inside `MixOscVoices` (`app/dsp/VoiceEnvelope.hpp:164-167`) |
| Two distinct bands | A **second** `ScopeWriter` + 3 layer states + a second `builder.Visualizer(...)` node | `ScopeVisualizer` is a template over any layer-state shape (`PortableUIBuilders.hpp:225-232`); `Builder::Visualizer` takes any number of nodes |
| Blue / yellow / magenta | Per-layer `scopeColor` | already a field of the layer state (`PortableUIBuilders.hpp:154-159`) |
| Stable, cycle-aligned | Port the skipped marker recording | `ScopeWriterHolder::RecordStart/RecordEnd` exist and need no Sheaf change (`DspScope.hpp:30-31, 242-251`); the visualizer is already `drawMarkers=true` |

**On not duplicating the envelope math (OMNI §8).** The three post-gate values live inside
`MixOscVoices`, which returns only their mean. Do **not** re-apply `adsr.apply` in
`RouteAudioSample` to recover them — that duplicates a ported formula and creates two call sites
that can drift. Give `MixOscVoices` an optional out-parameter for the three gated voices and have
the single existing call site pass it. That keeps one evaluation of the envelope, which is what
§8 asks for; a shared helper invoked twice would not.

**If, after the operator answers the open question above, the requirement genuinely cannot be met
by the generic visualizer**, then the `froggers-vco-topology` delta must be amended in the same
commit that introduces app-side drawing. Shipping code that contradicts a delta spec in its own
change is not an option.

### F5. Random S&H "dice roll" motion absent on mod pages
`FroggersRandomShVisualizer.hpp` exists, is included (`FroggersModulation.hpp:124`) and tested —
but whether it is ever attached to mod-page encoder cells (`slotState.cells[ix].visualizer`) is
unverified. Suspected further instance of the dropped-call-site class. The 1.4 call-site sweep —
the class fix — was planned and NEVER EXECUTED; it now runs before any further feature work.

### F5-bis. The S&H attachment — HYPOTHESIS REFUTED by the 6.1 sweep (2026-07-28)

F5 guessed that the missing dice-roll animation was a fourth instance of the dropped-call-site
class. **It is not.** The sweep traced the chain and the lead re-verified each link:

| Link | Evidence |
|---|---|
| Type exists, 5 instances | `app/FroggersModulation.hpp:631-635` — note the real name is **`RandomShLaneVisualizer`**, not `RandomShVisualizer`; the earlier note had it wrong, which is why the grep behind it came up empty |
| Bound to their UI states at construction | `app/FroggersModulation.hpp:232-236` |
| Registered into the slate with per-lane colours | `:504-511` |
| UI state populated **every block** | `:432`, inside `PublishUiState()`, called from `app/FroggersAppCore.hpp:494` |
| Pointer written at parameter registration | `app/FroggersParameters.hpp:329` (`.visualizer = visualizer`) |
| Pointer read and rendered | `app/FroggersUiSurface.hpp:648`, `:662-668` |

Nothing is dropped. The wiring is complete end to end, and it covers all five Random S&H lanes
plus Noise.

**So the symptom needs a different explanation.** The leading hypothesis is that F5 is not an
independent defect at all, but a **consequence of F3.1**: modulation detail pages are reached by
drilling in through an encoder press, and encoder press is double-click-only at this pin (E7). An
operator who reported being unable to change pages by clicking may simply never have reached the
page these visualizers render on. That is a hypothesis, not a finding — it is testable for free
once 6.3/6.4 land and the operator knows to double-click. Task 6.8 is re-scoped to that
observation and explicitly told not to write code first.

**Method note worth keeping.** F5 was written as "suspected, cell attachment unverified" and read
downstream as very nearly established. One grep against a wrong type name was the whole basis.
This is the cheap end of the same failure as F2 — believing a claim about the code without having
verified the specific thing being claimed.

### F7. New defect found by the sweep — no operator report behind it (2026-07-28)

`ParameterGroup::ConfigureProcessingTiming` (`External/Sheaf/…/src/ParameterModulation.cpp:859-865`)
is **never called by this app**. Verified: `grep -rn ConfigureProcessingTiming app/` returns
nothing; Braid 4 calls it on all three of its groups from its sample-rate hook
(`Braid4Core.hpp:218-220`).

Why it matters: Sheaf's parameter-smoothing constants are **defined at a 48 kHz reference** —
`kDefaultProcessLiteAlpha` is commented "1 kHz one-pole cutoff at 48 kHz",
`kDefaultUiDisplayCenterAlpha` "about 10 Hz at 48 kHz"
(`External/Sheaf/…/ParameterModulation.hpp:170-173`) — and `ParameterConfig` initialises to exactly
those values (`:199-202`). `ConfigureProcessingTiming` is the only thing that replaces them, and
Braid 4 feeds it values rescaled through `ConvertOnePoleAlpha`/`ConvertSampleInterval`
(`Braid4Core.hpp:205-217`). Never calling it means knob glide, modulation-depth smoothing and
UI-display slew all run at the wrong real-time rate whenever the host is not at 48 kHz: ~9% off at
44.1 kHz, **2× off at 96 kHz**, 4× at 192 kHz.

This is exactly what the sweep existed to find — a faithfully copied structure with a dropped
configuration call, invisible to unit tests, and invisible to the operator too at the sample rate
they happen to run. Owned by task 6.11, together with the `SetVoiceColor` colour mismatch on the
"Random S&H 6" lane.

### F6. Process failure that let all of this ship
Dispatch 4's agent could not visually verify (locked screen) and said so; the lead accepted the
dispatch and told the operator the build was ready anyway. That is task 12.2's sin, recommitted
the same day it was archived. Binding rule going forward: **a task whose spec says GUI
confirmation is not closeable by anyone who has not seen the pixels.** The operator seeing it
first counts as the postflight failing, not as testing.

## E7. Upstream status — CHECKED 2026-07-28, question closed

The handoff's "suggested first hour" item 1 (did jvictor0's plain-click Draw dispatch land?) is
**answered: NO.** `git fetch origin` in `External/Sheaf` moved `origin/main` from `1940ddcb` to
**`1dd4d275`**. In that tree, `RetainedDrawComponent` still overrides only `mouseDown`,
`mouseDrag` and `mouseDoubleClick`, and still dispatches solely from
`acceptsDoubleClick_ && doubleClickDispatch_`. There is no plain-click path for Draw nodes.
The only two commits touching the backend/UI headers in that range are `8253b923`
("suppress disabled portable actions") and `d166b799` ("stabilize cached controller discovery").

Also checked in the same tree, because the other two `UPSTREAM-SHEAF-ASK.md` items depend on them:

- `TextColourForNode` (`origin/main:…/PortableJuceBackend.hpp:1119-1138`) still has **no
  `node.selected` branch** — selected-button *text* inversion has not landed. `ButtonColourForNode`
  (`:1140`) does invert the background, so 6.3's background-only inversion is available today.
- `NodeKind::Slider` still calls `slider->setName(node.label)`
  (`origin/main:…:1242`) — **Slider labels still never render**. 6.6's Label-node workaround stays
  necessary.

**Consequences, binding on tasks 6.3 and 6.4:**
- 6.4 takes its **"NOT landed" branch**: Play/Stop revert to labelled `Button` nodes. No pin bump,
  so no operator approval is needed for one. Keep the Draw-command icon builders in the file —
  they are the implementation to restore when plain-click does land.
- 6.3 proceeds as written: `Button` nodes + post-`Build()` `node.selected`.

**⚠️ Consequence the handoff and §6 both missed: the encoder grid has the same disease.**
Every encoder cell is a `DrawInteractive` node whose press action is wired through
`SetNodeAction`, which sets `doubleClickAction` (`app/FroggersUiSurface.hpp:701-708`, applied at
`:728-731`). So **encoder press / modulation drill-in is double-click-only too**, exactly like
Play/Stop and the bank buttons. Unlike those two, encoders **cannot** revert to `Button` nodes —
they are custom-rendered via `BuildEncoderDrawCommands` and need bounds, which `Button` nodes do
not have (`app/FroggersUiSurface.hpp:18-27`). At pin `1940ddcb` this is unfixable app-side.
It is therefore a **known, accepted limitation**, not an open task: recorded here, tracked as
`UPSTREAM-SHEAF-ASK.md` item 1, and it must be stated to the operator rather than left to be
rediscovered as "the encoders don't work either". Task 6.10 records the disclosure.

## E5. Verification design — the lesson from the predecessor

The predecessor shipped two invisible defects behind 120 green tests. Both were invisible to unit
tests **by construction**. Therefore:

1. **No task in this change is complete on unit tests alone.** Every user-visible task carries a
   GUI confirmation step.
2. **Audio tests must drive the real path.** The existing repro
   (`app/FroggersRandomizeAllReproTests.cpp`) drove a **shadow copy** of `RouteAudioSample()` —
   formulas copied into separate instances. That is the predecessor's trap #1 repeated, and it
   makes those numbers indicative only. Task 2.1 rebuilds the repro against the real
   `Engine`/`Runtime`.
3. **The call-site sweep (task 1.4) is a first-class deliverable**, not cleanup. It is the only
   task that addresses the *class* rather than the instances.

## Context

Froggers desktop v2 inverts Sheaf’s host/app split: [`Main.cpp`](desktop-v2/Source/Main.cpp) + [`MainComponent`](desktop-v2/Source/MainComponent.cpp) own devices, runtime pages, and product chrome, while [`FroggersV2AppCoreFacade`](desktop-v2/Source/control/FroggersV2AppCoreFacade.hpp) only partially mirrors a Sheaf app core. Sheaf Miniapp/Braid prove the target: `SYNTH_RUNTIME_MAIN(App)` → `Runtime` (JUCE) → `Engine<App>` → `PortableSurface`.

Operator decisions (2026-07-18/19): use Runtime as the JUCE wrapper; dual Sheaf-style scopes; encoder visualizers; 2-deep mod pages; **delete** Random S&H module page and bag params (Sheaf has none); absorb unified-parameter-layout; **no MIDI mapping** yet; VST later; LFO EF viz uses Sheaf `ScopeVisualizer` path (no tap spike).

Constraints: [`froggers-v2-sheaf-runtime`](openspec/specs/froggers-v2-sheaf-runtime/spec.md) requires **local ownership** of any Sheaf scaffolding (no network FetchContent of Sheaf). OMNI: one data-flow pipeline; no parallel scope stacks; no parallel encoder bank models.

**Subagent model selection (execution, `omni-rule.md` §4/§15):** dispatch each subagent on the least powerful model that can do its bounded role, set explicitly — an omitted model inherits the session's expensive one. Cheap/mechanical tasks (running a build, grepping, applying a scoped edit from an immutable proposal) → Haiku; tasks needing real reasoning within the bounded role → Sonnet; reserve the top model for lead-agent work. "The dumber the task, the dumber the model."

## Objective (OpenSpec §2)

Desktop Froggers v2 boots as a Sheaf-style `SynthApplication` under an in-tree vendored Runtime, with dual multi-layer scopes, encoder visualizers, hard 2-deep mod drill-in, Random S&H as mod lanes only (module page + bag params deleted), absorbed unified layout (no carousel; ASR Envelope; cross-couplers removed as redundant; VCO morphs labeled "Shape"), ≤16 encoder bank with stable Crunchy/Crispy, and Froggers rand toggle/hold — without MIDI mapping or VST host work.

## Data-flow + definition-site TRACE (OMNI §1 / §14)

This section is the preflight deliverable. Claims cite file:line verified 2026-07-20.

### Pipeline (target after cutover)

```text
audio device / MIDI / config
        │
        ▼
Runtime (JUCE) ── MainPane: File / Audio / Controllers shell
        │ pumps blocks + UI timer
        ▼
Engine<FroggersApp> ── FroggersApp::ProcessBlock → existing AudioEngine DSP
                    ── PortableSurface → Application page
                         ├─ dual ScopeVisualizer panels (VCO trio | LFO EF trio)
                         ├─ ≤16 encoder bank / 4×4 (sparse OK)
                         └─ mod layer-1 (16-cell detail) only
UI Action / MessageIn ──► FroggersV2ControlCore (single authority)
Scope samples ──► ScopeWriter ──► ScopeVisualizer layers
```

### Pipeline (today — verified)

| Stage | Enter | Transform | Exit | Cite |
|-------|-------|-----------|------|------|
| Shell entry | JUCE app | `MainWindow` owns `MainComponent` | product UI root | `desktop-v2/Source/Main.cpp:44` |
| Audio host | `MainComponent` ctor | `std::optional<AudioEngine> m_audio` + facade | running engine | `MainComponent.h:48`, `MainComponent.cpp:36-40` |
| Device manager | `AudioEngine` | owns `juce::AudioDeviceManager m_deviceManager` | device I/O | `AudioEngine.h:138` (shell owns engine → owns devices; not a separate MainComponent field) |
| Runtime chrome | File/Audio/MIDI buttons | show `FilePatch` / `AudioRuntime` / `Controllers` pages | overlay pages | `MainComponent.h:58-65`, `MainComponent.cpp:96+` |
| Module nav | page select | `PageCarouselComponent m_carousel` | one module page visible | `MainComponent.h:54` |
| Scope viz | CV history + taps | `GlobalOscilloscopeDisplay` + `kOscilloscopeTaps` (3× VCO EF) | single global scope | `FroggersV2AppManifest.hpp:589-593`, `GlobalOscilloscopeDisplay.cpp:10` |
| Control | `MessageIn` | `FroggersV2ControlCore` | param/mod/rand state | `FroggersV2ControlCore.cpp` (`onModDrillIn` @711) |
| Encoder bank | physical slots | `kEncoderCount = 10` | slot→row map | `FroggersV2ControlCore.cpp:14` |
| Mod detail | drill-in | `m_modView` + 16 cells (`kModDetailCellCount`) | depth edit | `FroggersV2ControlCore.hpp:34-36`, manifest `kModDetailCellCount` @498 |
| Build Sheaf | n/a | product CMake FetchContent is **JUCE only** today | no Sheaf vendor yet | `desktop-v2/CMakeLists.txt:14-25` |

### Definition sites (page / Random / scope / rand / mod)

| Concept | Sites (enumerate all) | Disposition this change |
|---------|----------------------|-------------------------|
| Host page count/labels | `sim/V2ParamDisplayNames.hpp:10` (`kV2NumHostPages=7`); labels `27-29` + Pair-AR @52-56 | Drop page index 1; re-index; rename Pair-AR→Envelope |
| Page row names | `kLegacyHostRowGrid` @43-50; Pair-AR rows @31-33; expansion tails @35-41 | Delete Random grid row @45 + tails index 0 `{"Spread","Bias","Crispy"}` @36 |
| Row counts | `rowsForPage` @1184-1199 (`0→8`, `6→7`, else `10`) | Recompute after page deletion |
| Crispy rows | `CrispyRowForPage` @13-20 | Rebuild table without Random page |
| Mod sources Random S&H 1/2 | `kPermanentModulationSources` @490-491 | **Keep** |
| Oscilloscope taps | `kOscilloscopeTaps` @589-593 | Retire as **sole** viz; dual panels replace |
| Scope UI | `GlobalOscilloscopeDisplay` (source groups include LFO @55) | Replace sole-panel role with dual `ScopeVisualizer` |
| Cross-coupler | manifest JSON emit `vco_12`,`vco_23` @1026; UI label single bipolar `"Cross-coupler"` @44; DSP `XCPL` param @[`FroggersEngine.hpp:544`], producing `c12`/`c23` that scale the PM phase offsets @[`FroggersEngine.hpp:635-638`] | **REMOVE** entirely (see D11 — redundant with PM). Delete the `XCPL` Audio param + the `c12`/`c23` coupling terms + the two-ID emit + the bipolar UI row. Do NOT add three explicit couplers (reverses the superseded 2026-07-21 "three explicit" direction). |
| Rand authority | `executeRandomization` @268+; `RandPage` @933+; UI `GlobalStripV2.cpp:122-142` | Reuse; add toggle/hold arm; no parallel mutator |
| Mod drill-in | `onModDrillIn` @711-734 — **no** `m_modView.open` reject today | Add hard gate: refuse nested drill from depth cells |
| Carousel | `PageCarouselComponent` on `MainComponent` / `HostedMainComponentV2` | Retire primary pager |
| Host inventory comment | `HostParameterInventoryV2.hpp:13` — was 142 pre-change; **now 120** in the header comment but effective count is **126** (post-Random −20, post-ASR-sustain +6; `FROGGERS_EXPECT_HOST_PARAM_COUNT_V2 == 126`). Comment at `:13` is stale (still says 120). | Re-derive from the `ParameterGroup`/`Bank` graph (§13.9); drops again when Sequencer/GestureWeight axes are removed. Fix the stale `:13` comment. |

**Reuse vs creation:** Vendor Sheaf Runtime/Engine/PortableUI/`ScopeVisualizer`/`GangedRandomLfoVisualizer` once in-tree. Keep AudioEngine + control core. Do not add a second scope stack beside `GlobalOscilloscopeDisplay` — replace its sole-viz role. Do not add a second encoder bank beside `kEncoderCount` — raise one map 10→≤16.

**Call-graph de-duplication:** Global rand and per-module `RandPage` already share `FroggersV2ControlCore` (@268, @933). Per-module Randomize on the unified surface MUST call `RandPage` only (task 7.6).

## Structure plan

1. Hygiene + absorb unified-layout + lock migration default + vendor inventory stub  
2. Vendor Runtime/portable visualizers (pin ≥ `c1810393`)  
3. `FroggersApp` under Engine (delegate DSP)  
4. Delete Random module page + bag params; preset silent-drop  
5. Dual scopes + encoder visualizers  
6. Mod 2-deep gate  
7. Unified surface + Envelope/Audio content  
8. 16-slot bank + Crunchy/Crispy stability  
9. Rand arm gesture  
10. `SYNTH_RUNTIME_MAIN` shell cutover  
11. Validators + docs + QA  

Parallel code edits across packets are forbidden until the lead posts an explicit dependency map and the user accepts it (OMNI §4).

## Dependencies

- Local Sheaf vendor slice (no network Sheaf FetchContent)  
- Existing AudioEngine + `FroggersV2ControlCore` / facade  
- Baseline mod-detail 16-cell semantics (archived operator-truth-repair Packet 15 intent)  
- Absorbs and retires live change `desktop-v2-unified-parameter-layout`  
- Out of scope deps: MIDI mapping profiles; VST/AU Runtime host  

## Goals / Non-Goals

**Goals:**
- Desktop Froggers is a Sheaf-style `SynthApplication` under vendored Runtime.
- Dual multi-layer scopes (3 VCO + 3 LFO EF) and Sheaf encoder underlays on the Application surface.
- Mod drill-in depth hard-capped at 2; 16-cell mod page retained.
- Delete Random S&H module page + bag/deja-vu/slew/expansion params; keep Random S&H 1/2 mod lanes + ganged visualizer.
- Absorb unified layout: no carousel; unified surface; chrome beside scopes; cross-couplers removed (redundant with PM, see D11); VCO morphs labeled "Shape"; ASR Envelope.
- ≤16 encoder bank with stable Crunchy + Crispy; 4×4 grid (sparse OK).
- Rand: toggle = global; held = next-click local.

**Non-Goals:**
- MIDI Controllers mapping redesign / profile automation / per-row encoder MIDI targets.
- VST/AU Runtime/`AudioProcessor` host.
- Shipping Braid 4 as a Froggers product.
- Web parameter-subset chrome parity.
- Full ADSR (decay knee) — ASR only for Envelope.

## Decisions

### D1 — Vendor Sheaf Runtime/Engine/portable UI into FroggersTiga
**Choice:** Copy required Sheaf `projects/synth` slices in-tree with adoption inventory.  
**Why:** Matches `froggers-v2-sheaf-runtime` local-ownership rule; avoids path-dep on Desktop/Sheaf.  
**Alt:** Path-dep / submodule — rejected for configure fragility and OpenSpec conflict.

### D2 — FroggersApp wraps existing AudioEngine + control core
**Choice:** `ProcessBlock` / `ProcessFrame` delegates to current DSP + `FroggersV2ControlCore`; do not rewrite DSP in this change.  
**Why:** Runtime adoption is a host boundary change first.  
**Alt:** Port DSP into Sheaf `ParameterGroup` immediately — deferred.  
**PARTIALLY SUPERSEDED by D16 (2026-07-24):** the "port params/rand into Sheaf `ParameterGroup` — deferred" clause is reversed — the param + randomization *model* now migrates to Sheaf. The "do not rewrite DSP" clause **still holds** (the migration stops at the `FroggersV2HostBridge` boundary; `AudioEngine`/`src/core` DSP is untouched).

### D3 — Dual ScopeVisualizer panels (Sheaf skeleton)
**Choice:** Two panels; each uses multi-layer color-coded overlapping traces (VCO outs; LFO EFs). Retire single global EF-only scope as sole viz.  
**Why:** Miniapp dual-scope + Braid multi-layer pattern; operator request. LFO EF visualization follows Sheaf scope layer machinery (no Froggers-specific blocker).

### D4 — Mod depth max = 2
**Choice:** Hard gate: opening mod view from a depth cell is rejected. Layer 0 = module params; layer 1 = 15 lanes + Target(Back).  
**Why:** Sheaf allows recursion; Froggers product forbids infinite nest.  
**Alt:** Unlimited Sheaf recursion — rejected.

### D5 — Delete Random page params (not relocate)
**Choice:** Remove page index 1 and all bag-control host/engine parameters. Random S&H 1/2 remain catalog mod sources with `GangedRandomLfoVisualizer` only.  
**Why:** Sheaf random sources have no Step chance / Deja vu / Bag size / Slew page. Operator: we do not need them.  
**Alt:** Relocate bag UI onto mod lanes — rejected.

### D6 — Absorb unified-parameter-layout
**Choice:** This change owns carousel retirement, unified surface, chrome relocate, VCO 1/3, ASR Envelope. Standalone `desktop-v2-unified-parameter-layout` is cancelled/archived on accept. Unified surface lists modules **without** Random S&H section.  
**Why:** Operator: unified-layout absorbed here.

### D7 — Single 16-slot physical encoder map
**Choice:** Raise product bank toward 16 slots; pages may use fewer cells; Crunchy + Crispy occupy stable slots. One map only (no parallel `kEncoderCount` and Sheaf bank).  
**Why:** OMNI reuse; MIDI remapping deferred but slot stability prepares it.

### D8 — Rand arm Froggers-style
**Choice:** Toggle engages global rand authority; hold arms next parameter press/click as one-shot local rand, then clears.  
**Why:** Operator divergence from Sheaf `SetRandomHeld` while-press model.

### D9 — MIDI mapping deferred
**Choice:** Runtime Controllers page may exist as shell; no Froggers mapping/profile work in this change.

### D10 — Fork the page-label authority for the Random deletion (operator 2026-07-22)
**Context:** Packet 4 (delete Random S&H page) surfaced that `sim/V2ParamDisplayNames.hpp` is a **shared single-source authority**, consumed not only by desktop-v2's manifest but also by the **web/wasm V2 host** (`wasm/bindings.cpp:225` → `forHostPageRow`; `PagedHostIO` with `SimHostKind::Web`). v1 desktop is on a separate file (`sim/ParamDisplayNames.hpp`) and is unaffected. Editing the shared file down to 6 pages misaligns the web host's labels by one and breaks the gated `sim/build` test `V2ModuleExpansion_test`.
**Choice (C2):** **Do NOT edit the shared `sim/V2ParamDisplayNames.hpp`** (it stays at 7 pages incl. Random for the web host + the shared `FroggersEngine` PageManager, which also stays at 5 engine pages incl. Marbles). Give **desktop-v2 its own 6-page label authority** (a desktop-v2-owned display-name table) and repoint only desktop-v2 consumers at it. The web/wasm host and v1 keep working unchanged; `V2ModuleExpansion_test`/`PageBootNav_test` stay green **without** any dishonest re-baseline.
**Why not C1/C3:** C1 (edit shared file, degrade web, re-baseline the web test to mislabeled output) ships a real web regression and encodes a known-wrong label as truth. C3 (migrate the web host too) contradicts the standing Non-Goal "Web parameter-subset chrome parity."
**Deliberate deviation:** this creates two label sources (desktop-v2 6-page vs shared 7-page). That is a sanctioned divergence because the two products have genuinely diverged (desktop-v2 drops Random; web keeps it), not accidental duplication.
**Follow-up (next task):** **wasm/web V2 host integration is the operator's next task after this change** — that is where the web host is brought onto the post-Random page model (and the fork can later be reconciled). Tracked as the explicit next change, not part of this one.

### D11 — Remove the cross-coupler entirely (redundant with PM) (operator 2026-07-23, SUPERSEDES the 2026-07-21 "three explicit couplers" direction)
**Trace (why redundant):** the `XCPL` knob ([`FroggersEngine.hpp:544`](src/core/FroggersEngine.hpp)) does nothing on its own — it only produces two gate coefficients `c12`/`c23`, which are scalers on the phase-mod offsets: `pmOff1 = pm1d·c12·u2`, `pmOff2 = pm2d·(c12·u1 + c23·u3)`, `pmOff3 = pm3d·c23·u2` ([`FroggersEngine.hpp:635-638`](src/core/FroggersEngine.hpp)). That is the **same FM-depth axis the PM knobs already own** — the coupler is a bespoke second gain, not a distinct capability (and the bipolar form could never reach 1↔3).
**Choice:** remove the coupler from the **V2 product surface** — the bipolar `Cross-coupler` **host/UI param row** and the two-ID manifest emit (`crossCouplers`) come out, and on the V2 DSP path the PM phase offsets keep **zero** neighbor coupling. No coupler of any kind on V2 — not bipolar, not three explicit, none. PM stays as three **independent, self-contained** parameters (its value is a phase-mod frequency on its own VCO, D12). Every VCO→VCO (or any) modulation routing is done **only** through the drilldown matrix — nothing cross-VCO is ever hardcoded on V2.
**RECONCILED WITH D14 (2026-07-25):** the earlier "delete the `XCPL` Audio param + the `c12`/`c23` coupling terms in `StepOscillators`" language is **superseded by D14's flag-gating**. The shared-engine `XCPL` slot (`FroggersEngine.hpp:611`, `InitParam("XCPL",3,…)`, index 3 stable) and the `c12`/`c23` legacy terms (`FroggersEngine.hpp:745-755`, the `else` branch) are **RETAINED byte-for-byte for Daisy/v1**; only the V2 flag-on path (`m_simIndependentPm`, `:735-744`) drops them. So on V2 nothing cross-VCO is hardcoded; on Daisy/v1 the coupler is unchanged. Do NOT delete the engine `XCPL` slot or the `c12`/`c23` terms — that would break the gated Daisy/v1 path. (Verified in code 2026-07-25; task 7.4 is implemented this way.)
**Effect:** Audio **host page** drops 13→**10** (3 pitch + 3 Shape + 3 PM + Crispy); freed slots stay empty (operator: no new params now). The engine parameter table keeps `XCPL` at index 3 for legacy hosts.

### D12 — Uniform parameter model: knob is the value unmodulated, an attenuator when modulated (operator 2026-07-23)
**Model:** every parameter is a single numeric value. With **no** modulation assigned, the knob position **is** the value, and that value is functional — e.g. a PM parameter's value is the *frequency of phase modulation* on its VCO (LFO-like), pitch's value is pitch, Shape's is the morph position. When a source **is** assigned via drilldown, the knob position is **replaced in role** by becoming the **attenuator** of that assigned underlying signal. Modulation drill-in is exactly 2 levels (D4, capped — not Sheaf-infinite): level 1 = knob attenuates an underlying source; level 2 modulates that level-1 attenuator knob. This uniform model is *why* bespoke couplings (D11) are redundant — routing is the drilldown's job, uniformly, for every parameter.

### D13 — "LFO" mod sources are the VCO EFs at a slow timescale (relabel "LFO EF"); morphs labeled "Shape" (operator 2026-07-23)
**Trace:** Froggers has no free-running LFOs (unlike Sheaf's three per mod page). The `lfo_1/2/3` mod-source taps (permanent-rack indices 8–10) are intended to be the **VCO envelope followers at a slow (LFO-rate) timescale** — the same signal as `vco_*_ef` (fast), just slower. Today those taps are **dead**: `V2EnvelopeFollowerBank` ([`sim/V2EnvelopeFollowerBank.hpp:12-13`](sim/V2EnvelopeFollowerBank.hpp), `kNumTaps=5`, `kFirstModIndex=3`) populates only taps 3–7 (the fast VCO/pair EFs); no `SetTap` writes 8–10, and there is no LFO oscillator. So making "LFO EF" real is a **DSP addition**, not a relabel: a second, slow-coefficient EF pass populating taps 8–10 from the VCOs.
**Choice:** (a) add the slow-timescale EF pass feeding taps 8–10; (b) relabel the `lfo_1/2/3` mod sources **`LFO EF 1/2/3`** (keep `VCO 1/2/3 EF` for the fast ones) so the mod pages show `VCO EF` and `LFO EF` distinctly; (c) the two scope panels (packet 5) are these VCO EFs at the two timescales — the packet-5 LFO-EF panel currently points at the dead raw-LFO taps and must be repointed to the slow-EF source. (d) the three VCO waveform-morph controls are labeled **"Shape"** (continuous shape morph).

### D14 — Flag-gate the new modulation model to the V2 hosts; Daisy/v1 untouched (operator 2026-07-23, choice A)
The D11 coupler removal + self-contained PM and the D13 slow-EF LFO are DSP changes in code shared beyond desktop-v2: `StepOscillators`/`XCPL` live in `src/core/FroggersEngine.hpp`, used by the **Daisy firmware** ([`src/FroggersTiga/FroggersTiga.hpp`](src/FroggersTiga/FroggersTiga.hpp)) and the **web host** ([`src/core/PagedHostIO.hpp:29`](src/core/PagedHostIO.hpp) `FroggersEngine m_engine`) as well as desktop-v2 (`DesktopHostIO`).
**Choice (A):** gate the new model behind a host-kind flag, following the existing `SetSimWaveMorph` / `SetSimDedicatedPm3Knob` / `SetUseV2FilterParallel(UsesV2Fuego(hostKind))` pattern — set **only** by the V2 hosts (desktop-v2 + web). The **Daisy/v1 legacy oscillator path stays byte-for-byte unchanged** (coupler-gated PM, dead LFO taps). The slow-EF LFO is naturally V2-scoped — `V2EnvelopeFollowerBank` is a V2-host bank the Daisy does not use.
**Entanglement (traced):** PM is currently gated *by* the coupler — at `XCPL` center default `c12=c23=0`, so `pmOff*` all become 0 ([`FroggersEngine.hpp:635-638`](src/core/FroggersEngine.hpp)). So removing the coupler UI without the flag-gated DSP rework makes PM inert; the two must land together.
**Self-contained PM DSP (RESOLVED, operator 2026-07-23):** when a PM parameter has **no** external mod source assigned, it is a **separate sine LFO oscillator** running at frequency = the PM knob value, phase-modulating that VCO. When a source **is** assigned via drilldown, the PM knob attenuates that source instead (D12 uniform model). No self-feedback, no cross-VCO terms. **The knob's minimum position = PM fully OFF** (zero depth / no modulation); above that floor the value is the LFO frequency (operator 2026-07-23 — there must be a zero position). The phase-mod depth (and the frequency range) are implementer defaults to be tuned by ear. Task 7.4 is now **unblocked**. (The LFO-EF + Shape work was never blocked.)

### D15 — Add per-VCO Sustain to the V2 Envelope (true ASR), V2-scoped (operator 2026-07-23)
Task 7.5 wants ASR (Attack/Sustain/Release, no Decay). The V2 `VcoAdsrState` is currently AR — `Stage{Idle,Attack,Hold,Release}`, Attack/Hold ramp/hold at a hardcoded `1.0f` ([`VcoAdsrState.hpp:15-21,86,93-95`](src/core/VcoAdsrState.hpp)); applied per voice at [`FroggersEngine.hpp:762-769`](src/core/FroggersEngine.hpp). Sustain was deliberately removed 2026-06-30 (archived `desktop-v2-adsr-page` spec); U5 re-adds it, "no decay knee."
**Choices (locked):**
- **DSP: extend `VcoAdsrState` in place** with pure, dependency-free C++ — do NOT `#include` the vendored `synth::DspAdsr` (it's in the desktop-v2-only vendor tree; `src/core/` is compiled by the Daisy firmware + sim/web whose include paths exclude it — pulling it in breaks those builds). Attack ramps directly to the **sustain level** (no intermediate peak/decay), Hold holds at the sustain level, Release falls from it. **Normalized attack** (operator): the attack ramp is scaled so the Attack-time knob means the same wall-clock duration regardless of sustain level (`attackStep = sustainLevel/(attackSeconds·sr)`), not a constant rate. `apply()` gains a 3rd `sustainKnob` arg.
- **Row layout: per-VCO ASR triplets** `[Atk1,Sus1,Rel1, Atk2,Sus2,Rel2, Atk3,Sus3,Rel3, Crispy@9]` — Envelope page 7→10 rows, Crispy moves to row 9 (aligns with the other expanded pages). Host-param count re-baselines **120→126** (`FROGGERS_EXPECT_HOST_PARAM_COUNT_V2`); projection validators self-derive; re-run them as the same-packet re-baseline gate. Sustain default 0.8.
- **Prerequisite fix (same packet):** `DesktopHostIO.hpp:314` / `PagedHostIO.hpp:65` pass the ADSR page as `m_pages[6]`, but it lives at PageManager index **5** (`kPmAdsrPage`) — stale from the old 7-page scheme, leaving the envelope inert on V2 today. Fix to `[kPmAdsrPage]` (Attack/Release/Sustain are all inert until this lands) — distinct, separately-tested.
**V2-scoping (Daisy-safe, verified):** Daisy never calls `SetVcoAdsrState` (`m_vcoAdsr` stays null → the `PairArEnvelope` branch runs, [`FroggersEngine.hpp:764,770-792`](src/core/FroggersEngine.hpp)); the new code compiles-but-never-runs on Daisy. `PairArEnvelope.hpp`/`AudioPairArState.hpp` get a zero-line diff; the `IsV2SimHostKind` else-branches stay identical.

### D16 — REVERSE D2's param deferral: migrate params + randomization onto Sheaf (operator 2026-07-24)
**This supersedes D2's "port DSP/params into Sheaf `ParameterGroup` — deferred."** Being a real Sheaf app means *using* Sheaf's parameter/rand facilities, not reinventing them. The bespoke `FroggersV2ControlCore` + `HostParameterInventoryV2` param/rand layer migrates onto Sheaf's `ParameterManager`/`ParameterGroup`/`Bank`/`BankSlot`/`SceneState` + `StandardModulators` + `Parameter::RandomizeVisibleValue`/`Bank::RandomizeModulationDepths`, **preserving existing behavior** (per-page + global randomization of both parameter values and mod-depths; Crunchy global; Crispy per-page).

**What D16 does NOT reverse — the DSP stays (bridge boundary confirmed by trace).** The pipeline is already a bridge:
`FroggersV2ControlCore → FroggersV2HostBridge::syncToHost()/syncModRoutes() ([`FroggersV2HostBridge.cpp:123-217`](desktop-v2/Source/control/FroggersV2HostBridge.cpp)) → HostParameterRoutingV2::applyPageKnob/… → DesktopHostIO::SetPageKnob ([`DesktopHostIO.hpp:362-369`](src/core/DesktopHostIO.hpp)) → PageManager/Page ([`src/core/Page.hpp`](src/core/Page.hpp)) → AudioEngine DSP.`
The migration **replaces only the left side** (`FroggersV2ControlCore` → Sheaf `ParameterManager`) and rewrites the bridge to read Sheaf `Parameter` values (`GetRaw`/`CachedKnobValue`/`CurrentDepthForSource`). **`HostParameterRoutingV2`, `DesktopHostIO`, `PageManager`, `Page`, `src/core/Parameter` (DSP), and all of `src/core/` require ZERO changes** — they only receive `(page,row,value)` triples regardless of who produces them. So the Froggers DSP is NOT rewritten (D2's "host-boundary change first" spirit holds); only the param/rand *model above the bridge* becomes Sheaf-native.

**Migration map (from the 2026-07-24 grounding sweep — see `.superpowers/sdd/` scratch report):**
- Scenes/blend/gestures → Sheaf `SceneState` + `ParameterManager::Scene()`/gesture API (1:1).
- Crunchy (global) / Crispy (per-page) → ordinary `Parameter`s registered into a global bank / each page's bank — the `Axis::GlobalCrunchy`/`crispyRowForPage` special-casing and its 6 "skip this row in rand" sites disappear.
- Per-page value rand → `Bank::ApplyModifierToTopLevel(Modifier::Random, scene)`; global → loop it over all banks (small new "all-banks" glue Sheaf lacks). Mod-depth rand → `Bank::RandomizeModulationDepths` / `Modifier::RandomMod`.
- Random S&H sources → `synth::StandardModulators<N>::Register()` (ships the sources **with their `GangedRandomLfoVisualizer` already fed** — this also supplies the Random-S&H visualizer feed, correcting the false assumption that it was already handled; it is NOT — StandardModulators is never instantiated today, `FroggersScopePanels.hpp:49-56`).
- **DELETED, not migrated (operator 2026-07-25):** sequencer-step/pattern randomization + capture/recall (`onRandSequencerStep`, `FroggersV2HostBridge::captureLiveToSequencerStep`, `SequencerPanelComponent`) and gesture-weight randomization (`PerformanceBandV2`, `Axis::GestureWeight`). These are legacy old-Sheaf cruft; the desktop-v2 Froggers product retires them entirely (an earlier draft wrongly planned to keep them bespoke on Sheaf `Parameter` JSON snapshots). **Scenes stay** (`SceneState`/`SceneBlend`, per-page + global rand). **Daisy/v1 guard:** `sim/SequencerState.hpp` and `src/core/DesktopHostIO.hpp` (owns `m_sequencer` + the step gate, `:97,720-746`) are SHARED with v1 desktop (`desktop/`) + sim tests — the shared infra is NOT deleted; only desktop-v2's consumption/UI/rand/inventory is removed, after tracing the desktop-v2 gate source so note-triggering survives.

**Delta vs. the previous (D2-deferred) plan — what changes:**
- **SURVIVES unchanged:** packets 2 (vendor), 3 (FroggersApp shape), 4 (Random-page delete), 5 (scopes — DSP-fed via `GetCvOut`, below the bridge), 7.4 (coupler/PM — engine-row layer), 7.5 (Envelope ASR/sustain — DSP + inventory rows), 12 (LFO EF), and the packet-10 boot mechanism (`SYNTH_RUNTIME_MAIN(FroggersApp)`, host-agnostic).
- **REWORKED:** packet 6's 2-deep gate (re-express against `Bank`'s modulation-view API instead of `m_modView`); the **portable surface increments 1-4** (`FroggersAppSurface::BuildModuleGrid/BuildModDetailGrid` read `FroggersV2ControlCore.visibleRowForSlot/effectiveRow` today → repoint to Sheaf `Bank`/`Parameter` reads); packet-10 step-1 mod-view-reset (re-express against `Bank::Deselect`).
- **SUPERSEDED-DELETED:** the entire bespoke param/inventory/rand layer — `FroggersV2ControlCore`'s xorshift + rand methods, `HostParameterInventoryV2`'s `Axis`/index arithmetic, the Crunchy/Crispy special-casing. `HostParameterInventoryV2`'s VST-flat descriptor table gets re-derived from the `ParameterGroup`/`Bank` graph.

**Flagged bugs found while grounding (address in this migration):**
1. **Dual-control-core split (correctness bug):** `FroggersAppSurface` owns its OWN `m_audioCore` `FroggersV2ControlCore` ([`FroggersAppSurface.hpp:403`](desktop-v2/Source/ui/FroggersAppSurface.hpp)), a *second* instance disconnected from the one driving audio in `FroggersAppCore::m_facade` — so the portable surface currently edits/visualizes a parallel model that **never reaches the DSP**. Fixed for free by the single-shared-`ParameterManager` migration (step 5).
2. **Latent dead code:** `FroggersV2ControlCore::isAdsrPage` compares `page == 6` but `kNumHostPages == 6` (pages 0–5) ([`FroggersV2ControlCore.cpp:82-85`](desktop-v2/Source/control/FroggersV2ControlCore.cpp)). Verified 2026-07-25: the function has **zero callers** in `desktop-v2/Source` — it is entirely dead, not merely an unreachable branch, so **delete** it rather than re-point the constant. The **same** dead `page==6` branch also sits in `crispyRowForPage` (`:94-96`, `kCrunchyPage == kNumHostPages == 6` is never passed there) — remove it in the same decision-free step (omni §1 symmetric-duplication).

**DECISIONS (operator RESOLVED 2026-07-25):**
1. **Mod-depth rand semantics:** **all connected lanes.** Sheaf's `Bank::RandomizeModulationDepths` example is a geometric-random subset, but Sheaf accommodates both — the current app's subset is only an example. Migrated randomizer draws a fresh depth for every connected lane (preserves existing behavior); no coin-flip.
2. **RNG identity:** **accept Sheaf's default mt19937.** No xorshift injection via `SetRandomSource`; seeded-sequence reproducibility is not preserved (operator-accepted).
3. **Sequencer-step/gesture rand:** **DELETED, not migrated** (see migration map above). Scenes are kept.
4. **Per-sample vs per-block resolution:** **Sheaf per-sample slew** (`Parameter::ProcessSample`). Modulation smoothing becomes audible vs today's per-block pull; validation must re-check every mod path by ear.
5. **Ganged-visualizer feed:** **in scope** — fed for free via `StandardModulators`.
6. **Dual-control-core fix:** **folded into migration step 5** (`tasks.md` §13.5).
7. **`isAdsrPage` bug:** **fixed first** as a decision-free prerequisite — and the function is fully dead (no callers), so it is DELETED, not re-pointed; the twin dead `page==6` branch in `crispyRowForPage` (`FroggersV2ControlCore.cpp:94-96`) is removed in the same step.

**Migration sequence:** build-stays-green steps (delete dead `isAdsrPage`/twin → remove sequencer+gesture surface (shared infra kept) → spike one page → rand parity → Crunchy/Crispy → all pages → repoint surface + collapse dual-core → 2-deep gate → **scenes** glue (sequencer/gesture retired) → all-banks rand glue → re-derive inventory → retire `FroggersV2ControlCore`) — see `tasks.md` §13.

## Layout addendum — unified surface at 1280×920 (task 7.1, operator 2026-07-23)

**Chosen: Candidate A** (tabbed modules over one shared grid). Arrangement:
- **Top band (~200px):** the two `ScopeVisualizer` panels (VCO-EF trio | LFO-EF trio, ~300×180 each) beside the transport row + global-command band (7.3 "chrome beside scopes"). The global-command strip keeps its full-width two-row layout (validated by `test_global_strip_grid_at_1280` — do not squeeze it).
- **Tab strip (~40px):** 6 module tabs — Audio, Envelope, Filter, Drive, Reverb, Delay — direct-select (no prev/next arrows).
- **Work area (~640×620):** the selected module's ≤16-slot 4×4 encoder grid with large rings; on mod drill-in it swaps in-place to the 16-cell mod-detail grid (reuse the existing `SubmodulePagePanel::layoutDetailGrid` layer-0↔layer-1 swap + packet-6's 2-deep gate).

**Build target = the portable `FroggersApp` surface, NOT the legacy JUCE shell (operator 2026-07-23).** The entire point of this change is to make Froggers a **Sheaf app**: `Runtime → Engine<FroggersApp> → PortableSurface`. The current JUCE `MainComponent` host is the thing being retired, not extended — so Candidate A is built on `FroggersAppSurface` using the vendored portable UI (`synth::ui::Builder`), and the **shell cutover (packet 10) is what makes it the live, testable app**. Packets 7 (portable unified layout) and 10 (cutover) therefore deliver the testable Sheaf app together.

**Operator framing: Candidate A ≈ a tabbed carousel — but reuse the *concept*, not the JUCE component.** One module active at a time (the `PageCarouselComponent` model) is the right behavior, but `PageCarouselComponent` is a JUCE widget and is **retired**, not reused. Re-implement the one-module-active selection as portable state on `FroggersAppSurface`, rendered via `synth::ui` (a 6-tab selector + the active module's 4×4 grid + the scopes/chrome band). Single active-module authority (no parallel page-state). The module encoder panels (`SubmodulePagePanel`/`AdsrPagePanel`, JUCE today) are rebuilt in portable UI as part of this — that rebuild is the substance of the Sheaf-app port, expected, not scope creep. **Perf band + sequencer are DROPPED (operator confirmed 2026-07-25):** the sequencer and gesture-weight features are retired from the desktop-v2 product (legacy old-Sheaf cruft); scenes are retained. No overlay/tab relocation — removed outright (shared `sim`/`DesktopHostIO` infra kept for v1; see D16 migration map).

## Risks / Trade-offs

- **[BREAKING presets]** Removing Random page axes invalidates host-parameter indices → Migration: drop obsolete axes on load; re-baseline inventory validators; document in manual.
- **[Vendor drift]** Sheaf upstream moves → Mitigation: inventory pins source paths/SHAs; no live fetch.
- **[Mega-change]** Runtime + layout + viz + page deletion → Mitigation: packetized tasks; archive gates on spikes only where preset migration needs explicit decisions.
- **[Engine Random bags]** Deleting UI/host params may leave unused DSP → Mitigation: remove or hardwire defaults in the same packet that deletes the page; no orphan controls.
- **[Unified fit at 1280×920]** Six modules (no Random) still dense → Mitigation: fit spike with post-Random module set before locking geometry.

## Migration Plan

1. Land OpenSpec artifacts; cancel/archive `desktop-v2-unified-parameter-layout` with pointer to this change.
2. Vendor Runtime slice + inventory.
3. FroggersAppCore under Engine (facade parity tests).
4. Delete Random page + params; re-index pages; preset migration.
5. Dual scopes + visualizers; 2-deep gate.
6. Unified surface + Envelope/Audio content; chrome relocate.
7. Rand arm; 16-slot map.
8. `SYNTH_RUNTIME_MAIN` shell cutover.
9. Docs + validators + manual QA.

Rollback: keep previous desktop entry behind a CMake/target switch until shell cutover packet closes; after cutover, revert commit series.

## Locked defaults (formerly Open Questions)

- **Preset migration (Random page axes):** silent drop of deleted Random page host axes on load, with a log line. No one-time converter in this change.
- **Preset migration (cross-coupler): DEAD SCOPE under D14 (choice A) — no drop, no log line needed.** The earlier "silent drop + log line" note assumed the `XCPL` param would be *deleted*. It is not: D14 **retains** the shared-engine `XCPL` slot (index 3 stable, for Daisy/v1), and only neutralizes its effect on the V2 flag-on `StepOscillators` path. So a legacy preset's coupler value still round-trips into the slot harmlessly and is simply **ignored** on the V2 path (no coupling applied) — nothing is deleted or index-shifted, so there is nothing to drop and no operator-visible change to log. (Confirmed by the 2026-07-23 omni-consistency review.)
- **16-slot Crunchy/Crispy indices:** not fixed in this design body; packet 8 **must** record the exact map in a design addendum before that packet closes. Preflight does not block on the numeric map; it blocks on “one physical map only.”

## Sheaf tip sync (origin/main @ c1810393, fetched 2026-07-19)

Local clone had been at `eae12ea3`; **~90 commits** pushed since. Diffstat overall ~+23k/−1.7k (dominated by **browser catalog/launcher**). Froggers desktop Runtime vendor slice MUST pin **at least** this tip and account for:

| Upstream theme | Relevance to this change |
|----------------|---------------------------|
| `ProcessSamplePhase1` / `Phase2` + braid filter caches | Vendor `ParameterModulation` / modules as-is; do not strip phases |
| `DspAdsr` + polyphonic ADSR **module** (full ADSR w/ Decay) | Available DSP; Froggers product Envelope remains **ASR** (Decay unused / fixed) |
| `ButtonGrid` / `GridManager` / `MessageIn::Grid*` / Controllers grid mapping | **Out of scope** with MIDI mapping deferred; Engine init now creates grid UI state — vendor must compile cleanly even if FroggersApp does not use grids |
| `RuntimeUIState` = `{parameters, grids}` | AppContext/Engine wiring changed; FroggersApp Init must follow new Engine initialize order |
| `PortableJuceBackend` value-action **prefix append** (`:` + value) | Required when hosting portable UI; do not reintroduce overwrite-only dispatch |
| MIDI endpoint IDs allow `:` | Bring fix with MidiController vendor |
| Browser catalog / Pages / packages | **Ignore** for desktop Froggers Runtime host |

**Non-goal remains:** adopting browser launcher or Launchpad grid UX in this change.

## OMNI preflight (remediated 2026-07-20)

**Prior defect:** the Cursor plan’s 2026-07-19 “PASS — Data-flow map present” was false relative to OMNI §1/§14 — the OpenSpec change had no cited TRACE; the plan’s OMNI framing table named stages without file:line verification.

**Now:**
- §1 TRACE + definition-site table: present in this `design.md`
- §2 objective / data flow / constraints / structure / dependencies: present
- Random page = **delete** (not relocate) — consistent across proposal/design/specs/tasks
- Dual encoder-bank risk called out (single map only)
- Hedge: migration + slot-index open questions closed or explicitly deferred to a named packet deliverable
- Verification (§16.1): full `ctest` / rebuild runs via Task subagent; parent reads pass/fail summary only

**Preflight status for execution:** PASS only after implementers treat this TRACE as immutable input and do not re-open deleted Random bag params or MIDI/VST scope.
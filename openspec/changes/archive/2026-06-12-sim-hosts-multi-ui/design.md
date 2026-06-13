## Context

FroggersTiga firmware runs on Daisy Field: five logical **pages** (Audio, Marbles, Reverb, Filter, Drive) share eight physical knobs. `PageManager` routes `KnobUpdate` to `m_pages[m_currentPage]` only. `DaisyIO` owns SW1/SW2 page switches, CV presence, gate, A/B button grid, and OLED.

Partial work already exists: portable headers in `src/core/`, `FroggersEngine`, a monolithic `HostIO.hpp`, and a web scaffold hardcoded to 48 kHz and Field-clone UI. Firmware builds after extraction.

OMNI audit findings that must shape this change:

| Finding | Resolution |
|---------|------------|
| One `HostIO` for all hosts | Split **PagedHostIO** (web, VCV) vs **DesktopHostIO** (JUCE) |
| Desktop assumed SW1/SW2 page UI | Desktop shows **five adjacent sub-modules**; pages stay in core for DSP only |
| `KnobUpdate` only touches current page | Add **`KnobUpdateOnPage`** for desktop multi-panel |
| 48 kHz locked in plan and code | **`SetSampleRate(44100)`** default for sims; firmware stays 48 kHz |
| `48000` literals scattered in engine | Single `m_sampleRate` drives all time-based mappings |
| TS mirrored host logic risk | WASM owns C++ host; TS sends events only |
| `tickControls` per-sample (old plan typo) | Once per audio block, before `ProcessBlock` |

## Goals / Non-Goals

**Goals:**

- One portable `froggers_core` linked by firmware shim, WASM, JUCE, and (later) VCV.
- **Web**: mobile-friendly paged UI (8 knobs + SW1/SW2 + OLED mock), 44.1 kHz, GitHub Pages.
- **Desktop**: wide layout — mod rack + patch cables above five identical sub-module panels side by side, each with 7 param knobs + FUEG; global shared strip for cross-page actions only (mod assign via patch bay, not strip).
- **VCV**: compact one-page-at-a-time module with page buttons (phase 2).
- OMNI-clean data flow: UI → host adapter → `PageManager` / `FroggersEngine` → audio out.

**Non-Goals:**

- Desktop page switching (SW1/SW2) — not in desktop v1.
- Field A1–A7 hold-to-assign on desktop (patch bay / dropdown instead).
- VCV release before GPL strategy is decided.
- Changing Daisy Field firmware UX or sample rate.
- VCO morph UI, MIDI bridge, external audio picker on Daisy hardware — **sim hosts only** (desktop, VCV; web optional mic via toggle, default off).

## Decisions

### 1. Core extraction (unchanged direction, complete the migration)

Move portable headers to `src/core/`; `src/common/` re-exports for firmware. `FroggersEngine` exposes `ProcessSample`, `ProcessBlock`, `Config`, `ButtonCallback`. Firmware `FroggersTiga` inherits engine and wraps `Process(AudioHandle::...)`.

### 2. Sample rate: 44.1 kHz for sim hosts, parameterized engine

```text
Host opens audio at 44100
    → engine.SetSampleRate(44100.f)
    → ReadParamsBlock / reverb delay / Marbles filters use m_sampleRate
Firmware Init (48 kHz hardware)
    → engine.SetSampleRate(48000.f)  // explicit in shim
```

Replace every `48000.0f` literal in `FroggersEngine`, `Marbles.hpp`, `RuntimeParam.hpp`, `EQ.hpp`, and reverb pre-delay sample scaling with `m_sampleRate` — zero bare `48000` literals remain in those files after migration. `SetSampleRate(float)` recomputes `RuntimeParam` filter alphas and is read by `Marbles::UpdateParams` and `ReadParamsBlock`. Firmware shim sets 48000 on init; sim hosts call `SetSampleRate` with device/context rate on audio start (44100 typical).

**Rationale:** User requirement; also matches common macOS default interface rate. Rack often runs 44.1 or 48 — VCV reads `APP->engine->getSampleRate()`.

### 3. Two host adapters, not one

```text
                    ┌─────────────┐
   Web / VCV ──────▶│ PagedHostIO │──┐
                    └─────────────┘  │
                                     ▼
                    ┌──────────────────────────┐
                    │ PageManager + Engine     │
                    └──────────────────────────┘
                                     ▲
                    ┌─────────────┐  │
   JUCE desktop ───▶│ DesktopHostIO│──┘
                    └─────────────┘
   Daisy Field ────▶ DaisyIO (unchanged)
```

**PagedHostIO** (evolves current `HostIO.hpp`):

- SW1/SW2 → `PagePrevious` / `PageNext`
- `SetKnob(i, v)` → `m_knobPositions[i]` + `KnobUpdate(i, v)` on **current** page
- OLED queries via `GetNameCurrentPage` etc.
- CV presence loop, gate → `ButtonCallback`

**DesktopHostIO**:

- No page switch methods in v1 UI
- `SetPageKnob(page, i, v)` → `KnobUpdateOnPage(page, i, v)`
- Five `SubModulePanel` components in JUCE, each bound to page index 0–4
- **Mod rack** (above panels, v2.1): four module boxes — MIDI, VCO feat, Marbles 1, Marbles 2 — each with meter + **output jack**; `PatchCableOverlay` handles VCV-style drag/drop to per-row **input jacks** (see §11).
- **Shared strip** (one row for whole app — human labels, no hardware A/B grid):
  - **Randomize all** (B2): `RandomizeAllPagesIndependent()`
  - **Randomize mod** (B4, all pages): `RandomizeAllPagesModIndependent()`
  - **Marbles** (B5): Marbles increment
  - ~~**XCPL −** / **XCPL +** (B6/B7)~~ — **dropped:** XCPL is knob-only on Audio panel (strip buttons duplicate knob; never shipped)
  - **Randomize waves** (B′): `RandomizeVcoMorphs()`
- **Per-panel actions** (inside each sub-module):
  - **Randomize** (B1): `RandomizePage(page)` — knobs 0–6 only; FUEG skipped by `Parameter::Randomize`
  - **Randomize mod** (B3): `RandomizePageMod(page)` — all 8 positions including FUEG
- Desktop skips hardware pickup: `DesktopHostIO::Init()` sets every parameter on every page to `TrackingState::Tracking`, then `SetPageKnob` calls `KnobUpdateOnPage` (immediate `m_knobValue` update)

### 4. PageManager desktop routing and randomize

```cpp
void KnobUpdateOnPage(uint8_t page, uint8_t position, float knobPosition) {
    m_pages[page].KnobUpdate(position, knobPosition, m_modIndex);
}

void RandomizePage(uint8_t page) {
    for (size_t j = 0; j < Parameter::x_numParameters; j++) {
        float knobPos = m_pages[page].m_parameters[j].m_knobValue;
        m_pages[page].m_parameters[j].Randomize(knobPos);
    }
}

void RandomizeAllPagesIndependent() {
    for (uint8_t i = 0; i < m_numPages; i++) {
        RandomizePage(i);
    }
}
```

Desktop does **not** update shared `m_knobPositions` for knob writes — each page keeps independent `m_knobValue` on its `Parameter`s. Paged hosts continue using shared `m_knobPositions` + `RandomizeAllPages()` (hardware semantics). Per-panel **Randomize** calls `RandomizePage(panelIndex)`; global **Randomize all** calls `RandomizeAllPagesIndependent()` — not `RandomizeAllPages()`, which passes stale `m_knobPositions[j]` to every page.

**Rationale:** Hardware reuses one knob bank; desktop has 5×8 virtual knobs. Core data model already stores per-page parameters; only the routing layer changes.

### 5. Desktop UI layout

```text
┌─ Play ─ Stop ─ External ─ Audio... ─ MIDI... ─────────────────────────────┐
├─ MOD RACK ──────────────────────────────────────────────────────────────┤
│ [MIDI ●out] [VCO feat ●out] [Marbles 1 ●out] [Marbles 2 ●out]           │
├──────────┬──────────┬──────────┬──────────┬──────────────────────────────┤
│ Audio    │ Marbles  │ Reverb   │ Filter   │ Drive                        │
│ [Randomize] [Randomize mod]  (identical chrome on all five)             │
│ V1VO [~] ●in ━━━━━━━   ← row: name, wave btn (VCO rows), input jack, slider │
│ ...                                                                       │
│ FUEG     ●in ━━━━━━━                                                      │
└───────────────────────────────────────────────────────────────────────────┘
┌──────────────── Shared strip ────────────────────────────────────────────┐
│ Rand all │ Randmod all │ Marbles │ Rand waves │  (compact labels per desktop-compact-layout)
└──────────────────────────────────────────────────────────────────────────┘
```

Each sub-module is **identical chrome**: title, **Randomize** + **Randomize mod**, eight rows (7 params + FUEG). Each row is `[GetName() label][wave button on Audio VCO rows][●in jack][vertical slider]`. **No** per-knob mod dropdowns; **no** duplicate mini-OLED rows (names live on the row label). Mod assignment is patch-cable only (§11).

### 6. Web UI stays paged (Field-ish)

Matches mobile narrow viewport. SW1/SW2 + 8 knobs + OLED mock + **Mic** button (default **off**). Uses **PagedHostIO** in WASM. On audio start, call `SetSampleRate(audioContext.sampleRate)` (engine default before that is 44100). With mic off, `ProcessBlock` feeds **0** to the external input path (VCO-only); enabling **Mic** requests `getUserMedia` and routes the stream to `ProcessSample` input.

**Sim-only VCO morph UI** on Audio page: inline wave **glyph button** beside each of `V1VO`, `V2VO`, `V3VO` labels (rows 0–2). Button click cycles morph; glyph on the button reflects current morph — **not** an eighth OLED row or full-width bar. Desktop uses the cycle button; web may add a compact morph slider on the OLED row in a later pass.

### 6b. VCO waveform morph — sim-only, CV-modulatable

```text
Per VCO (sim path):
  knob 0..1 ──exp map──▶ morph
  CV + mod assign ──ModMgr──▶ attenuated morph (same contract as Parameter)
  morph ──▶ EvalWaveMorph(phase, morph) ──▶ sine ⇄ saw ⇄ square (smooth)

EvalWaveMorph:
  morph in [0,1] after exp mapping
  segment 0..0.5: blend sine → saw
  segment 0.5..1: blend saw → square
  (continuous at joins)

Audio page UI (sim hosts):
  Desktop row:  V1VO [~] ●in ━━━━━━━   ← glyph button cycles morph; ●in = mod jack
  Web OLED row: V1VO ~∿  [====bar====]  ← glyph on OLED; morph slider optional later
  │ V2VO ...  V3VO ...  XCPL ...      │
```

**Mod routing:** Three sim-only `VcoWaveMorph` targets (engine-side `Parameter` or equivalent) register with `ModMgr` when a sim host starts. They are mod-assignable like knobs 0–7. Firmware does not register them.

**Firmware path (unchanged):**

```text
DaisyIO → ButtonCallback(3/4) → m_vco1Wave/m_vco2Wave uint8 discrete
VCO3 → SDDSine only (m_vco3Wave morph stays 0 / unused)
StepOscillators on device: EvalWave(ph, uint8) for V1/V2, sine for V3
```

**Sim path:**

```text
Sim host Init() → engine.SetSimWaveMorph(true) + register VcoWaveMorph[3] with ModMgr
Host sets morph knobs → engine uses EvalWaveMorph for all three VCOs
A8/B8/A′ optional shortcuts (step morph); B′ = RandomizeVcoMorphs()
```

**Activation gate:** `FroggersEngine::SetSimWaveMorph(bool)` defaults to **false**. Only sim host adapters (`PagedHostIO` / `DesktopHostIO` `Init()` on WASM/JUCE/VCV) call `SetSimWaveMorph(true)`. Firmware shim never calls it — `StepOscillators` stays on discrete `EvalWave` + sine VCO3.

**Morph read rate:** Effective morph per VCO is resolved **per sample** via `ModMgr::Modulate` (same contract as `Parameter::Get`), so audio-rate CV timbre modulation works.

**Rationale:** Sims get expressive continuous waves + CV performance; hardware behavior stays bit-identical when morph flag is off and DaisyIO is untouched.

### 6c. MIDI I/O + envelope CV→MIDI (desktop + VCV only)

Daisy Field hardware exposes TRS **MIDI in/out** (`MidiUartHandler` on D13/D14), **CV1–4 in**, **CV out 1–2**, and **gate in**. FroggersTiga firmware today uses CV/gate/audio but **does not route MIDI in `DaisyIO`** — sim hosts add MIDI without changing firmware.

```text
External audio in ──▶ |input| ──▶ m_extEnvFilter ──▶ envelope 0..1
                                                    │
                    CvMidiBridge ◀──────────────────┘
                         │
                         ▼ MIDI CC out (default ch 1, CC 74 or user-config)
                         envelope × 127 per audio block

MIDI CC in (one ch + one CC, sim hosts) ──▶ m_modMgr.m_mods[0]
                                          (0..1 range, same as Field GetCvValue)

VCV phase 2 (Field parity): CV1..CV4 jacks ──▶ m_modMgr.m_mods[0..3] via DaisyIO path;
  desktop/web sim hosts do NOT expose four separate MIDI CV lanes.
```

**`FroggersEngine`** exposes `GetEnvelopeLevel()` — value of `m_extEnvFilter` after the **last** `ProcessSample` in a block. `tickMidiOut` reads that value once per block (no peak hold).

**Desktop (JUCE):** MIDI device picker for in/out; **one** in channel + in CC for the MIDI mod source. **VCV:** `midi::Input` / `midi::Output` on module plus Field-parity CV jacks; same bridge for envelope out. MIDI **complements** Field CV/gate jacks on VCV — it does not replace them (see vcv-module spec).

**Not in web v1** (no system MIDI; optional mic via **Mic** toggle).

### 6d. Desktop external audio input selector

Desktop ring-mod needs a **dedicated external input** path, not only the default input device:

```text
┌─────────────────────────────────────────┐
│ Audio settings                          │
│  Output device: [Built-in ▼]            │
│  External (ring-mod) in: [Interface ▼]  │
│  Channel: [In 1 ▼]                      │
└─────────────────────────────────────────┘
         │
         ▼
ProcessSample(externalSample)  →  MixExternalAndOsc(...)
```

JUCE: secondary input callback or sub-device routing from `AudioDeviceManager`. User-selected channel feeds engine input; main output device drives speakers/headphones.

**Web:** **Mic** toggle (default off). When on, `getUserMedia` mic is the external source (no device picker in v1). When off, no mic permission prompt and external input is silent.

### 7. Canonical audio block (all hosts)

```cpp
host.tickControls();              // once per callback — see ordering below
engine.ProcessBlock(in, out, n);
host.tickMidiOut();               // desktop + VCV only: envelope → MIDI CC
```

**`tickControls` ordering (MIDI hosts):**

```text
1. CvMidiBridge::drainMidiIn()  → sim hosts: one CC → m_mods[0] only
                                  VCV phase 2: CV jacks may also write m_mods[0..3]
2. applyCvPresence()            → m_externalCvActive[0..3]
3. gate / SW edges              → ButtonCallback (PagedHostIO: SW1/SW2)
```

**`tickControls` (web):** steps 2–3 only (no MIDI). CV values come from WASM API if exposed later.

PagedHostIO `tickControls` handles SW edges; DesktopHostIO handles gate/CV only (no SW). Desktop knob changes write directly via `SetPageKnob` on the UI thread (params already in `Tracking` state).

### 8. WASM / web build

CMake + Emscripten → standalone `froggers.wasm` with C exports. AudioWorklet owns WASM instance. TS transport only. CI commits `docs/`.

### 9. Refactor partial implementation

- Rename/split `HostIO.hpp` → `PagedHostIO.hpp`; add `DesktopHostIO.hpp`
- Fix `web/src/main.ts` sample rate to 44100
- Do not ship desktop with `[` `]` page keys in v1

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Desktop bypasses knob pickup → behavior differs from hardware | Documented non-goal for desktop; A/B audio parity tests use same parameter values, not pickup gesture |
| `m_knobPositions` stale on desktop for paged randomize APIs | `RandomizePage(page)` + `RandomizeAllPagesIndependent()` read each page's stored knob values; desktop never calls `RandomizeAllPages()` |
| 44.1 vs 48 firmware timbre drift | Accept for sims; firmware remains golden reference at 48 kHz |
| Dual host adapters duplicate CV/gate loops | Extract `applyCvPresence` in `src/core/CvPresence.hpp`; both adapters call it from `tickControls` |
| Partial web code on disk misleads | tasks.md starts with adapter split + sample rate before finishing web |
| Morph params outside Audio 8-knob page | Sim-only `VcoWaveMorph[3]` sidecar mod targets; not in Field OLED page slots |
| Dual audio devices on desktop | JUCE documents external-in vs monitor-out; fallback to single device if OS cannot split |
| MIDI CC rate vs audio block | Send envelope CC once per `ProcessBlock`; throttle to ≤1 kHz if block size is tiny |

## Migration Plan

1. Land `SetSampleRate`, `KnobUpdateOnPage`, and page-aware randomize APIs; verify firmware build (48 kHz shim).
2. Split host adapters; wire PagedHostIO to existing WASM bindings.
3. Complete web sim + Pages CI.
4. Scaffold JUCE desktop with five panels + shared strip.
5. VCV after license decision.

Rollback: firmware shim can keep including engine from `src/core/` even if sim hosts are reverted.

### 10. v2 sim UX (transport, labels) — shipped

Play/Stop, external audio Off/L/R, human strip labels, VCO wave on row, no duplicate OLED rows, refresh skips drags — done. Desktop per-knob mod **dropdowns** were wrong host UX; superseded by §11.

### 11. v2.1 sim mod model and patch bay

#### 11a. Sim vs hardware mod sources

```text
HARDWARE (Daisy Field)          SIM HOSTS (desktop / web)
─────────────────────          ─────────────────────────
M1..M4  CV in (4 jacks)   →    ONE "MIDI" source → m_mods[0]
M5      VCO feature       →    VCO feat          → m_mods[4]
M6      Marbles 1         →    Marbles 1         → m_mods[5]
M7      Marbles 2         →    Marbles 2         → m_mods[6]
                               m_mods[1..3] unused in sim UI
```

**SimModSource** (UI / API enum for hosts):

| Enum | Label | `Parameter::m_modIndex` |
|------|-------|-------------------------|
| `None` | — | `255` |
| `Midi` | MIDI | `0` (desktop only) |
| `VcoFeat` | VCO feat | `4` |
| `Marbles1` | Marbles 1 | `5` |
| `Marbles2` | Marbles 2 | `6` |

**MIDI in (sim):** `CvMidiBridge` listens on **one** configured channel + CC; writes `m_mods[0]`; `m_externalCvActive[0]` from MIDI presence. Do not present M1–M4 as four sim sources.

**One-to-many:** Core already supports one `m_modIndex` on many `Parameter`s with different `m_modAmount`. Patch UI assigns per destination; patching Marbles 1 → V1VO and Marbles 1 → FUEG is valid.

#### 11b. Desktop layout (patch cables — VCV Rack interaction)

Reference: [VCV Rack Getting Started](https://vcvrack.com/manual/GettingStarted) — “Drag from port to port to create a cable. Move or delete an existing cable by dragging one of its plugs.”

```text
┌─ Play ─ Stop ─ External ─ Audio... ─ MIDI... ─────────────────────────────┐
├─ MOD RACK (~90px tall) ───────────────────────────────────────────────────┤
│ ┌─MIDI──┐ ┌VCO feat┐ ┌Marbles1┐ ┌Marbles2┐   ← module box: label, meter  │
│ │  ●out │ │  ●out  │ │  ●out  │ │  ●out  │   ← drag FROM here            │
│ └───────┘ └────────┘ └────────┘ └────────┘                                │
│         ╲─────────── bezier cable (overlay, follows mouse while drag) ─╱  │
├──────────┬──────────┬──────────┬──────────┬──────────────────────────────┤
│ Audio    │ Marbles  │ Reverb   │ Filter   │ Drive    (current 5-col width) │
│ [Randomize] [Randomize mod]                                               │
│ V1VO [~] ●in ━━━━━━━  ← ●in = mod input jack; drop cable HERE to connect  │
│ ...                                                                       │
│ FUEG     ●in ━━━━━━━                                                      │
└───────────────────────────────────────────────────────────────────────────┘
```

**Cable state machine (match VCV gesture, Field routing rules):**

```text
  IDLE
    │ mousedown on OUTPUT jack (mod source) OR empty/connected INPUT jack OR grab existing cable plug
    ▼
  DRAGGING — bezier from port → cursor; cable visible after ≥4px move (avoid accidental cables on click)
    │ mouseover INPUT jack while dragging → highlight valid target
    │
    ├─ mouseup on valid INPUT jack (new cable) → CONNECT (SetPageModSource)
    ├─ mouseup on valid INPUT jack (existing plug, different target) → MOVE route
    ├─ mouseup on empty space / invalid target (new cable) → CANCEL (no assignment change)
    └─ drag existing plug to void → DISCONNECT (SetPageModSource(..., 255))

  CONNECTED — persistent bezier; slider on row = mod depth
    │ grab cable plug at either end → back to DRAGGING (move or disconnect)
    │ RandomizePageMod / global Randomize mod → overlay repaints from core assignments
```

**Port hit-testing:** each jack registers a screen-space bounds rect (≥14px diameter hit radius). `PatchCableOverlay` sits above panels, owns mouse during DRAGGING, z-order above panel chrome.

**VCV vs FroggersTiga rules:**

| VCV Rack | Desktop sim mod |
|----------|-----------------|
| Drag out → drag in | Same gesture |
| Drop on empty rack → delete cable | Same — cancel / disconnect |
| Multiple cables stack on one **input** (sum voltages) | **One** mod source per param — new patch **replaces** prior source on that input |
| Multiple cables from one **output** | Same — Marbles 1 → many ●in jacks |
| Right-click port → delete cable | v2.1: drag plug to void; optional port context menu later |

**Disconnect:** drag input-side or output-side plug to empty space (VCV default). Optional: right-click input jack → “Clear mod”.

**Depth:** not on the cable — row slider sets `SetPageModDepth` while patched (Field knob-after-assign).

**No** per-knob `<select>` on desktop. Remove `ModBayPanel` text-row and per-row `ComboBox` implementations.

#### 11c. Web layout (dropdown below knob)

Each knob column stacks **vertically** — mod assign is not inline beside the name:

```text
  V1VO          ← name on OLED row only; knobs column is control-only
  ┌───┐
  │ K │         ← vertical slider
  └───┘
 [Mod: None ▼]   ← dropdown BELOW slider, not beside label
```

Per-knob dropdown on current page: `None | VCO feat | Marbles 1 | Marbles 2`. Compact mod meter strip for three internal sources. Reference: [thenoriegas.info](https://thenoriegas.info) — **not** cables, **not** per-module play/stop.

#### 11d. Implementation notes

- `PageManager::SetPageModSource` / `SetPageModDepth` — keep; add `SimModSource` ↔ index mapping in host layer.
- **`PatchCableOverlay`**: top-level component; owns drag state, paints beziers, hit-tests ports in screen space. `ModModuleBox` registers output port bounds; `SubModulePanel` registers input port bounds per row (incl. FUEG). Repaint cables on resize.
- `CvMidiBridge`: sim profile uses one `m_inChannel` + one `m_inCc` → `mods[0]` only; remove four-lane CC UI and `m_inCvCc[4]` loop from desktop/web.
- `SetPageModSource` in host layer: reject sim-invalid indices 1–3 (Field CV only); accept 0, 4, 5, 6 via `SimModSource` mapping.
- `AudioEngine`: reuse member `inBlock` buffer across callbacks (no per-block `std::vector` allocation).
- `SubModulePanel`: row = `[name][wave?][●in jack][slider]`; removing mod `ComboBox` frees label width — **do not** widen panels.
- Web `knob-col`: order = slider, then mod `<select>` underneath (label lives on OLED row).
- `RandomizePageMod(page)` wired to per-panel **Randomize mod** button.
- Fix FUEG display `nan` — guard `GetPageParam` / refresh when fuegoization unset.
- Hover tooltip on port (optional v2.1): `"Marbles 1 → Audio V1VO, depth 0.62"`.

## Resolved decisions (audit follow-up)

- **Per-panel Randomize** (B1): `RandomizePage(page)`; **per-panel Randomize mod** (B3): `RandomizePageMod(page)`; **global Randomize all** (B2) / **Randomize mod** (B4) on shared strip.
- **Sim mod sources (v2.1):** four logical sources (MIDI + 3 internal); not M1–M7 CV clone on desktop/web.
- **Desktop mod assign (v2.1):** VCV drag-from-output / drop-on-input patch cables + module rack; web dropdown only.
- **Web sample rate:** `SetSampleRate(audioContext.sampleRate)` on start; engine pre-init default 44100.
- **Desktop pickup:** `DesktopHostIO::Init()` forces all page params to `Tracking` before first `SetPageKnob`.
- **Sim morph gate:** `SetSimWaveMorph(true)` only in sim host `Init()`; firmware default false.
- **MIDI tick order:** drain MIDI in inside `tickControls` before `ProcessBlock`; envelope CC out in `tickMidiOut` after block using last-sample `GetEnvelopeLevel()`.

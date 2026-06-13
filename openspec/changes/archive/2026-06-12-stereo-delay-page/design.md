## Context

- **Core**: five pages; `ApplyOutputFx` = pureDelay → comb → bump → **ProcessReverb** → rvMix (mono).
- **Sim overlay**: UI six pages; WASM five; Delay = host index 5.
- **Firmware**: never registers hook or Delay state.

## Goals / Non-Goals

**Goals:**

- Pre-reverb stereo delay, sim-only.
- Full Field parity on Delay page: 7 + **FUEG** (functional), mod, randomize, patch cables.
- Stereo out when available; mono fallback.
- `HostPageController` owns host page index.

**Non-Goals:**

- Sixth `PageManager` page; Daisy flash changes; VCV delay v1; tempo sync; stereo reverb.

## Decisions

### 1. Sim overlay + two-tier pages

| Host page | Params / UI | Engine |
|-----------|-------------|--------|
| 0–4 | WASM / `DesktopHostIO` | Core |
| 5 **Delay** | `DelayState` | Hook + delay exports |

`HostPageController` owns `hostPage` 0–5. WASM `froggers_current_page()` is authoritative only for pages 0–4. UI shows `(hostPage + 1) / 6`.

**Page navigation (web) — state machine:**

| Event | Action |
|-------|--------|
| ◀ / ▶ on host | `hostPage = (hostPage + Δ + 6) % 6` |
| `hostPage` → 0–4 | Call **`froggers_select_page(host, hostPage)`** (new sim export) so WASM page matches UI |
| `hostPage` → 5 | **Do not** call `froggers_page_next/prev` or `froggers_select_page` |
| Knob on 0–4 | WASM `froggers_set_knob` / mod exports |
| Knob on 5 | `froggers_delay_set_knob` / delay mod exports |
| `postScreen` | If `hostPage ≤ 4`: WASM rows; if `hostPage === 5`: synthesize from delay exports |

Desktop shows all panels at once — no `hostPage`; each panel writes to its own backend (0–4 `DesktopHostIO`, 5 `DelayState`).

**Screen refresh:** see table above.

### 2. Pre-reverb insert (minimal core hook)

```text
ApplyOutputFx (mono per sample):
  pureDelay → comb → bump
       ↓
  if (m_simFxInsert) bump = m_simFxInsert(bump, m_simFxInsertCtx)
       ↓
  ProcessReverb (RPRE → tank) → rvMix → mono out
```

**Hook contract (in `FroggersEngine`):**

```cpp
using SimFxInsertFn = float (*)(float bumpIn, void* ctx);

SimFxInsertFn m_simFxInsert = nullptr;
void* m_simFxInsertCtx = nullptr;

void SetSimFxInsert(SimFxInsertFn fn, void* ctx); // sim hosts only

// ApplyOutputFx, after bump:
if (m_simFxInsert) {
    bump = m_simFxInsert(bump, m_simFxInsertCtx);
}
```

- **Firmware:** never calls `SetSimFxInsert`; members stay `nullptr` → bit-identical to pre-change.
- **Sim callback:** runs `StereoDelay::process`, returns `toReverbMono(bump, wet, dmix)`; stores last wet for output bus.
- **Verification:** 4096-sample noise buffer through engine with hook null; max abs diff vs baseline = 0.

**RPRE + DTIM:** independent controls (unchanged).

### 3. `DelayState`

```text
DelayState
├── knobs[8]              // 0–6 params, 7 = FUEG
├── modSource[8], modDepth[8]
├── smoothed[8]           // host RuntimeParam-equivalent
├── StereoDelay           // L/R lines, exposes last wetL/wetR
└── readParam(row, mods[]) // mod → Fuegoize → DSP
```

Param pipeline per row 0–6: `smoothed knob → mod blend → Fuegoize(value, knobs[7], row) → DSP`.

**Mod blend (same as `ModMgr` / `Parameter::GetPreFuegoization`):**

```cpp
float blendMod(float knob, uint8_t modIndex, float depth, const float* mods) {
    if (modIndex >= ModMgr::x_numMods || depth <= 0.f) return knob;
    return std::min(std::max(knob * (1.f - depth) + mods[modIndex] * depth, 0.f), 1.f);
}
```

- **Desktop:** `DelayState::readParam` receives `DesktopHostIO::m_pageManager.m_modMgr.m_mods` (same array core pages use).
- **WASM:** callback reads engine `m_modMgr.m_mods` inside sim host wrapper — no duplicate mod bus.

**Smoothing:** reuse `RuntimeParam` pattern — `OPLowPassFilter` with `SetAlphaFromNatFreq(1000.f / sampleRate)` per knob; `Process()` once per sample before read.

**Buffer sizing:** `StereoDelay::kMaxDelaySamples = 144000` (3 s @ 48 kHz). `setSampleRate(hz)` sets `m_capacity = min(kMaxDelaySamples, ceil(3.0 * hz))`. Sim-only allocation; firmware does not link `StereoDelay`.

### 4. `Fuegoize.hpp` (v1, required)

Extract bit-mangle from `Parameter::Get` (lines 112–125) into:

```cpp
float Fuegoize(float value, float fuegKnob, uint8_t row);
```

- Lives in `sim/Fuegoize.hpp` — linked by Delay path only in v1.
- Same mask/XOR/shift semantics as core; `row` = Delay param index 0–6.
- **Parity test:** `sim/Fuegoize_test.cpp` (or firmware test host) compares `Fuegoize(v, f, row)` against `Parameter` fuegoization path for ≥16 `(v, f, row)` tuples including edge `f=0`, `f=1`, all rows 0–6; max diff = 0.

### 5. Patch cable fix

```text
kDelayPageIndex = 5

PatchCableOverlay + cable draw:
  if port.page >= kDelayPageIndex
    read/write DelayState.modSource/modDepth
  else
    DesktopHostIO.SetPageModSource(...)
```

- `PatchCableOverlay` holds `DelayState*` (or `AudioEngine*` accessor).
- Persistent routes for page 5 read from `DelayState`, not `GetPageModSource(5, …)`.
- `mouseDown` grab-existing on Delay inputs reads `DelayState` mod index.

### 6. Randomize parity

| Control | Core 0–4 | Delay 5 |
|---------|----------|---------|
| Panel **Randomize** | `RandomizePage` | `DelayState.randomizeKnobs(0..6)` |
| Panel **Randomize mod** | `RandomizePageMod` | `DelayState.randomizeMod()` |
| **Randomize all** | `RandomizeAllPagesIndependent` | + `DelayState.randomizeKnobs(0..6)` |
| **Randomize mod (all)** | `RandomizeAllMod` | + `DelayState.randomizeMod()` |

FUEG (row 7) follows core convention: panel Randomize randomizes 0–6 only; FUEG unchanged unless explicitly included in `randomizeKnobs` policy (match core: **exclude FUEG** from panel randomize, include in global only if core does — core excludes FUEG from `RandomizePage`; **match that**).

### 7. Stereo output bus (host, after ProcessBlock)

Core `ProcessBlock` output is **mono** (`coreMono`) — reverb included. Delay runs pre-reverb; wet **L/R** is preserved in `StereoDelay` for the output bus.

**Insert (inside `ApplyOutputFx`, per sample):**

```text
bumpIn  → StereoDelay.process(bumpIn, params) → wetL, wetR
monoWet = (wetL + wetR) * 0.5
toReverb = (1 - DMIX) * bumpIn + DMIX * monoWet     // mono into ProcessReverb
```

**Output bus (host, after `ProcessBlock`, per sample):**

Restore stereo width collapsed by `monoWet` without double-counting delay energy:

```text
deltaL = wetL - monoWet          // == (wetL - wetR) * 0.5
deltaR = wetR - monoWet         // == (wetR - wetL) * 0.5
outL = coreMono + DMIX * deltaL
outR = coreMono + DMIX * deltaR
monoFallback = (outL + outR) * 0.5
```

| Condition | Result |
|-----------|--------|
| **DWID** = 0 | `wetL ≈ wetR` → `deltaL ≈ deltaR ≈ 0` → `outL ≈ outR ≈ coreMono` |
| **DMIX** = 0 | No width restore; `outL = outR = coreMono` |
| **DSND** = 0 | `wetL/wetR` silent → same as DMIX=0 case |

- **Desktop**: write `outL`/`outR` to ch0/ch1 when `numOutputChannels >= 2`; else `monoFallback` to ch0.
- **Web**: `outputChannelCount: [2]`; mono fallback when `outputs[0].length === 1`.

### 7b. `StereoDelay` public API (fixed contract)

```cpp
struct DelayParams {
    float dtim;  // 0–1 → 0–3 s exponential
    float dsnd;  // send
    float dfbk;  // feedback
    float dwid;  // stereo width
    float dton;  // tone LP in feedback
    float dmod;  // LFO depth on delay time
    float dmix;  // wet mix (insert + output bus)
};

struct WetPair { float l; float r; };

struct StereoDelay {
    static constexpr float kMaxDelaySeconds = 3.0f;
    static constexpr size_t kMaxDelaySamples = 144000; // ceil(3 s @ 48 kHz)

    void setSampleRate(float hz);
    WetPair process(float bumpIn, const DelayParams& p); // updates lines; returns wet L/R
    WetPair getLastWet() const;                          // same as last process() wet

    float toReverbMono(float bumpIn, WetPair wet, float dmix) const {
        const float monoWet = (wet.l + wet.r) * 0.5f;
        return (1.0f - dmix) * bumpIn + dmix * monoWet;
    }
};
```

Host output bus uses `getLastWet()` + `readParam(DMIX row)` after each sample. **Per-block hold:** if stereo bus runs once per `ProcessBlock` instead of per sample, use the **last sample's** wet pair from that block (acceptable for v1 when block size ≤ 128).

### 8. Panel adapter (desktop)

Sixth panel reuses `SubModulePanel` chrome via a thin backend interface — no forked panel layout.

```cpp
struct IPanelBackend {
    virtual void setKnob(uint8_t row, float value) = 0;
    virtual float getKnob(uint8_t row) const = 0;
    virtual void setModSource(uint8_t row, uint8_t modIndex) = 0;
    virtual void setModDepth(uint8_t row, float depth) = 0;
    virtual uint8_t getModSource(uint8_t row) const = 0;
    virtual float getModDepth(uint8_t row) const = 0;
    virtual const char* getRowName(uint8_t row) const = 0;
    virtual void randomizeKnobs() = 0;   // rows 0–6 only
    virtual void randomizeMod() = 0;
};

struct DelayHostBackend : IPanelBackend { /* wraps DelayState */ };
// Pages 0–4: existing DesktopHostBackend(pageIndex, DesktopHostIO&)
```

`SubModulePanel` constructor takes `IPanelBackend&` instead of hard-coded `DesktopHostIO&`.

### 9. WASM / web

- WASM keeps 5 engine pages.
- **New sim exports:** `froggers_select_page(host, page)` (page 0–4), `froggers_delay_set_knob`, `froggers_delay_get_knob`, `froggers_delay_set_row_mod_source`, `froggers_delay_get_row_mod_source`, `froggers_delay_set_row_mod_depth`, `froggers_delay_get_row_mod_depth`, `froggers_delay_row_name`, `froggers_delay_randomize_knobs`, `froggers_delay_randomize_mod`.
- Hook + `StereoDelay` compiled into sim WASM; registered in `froggers_create`.
- Worklet owns `hostPage`; routes per §1 state machine.

## Risks / Trade-offs

| Risk | Mitigation | Status |
|------|------------|--------|
| Mono reverb + stereo width | §7 exact `deltaL/deltaR` bus math | **Specified** |
| Core hook in shared tree | `nullptr` default; firmware max-diff = 0 test | **Specified** |
| Fuegoize drift | Golden-vector test vs `Parameter` | **Specified** |
| Web page desync | `froggers_select_page` on hostPage 0–4 | **Specified** |
| StereoDelay API ambiguity | §7b fixed struct | **Specified** |
| 3 s buffer RAM | 144k samples/ch, sim-only link | **Specified** |
| Patch overlay dual path | `kDelayPageIndex = 5` branch | **Specified** |
| Mod bus duplication | Delay reads same `m_modMgr.m_mods` as core | **Specified** |
| Panel backend coupling | `IPanelBackend` / `DelayHostBackend` §8 | **Specified** |
| Per-block vs per-sample wet | Last-sample hold per block ≤128 | **Specified** |

## Migration Plan

1. `Fuegoize.hpp` + `DelayState` + `StereoDelay` + hook.
2. Firmware verify null hook identity.
3. WASM delay exports + desktop panel/overlay + web host page.

## Open Questions

- (none blocking v1)

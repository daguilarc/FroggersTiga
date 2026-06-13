## Context

**Screenshot anatomy (what you’re seeing):**

```
┌─ page-chrome (Audio 1/6 + blurb + Randomize) ─────────────┐
├─ #mod-route-summary ─── EMPTY BOX (min-height + border)     │  ← always in DOM
├─ knobs row (labels OK now) ─── no visual groups             │
├─ #oled ─── 220px #000 black void (empty innerHTML)          │  ← min-height desktop
└─ page pills / global strip                                  │
```

**Audio “split second then dead” — likely causes (verified in code):**

```
Play click
    │
    ▼
startAudio: audioRunning=true, setRunning(true), connect graph
    │
    ├── screen message arrives: onScreenUpdate sets audioRunning = data.audioRunning
    │   (worklet may still post false on early frames → main thread desync)
    │
    ├── OR AudioContext → suspended (macOS / tab policy) → silence, UI still "Playing"
    │
    └── OR worklet error → playBtn.disabled=!audioRunning → stuck disabled
```

`onScreenUpdate` line 336–337 trusts WASM for transport state. Transport buttons are updated only in `startAudio`/`stopAudio`, not when screen flips `audioRunning`.

## Goals / Non-Goals

**Goals:**

- Play stays audible until user clicks Stop (or explicit error with recovery)
- No empty bordered regions in the layout
- Knob modules read as bounded panels (VCO / coupling / output)
- Finish OLED compact/collapse from archived bootstrap-repair

**Non-Goals:**

- Desktop patch-cable UI
- Rewriting RotaryKnob component
- Moving page pills or global strip layout

## Decisions

### D1: Main thread owns transport; screen is display-only

**Choice:** Remove `audioRunning = data.audioRunning` from `onScreenUpdate`. Add `transportIntentPlaying` (true after Play click, false after Stop or worklet error). Main-thread `audioRunning` follows user intent and error recovery only — not WASM screen ticks. Add `syncTransportUi()` called from `startAudio`, `stopAudio`, `handleWorkletMessage` (error), and `audioContext.onstatechange`; it reads `audioRunning` and `transportIntentPlaying` to set Play/Stop disabled state, status suffix, and mod-bay idle. WASM `screen.audioRunning` may remain in processor posts but main thread does not mirror it.

**Why:** OMNI single authority; fixes race where screen posts `false` after Play. `transportIntentPlaying` distinguishes user Stop from OS `AudioContext` suspend (D5).

### D2: Mod route summary hidden when empty

**Choice:** `renderModRouteSummary`: if zero routes, set `modRouteSummaryEl.hidden = true` and `innerHTML = ""`. If routes exist, show panel. Remove `.route-empty` placeholder copy. On page load (before first `screen` message), call `renderModRouteSummary` with eight idle rows (all `modSource === 255`) or set `#mod-route-summary hidden` in `index.html` / init — pre-Play must not show bordered chrome.

**Why:** Empty bordered `min-height: 2rem` box is the “random fucking box” between header and knobs. Hiding only on screen ticks leaves the box visible until Play.

### D3: OLED collapsed when not playing

**Choice:**

| State | Desktop | Mobile |
|-------|---------|--------|
| Stopped / pre-Play | `#oled` `display: none` or `hidden` | same |
| Playing | full 8-row mock | compact strip ≤48px |

Remove desktop `min-height: 220px` when `#oled:empty` or `.oled--stopped`.

**Why:** Black void is `#oled { background:#000; min-height:220px }` with no children before first `renderOled`.

### D4: `HOST_PAGE_GROUPS` — one table, loop-built DOM

**Choice:** `HOST_PAGE_GROUPS: { label: string; rows: number[] }[][]` per host page (six pages). On page change, `applyKnobGroups(hostPage)` wraps existing knob cols in `.knob-group` panels in one loop — no per-page copy-paste markup. `knobCols[]` indices stay stable (0–7); only parent containers change.

**Full table (grep-verifiable in `main.ts`):**

| Page | Group | Rows |
|------|-------|------|
| 0 Audio | VCOs | 0, 1, 2 |
| 0 Audio | Coupling | 3, 4, 5 |
| 0 Audio | Output | 6, 7 |
| 1 Marbles | Marbles 1 | 0, 1, 2, 3 |
| 1 Marbles | Marbles 2 | 4, 5, 6 |
| 1 Marbles | FUEG | 7 |
| 2 Reverb | Mix & room | 0, 1, 2 |
| 2 Reverb | Tail | 3, 4, 5 |
| 2 Reverb | Space & FUEG | 6, 7 |
| 3 Filter | Pre-comb | 0 |
| 3 Filter | Peak EQ | 1, 2, 3 |
| 3 Filter | Comb filter | 4, 5, 6 |
| 3 Filter | FUEG | 7 |
| 4 Drive | Drive | 0, 1 |
| 4 Drive | Sample rate | 2, 3 |
| 4 Drive | Grit | 4, 5, 6 |
| 4 Drive | FUEG | 7 |
| 5 Delay | Time & send | 0, 1 |
| 5 Delay | Feedback & width | 2, 3, 4 |
| 5 Delay | Mod & mix | 5, 6, 7 |

**Why:** OMNI repetition rule; row groupings follow `HOST_PAGE_LABELS` semantics and Field submodule boundaries.

### D4b: Bordered column cells inside each group

**Choice:** Style existing `.knob-col` divs as bordered vertical cells inside `.knob-group`. Each cell contains label → knob → mod-source label → `<select>`, top to bottom. One CSS rule on `.knob-group .knob-col` — no per-column markup duplication. Group panel = outer boundary; knob-col = inner column boundary.

```
┌─ .knob-group "VCOs" ────────────────────────────────┐
│ ┌ .knob-col ┐ ┌ .knob-col ┐ ┌ .knob-col ┐          │
│ │ VCO1      │ │ VCO2      │ │ VCO3      │          │
│ │  (knob)   │ │  (knob)   │ │  (knob)   │          │
│ │ Mod src ▾ │ │ Mod src ▾ │ │ Mod src ▾ │          │
│ └───────────┘ └───────────┘ └───────────┘          │
└─────────────────────────────────────────────────────┘
```

**Why:** User-visible "boundaries" means both module groups **and** per-parameter columns. Groups alone still read as a flat knob strip.

**Non-goal:** Duplicate label/knob/mod markup — reuse existing `knobCols[]` DOM; CSS only.

**Choice:** On `suspended` while `transportIntentPlaying` (user clicked Play, not Stop): set status “Audio suspended — click Play”, `playBtn.disabled = false`. On `running`, reconnect if needed.

**Why:** macOS Safari/Chrome can suspend after brief output without user Stop.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Hiding OLED loses value preview when stopped | Knob positions + labels are primary; OLED only when playing |
| Group borders tighten mobile | Groups stack vertically ≤720px; knobs scroll horizontal inside group |
| Removing screen audioRunning sync | Mod scopes use main `audioRunning` only |

## Migration Plan

1. Transport fix (D1, D5) — unblocks Play
2. Hide empty mod-route + collapse OLED (D2, D3)
3. HOST_PAGE_GROUPS panels (D4)
4. Browser verify: Play 30s continuous, 390px no black void

## Cross-change dependency

`web-ext-in-meter` uses `active = externalEnabled && audioRunning` on the main thread. Task §1 (transport authority) MUST land before or with `web-ext-in-meter` apply — otherwise the meter inherits the WASM `audioRunning` overwrite race.

## Open Questions

None blocking.

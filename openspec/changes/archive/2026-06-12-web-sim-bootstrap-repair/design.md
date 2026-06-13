## Context

```text
main.ts (UI thread, has fetch)
  │ fetch(wasmUrl) → ArrayBuffer
  │ AudioWorkletNode({ processorOptions: { wasmBytes } })
  ▼
froggers-processor.ts (AudioWorkletGlobalScope — NO fetch)
  │ WebAssembly.instantiate(wasmBytes)
  ▼
froggers.wasm → PagedHostIO → screen payload → knob labels / OLED
```

**Current failure:** `web-sim-core-fix` task 1.1 put `fetch(wasmUrl)` inside `FroggersProcessor.loadWasm()`. AudioWorkletGlobalScope does not define `fetch`. The worklet posts `{ type: "error", message: "fetch is not defined" }`, `engineReady` stays false, Play stays disabled, and no `screen` messages arrive. Hardcoded knob init (`—` × 7, `Crunch` × 1) never updates.

**Secondary UX:** Two mod-bay hints (`index.html` + `initModBay`), page blurb substitutes for per-knob names in user perception, OLED `min-height: 220px` renders as empty black void when `screen` never arrives.

## Goals / Non-Goals

**Goals:**

- WASM loads in all targets (dev, build, GitHub Pages) without AudioWorklet `fetch`.
- First `screen` before Play; all eight knob labels from `ParamDisplayNames`.
- Play/Stop/External functional; page pills and randomize work while stopped.
- Single mod-bay hint; compact mobile layout without black OLED void.
- OMNI: one fetch site (`main.ts`); one hint element; label update in `updateKnobLabels` only.

**Non-Goals:**

- WASM/C++ changes.
- Patch cables, MIDI, RECORD.
- Removing OLED on desktop (Field parity).
- Replacing page chrome entirely — title + scoped randomize stay.

## Decisions

### 1. Main-thread WASM load (fixes Play)

**Pattern:** Denaudio / Web Audio spec guidance — fetch on main thread, transfer bytes to worklet.

```typescript
// main.ts
import wasmUrl from "../public/froggers.wasm?url";

async function initWorklet(): Promise<void> {
  const [wasmBytes] = await Promise.all([
    fetch(wasmUrl).then((r) => {
      if (!r.ok) throw new Error(`WASM fetch failed: ${r.status}`);
      return r.arrayBuffer();
    }),
    audioContext!.audioWorklet.addModule(processorUrl),
  ]);
  workletNode = new AudioWorkletNode(audioContext!, "froggers-processor", {
    processorOptions: { wasmBytes },
    numberOfInputs: 1,
    numberOfOutputs: 1,
    outputChannelCount: [2],
  });
}
```

**Worklet constructor:**

```typescript
constructor(options: AudioWorkletNodeOptions & { processorOptions?: { wasmBytes: ArrayBuffer } }) {
  super();
  const bytes = options.processorOptions?.wasmBytes;
  if (!bytes) {
    this.port.postMessage({ type: "error", message: "Missing wasmBytes in processorOptions" });
    return;
  }
  const { instance } = WebAssembly.instantiate(bytes, {});
  // ... existing host init, post ready + screen
}
```

Remove async `loadWasm()` and `fetch` import from processor file.

**Alternative rejected:** `importScripts` + inline WASM — breaks Vite asset hashing.

**Alternative rejected:** Compile on main, transfer `WebAssembly.Module` — extra complexity; `ArrayBuffer` transfer is sufficient for ~3 MB one-time load.

### 2. Knob label init

Remove split placeholder:

```typescript
mainLabel.textContent = "";  // not "—" / "Crunch"
```

`updateKnobLabels(rows)` unchanged — sets `row.name` from `screen`. Crunch comes from WASM row 7 like every other name.

Show `…` or empty until first `screen`; status line shows **Loading engine…** during fetch.

### 3. Mod bay hint dedup

- Delete `<p class="mod-bay-hint">` from `index.html`.
- Remove hint creation inside `initModBay()`.
- Add hint as sibling of `#mod-bay-toggle`: **CV trace while playing** (scopes, not level meters — matches `CvScopeCanvas` implementation).

Update `web-mod-meter-visibility` delta: hint text **CV trace while playing**, single DOM node.

### 4. Mobile OLED compact strip

| Viewport | OLED behavior |
|----------|---------------|
| >720 px | Full 8-row OLED mock (names, bars, wave SVG, badges) |
| ≤720 px | `.oled--compact`: hide name column and bar rows; show only wave buttons (Audio VCO rows) and mod badges in one ~48 px row |

Knob column labels remain authoritative for parameter names on mobile.

CSS changes:

```css
@media (max-width: 720px) {
  .oled {
    min-height: 0;
    padding: 0.35rem 0.5rem;
  }
  .oled--compact .oled-name,
  .oled--compact .oled-bar-wrap {
    display: none;
  }
}
```

`renderOled` toggles `oled--compact` class via `matchMedia` or CSS-only descendant rules.

**Alternative rejected:** Hide OLED entirely on mobile — loses wave morph buttons and mod badges.

### 5. Page chrome blurb

Keep `PAGE_BLURBS` as one-line module description under title. Do not duplicate parameter lists. Spec already requires knob columns as naming surface (`web-parameter-labels`).

### 6. Play disabled state UX

When `engineReady === false`, Play uses `disabled` + `cursor: not-allowed` (existing). After fix, Play enables on `ready`. External and Stop remain wired; only Play blocked until engine loads.

Error path: `Play is disabled` when error — status shows explicit message (not clickable affordance that silently fails).

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Large WASM copy to worklet | One-time at init; acceptable |
| `processorOptions` clone copies bytes | Still faster than broken fetch; document in design |
| Compact OLED hides value bars on mobile | Knob position shows value; desktop keeps bars |
| Regression in `web-sim-core-fix` "complete" status | This change supersedes tasks 1.1, 1.5, 5.1, 6.2 verification |

## Migration Plan

1. Implement main-thread fetch + processorOptions (unblocks all WASM UI).
2. Fix label init + verify `screen` on load.
3. Dedup mod hint + compact OLED CSS.
4. Manual: dev + build + docs path; confirm labels before Play, Play audible, page pill while stopped.

## Open Questions

None — root cause verified in code and Web Audio spec.

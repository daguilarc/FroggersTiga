## Context

```
User clicks Play
    → main: unlockAudioContext, initWorklet, setRunning(true), audioRunning=true
    → worklet: process() runs, audio ~20 frames
    → frameCount % 20 === 0 → postScreen()
        → readCString(namePtr) → new TextDecoder()  ← THROWS in AudioWorkletGlobalScope
    → worklet dead; main still audioRunning=true, Play disabled, status "Playing"
```

Confirmed console error (user screenshot):

```
Uncaught ReferenceError: TextDecoder is not defined
  at FroggersProcessor.readCString (froggers-processor.ts:202)
  at FroggersProcessor.postScreen (froggers-processor.ts:240)
  at FroggersProcessor.process (froggers-processor.ts:315)
```

**Already landed this session (do not revert):**

| Area | File | State |
|------|------|-------|
| Knob idle = `row.value` | `main.ts` `syncKnobUi` | Done |
| Patched drag snap to `modDepth` | `main.ts` + `RotaryKnob.ts` `onDragStart` | Done |
| Static knob labels | `main.ts` | Done |
| Transport intent + suspend recovery | `main.ts` | Done |
| Worklet error handler on main | `main.ts` `handleWorkletMessage` | Done — never fires for process crash |
| Page randomize → `postScreen()` | `froggers-processor.ts` | Done |
| Global randomize → no `postScreen()` | `froggers-processor.ts` | Bug — looks dead when worklet alive |

**Project paths:** `/Users/diegoaguilar-canabal/Desktop/FroggersTiga/web/` — Vite dev `http://localhost:5173/`. WASM at `web/public/froggers.wasm`.

## Goals / Non-Goals

**Goals:**

- Worklet survives every `postScreen()` call in all supported browsers
- Play → continuous audio ≥30 s; mod scopes animate; knobs wiggle under CV
- Randomize (page, all, mod, marbles) visibly moves knobs immediately
- Worklet death resets transport UI (Play enabled, Stop disabled)

**Non-Goals:**

- Re-architecting WASM string exports (future optimization only)
- Desktop sim changes
- Full `#oled` removal (see `web-knob-live-values`)
- OMNI-compliant external input meter (separate change `web-ext-in-meter`)

## Decisions

### D1: Manual ASCII/UTF-8 decode in worklet (not TextDecoder)

**Choice:** Replace `readCString` with a loop over `Uint8Array` until `0`, decoding bytes to string. Parameter names from WASM are ASCII (`VCO1`, `Mix`, `Crunch`, etc.); implement minimal UTF-8 for multi-byte safety without pulling DOM globals.

**Why:** `TextDecoder` is undefined in `AudioWorkletGlobalScope` in Brave/Chrome builds used by the user. Manual decode is the obvious default for worklets — same constraint as any audio thread that cannot assume browser main-thread APIs.

**Alternative rejected:** Pre-decode names on main thread — extra message schema and duplication; decode once in worklet is simpler.

### D2: try/catch in `process()` posts error to port

**Choice:** Wrap audio + screen tick logic in try/catch; on catch, `this.port.postMessage({ type: "error", message })`, zero outputs, return `true` once.

**Why:** Main already handles `error` and calls `stopAudio()`. Silent worklet death is why Play stays stuck gray.

**Alternative rejected:** Main-thread watchdog timer — laggy, does not fix root cause.

### D3: postScreen on setRunning(true) and global randomize

**Choice:** After `this.audioRunning = true` in `setRunning` handler, call `postScreen()`. After `marbles`, `randomizeAll`, `randomizeMod`, call `postScreen()` (match existing `randomizePage` pattern).

**Why:** noriegas and desktop refresh UI on every state change, not only on audio frame cadence. When stopped, randomize still must move knobs.

### D4: Keep knob live-value code as-is

**Choice:** Do not touch `syncKnobUi` / `RotaryKnob` in this change except verification. Check off `web-knob-live-values` tasks after manual test.

**Why:** Code matches spec; worklet crash masked the fix.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| UTF-8 edge case on future non-ASCII names | Full minimal UTF-8 decoder in `readCString`; names today are ASCII |
| Double postScreen on Play (setRunning + frame 20) | Idempotent screen handler; acceptable |
| process() catch hides recurring errors | Log message once via port; main shows retry hint |
| Stale Vite on 5174/5175 | Hard refresh `:5173`; kill extra dev servers |

## Migration Plan

1. Fix `readCString` in `froggers-processor.ts`
2. Add process try/catch + error post
3. Add postScreen calls on setRunning(true) and global randomize
4. `cd web && npm run build`
5. Manual verification checklist (tasks.md)
6. Check off `web-knob-live-values` tasks where code matches

## Open Questions

None — root cause confirmed in DevTools. Implement D1–D3 in order.

## 1. Blocker — worklet string decode (do first)

- [x] 1.1 Replace `readCString` in `web/src/froggers-processor.ts`: scan `Uint8Array` from `ptr` until `0`, decode without `TextDecoder` (ASCII names today; minimal UTF-8 for safety)
- [x] 1.2 Confirm `postScreen()` completes for core pages and Delay page (both `froggers_row_name` and `froggers_delay_row_name` paths)
- [ ] 1.3 Hard refresh `http://localhost:5173/` — DevTools console shows no `TextDecoder` error after Play

## 2. Worklet crash recovery

- [x] 2.1 Wrap `process()` body in try/catch; on catch post `{ type: "error", message }`, zero outputs, return `true`
- [x] 2.2 Verify main `handleWorkletMessage` error path: Play enabled, Stop disabled, status shows retry hint
- [ ] 2.3 Temporarily throw inside `postScreen` in dev to confirm recovery path, then remove test throw

## 3. Immediate UI refresh (noriegas parity)

- [x] 3.1 After `setRunning` sets `this.audioRunning = true`, call `postScreen()`
- [x] 3.2 After `marbles`, `randomizeAll`, `randomizeMod` in `handleUi`, call `postScreen()` (match `randomizePage` pattern)
- [ ] 3.3 Manual: Rand All / Rand Mod / Marbles move knobs while stopped and while playing without waiting ~20 frames

## 4. Verify prior session knob fixes (already in code)

- [x] 4.1 Confirm `syncKnobUi` uses `row.value` when `!knobDragging[i]` (`web/src/main.ts` ~352–363)
- [x] 4.2 Confirm patched-row `onDragStart` snaps to `lastScreenRows[i].modDepth` (`RotaryKnob.ts` + knob factory)
- [x] 4.3 Check off matching tasks in `openspec/changes/web-knob-live-values/tasks.md` after manual pass

## 5. End-to-end verification

- [x] 5.1 `cd web && npm run build` — zero errors
- [ ] 5.2 Play → continuous audio ≥30 s; status stays Playing; Play grayed, Stop enabled
- [ ] 5.3 Mod bay scopes animate while playing; patch Marbles → knob wiggles
- [ ] 5.4 Stop → Play again works; no stuck transport
- [ ] 5.5 Page Randomize, Rand All, Rand Mod, Marbles all visibly update knobs

## 6. Agent notes (read before coding)

- Root cause: `TextDecoder is not defined` in AudioWorklet — not a transport-only bug
- Do not revert `main.ts` transport/knob changes from prior session
- Transcript: `.cursor/projects/.../agent-transcripts/dff58bd4-f9a4-4a3c-8c10-8ad6cb4f0177.jsonl`
- Related OpenSpec: `web-knob-live-values`, `web-sim-layout-ux` (transport ownership)
- Reference UX: https://thenoriegas.info — knobs/scopes/randomize driven by live state

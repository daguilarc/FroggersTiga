## Why

Desktop **Ext. In.** has a peak meter and route diagnostics (`desktop-ext-in-fix`). Web **External** captures mic/line via `getUserMedia` but gives no level feedback — users cannot tell if permission succeeded, the stream is silent, or ring mod is below the Schmidt gate. After `web-sim-bootstrap-fix`, external routing works; the missing piece is visible input level beside the toggle.

## What Changes

### Input peak meter (transport row)

- Add a compact peak bar immediately right of **External: Off/On**, matching desktop `InputEnvelopeIndicator` semantics: idle track when External off or audio stopped; active fill when External on + Play + signal.
- Peak value computed once per worklet `screen` tick from external input samples already in the processor — no second AnalyserNode graph.

### Route hint (status line)

- When External on + Play but peak stays near zero for ~1 s, append a short diagnostic to `#status` (permission blocked, muted mic, or no signal) — parity with desktop `m_routeHint`.

### Docs

- `SIM_MANUAL.md` web section: mention input meter beside External.

## Capabilities

### New Capabilities

- `web-external-input-meter`: Peak bar beside External toggle; idle/active paint; driven by worklet-reported peak.
- `web-external-route-hint`: Status-line diagnostic when External + Play but no input energy.

### Modified Capabilities

- (none — meter is additive; `web-external-audio-permission` flow unchanged)

## Impact

- `web/src/froggers-processor.ts` — accumulate `inputPeak` per block; include in `screen` message
- `web/src/main.ts` — `renderInputMeter(peak, active)`; route-hint logic in status updates
- `web/index.html` — `#external-meter` element beside `#external-btn`
- `web/src/style.css` — meter chrome (idle tick + active fill)
- `SIM_MANUAL.md` — one line on web meter

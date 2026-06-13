## 1. Transport state fix (Play once bug)

- [x] 1.1 `main.ts`: remove `audioRunning = data.audioRunning` from `onScreenUpdate`
- [x] 1.2 Add `transportIntentPlaying` — set true in `startAudio`, false in `stopAudio` and worklet error path
- [x] 1.3 Add `syncTransportUi()` — sets Play/Stop disabled, mod-bay idle from main `audioRunning` only; call from start/stop/error/statechange
- [x] 1.4 `audioContext.onstatechange`: on `suspended` while `transportIntentPlaying`, re-enable Play + status hint; on `running` reconnect if needed
- [x] 1.5 `handleWorkletMessage` error: call `stopAudio()`, clear `transportIntentPlaying`, `syncTransportUi()` — Play enabled, Stop disabled
- [ ] 1.6 Manual: Play 30+ seconds continuous audio; Stop; Play again

## 2. Hide empty mod-route box

- [x] 2.1 `renderModRouteSummary`: if zero routes, `hidden=true` and clear innerHTML; remove `.route-empty` placeholder
- [x] 2.2 Init: hide `#mod-route-summary` on page load (init call or `hidden` in `index.html`) before first screen tick
- [x] 2.3 `style.css`: remove min-height reserved chrome when hidden (no layout gap)
- [ ] 2.4 Pre-Play Audio page: no bordered strip between chrome and knobs

## 3. OLED collapse

- [x] 3.1 Stopped/pre-Play: hide `#oled` (`hidden` or `.oled--stopped { display:none }`)
- [x] 3.2 Playing + desktop: show full OLED; drop forced `min-height: 220px` when empty
- [x] 3.3 Playing + mobile ≤720px: compact strip only (waves + badges); hide name/bar rows
- [x] 3.4 `renderOled`: gate on `audioRunning`; collapse on `stopAudio`

## 4. Knob column cells (no group meta-panels)

- [x] 4.1 `style.css`: `.knob-col` bordered column cell (label + knob + mod source); one CSS rule
- [x] 4.2 `main.ts`: flat `.knobs` row of eight `.knob-col` — no `knob-group` wrappers, no `HOST_PAGE_GROUPS`
- [x] 4.3 Mobile: `.knobs` horizontal scroll when eight columns overflow
- [ ] 4.4 Audio page: eight bordered column cells, no VCOs/Coupling/Output meta-panels
- [ ] 4.5 Filter page: same eight-cell layout; labels update on pill nav

## 5. Verification (required before archive)

- [ ] 5.1 No empty box between page chrome and knobs on Audio pre-Play
- [ ] 5.2 No black 220px void below knobs when stopped (desktop + 390px mobile)
- [ ] 5.3 Play → audible ≥30s → Stop → Play again
- [ ] 5.4 Pill nav updates column labels on Filter page; no group meta-panels in DOM
- [ ] 5.5 Mark archived `web-sim-bootstrap-repair` task §4.4 complete if verified

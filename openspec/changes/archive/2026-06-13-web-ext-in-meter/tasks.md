## 1. Worklet peak tap

- [x] 1.1 `froggers-processor.ts`: accumulate smoothed `inputPeak` from `inputs[0]` when `externalEnabled`
- [x] 1.2 Include `inputPeak` in periodic `screen` postMessage payload

## 2. Meter DOM + styles

- [x] 2.1 `index.html`: add `#external-meter` with inner `#external-meter-fill` after `#external-btn`
- [x] 2.2 `style.css`: idle track (grey + centre tick) and active fill (desktop-parity blue); min-width ~64px, height matches transport buttons

## 3. Main-thread meter + route hint

- [x] 3.1 `main.ts`: `renderInputMeter(peak, active)` — one update sets fill width and idle/active class
- [x] 3.2 `onScreenUpdate`: read `inputPeak`, call `renderInputMeter` when `externalEnabled && audioRunning`
- [x] 3.3 Track sustained silence on main thread; append/clear status hint per `web-external-route-hint`
- [x] 3.4 External off / Stop: force meter idle and clear route hint

## 4. Docs

- [x] 4.1 `SIM_MANUAL.md` web section: peak meter beside **External** when on + Play

## 5. Verification (required before archive)

- [ ] 5.1 External off → idle meter with centre tick
- [ ] 5.2 External on + Play + speak → meter fill moves
- [ ] 5.3 External on + Play + muted mic ~1 s → status silent-input hint
- [ ] 5.4 Signal returns → hint clears, meter reflects peak

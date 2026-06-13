## 1. Remove duplicate panels

- [x] 1.1 `index.html`: remove `#mod-route-summary` and `#oled` elements
- [x] 1.2 `main.ts`: delete `renderModRouteSummary`, route-summary click listener, `renderOled`, `syncOledVisibility`, `highlightKnobRow`, `oledEl`/`modRouteSummaryEl` refs, and all call sites
- [x] 1.3 `style.css`: remove `.mod-route-summary`, `.oled*`, `.oled-row*`, `.knob-col-highlight` styles

## 2. Inline VCO morph buttons

- [x] 2.1 Knob factory: cols 0–2 get `.knob-row` (rotary knob + `.vco-morph-btn`); cols 3–7 keep knob only
- [x] 2.2 Add module-level `lastMorphs: number[] = [0, 0, 0]`; assign in `onScreenUpdate`
- [x] 2.3 Add `renderVcoMorphButtons(wasmPage: number)` — update SVG innerHTML only; visible when `hostPage === 0 && wasmPage === 0`
- [x] 2.4 Wire morph button `click` once in factory → `send({ type: "cycleVcoMorph", index })` — no listener rebind in render
- [x] 2.5 `onScreenUpdate`: call `renderVcoMorphButtons` after `syncKnobUi`; remove route/OLED calls
- [x] 2.6 `setHostPage` / `changeHostPage`: call `renderVcoMorphButtons` after label update
- [x] 2.7 `style.css`: `.knob-row` flex; `.vco-morph-btn` bare blue SVG (28×28, no bordered box)
- [x] 2.8 `style.css`: remove `.vco-morph-btn` border/background; hover dims SVG only; delete unused `.wave-btn`

## 3. Page / transport cleanup

- [x] 3.1 Remove all `syncOledVisibility` call sites (start/stop/statechange/init)
- [x] 3.2 Confirm Play/Stop no longer toggles any panel below knobs

## 4. Docs

- [x] 4.1 `SIM_MANUAL.md`: VCO morph — click waveform beside VCO1–VCO3 knob, not OLED row

## 5. Verification

- [x] 5.1 `cd web && npm run build` — zero errors
- [ ] 5.2 Load page stopped: no text box above knobs, no black box below
- [ ] 5.3 Play with patches: still no route summary or OLED; knobs/scopes animate
- [ ] 5.4 Audio page: waveform buttons beside VCO1–VCO3; click cycles morph; Rand waves updates all three
- [ ] 5.5 Other pages: no morph buttons; knob labels unchanged

## 6. Agent notes

- **Prerequisite:** `web-sim-agent-handoff` applied — worklet must survive `postScreen`
- **Supersedes:** `web-sim-layout-ux` tasks for `web-mod-route-summary` and `web-oled-collapse` — do not re-add those panels
- **Reference:** [thenoriegas.info](https://thenoriegas.info)
- **Follow-on:** `web-ext-in-meter` (apply after this change)

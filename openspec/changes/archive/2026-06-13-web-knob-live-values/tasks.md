## 1. Data path

- [x] 1.1 Add module-level `lastScreenRows: ScreenRow[]` in `web/src/main.ts`; assign in `onScreenUpdate` after `rows` is parsed
- [x] 1.2 Change `syncKnobUi`: when `!knobDragging[i]`, always `setValue(rows[i].value)` — remove `modSource === 255` branch

## 2. Drag snap (desktop parity)

- [x] 2.1 Add optional `onDragStart?: () => void` to `RotaryKnob` — call at pointer-down before `dragStartValue` capture
- [x] 2.2 In knob factory loop: when mod select ≠ None, `onDragStart` snaps knob to `lastScreenRows[i].modDepth` via `setValue`
- [x] 2.3 Confirm patched drag still sends `modDepth` / `delayModDepth`; unpatched still sends `knob` / `delayKnob`

## 3. Labels

- [x] 3.1 Update `updateKnobLabels`: remove "Mod depth" overlay; keep `HOST_PAGE_LABELS[hostPage][i]` as primary label (delay hints unchanged)
- [x] 3.2 Verify `applyStaticKnobLabels` on page change still sets all eight names without waiting for screen

## 4. OLED demotion (no expansion)

- [x] 4.1 Confirm `renderOled` / `#oled` are not required for live value display after 1.2 — document in code comment or leave as-is for optional future removal
- [x] 4.2 Do not add new OLED requirements; knobs are the live-value surface

## 5. Build and verification

- [x] 5.1 `cd web && npm run build` — zero errors
- [ ] 5.2 Manual: Play → patch Marbles 1 to VCO1 → knob wiggles with mod bay scope
- [ ] 5.3 Manual: pointer-down patched knob → snaps to depth; drag changes depth; release → knob resumes wiggle from `row.value`
- [ ] 5.4 Manual: page Randomize and Rand All → idle knobs jump to new positions (patched and unpatched)
- [ ] 5.5 Manual: unpatched row drag still sets base parameter; label stays VCO1/Mix/Crunch etc. when patched

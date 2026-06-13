## 1. Sim delay core (hook + DSP + FUEG)

- [x] 1.1 Add `sim/Fuegoize.hpp` + `sim/Fuegoize_test.cpp` — ≥16 golden tuples vs core `Parameter` path; max diff 0
- [x] 1.2 Add `sim/StereoDelay.hpp` per design §7b (`kMaxDelaySamples=144000`, `process`, `getLastWet`, `toReverbMono`)
- [x] 1.3 Add `sim/DelayState.hpp` — `RuntimeParam`-style smoothing (1000 Hz nat freq); mod matrix; randomize
- [x] 1.4 Add `SimFxInsertFn` + `SetSimFxInsert` to `FroggersEngine`; call after bump; default null
- [x] 1.5 Firmware smoke: 4096-sample noise, hook null, max diff vs baseline = 0; `make` in `src/FroggersTiga`
- [x] 1.6 Sim callback: `StereoDelay::process` → `toReverbMono` into reverb; store last wet
- [x] 1.7 Link `sim/*` into desktop + sim WASM; delay exports + `froggers_select_page(host, 0..4)`
- [x] 1.8 Host stereo bus: `outL/R = coreMono + DMIX * deltaL/R` per design §7; mono fallback

## 2. Patch cables + mod (desktop)

- [x] 2.1 `PatchCableOverlay`: `page >= kDelayPageIndex (5)` → `DelayState` only; grep guard no `SetPageModSource(5`
- [x] 2.2 Cable draw + grab read `DelayState.modSource`; repaint on repatch and Randomize mod
- [x] 2.3 Delay param read: smoothed → mod → `Fuegoize` → DSP

## 3. Panel adapter + randomize (desktop)

- [x] 3.1 Refactor `SubModulePanel` to `IPanelBackend&`; add `DelayHostBackend(DelayState&)` per design §8
- [x] 3.2 Per-panel Randomize / Randomize mod (rows 0–6; FUEG excluded)
- [x] 3.3 `GlobalStrip`: Randomize all + Randomize mod (all) include Delay
- [x] 3.4 Six panels, `width/6`, default ~1680×720 (was 2016; `desktop-compact-layout`); stop clearing output ch1 in `AudioEngine`

## 4. Web — host page overlay

- [x] 4.1 `hostPage` 0–5 state machine per design §1; `froggers_select_page` on 0–4; delay exports on 5
- [x] 4.2 Large ◀ ▶ arrows; 44×44 px; mobile layout
- [x] 4.3 Synthetic OLED on page 5; WASM `postScreen` only when `hostPage ≤ 4`
- [x] 4.4 Delay mod dropdowns → delay exports; global randomize includes Delay
- [x] 4.5 Worklet `outputChannelCount: [2]`; mono fallback; §7 bus math in worklet after `froggers_process`

## 5. Verification (automated)

- [x] 5.1 `froggers_num_pages()` === 5; firmware five pages (firmware `make` passes; WASM export unchanged)
- [x] 5.2 `Fuegoize_test` passes; firmware hook-null noise test passes (`HookIdentity_test`)
- [ ] 5.3 Pre-reverb: DTIM + RPRE independent; DMIX=0 → outL=outR=coreMono — **manual §C**
- [ ] 5.4 Patch cables; global + panel randomize on Delay — **manual §E**
- [ ] 5.5 Stereo width at DMIX=1, DWID=max; mono fallback on 1-ch — **manual §C/G**
- [ ] 5.6 Web: hostPage 3 → `froggers_current_page()` === 3; hostPage 5 → WASM page unchanged — **manual §F**

---

## 6. Manual test plan (for you)

Run these after `/opsx:apply` completes. Check off each box as you go.

### Prerequisites — build everything

```bash
# Repo root
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga

# 1) Automated tests (when wired)
cmake -B build-test -S sim && cmake --build build-test && ctest --test-dir build-test --output-on-failure
# Expect: Fuegoize_test PASS, HookIdentity_test PASS

# 2) Firmware unchanged (no delay in binary behavior)
cd src/FroggersTiga && make clean && make
# Expect: build succeeds; flash optional — only five pages on hardware

# 3) Desktop sim
cmake -B desktop/build -S desktop && cmake --build desktop/build
open desktop/build/FroggersTigaDesktop_artefacts/FroggersTiga.app

# 4) Web sim (requires emscripten for WASM)
cd web && npm run build:all && npm run dev
# Open the URL Vite prints (usually http://localhost:5173)
```

Use **headphones** for stereo checks. Start with **External: Off** (VCO-only) unless a step says otherwise.

---

### A. Automated / regression (no ears required)

| Step | Action | Pass |
|------|--------|------|
| A.1 | Run `Fuegoize_test` | Max diff vs core = 0 |
| A.2 | Run firmware hook-null noise test (4096 samples, insert not registered) | Max sample diff vs baseline = 0 |
| A.3 | In web devtools or a one-liner: call `froggers_num_pages()` after load | Returns **5** |
| A.4 | `grep -r SetPageModSource.*5 desktop/` (or runtime: no crash patching Delay) | No call with page index 5 |

---

### B. Desktop — layout and sixth panel

| Step | Action | Pass |
|------|--------|------|
| B.1 | Launch desktop at default size (~1680×720) | Six panels visible: Audio … Drive, **Delay** |
| B.2 | Resize window narrower (~1200 px) | Panels shrink or scroll; Delay panel still reachable |
| B.3 | Delay panel labels | Rows show **DTIM**, **DSND**, **DFBK**, **DWID**, **DTON**, **DMOD**, **DMIX**, **FUEG** |
| B.4 | Audio settings → stereo output device | Two channels active (not silent on R) |

---

### C. Desktop — delay sound (pre-reverb)

Setup: **Drive** page — bump/drive audible. **Reverb** — **RVMX** mid, **RPRE** low. **Delay** — defaults then tweak.

| Step | Knobs | Listen for | Pass |
|------|-------|------------|------|
| C.1 | **DSND** ↑, **DTIM** ~50%, **DFBK** ~40%, **DMIX** ↑ | Clear repeats; not just reverb wash | Delay audible |
| C.2 | **DTIM** min → max | Echo spacing stretches smoothly (no zipper clicks) | Exponential 0–3 s feel |
| C.3 | **DMIX** = 0 | Repeats gone; dry/core tone unchanged in level | Wet bypass |
| C.4 | **DSND** = 0 | Silent delay regardless of **DMIX** | Send gate works |
| C.5 | On **Reverb**, **RPRE** ↑ while Delay **DTIM** fixed | Reverb pre-gap changes; echo spacing unchanged | RPRE ⊥ DTIM |
| C.6 | **DWID** = 0 vs max, **DMIX** high, headphones | Max width: L/R repeats feel separated; zero: centered/mono repeats | Width works |
| C.7 | **DTON** ↑ with high **DFBK** | Feedback darkens; less harsh treble hash | Tone LP works |
| C.8 | **DMOD** ↑ | Pitch wobble on delay time | Mod depth works |

---

### D. Desktop — FUEG

| Step | Action | Pass |
|------|--------|------|
| D.1 | Fix **DTIM** ~50%; sweep **FUEG** 0 → 1 | Effective delay time jumps/scrambles (same character as Reverb **FUEG**) |
| D.2 | Per-panel **Randomize** on Delay | Rows 0–6 move; **FUEG** unchanged |
| D.3 | Compare Reverb + Delay **FUEG** at same knob value on same row index | Similar discontinuous scramble (not identical timbre, same *style*) |

---

### E. Desktop — patch cables and randomize

| Step | Action | Pass |
|------|--------|------|
| E.1 | Drag **Marbles 1** output → Delay **DTIM** input jack | Cable persists; **DTIM** wobbles with Marbles |
| E.2 | Repatch same cable to **DFBK** row | Old row clears; **DFBK** modulates |
| E.3 | Drag connected plug to empty space | Row mod source = none; cable gone |
| E.4 | Delay panel **Randomize mod** | Delay mod **sources and depths** change (sim-valid only) |
| E.5 | Global **Randomize all** | All six panels' knobs change (Delay 0–6) |
| E.6 | Global **Randomize mod (all)** | Core pages + Delay mod **sources and depths** change (sim-valid only) |
| E.7 | Drag from empty Delay **DTIM** input → **Marbles 1** output | Cable persists; DTIM modulates (bidirectional patch per `desktop-host-mutation-safety`) |

---

### F. Web — navigation and OLED

| Step | Action | Pass |
|------|--------|------|
| F.1 | Tap **▶** from Drive until page reads **Delay (6/6)** | Sixth page; labels **DTIM…FUEG** (not WASM Reverb names) |
| F.2 | Tap **◀** back to **Filter (4/6)** or **Reverb (3/6)** | WASM page matches (knob/OLED names match that core page) |
| F.3 | Go to **Reverb (3/6)**, note OLED; go to **Delay (6/6)** | Delay OLED shows delay params, not stale Reverb rows |
| F.4 | Narrow viewport ≤720 px width (phone or devtools) | ◀ ▶ remain beside knobs; no horizontal knob scroll |

---

### G. Web — delay sound and stereo

Repeat **section C** on web (same knob semantics). Additional checks:

| Step | Action | Pass |
|------|--------|------|
| G.1 | **Mic Off**, play with Delay send | Same pre-reverb delay behavior as desktop (roughly) |
| G.2 | **Mic On** + external input | Delay still on output bus after core chain |
| G.3 | Stereo headphones | **DWID** max + **DMIX** high → L/R separation audible |
| G.4 | Global **Randomize all** while on Delay page | Core + Delay both randomize |

---

### H. Firmware / Daisy Field (must stay unchanged)

Only if you flash hardware:

| Step | Action | Pass |
|------|--------|------|
| H.1 | Flash `src/FroggersTiga` build | Success |
| H.2 | Cycle **SW1/SW2** | Exactly **five** pages; no **Delay** label |
| H.3 | A/B same patch on pre-change vs post-change firmware | No new FX; timbre matches at same knobs |

---

### I. Sign-off checklist

- [ ] All of **A** pass
- [ ] Desktop **B–E** pass
- [ ] Web **F–G** pass
- [ ] Firmware **H** pass (or skipped if no hardware flash)
- [ ] No crash when patching Delay cables or spamming Randomize
- [ ] No audible clicks when sweeping **DTIM** during playback

If anything fails, note which step ID (e.g. **C.5**, **E.1**) and host (desktop/web/firmware) before fixing.

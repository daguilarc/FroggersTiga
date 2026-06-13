## 1. StereoDelay DSP

- [x] 1.1 `kMaxDelaySeconds = 2.0f`, `kMaxDelaySamples = 96000`
- [x] 1.2 Remove `m_toneL/R`, `m_toneAlpha`, tone smooth block and `setSampleRate` tone-alpha computation
- [x] 1.3 Add detune: map row-4 knob to ±`maxCents` (50); `ratio = 2^(±cents/1200)` on `timeL`/`timeR` before `readAt`
- [x] 1.4 Rename `DelayParams::dton` → `ddet` (update `DelayState` read path)

## 2. Display names + docs

- [x] 2.1 `ParamDisplayNames`: Detune, Comb line, Peak freq/gain/Q, XOR, Bit depth, Reverb **Stereo width** + **Diffusion**
- [x] 2.2 `QUICK_DICT.md` + `web/public/quick-dict.md` (via `npm run sync:docs`): all touched rows; Delay time `~0–2 s`
- [x] 2.3 `web/public/manual.md` Reverb/Filter/Drive sections; `web/src/main.ts` `DELAY_HINTS[0]` `~0–2 s`; `PAGE_BLURBS[5]` drop "tone"
- [x] 2.4 Grep obsolete sim labels: Tone, Pure delay, Bump, Reorganizer, LFO depth/rate, `3.0 s`, `~0–3`

## 2b. Reverb stereo width + diffusion

- [x] 2b.1 `FroggersEngine.hpp`: remove `m_rvLfoPhase`; rename `m_rvModDepth`/`m_rvModRate` → `m_rvWidth`/`m_rvDiffusion`; linear `SetTarget` from params 5–6
- [x] 2b.2 Single feedback matrix with `diffusion` cross-blend; width pan law; store `m_reverbWetL/R`
- [x] 2b.3 `getReverbWetL/R()` + `getReverbMonoWet()` on engine; `ApplyOutputFx` uses mono wet sum for dry mix
- [x] 2b.4 `applyStereoBus` in `sim/DelayState.hpp` + `desktop/Source/AudioEngine.cpp`: one loop merges delay + reverb stereo deltas
- [x] 2b.5 Manual verify: width widens stereo reverb; diffusion densifies tail; no LFO chorus

## 3. Verify

- [x] 3.1 `npm run build:wasm`
- [x] 3.2 Detune max: stereo pitch spread on repeats; Mod depth still LFO-only
- [x] 3.3 DTIM max ≈ 2 s; labels: Comb line, Detune, Peak EQ, XOR, Bit depth, Reverb width/diffusion

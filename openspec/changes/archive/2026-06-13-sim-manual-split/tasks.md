## 1. Author SIM_MANUAL.md

- [x] 1.1 Create `SIM_MANUAL.md` at repo root (intro, quick start, transport, mod bay, pages 1–6 + Delay, desktop/web notes, firmware pointer footer)
- [x] 1.2 Knob tables use `ParamDisplayNames.hpp` strings exactly (grep verify: Comb offset, Stereo width, Diffusion, XOR, Bit depth, Crunch)
- [x] 1.3 No Pure delay, LFO depth/rate, Digital reorganizer, or FUEG as primary sim labels

## 2. Sync and web

- [x] 2.1 `scripts/sync-help-docs.sh`: copy `SIM_MANUAL.md` → `web/public/sim-manual.md`; remove `MANUAL.md` copy
- [x] 2.2 `web/src/main.ts`: `HELP_DOC_PATHS.manual.path` → `/sim-manual.md`
- [x] 2.3 Delete stale `web/public/manual.md` (or let sync stop producing it)
- [x] 2.4 Web Help → Manual shows Comb offset in Filter section

## 3. Desktop embed

- [x] 3.1 `desktop/CMakeLists.txt`: swap `MANUAL.md` for `SIM_MANUAL.md` in BinaryData
- [x] 3.2 `desktop/Source/AppMenuBar.cpp`: load `BinaryData::SIM_MANUAL_md`
- [ ] 3.3 Rebuild desktop; About → Manual shows sim manual

## 4. Quick Dict intro

- [x] 4.1 `QUICK_DICT.md` line 3: in-app Manual = sim guide; Field hardware = repo `MANUAL.md`
- [x] 4.2 `npm run sync:docs` updates `web/public/quick-dict.md`
- [x] 4.3 Intro satisfies `quick-dict-format` "Depth deferred to Manual" (sim vs Field split)

## 5. README

- [x] 5.1 `README.md`: document `SIM_MANUAL.md` (sim) vs `MANUAL.md` (firmware) as separate files

## 6. Verification

- [x] 6.1 Grep sim assets: no `Pure delay` in `sim-manual.md` / embedded binary
- [x] 6.2 `MANUAL.md` unchanged (git diff empty on that file)
- [x] 6.3 Quick Dict line 3 does not say "Field OLED symbols … → **Manual**" without clarifying sim vs firmware

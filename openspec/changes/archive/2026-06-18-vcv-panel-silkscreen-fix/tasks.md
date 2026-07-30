## 0. Hygiene

- [x] 0.1 Delete stale `FroggersTigaVoicing*.svg` (v2.4; superseded by 72 HP main)
- [x] 0.2 Add `sim/check_vcv_panel_svg.sh` — fail on live `<text>` in shipped panel SVGs

## 1. Postmortem + pipeline design

- [x] 1.1 Confirm Rack ignores `<text>` on current SVGs (repro in design.md; CI gate added)
- [x] 1.2 **Decision:** static path SVG via `fontTools` (typical VCV workflow; no runtime `ui::Label`)
- [x] 1.3 Parse `ParamDisplayNames.hpp` + `VcvPanelLayout.hpp` in generator (no duplicate `ROWS`/`PAGES`)
- [x] 1.4 Layout anchors mirror `FieldParityWidget` math + `kHeaderStripGridY` offset

## 2. Branding + panel SVGs

- [x] 2.0 Trace frog logo from `web/public/froggerstiga.png` → `vcv/res/frog_logo.svg` (path-based)
- [x] 2.1 Main 72 HP: header + path column titles, row labels, mod rack, I/O, CC, MIDI
- [x] 2.2 FX 36 HP: header + path Reverb/Delay columns + stereo jack labels
- [x] 2.3 Dark variants (`*-dark.svg`)
- [x] 2.4 Font size pass — 2.5–3.4 mm cap heights for 100% non-HiDPI

## 3. Build + install

- [x] 3.1 `generate_panels.py` + `trace_frog_logo.py` wired in `build.sh`
- [x] 3.2 `arch -x86_64 make dist` exit 0 → `FroggersTiga-2.5.1-mac-x64.vcvplugin`
- [x] 3.3 `./build.sh --install` → `plugins-mac-x64/FroggersTiga-2.5.1-mac-x64.vcvplugin`
- [x] 3.4 Installed bundle SVG check: extracted `.vcvplugin` has paths, no live `<text>`

## 4. Visual verification (gate — manual in Rack)

- [ ] 4.1 Rack 100% zoom: main module — all labels visible without hover
- [ ] 4.2 Rack 100% zoom: FX module — all labels visible without hover
- [ ] 4.3 PM3 / Crispy labels match desktop on Audio column
- [ ] 4.4 No CC/gate/I/O label overlap with jacks at 100%
- [ ] 4.5 Light + dark theme check

## 5. Docs

- [x] 5.1 `vcv/DEVELOPMENT.md`: nanosvg limitation + regen commands + SVG CI check
- [x] 5.2 `design.md` amended — D2: SVG silkscreen must be paths; supersedes corner `<text>` watermark

## 1. Layout authority (`FieldParityWidget.hpp`)

- [ ] 1.1 Finalize constants: `kPrimaryHp=24`, `kExpanderHp=36`, `kExpanderColumns=3`, `kRows` from `ParamDisplayNames::kNumRows`
- [ ] 1.2 Add `constexpr` precomputed `kColumnCenters3[3]` and row Y helper `rowCenterY(row)` — O(1) lookup at widget build
- [ ] 1.3 Add `addPageTitleLabel(ModuleWidget*, x, y, hostPage)` using `ParamDisplayNames::forHostPage`
- [ ] 1.4 Remove dead APIs; ensure header includes `HostPanelLayout.hpp` / `ParamDisplayNames.hpp` only

## 2. Expander module split (`plugin.cpp`)

- [ ] 2.1 Replace `FroggersTigaExpanderModule` with template `FroggersTigaExpanderModuleT<kFirstPage>`
- [ ] 2.2 Typedef Expander A (`kFirstPage=0`) and Expander B (`kFirstPage=3`)
- [ ] 2.3 Implement `primaryModule()` chain walk + cached `FroggersTigaModule*` (invalidate on unlink — H2)
- [ ] 2.4 Override `onExpanderChange` or equivalent to refresh cache when Rack relinks expanders

## 3. Widget rewrite

- [ ] 3.1 Primary widget: `primaryPanelSize()`, reposition mod rack + I/O within 24 HP
- [ ] 3.2 Expander widget template: 3 columns, page title + knob + mod jack only (no row labels)
- [ ] 3.3 Register `modelFroggersTigaExpanderA` and `modelFroggersTigaExpanderB`; remove single expander model
- [ ] 3.4 Update `plugin.json` slugs/names/description; bump version to 2.3.0

## 4. Verification scripts and CI

- [ ] 4.1 Add `sim/check_vcv_panel_bounds.sh` — parse HP constants, assert widget math fits columns
- [ ] 4.2 Wire bounds script into CI (Pages workflow or dedicated VCV job)
- [ ] 4.3 `make` clean build with Rack SDK 2.4.1 x64 — exit 0

## 5. Build, install, manual Rack tests

- [ ] 5.1 Rebuild with `arch -x86_64 make dist && make install` → `~/Documents/Rack2/plugins-mac-x64/`
- [ ] 5.2 Manual: module browser preview — Primary, Expander A, Expander B — no crash
- [ ] 5.3 Manual: place Primary + A + B chained; all 48 knobs/jacks visible and patchable at 100% zoom
- [ ] 5.4 Manual: Expander B alone in browser (unlinked) — no crash; process no-ops safely
- [ ] 5.5 Update `vcv/DEVELOPMENT.md` with 3-module topology and Rosetta one-liner

## 6. Cross-change hygiene

- [ ] 6.1 Note in `vcv-vst-field-parity-panel` tasks that D3 fallback is satisfied by this change
- [ ] 6.2 Mark `vcv-vst-field-parity-panel` manual verification tasks 5.x ready after 5.3 passes

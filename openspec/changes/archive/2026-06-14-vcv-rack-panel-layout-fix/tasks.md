## 1. Layout authority (`FieldParityWidget.hpp`)

- [x] 1.1 Finalize constants: `kPrimaryHp=24`, `kExpanderHp=36`, `kExpanderColumns=3`, `kRows` from `HostPanelLayout::kNumRows`
- [x] 1.2 Add `constexpr` precomputed `kColumnCenters3[3]` and row Y helper `rowCenterY(row)` — O(1) lookup at widget build
- [x] 1.3 Add `addPageTitleLabel(ModuleWidget*, x, y, hostPage)` using `ParamDisplayNames::forHostPage`
- [x] 1.4 Remove dead APIs; ensure header includes `HostPanelLayout.hpp` and `ParamDisplayNames.hpp`
- [x] 1.5 Fix compile break: add `#include "HostPanelLayout.hpp"` (line 12 references `HostPanelLayout::kNumRows` today without include)
- [x] 1.6 Add `addCornerScrews(ModuleWidget*, Vec panelSize)` — shared by primary and expander widgets (design D7)
- [x] 1.7 Add primary I/O jack placement table (`constexpr` x/id/isOutput rows) + single placement loop (design D7)

## 2. Expander module split (`plugin.cpp`)

- [x] 2.1 Replace `FroggersTigaExpanderModule` with template `FroggersTigaExpanderModuleT<kFirstPage>`
- [x] 2.2 Typedef Expander A (`kFirstPage=0`) and Expander B (`kFirstPage=3`)
- [x] 2.3 Implement `primaryModule()` chain walk + cached `FroggersTigaModule*` (invalidate on unlink — H2)
- [x] 2.4 Override `onExpanderChange` or equivalent to refresh cache when Rack relinks expanders
- [x] 2.5 Unify `columnParamOffset` / `columnInputOffset` into single `columnBaseOffset(column)` (audit repetition fix)

## 3. Widget rewrite

- [x] 3.1 Primary widget: `primaryPanelSize()`, reposition mod rack + I/O within 24 HP using table from 1.7
- [x] 3.2 Expander widget template: 3 columns, page title + knob + mod jack only (no row labels)
- [x] 3.3 Register `modelFroggersTigaExpanderA` and `modelFroggersTigaExpanderB`; remove single expander model
- [x] 3.4 Update `plugin.json` slugs/names/description; bump version to 2.3.0
- [x] 3.5 Replace duplicated screw blocks with `addCornerScrews` in primary and expander widget ctors

## 4. Verification scripts and CI

- [x] 4.1 Add `sim/check_vcv_panel_bounds.sh` — parse HP constants, assert widget math fits columns
- [x] 4.2 Wire bounds script into CI (Pages workflow or dedicated VCV job)
- [x] 4.3 `make` clean build with Rack SDK 2.4.1 x64 — exit 0
- [x] 4.4 Merge gate: header API renames and `plugin.cpp` consumer updates land in same commit; verify `make` exit 0 before merge

## 5. Build, install, manual Rack tests

- [ ] 5.1 Rebuild with `arch -x86_64 make dist && make install` → `~/Documents/Rack2/plugins-mac-x64/`
- [ ] 5.2 Manual: module browser preview — Primary, Expander A, Expander B — no crash
- [ ] 5.3 Manual: place Primary + A + B chained; all 48 knobs/jacks visible and patchable at 100% zoom
- [ ] 5.4 Manual: Expander B alone in browser (unlinked) — no crash; process no-ops safely
- [x] 5.5 Update `vcv/DEVELOPMENT.md` with 3-module topology and Rosetta one-liner
- [ ] 5.6 H5 decision gate: if 5.2 still crashes, remove `SmallLight` on mod outputs and re-test; document outcome

## 6. Cross-change hygiene

- [x] 6.1 Note in `vcv-vst-field-parity-panel` tasks that D3 fallback is satisfied by this change
- [ ] 6.2 Mark `vcv-vst-field-parity-panel` manual verification tasks 5.x ready after 5.3 passes
- [x] 6.3 Mark `vcv-vst-field-parity-panel` design D3 fallback row as implemented (3+3 split)

## 7. Local-only VCV and VST policy (design D8)

- [x] 7.1 Change `desktop/CMakeLists.txt` default: `option(BUILD_VST ... OFF)` (was ON)
- [x] 7.2 Confirm `.git/info/exclude` documents `vcv/` local-only; add note for VST build artifacts (`desktop/build/`, `FroggersTigaPlugin_artefacts/`)
- [x] 7.3 Verify public GitHub Actions workflow does not invoke VCV `make` or `BUILD_VST=ON`
- [x] 7.4 Document in `vcv/DEVELOPMENT.md` and `desktop/PACKAGING.md`: VCV and VST are local-only targets; public `main` ships desktop + web only
- [ ] 7.5 VST smoke (local only): `cmake -B build -DBUILD_VST=ON && cmake --build build` — confirm `MainComponent` editor opens; no VCV layout files required

## Audit checklist (pre-apply verification)

| Check | Pass criteria |
|-------|---------------|
| Nesting depth ≤ 4 | All modified functions verified |
| No copy-paste I/O jacks | Table + loop in primary widget |
| No duplicate screw blocks | `addCornerScrews` used twice only |
| No removed API references | `panelSize`, `addRowLabel`, 2-arg `columnCenterX` absent |
| Header includes complete | `HostPanelLayout.hpp` included |
| VST unaffected | No edits to `PluginEditor.cpp` / `MainComponent` layout for this change |
| Public CI local-only | `BUILD_VST=OFF`, no VCV build on `main` |

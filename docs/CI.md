# CI workflows

Workflow definitions live in `.github/workflows/`. Policy is documented here in public repo docs. `openspec/` is local-only planning state on this machine and is not a git-backed source of CI truth.

## Workflows

| Workflow | Trigger | Purpose |
|----------|---------|---------|
| `pages.yml` | `main` push, manual | Web-only checks, WASM + Vite build, GitHub Pages deploy |
| `desktop-release.yml` | Tag `froggerstiga-v1` | macOS DMG + Windows EXE release assets |

## Local Checks

These checks are intentionally local. They are useful before pushing changes, but they do not need public GitHub workflow files.

| Script | What it guards |
|--------|----------------|
| `scripts/check_host_artifact_hygiene.sh` | No public build/cache artifacts; shared path classes from `scripts/repo_path_policy.sh` |
| `scripts/check_openspec_hygiene.sh` | Local OpenSpec lifecycle using filesystem/OpenSpec CLI only; no git operations |
| `desktop/scripts/verify-release-metadata.sh` | CMake, web package, README/SIM_MANUAL version alignment |
| `sim/check_common_core_wrappers.sh` | `src/common` headers that mirror `src/core` stay thin firmware compatibility wrappers |
| `sim/check_vcv_panel_bounds.sh` | VCV panel HP/layout constants |
| `sim/check_vcv_panel_svg.sh` | VCV SVG panel files |
| `sim/check_vcv_midi_boundary.sh` | No MIDI/CC leakage into `vcv/src` |
| `sim/check_vcv_license_boundary.sh` | GPL boundary for Rack plugin tree |
| `npm --prefix web run test:e2e` | Browser regression coverage after WASM/web changes |

Run locally:

```bash
scripts/check_host_artifact_hygiene.sh
scripts/check_openspec_hygiene.sh
desktop/scripts/verify-release-metadata.sh
sim/check_common_core_wrappers.sh
sim/check_vcv_panel_bounds.sh
sim/check_vcv_panel_svg.sh
sim/check_vcv_midi_boundary.sh
sim/check_vcv_license_boundary.sh
npm --prefix web run test:e2e
```

## pages.yml

Web publication gate only. Fails deploy if the live sim would show wrong labels or stale help mirrors.

| Script | What it guards |
|--------|----------------|
| `sim/check_operator_docs_sync.sh` | `SIM_MANUAL.md` / `QUICK_DICT.md` match `docs/` and `web/public/` mirrors; public SIM manual stays desktop/web-only until VST/AU and VCV launch |
| `scripts/generate-host-display.mjs --check` | `web/src/hostDisplay.generated.ts` is fresh |
| `scripts/verify-host-display-shape.mjs` | Host display projection shape |
| `sim/check_host_display_projections.sh` | Desktop catalog + web WASM pool projections |
| `sim/check_mod_source_labels.sh` | Mod source labels match `ParamDisplayNames` |

Run locally:

```bash
sim/check_operator_docs_sync.sh
node scripts/generate-host-display.mjs --check
node scripts/verify-host-display-shape.mjs
sim/check_host_display_projections.sh
sim/check_mod_source_labels.sh
```

## Local full sweep

`scripts/verify_clean_rebuild.sh` rebuilds sim, web, and desktop from clean trees and catches stale generated files.

## Path Classes

`scripts/repo_path_policy.sh` is the shared local source for hygiene path classes:

- public repo source: reviewed project code, scripts, docs, and intentional Pages output
- generated publication output: `docs/` and `web/public/` mirrors/assets
- local cache/build output: build directories, downloaded Node runtimes, Emscripten SDK, Rack SDK, JUCE/CMake caches
- local-only product surfaces: VCV and VST/AU development paths that are not public release surfaces yet
- local-only planning state: `openspec/`
- firmware exclusions: Daisy firmware/source surfaces that are outside host cleanup

## Shared Code Boundary

Shared DSP, parameter, display-name, and control logic should land in `src/core/` or `sim/` first. `src/common/` remains a firmware compatibility layer: headers that share a name with `src/core/<name>.hpp` are expected to be two-line include wrappers, and firmware/hardware adapter files such as `App.hpp`, `DaisyIO.hpp`, and `Include.hpp` are treated as firmware-side exceptions.

Existing firmware build outputs under `src/FroggersTiga/build`, `src/TestControl/build`, and `src/Blink/build` are outside this host cleanup. Removing or reclassifying those artifacts should be handled by a separate firmware-scoped change.

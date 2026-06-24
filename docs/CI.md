# CI workflows

Workflow definitions live in `.github/workflows/`. Policy is documented here (tracked source), not under `openspec/` (local-only planning on this machine).

## Workflows

| Workflow | Trigger | Purpose |
|----------|---------|---------|
| `host-preflight.yml` | `main` push, PRs, manual | Repo hygiene, release metadata, VCV layout constants |
| `pages.yml` | `main` push, manual | Web-only checks, WASM + Vite build, GitHub Pages deploy |
| `web-e2e.yml` | `main` / PR (web paths), manual | Playwright e2e after WASM build |
| `desktop-release.yml` | Tag `froggerstiga-v1` | macOS DMG + Windows EXE release assets |

## host-preflight.yml

Runs before or alongside deploy; failures here do **not** block Pages by design.

| Script | What it guards |
|--------|----------------|
| `scripts/check_host_artifact_hygiene.sh` | No tracked build artifacts; no blanket `openspec/` in `.gitignore` |
| `scripts/check_openspec_hygiene.sh` | OpenSpec lifecycle (skipped on CI when `openspec` CLI absent) |
| `desktop/scripts/verify-release-metadata.sh` | CMake, web package, README/SIM_MANUAL version alignment |
| `sim/check_vcv_panel_bounds.sh` | VCV panel HP/layout constants |
| `sim/check_vcv_panel_svg.sh` | VCV SVG panel files |
| `sim/check_vcv_midi_boundary.sh` | No MIDI/CC leakage into `vcv/src` |
| `sim/check_vcv_license_boundary.sh` | GPL boundary for Rack plugin tree |

Run locally:

```bash
.github/workflows/host-preflight.yml  # see steps in file, or:
scripts/check_host_artifact_hygiene.sh
desktop/scripts/verify-release-metadata.sh
sim/check_vcv_panel_bounds.sh
sim/check_vcv_panel_svg.sh
sim/check_vcv_midi_boundary.sh
sim/check_vcv_license_boundary.sh
```

## pages.yml

Web publication gate only. Fails deploy if the live sim would show wrong labels or stale help mirrors.

| Script | What it guards |
|--------|----------------|
| `sim/check_operator_docs_sync.sh` | `SIM_MANUAL.md` / `QUICK_DICT.md` match `docs/` and `web/public/` mirrors |
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

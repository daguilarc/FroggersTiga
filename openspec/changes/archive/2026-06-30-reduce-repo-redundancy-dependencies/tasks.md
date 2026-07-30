## 1. Local planning truth

- [x] 1.1 Document OpenSpec as local-only planning state that does not run git commands or require git worktrees
- [x] 1.2 Preserve local-only OpenSpec ignore/exclude behavior while ensuring cache/session ignores remain explicit
- [x] 1.3 Update OpenSpec hygiene checks so they validate local OpenSpec semantics without git inspection
- [x] 1.4 Document local-only VCV/VST source policy separately from public repo docs so CI coverage claims stay honest

## 2. Shared repository path classification

- [x] 2.1 Add a small shared path classification file or script helper for public repo source, generated publication output, local cache/build output, local-only planning/product surfaces, and firmware exclusions
- [x] 2.2 Refactor `scripts/check_host_artifact_hygiene.sh` to consume the shared classification
- [x] 2.3 Refactor `scripts/check_openspec_hygiene.sh` to consume the OpenSpec portions of the shared classification
- [x] 2.4 Update `docs/CI.md` and README maintenance notes to explain the path classes and local-only exceptions

## 3. Dependency and cache surface reduction

- [x] 3.1 Document current large surfaces and policy: public `External/`, local `.emsdk/`, local `node-v*`, local `Rack-SDK/`, JUCE FetchContent cache, and build output directories
- [x] 3.2 Add or verify ignore rules for downloaded Node runtimes, tarballs, desktop VST test build directories, and other local dependency caches
- [x] 3.3 Add desktop/VST build documentation for pinned JUCE version and local cache/override usage so configure is not silently network-dependent
- [x] 3.4 Confirm no new npm, CMake, or system dependency is added by this cleanup

## 4. Mirrors and generated publication artifacts

- [x] 4.1 Make root manuals/license the declared authorities for `docs/` and `web/public/` help mirrors
- [x] 4.2 Remove VST/AU, plugin, VCV, and Rack availability wording from `SIM_MANUAL.md`
- [x] 4.3 Rewrite getting-started, Delay, mod routing, host guide, and host input-boundary sections for desktop standalone and web sim only
- [x] 4.4 Sync `docs/sim-manual.md` and `web/public/sim-manual.md` from root `SIM_MANUAL.md`
- [x] 4.5 Extend docs sync verification so every intentionally mirrored help/public file is checked or regenerated from its authority
- [x] 4.6 Add or update docs verification to reject pre-launch VST/AU, plugin, VCV, and Rack user-facing mentions in public sim manual mirrors while exempting internal OpenSpec specs and development docs
- [x] 4.7 Keep intentional Pages artifacts named as publication outputs, and ensure hygiene does not allow unrelated build outputs by accident
- [x] 4.8 Verify `npm --prefix web run build` or equivalent still produces fresh web/public/docs expectations without manual copy steps beyond the documented sync

## 5. Shared code boundary guardrails

- [x] 5.1 Add a guard for mirrored `src/common/<name>.hpp` and `src/core/<name>.hpp` headers: wrappers must remain thin includes unless listed as firmware-only adapter exceptions
- [x] 5.2 Document that shared DSP/control logic lands in `src/core` or `sim` first, while `src/common` remains firmware compatibility and hardware adapter code
- [x] 5.3 Review firmware build outputs under `src/FroggersTiga/build`, `src/TestControl/build`, and `src/Blink/build`; either document them as firmware exceptions or propose a separate firmware-scoped cleanup

## 6. Verification and closure

- [x] 6.1 Run OpenSpec validation for `reduce-repo-redundancy-dependencies` and affected baseline specs
- [x] 6.2 Run host artifact hygiene, OpenSpec hygiene, docs sync checks, host display checks, and release metadata checks
- [x] 6.3 Run web build checks that cover docs sync and generated host display freshness
- [x] 6.4 Inspect local ignore/path policy, OpenSpec local-only behavior, and prohibited generated/cache artifacts; record final evidence in the change notes
- [x] 6.5 Confirm no package version bump, release publication, remote tag mutation, or forced deletion of local cache/build output occurred

## Verification evidence

- `openspec validate reduce-repo-redundancy-dependencies --strict`
- `openspec validate --specs --strict`
- `bash scripts/check_openspec_hygiene.sh`
- `bash scripts/check_host_artifact_hygiene.sh`
- `bash sim/check_operator_docs_sync.sh`
- `bash sim/check_common_core_wrappers.sh`
- `node scripts/generate-host-display.mjs --check`
- `node scripts/verify-host-display-shape.mjs`
- `bash sim/check_host_display_projections.sh`
- `bash sim/check_mod_source_labels.sh`
- `bash desktop/scripts/verify-release-metadata.sh`
- `npm --prefix web run build`
- Public SIM manual launch-gate scan for `VST`, `AU`, `plugin`, `VCV`, and `Rack` returned no matches in `SIM_MANUAL.md`, `docs/sim-manual.md`, or `web/public/sim-manual.md`.
- Mini subagent review used `gpt-5.4-mini` with no git, no install, no network, and no escalation. It found one regex edge case in `scripts/check_openspec_hygiene.sh`; the parent fixed it and reran the OpenSpec hygiene check successfully.

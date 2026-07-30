## Context

`omni-repository-harmonization` already establishes the central rule for host behavior: shared concepts need one authority, while host-specific UI and routing differences are allowed when expressed as explicit projections. This sweep found a second maintenance layer that still makes future updates harder:

- `.git/info/exclude` locally ignores `openspec/`; that is intentional for this workspace. OpenSpec is local planning state, not a git-backed source of truth.
- `.gitignore` and `.git/info/exclude` also hide local-only VCV and VST/AU sources. That can be valid for public release policy, but it must be explicit because public CI cannot enforce those surfaces.
- `External/` is about 325 MB and 12,993 files. `.emsdk/` is about 1.6 GB local, `node-v22.16.0-darwin-arm64/` plus its tarball are about 224 MB local, and `desktop/build/` is about 679 MB local output.
- Published help assets exist in root, `docs/`, and `web/public/`. `scripts/sync-help-docs.sh` copies most mirrors, while `sim/check_operator_docs_sync.sh` checks parity, but the ownership model is still scattered across scripts/docs.
- The public SIM manual currently describes VST/AU and VCV surfaces that are still local-only/unlaunched pending testing. Internal specs may keep those contracts, but user-facing manuals should not present them as launched.
- `src/common` contains firmware compatibility wrappers for many `src/core` headers plus hardware-only helpers. This is not a DSP duplication problem today because most wrappers are two-line includes, but it needs a rule so future shared DSP work does not drift into both places.
- Verification scripts repeat path classifications: host artifacts, OpenSpec artifacts, docs mirrors, VCV license/MIDI boundaries, and host display checks each carry local allow/deny lists.

The existing desktop release constraint remains mandatory: only `froggerstiga-v1` is the desktop release channel, web links must keep that tag, no `desktop-v*` tags are introduced, and desktop-only cleanup does not bump CMake/package version.

## Goals / Non-Goals

**Goals:**

- Make local-only planning and product surfaces explicit so docs and checks do not imply public-source verification.
- Reduce future update cost by classifying dependency/tooling surfaces into tracked source, generated publication output, local cache, and external install/cache.
- Preserve host/version UI differences as valid projections rather than forcing identical UI across desktop, web, VCV, and VST/AU.
- Keep firmware compatibility wrappers thin and prevent shared DSP ownership from splitting between `src/common` and `src/core`.
- Consolidate path and artifact classifications so hygiene scripts do not drift.
- Add gates that catch stale docs/public mirrors, pre-launch host mentions in public manuals, and accidental dependency/tool output leakage.

**Non-Goals:**

- No application behavior change, DSP change, release publication, remote tag mutation, or version bump.
- No deletion of user-local caches or build products during proposal or apply; cleanup should be index-only or clearly documented.
- No forced public tracking of local-only OpenSpec, VCV, or VST sources unless the project owner chooses to change that distribution policy.
- No attempt to replace intentional `docs/` Pages artifacts unless the Pages publication workflow is redesigned.
- No audit of upstream vendored code internals beyond footprint and boundary classification.
- No removal of VST/AU or VCV implementation/spec/test planning; only public SIM manual availability language is launch-gated.

## Decisions

### D1 — Classify every large/dependency surface before changing it

Use four categories:

| Category | Examples | Rule |
|----------|----------|------|
| Public repo source | project C++/TS/scripts/docs, intentional Pages assets | Repo-visible source; hygiene allows only named generated exceptions |
| Generated publication output | `docs/assets/*`, `docs/froggers.wasm`, `web/public/*` mirrors | Either generated during release/build or freshness-checked from a root authority |
| Local cache/build output | `.emsdk/`, `node-v*`, `desktop/build/`, `sim/build/`, `wasm/build/`, `desktop/build-vst-test/` | Ignored and never required for clean-clone correctness |
| External/local-only product surfaces | VCV SDK/plugin, local VST/AU source policy | Explicitly documented as local-only or moved into tracked source by separate decision |

Alternative rejected: a single blanket ignore strategy. It hides real source truth, as shown by the current `openspec/` exclude.

### D2 — Keep OpenSpec local-only and git-free

OpenSpec is local planning state for this workspace. OpenSpec commands, helpers, and subagents must not perform git operations or require git worktrees. If a separate checkout needs a plan, the primary agent or user can copy local files by ordinary filesystem means, but this change does not add a helper for that.

Alternative rejected: making OpenSpec git-aware or worktree-aware. That adds machinery the project does not need for local-only planning.

### D3 — Preserve host UI differences through projection metadata

Different surfaces may keep different UI features. The rule is:

```
shared concept -> one authority -> host projection -> host UI
```

Violations are copied, independently edited authorities. Valid differences include desktop dual CC cells, web CC 1-only, VCV CV-only routing, VST/AU host-parameter routing, and desktop/web layout differences.

Alternative rejected: making all hosts visually identical. That would erase host-native behavior and contradict existing host contracts.

### D4 — Treat `src/common` as firmware compatibility, not shared DSP ownership

The two-line wrappers in `src/common` may remain so firmware include paths keep working. New shared DSP/control logic should land in `src/core`, while `src/common` may adapt hardware I/O or include compatibility only. Add a guard that flags non-wrapper growth in mirrored `src/common/<core-name>.hpp` files unless explicitly approved as firmware-only adapter code.

Alternative rejected: remove `src/common` immediately. That would risk firmware build behavior, which the existing omni scope explicitly excludes.

### D5 — Consolidate script path classifications

Introduce one small machine-readable or shell-readable path-classification source consumed by hygiene scripts. It should name:

- firmware-excluded paths;
- host source paths;
- generated publication exceptions;
- local-only cache/build outputs;
- local-only VCV/VST policy paths;
- OpenSpec local-only paths and ephemeral cache/session paths.

The first implementation can be conservative and shell-friendly. The value is not abstraction theater; it removes repeated allow/deny lists that currently have to be edited in several files.

Alternative rejected: keep each script's path list independent. That is exactly the kind of quiet drift the omni rule is trying to stop.

### D6 — Limit new dependencies and make existing network dependencies cache-aware

Do not add new package or CMake dependencies for this cleanup. For existing remote dependencies, prefer pinned versions plus documented cache/local override paths. In particular, JUCE `FetchContent` should have a local-cache path option or clear offline instructions so configuring desktop/VST does not silently depend on live network access.

Alternative rejected: vendoring more tooling into the repo. The repo already carries enough weight; the smoother path is explicit local caches and smaller tracked authority.

### D7 — Launch-gate public SIM manual hosts

Keep VST/AU and VCV in internal host specs and test plans, but remove them from `SIM_MANUAL.md`, `docs/sim-manual.md`, and `web/public/sim-manual.md` until testing is complete and a later launch/documentation change adds verified user-facing behavior back.

The public manual scope becomes:

```
desktop standalone + web sim
```

The internal spec scope may remain:

```
desktop standalone + web/WASM + VST/AU + VCV
```

Alternative rejected: deleting VST/AU and VCV from internal specs. That would throw away useful readiness contracts while solving only a public-documentation problem.

## Risks / Trade-offs

- **[Risk] Local-only OpenSpec state is not visible in another checkout** -> Mitigate by treating OpenSpec as local planning and not pretending public CI or subagents own it.
- **[Risk] Public repo policy conflicts with local-only VCV/VST work** -> Mitigate by making the policy explicit and ensuring CI/docs do not claim coverage for hidden surfaces.
- **[Risk] Removing mirrors breaks Pages/local preview** -> Mitigate by either keeping mirrors with freshness gates or changing the build flow in a separate publication change.
- **[Risk] Internal VST/VCV specs appear to conflict with public docs** -> Mitigate by explicitly naming internal specs as pre-launch contracts and SIM manual mirrors as launch-gated user docs.
- **[Risk] Removing plugin wording also removes useful desktop patch-cable guidance** -> Mitigate by rewriting those passages as desktop/web guidance rather than deleting the underlying instructions.
- **[Risk] `src/common` guard blocks legitimate firmware adapter edits** -> Mitigate with a documented firmware-only exception and no automatic deletion.
- **[Risk] Central path classification becomes another stale file** -> Mitigate by making existing hygiene scripts consume it, so drift breaks checks.
- **[Risk] Offline dependency work is platform-specific** -> Mitigate by documenting local override variables and keeping network fetch as an explicit fallback rather than a hidden assumption.

## Migration Plan

1. Encode OpenSpec policy as local-only planning state that does not perform git operations.
2. Add shared path classification and migrate hygiene scripts to consume it.
3. Add checks for generated/public mirror freshness and local-only policy documentation.
4. Rewrite `SIM_MANUAL.md` and mirrors to desktop/web-only public host scope, with a search gate for pre-launch VST/AU/VCV terms.
5. Add wrapper guard for mirrored `src/common`/`src/core` headers.
6. Document dependency/cache categories and add local override guidance for JUCE, Node, Emscripten, and Rack SDK without installing new dependencies.
7. Review whether firmware build outputs currently tracked under `src/*/build/` should remain a firmware exception or become ignored/index-removed in a separate firmware-scoped change.
8. Run existing host hygiene, OpenSpec validation, docs sync checks, web build checks, and any dependency/offline smoke checks available locally.

Rollback is simple: restore prior ignore/script behavior. No remote releases, tags, package versions, or local caches are mutated by this plan.

## Open Questions

- Should local-only VCV/VST source remain hidden from the public repo, or should it move into public source later?
- Should firmware build outputs under `src/FroggersTiga/build`, `src/TestControl/build`, and `src/Blink/build` remain intentionally public artifacts, or should a separate firmware hygiene change remove them?

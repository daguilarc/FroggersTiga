## Context

```
Today (broken):

  Developer reads vcv/DEVELOPMENT.md
       │
       ▼
  git clone github.com/VCVRack/Rack-SDK  ──▶ 404 / auth failure
       │
       ▼
  ./build.sh --install
       │
       ├── make          (plugin.dylib in vcv/ only)
       └── cp dist/…/plugins/   (wrong path; dist may not exist)

Official VCV flow:

  vcvrack.com/downloads/Rack-SDK-<ver>-<os>-<cpu>.zip
       │
       ▼
  export RACK_DIR=/path/to/Rack-SDK
       │
       ├── make dist     → dist/FroggersTiga/ + *.vcvplugin
       └── make install  → $(RACK_USER_DIR)/plugins-<os>-<cpu>/
```

Verified facts (2026-06-14):

- `https://github.com/VCVRack/Rack-SDK` → HTTP 404
- `https://vcvrack.com/downloads/Rack-SDK-2.6.6-mac-arm64.zip` → HTTP 200, contains `Rack-SDK/plugin.mk`
- SDK `plugin.mk` sets `PLUGINS_DIR := $(RACK_USER_DIR)/plugins-$(ARCH_OS)-$(ARCH_CPU)` (not plain `plugins/`)

Related change: `vcv-vst-field-parity-panel` — plugin source complete; manual Rack verification blocked on tooling.

## Goals / Non-Goals

**Goals:**

- One-command path: `./build.sh --fetch-sdk --install` on macOS arm64 (primary dev machine).
- Correct documentation with zero GitHub SDK references.
- Pin SDK version in fetch script (default `2.6.6`; overridable via `RACK_SDK_VERSION` env).
- Reuse SDK `make install` — no duplicate install path logic in bash.

**Non-Goals:**

- CI matrix for VCV (remains local-only per repo policy).
- Bundling Rack-SDK in git.
- Changing plugin DSP, widget layout, or GPL boundary.

## Decisions

### D1 — Zip download, not git

**Choice:** `fetch-rack-sdk.sh` uses `curl -fsSL` against vcvrack.com/downloads.

**Why:** GitHub repo does not exist; official manual says download SDK zip.

**Alternative rejected:** git clone — fails with 404/auth errors.

### D2 — Default SDK version pin

**Choice:** Default `RACK_SDK_VERSION=2.6.6` in fetch script; env override allowed.

**Why:** Reproducible builds; matches latest on downloads page at time of change.

### D3 — build.sh calls make dist + make install

**Choice:** Replace bare `make` and manual `cp` with `make dist` then conditional `make install`.

**Why:** Matches SDK contract; handles codesign, strip, `.vcvplugin` packaging, arch-specific plugins dir.

**Alternative rejected:** Keep manual `cp dist/FroggersTiga` — wrong directory and skips `.vcvplugin` format.

### D4 — Extract to repo-root Rack-SDK/

**Choice:** Default target `$(vcv/../Rack-SDK)` — already in root `.gitignore`.

**Why:** Matches existing `build.sh` auto-detect candidate list.

### D5 — Host deps documented, not vendored

**Choice:** Document `brew install jq zstd`; fail with clear message if `jq` missing (required by plugin.mk).

**Why:** Same as upstream VCV plugin development; no repo bloat.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| vcvrack.com URL or version string changes | Centralize version in fetch script; error message links to downloads index |
| User has no Rack installed (no user folder) | Document "launch Rack once" prerequisite; `make install` creates plugins subdir via `mkdir -p` |
| SDK version mismatch with installed Rack | Pin version; note in DEVELOPMENT.md to match Rack major version |
| Windows unsupported in fetch script v1 | Document manual zip extract; build.sh already Unix-oriented |

## Migration Plan

1. Land fetch script + fixed build.sh + DEVELOPMENT.md.
2. Developer runs `./build.sh --fetch-sdk --install`.
3. Verify module appears in Rack Module Browser.
4. Resume `vcv-vst-field-parity-panel` manual tasks (4.6, 5.1, 5.5).

Rollback: revert scripts; no runtime code changed.

## Open Questions

1. Auto-detect latest SDK version from downloads HTML vs pinned version — **pinned for v1** (simpler, deterministic).
2. Add `make` prerequisite check for Xcode CLT — optional follow-up.

## Why

Sim hosts (desktop + web) embed firmware `MANUAL.md` in Help → **Manual**, but sim UI uses `ParamDisplayNames` (Comb offset, Stereo width, XOR, Crunch, …). Firmware manual documents OLED symbols (`DELF`, `RMOD`, Pure delay, FUEG) and Field hardware workflow. Users read the wrong doc. **`MANUAL.md` must not change** — it stays the Daisy Field operator manual.

## What Changes

- **New `SIM_MANUAL.md`** at repo root — sim-only operator doc: pages, transport, mod bay, per-page knob tables using **sim display names** from `ParamDisplayNames.hpp`.
- **Help menu unchanged in structure** — **Manual** + **Quick Dict** + **License**. **Manual** opens `SIM_MANUAL.md`, not `MANUAL.md`.
- **Sync / embed** — `sync-help-docs.sh` and desktop `BinaryData` copy/embed `SIM_MANUAL.md` as `sim-manual.md` (web) / `SIM_MANUAL_md` (desktop).
- **`MANUAL.md` untouched** — not copied to sim assets, not in sim Help menu. One-line pointer in `SIM_MANUAL.md` footer: firmware detail lives in repo `MANUAL.md`.
- **`QUICK_DICT.md` intro** — update first blurb: sim glosses here; full sim guide in **Manual** (not firmware manual).

## Capabilities

### New Capabilities

- `sim-manual-doc`: Authoritative sim operator manual content and structure.

### Modified Capabilities

- `app-help-menu`: Manual item shows `SIM_MANUAL.md`; sync script no longer ships firmware `MANUAL.md` to sim hosts.
- `quick-dict-doc`: Intro line references sim Manual, not Field manual.
- `quick-dict-format`: Upfront "deferred to Manual" note points to in-app sim Manual; Field hardware → repo `MANUAL.md`.

## Impact

- `SIM_MANUAL.md` (new)
- `scripts/sync-help-docs.sh`
- `web/public/sim-manual.md` (replaces `manual.md` in sync)
- `web/src/main.ts` — `HELP_DOC_PATHS.manual.path`
- `desktop/CMakeLists.txt` — `juce_add_binary_data` source swap
- `desktop/Source/AppMenuBar.cpp` — `BinaryData::SIM_MANUAL_md`
- `quick-dict-format/spec.md` — intro "deferred to Manual" split (sim vs Field)
- Delete or stop syncing `web/public/manual.md` (stale copy removed on next build)

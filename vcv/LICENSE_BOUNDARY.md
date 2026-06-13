# MIT core / GPL VCV wrapper — distribution decision

**Recorded:** 2026-06-11  
**Status:** Accepted for in-tree development; Rack Library publication remains gated separately.

## Split

| Tree | License | What it is |
|------|---------|------------|
| `src/core/` | MIT (repo root `LICENSE`) | Portable `FroggersEngine`, `PageManager`, `PagedHostIO`, DSP headers |
| `src/FroggersTiga/`, `src/common/` | MIT | Daisy firmware shim |
| `wasm/`, `web/`, `desktop/` | MIT | Sim hosts (JUCE, WASM) |
| `vcv/` | **GPL-3.0-or-later** (`vcv/LICENSE`) | VCV Rack 2 plugin only |

## Rule

- **Never** include VCV Rack SDK headers (`rack.hpp`, etc.) outside `vcv/`.
- **Never** link the MIT sim hosts (desktop, web) against GPL Rack SDK code.
- The VCV plugin **may** `#include` and link MIT `src/core` headers — the **plugin binary** is GPL when distributed.

## Rationale

VCV Rack and its plugin SDK are GPL. A Rack module that links the SDK must be GPL. Keeping all Rack-facing code in `vcv/` preserves MIT licensing for firmware, desktop, and web.

## Rack Library

Do not publish to the VCV Rack Library until the full module (task §6.3 I/O) is implemented and reviewed. This wrapper satisfies the license gate for building locally.

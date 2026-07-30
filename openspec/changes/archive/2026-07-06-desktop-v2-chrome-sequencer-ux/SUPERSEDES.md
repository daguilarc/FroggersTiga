# Change lineage (desktop v2)

## Superseded: `desktop-v2-ux-and-sequencer`

Archived **2026-07-01** as `openspec/changes/archive/2026-07-01-desktop-v2-ux-and-sequencer/`.

Operator-rejected decisions in that change (Engine label, blank-only Rand-seq Pattern, sequencer **Record** stealing v1 audio export) are **not** ground truth.

**Active replacement:** `openspec/changes/desktop-v2-chrome-sequencer-ux/` — implement via `/opsx:apply`, archive after tasks complete.

## Archived (completed): `desktop-v2-boot-sync-fix`

Archived **2026-07-01** as `openspec/changes/archive/2026-07-01-desktop-v2-boot-sync-fix/`.

- **Shipped:** `6e8fa27` on `froggerstiga-desktop-v2` — callback lifetime (`[ctxPtr = &ctx]`), boot double-sync fix, `CallbackLifetime_test` + `BootSmoke_test`.
- **Specs synced:** `openspec/specs/desktop-v2-boot-host-sync/spec.md` (new), `sim-operator-doc-parity` (boot outcome + carousel nav gloss).
- **Tasks 3.4 / 3.6:** closed as superseded — change-specific tests pass (OMNI.4); full `ctest` blocked by pre-existing `ControlCoreBridge_test` `SetPageParam`; v1 N/A (desktop-v2-only diff).


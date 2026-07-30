## ADDED Requirements

**Audit 2026-06-30:** `FroggersV2HostBridge::syncToHost` pushes knob values host←core only (`FroggersV2HostBridge.cpp` L29–44). After `EnqueueRandomizePanelMod`, host owns new mod routes but core `ParamState.modSource` and UI dropdowns are stale until a pull path exists.

### Requirement: host-mod-routes-sync-to-ui

After host-side modulation randomization (`EnqueueRandomizePanelMod` or `EnqueueRandomizeAllMod`), the desktop v2 and VST v2 UI SHALL reflect assigned mod sources in each row's mod dropdown and painted mod-source label.

#### Scenario: Rand Mod updates dropdown

- **WHEN** operator clicks Rand Mod on Audio with host assigning a non-None mod to row 0
- **THEN** row 0 mod cell shows the assigned source name from `V2ModSourceLabel`
- **THEN** the dropdown selected item matches the assignment

### Requirement: sync-from-host-mod-routes

`FroggersV2HostBridge` SHALL implement `syncFromHostModRoutes()` reading mod assignment indices from `DesktopHostIO` for every page row (same authority as web/desktop v1 host state).

The bridge SHALL call `syncFromHostModRoutes()` after `DrainPendingMutations()` when host mod routes change, including immediately after Rand Mod / Rand All Mod enqueue completes.

Mod route read/write SHALL share one helper `syncModRoutes(page, row, direction)` — OMNI: no duplicate host-index lookup in push and pull paths.

#### Scenario: Rand Mod pulls routes into core

- **WHEN** Rand Mod completes on the host for page 2
- **THEN** `syncFromHostModRoutes` updates `m_params[2][row].modSource` for each row
- **THEN** `ModSourceCell` refresh shows non-None assignments without manual page change

#### Scenario: VST Rand Mod parity

- **WHEN** FroggersTigaPluginV2 editor triggers Rand Mod
- **THEN** the same `syncFromHostModRoutes` path runs in `HostedMainComponentV2` timer callback
- **THEN** mod dropdowns match DAW-visible host parameter state

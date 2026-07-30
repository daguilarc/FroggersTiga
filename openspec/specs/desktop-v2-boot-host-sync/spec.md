# desktop-v2-boot-host-sync Specification

## Purpose
Desktop v2 and VST v2 host callback wiring uses one stable `HostCallbackContext`, captured by reference for the owning component's full lifetime, so boot and page-change flows cannot reference dangling captures, double-sync the host, or crash on launch.
## Requirements
### Requirement: Stable host callback context lifetime

Carousel and desktop-v2 host callback lambdas SHALL capture the `HostCallbackContext` object by pointer (`[ctxPtr = &ctx]` with non-temporary storage), with lifetime equal to or greater than `MainComponent` (or `HostedMainComponentV2`). Callers SHALL pass `m_hostCallbacks` by reference, not a braced temporary.

#### Scenario: wireCallbacks returns without dangling references

- **WHEN** `MainComponent` constructs and calls `desktop_v2::refreshAndWireHostCallbacks`
- **THEN** stored `PageCarouselComponent::onPageChanged`, `onRandomize`, and `onRandomizeMod` callables remain valid after the wiring function returns

#### Scenario: Page change after construction

- **WHEN** the user changes carousel page after the main window is shown
- **THEN** `pushSelectPage` runs with a valid `DesktopHostIO&` and `syncToHost` completes without memory access faults

### Requirement: HostedMainComponentV2 callback parity

`HostedMainComponentV2` SHALL use the same stable `HostCallbackContext` member and `refreshAndWireHostCallbacks` entry point as `MainComponent`. VST plugin editor boot SHALL not reintroduce dangling captures or double boot sync.

#### Scenario: Plugin editor construction completes

- **WHEN** the VST v2 editor constructs `HostedMainComponentV2` on a clean build
- **THEN** carousel callbacks reference a valid `HostCallbackContext` and the editor presents without memory access faults during initial page 0 setup

### Requirement: Single host callback context authority

All `pushSelectPage` and `pushRandomizeMod` calls on a host component SHALL use that component's `m_hostCallbacks` member, not a braced temporary `HostCallbackContext`.

#### Scenario: Direct pushSelectPage from host component

- **WHEN** `MainComponent::pushSelectPage` is invoked after construction
- **THEN** it passes the same `m_hostCallbacks` object wired at construction time

### Requirement: Boot page initialization does not double-sync through dangling context

Startup page selection SHALL perform exactly one host sync via `pushSelectPage` (or equivalent) and SHALL NOT invoke `onPageChanged` with a stale context during `MainComponent` or `HostedMainComponentV2` construction.

#### Scenario: Main window construction completes

- **WHEN** `FroggersTigaV2` standalone launches on a clean build
- **THEN** the process remains alive for at least 3 seconds and presents the main window

#### Scenario: Initial page 0 without redundant callback

- **WHEN** `MainComponent` finishes construction with default page 0
- **THEN** host knob state and carousel UI both reflect page 0 without a second `syncToHost` driven by a stale `onPageChanged` capture

### Requirement: Boot smoke verification

The desktop-v2 test target SHALL include an automated or scripted boot smoke check that fails if the standalone binary exits immediately on launch.

#### Scenario: CI or local ctest boot gate

- **WHEN** `ctest` runs desktop-v2 tests on a machine where `FroggersTigaV2` is built
- **THEN** the boot smoke test passes (process still running after launch delay) or skips with explicit message if binary absent

### Requirement: Callback lifetime regression test

The desktop-v2 test target SHALL include a unit test that wires carousel callbacks and invokes them after the wiring function returns, proving captures do not reference the `wireCallbacks` parameter stack frame.

#### Scenario: Post-wire invoke without fault

- **WHEN** `CallbackLifetime_test` runs `refreshAndWireHostCallbacks` and then invokes the stored `onPageChanged` callable
- **THEN** the test completes without memory access faults and the host reference remains valid

### Requirement: Global Crunchy sync uses valid host object

`FroggersV2HostBridge::syncToHost` SHALL call `DesktopHostIO::SetGlobalCrunchy` only through a `DesktopHostIO&` that refers to `AudioEngine::m_host` for the active audio engine instance.

#### Scenario: Crunchy write after visible-row knob loop

- **WHEN** `syncToHost` finishes iterating visible rows on page 0 during boot
- **THEN** `SetGlobalCrunchy` stores the clamped value in `m_globalCrunchy` without fault


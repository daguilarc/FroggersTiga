## ADDED Requirements

### Requirement: Ext. In. toggle in the Audio settings page enables external audio routing
Desktop v2's `AudioSettingsComponent` (reached via the existing runtime-page rail's Audio button, the same rail as the MIDI/Controllers button) SHALL expose an "Ext. In." toggle that calls `AudioEngine::setExternalInputEnabled` with its toggle state, mirroring v1's equivalent control.

#### Scenario: Enabling the toggle
- **WHEN** the operator opens the Audio settings page and clicks the "Ext. In." toggle to the on state
- **THEN** `AudioEngine::isExternalInputEnabled()` returns `true`
- **THEN** the change is reflected on the next periodic refresh without requiring an app restart

#### Scenario: Disabling the toggle
- **WHEN** the operator clicks the "Ext. In." toggle to the off state
- **THEN** `AudioEngine::isExternalInputEnabled()` returns `false`

### Requirement: Enabling external audio makes modulation lanes reachable
Once external audio is enabled and an input device is actively routing signal, external-audio modulation lane depths SHALL reach both the UI effective-value computation and the audio engine's per-lane sum, using the existing `isModLaneAssignable` assignability gate (not the rand-only `isModSourceEligibleForRow` gate).

#### Scenario: External-audio lane contributes to the UI effective value
- **WHEN** `FroggersV2ControlCore::setExternalAudioAvailable(true)` has been called and a non-zero depth is set on an external-audio modulation lane
- **THEN** `computeEffective`'s page/row-aware overload includes that lane's contribution in its sum

#### Scenario: External-audio lane contributes to the engine sum
- **WHEN** `FroggersV2ControlCore::setExternalAudioAvailable(true)` has been called and a non-zero depth is set on an external-audio modulation lane
- **THEN** `FroggersV2HostBridge::syncModRoutes`'s ToHost lane push writes that lane's depth into `V2LaneDepthStore` (not zero)

### Requirement: Input-level meter and route-status hint reflect engine state
The Audio settings page SHALL display an input-level meter (`AudioEngine::getInputPeakLevel()`) and a route-status hint (`AudioEngine::getInputRouteStatus()` / `getInputRouteMessage()`) alongside the Ext. In. toggle, mirroring v1.

#### Scenario: Meter shows input level while enabled and running
- **WHEN** external audio is enabled and the audio engine is running
- **THEN** the input-level meter reflects `getInputPeakLevel()`

#### Scenario: Route-status hint surfaces routing problems
- **WHEN** external audio is enabled, the engine is running, and `getInputRouteStatus()` is not `Ok`
- **THEN** the route-status hint displays `getInputRouteMessage()`

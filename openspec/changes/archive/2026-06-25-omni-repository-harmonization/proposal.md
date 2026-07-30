## Why

The desktop, web/WASM, VCV, and VST/AU surfaces pass their current automated suite, but the host-focused OMNI audit found contracts that are green only because they are duplicated, untested, ignored, or internally contradictory: release automation accepts extra channel tags, host labels, mod-rack topology, scope protocol constants, and versions have multiple authorities, repository-owned host audio buffers allocate or grow in realtime paths, host build products are tracked, and the entire `openspec/` planning tree is ignored while active host artifacts disagree with their own tasks. The MIDI boundary is also inverted: VCV directly owns MIDI queues, ports, CC switches, and two CC mod sources despite Rack already providing MIDI-to-CV modules, while VST/AU exposes only two fixed CC pairs and no complete host-parameter surface for DAW-owned routing. Fifteen other active changes preserve stale, superseded, or already-applied plans. These gaps make routine edits capable of silently changing host release topology, behavior, or specification truth.

## Scope Boundary

This change applies only to desktop standalone, web/WASM, VCV Rack, and VST/AU, plus `sim/` and shared `src/core/` code required by those hosts. The original Daisy Field firmware application, other hardware-facing `src/` applications/support, `src/common/`, `src/mk/`, `External/libDaisy/`, firmware build outputs, and `MANUAL.md` are excluded from audit, implementation, cleanup, and verification.

## What Changes

- Enforce `froggerstiga-v1` as the only desktop release channel while retaining the mandated `froggerstiga-v*` workflow trigger pattern; non-channel tags cannot build or publish a release.
- Make desktop/web release metadata and host-visible version strings derive from or verify against the CMake package version, without incrementing that version as part of this change.
- Generate the web's pre-audio page, row, pair-AR, global-action, and mod-rack presentation metadata from shared C++ display/layout tables; remove hand-maintained TypeScript page/topology/capacity tables.
- Correct the archived web mod-rack contract to a browser-specific four-entry topology—MIDI CC 1, VCO Envelope, Random 1, Random 2—with no MIDI CC 2 input, scope, enable control, or assignment path.
- **BREAKING — VCV:** remove all MIDI input/output queues, ports, controls, CC mod sources, and MIDI-specific state from Froggers Tiga Rack modules, with versioned explicit remapping of legacy Rack parameter IDs so existing knob state remains attached to the correct controls. External MIDI belongs to a separate Rack MIDI-to-CV module patched into Froggers' native per-parameter CV inputs.
- **BREAKING — VST/AU:** remove the two fixed `CvMidiBridge` CC inputs and hosted MIDI-settings/mod-rack CC cells, stop advertising raw MIDI input, and expose every persistent continuous control as a stable host parameter so the DAW can map any MIDI channel/CC to any parameter without a Froggers-imposed two-source limit. Apply portable JUCE parameter targets at render-block boundaries and use existing per-sample DSP smoothing rather than claiming format-specific sample offsets.
- Correct the VCV per-parameter CV jacks so voltage adds to the parameter value already produced by its stored internal 4/5/6 modulation route instead of being quantized into an internal mod-source selector; both modulation paths combine without mutating the saved route.
- Absorb the still-valid VST hosted-UX findings—transport-stopped mutation drain, overlay resync, hosted chrome/keyboard policy, state compatibility, editor sizing, format validation, and correct VST3-versus-AU host coverage—into this change.
- Route pair-AR modulation through `ModMgr::Modulate` so external-CV availability gating and blend behavior have one implementation.
- Remove repository-controlled native/WASM heap allocation and dynamic buffer growth from Web/WASM, standalone, VST, VCV, and recording producer callback paths; keep Web Audio UI telemetry bounded and explicitly exclude browser-internal structured-clone/GC behavior from the zero-owned-allocation claim.
- Stop tracking generated host/sim build products; add scoped ignore and verification rules while preserving intentionally published Pages artifacts.
- Replace the blanket `openspec/` ignore with selective ephemeral-state rules and put canonical OpenSpec configuration, baseline specs, changes, and archive history under version control.
- Reconcile active OpenSpec contradictions, replace placeholder baseline purposes, and close every other active change: merge code-backed durable deltas before archive, and archive superseded/stale plans with `--skip-specs` so they cannot rewrite baseline truth.
- Correct desktop release documentation that currently advertises wildcard channel semantics or stale verification behavior.

## Capabilities

### New Capabilities

- `desktop-release-channel-integrity`: Exact release-channel admission, update-in-place publication, and one checked package/application version authority.
- `host-display-authority`: Generated web label metadata plus one shared host-aware mod-rack topology and scope protocol.
- `realtime-audio-safety`: Owned-allocation-free native/WASM steady-state processing plus bounded, explicitly scoped Web Audio telemetry.
- `repository-artifact-hygiene`: Host-source-only tracking rules with explicit published-artifact exceptions, firmware exclusions, and clean-build verification.
- `openspec-lifecycle-integrity`: Version-controlled planning truth, internally consistent active changes, meaningful baseline purposes, and deterministic archive readiness.
- `vcv-midi-boundary`: Rack-native CV-only control, direct per-parameter CV semantics, and complete removal of Froggers-owned MIDI I/O.
- `vst-host-parameter-routing`: Stable VST/AU parameter exposure with DAW-owned, non-two-pair MIDI routing.
- `vst-hosted-ux-integrity`: Hosted mutation, overlay, chrome, keyboard, state, and sizing behavior absorbed from the stale VST plan.

### Modified Capabilities

- `mod-blend-semantics`: Extend the existing shared blend/gating authority to pair-AR and make invalid modulation indices safe.
- `midi-cc-mod-gating`: Limit fixed CC-to-mod gating to desktop standalone (two pairs) and web (CC 1 only); VST/AU and VCV leave this model.
- `web-midi-mod-rack`: Replace the stale dual-CC/five-entry browser contract with the intended CC 1-only/four-entry behavior.
- `vcv-cc-mod-gating`: Remove the obsolete VCV MIDI ingest, output, enable-toggle, and CC-source requirements.
- `juce-vst-cc-mod-gating`: Replace fixed hosted CC-pair requirements with the standard host-parameter routing capability.

## Impact

- Release: `.github/workflows/desktop-release.yml`, `desktop/scripts/verify-tag-version.sh`, `desktop/PACKAGING.md`, `README.md`
- Metadata/labels/topology: `desktop/Source/Main.cpp`, `desktop/CMakeLists.txt`, `sim/HostPanelLayout.hpp`, `web/package*.json`, `web/src/paramDisplayNames.ts`, `web/src/main.ts`, `web/src/froggers-processor.ts`, VCV panel generation, WASM display bindings, generation/check scripts
- DSP/hosts: `src/core/AudioPairArState.hpp`, `src/core/ModMgr.hpp`, host-specific mod-source policy, `sim/WasmSimHost.hpp`, `vcv/src/plugin.cpp`, `desktop/Source/AudioEngine.*`, `desktop/Source/AudioRecorder.*`, `desktop/Source/PluginProcessor.*`, parameter adapters/attachments, tests
- Hygiene/docs: `.gitignore`, tracked `sim/build/` products, host build/package outputs, desktop release sections in `README.md`
- OpenSpec: ignored planning tree, all fifteen non-omni active changes, baseline spec purposes, archive history, and a local hygiene validator

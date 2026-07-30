# Sheaf adoption inventory

Pin: Sheaf `origin/main` @ `c1810393` (commit exists as a fetched object in the
user's `/Users/diegoaguilar-canabal/Desktop/Sheaf` clone; vendored via a
throwaway `git worktree add /tmp/sheaf-c1810393 c1810393`, removed after
copying — the user's own Sheaf working tree was never checked out or
otherwise disturbed and remains at `eae12ea3`).

Vendor root: `desktop-v2/External/Sheaf/` (mirrors this repo's existing
top-level `External/` vendoring convention — used for DaisySP/libDaisy — but
scoped under `desktop-v2/` since desktop-v2 is the sole consumer of this
slice; avoids touching top-level repo layout for a change confined to one
product). Directory structure mirrors Sheaf's own `projects/synth/` layout
(`include/synth/`, `src/`, `runtime/`, `juce/`) so future re-vendoring at a
newer pin is a straightforward diff.

Product builds MUST NOT FetchContent Sheaf from the network. All 65 files
below were vendored in packet 2 (tasks 2.1-2.3); no orphan copies exist
outside `desktop-v2/External/Sheaf/`.

Selection method: started from the exact file set Sheaf's own
`runtime/juce_build.mk` compiles for a JUCE app (`SYNTH_SRC` compiled units +
`SYNTH_HEADERS`/`SYNTH_JUCE_HEADERS` header list), then computed the full
transitive `#include` closure from tasks.md 2.1's named targets (Engine,
Runtime, Shell/MainPane, portable UI, ScopeWriter, ScopeVisualizer,
GangedRandomLfoVisualizer) to make sure nothing upstream of those was missed.

## Compiled sources (`src/`, `runtime/*.cpp`)

| Sheaf source path (under `Sheaf/projects/synth/…`) | FroggersTiga destination | Notes |
|---|---|---|
| `src/ParameterModulation.cpp` | `desktop-v2/External/Sheaf/src/ParameterModulation.cpp` | Phase1/2 param processing (design.md tip-sync theme); `SheafRuntimeVendor` target |
| `src/ButtonGrid.cpp` | `desktop-v2/External/Sheaf/src/ButtonGrid.cpp` | `GridManager` impl; grids unused by FroggersApp per 2.2 but must compile |
| `src/MidiController.cpp` | `desktop-v2/External/Sheaf/src/MidiController.cpp` | Carries the MIDI endpoint-ID `:`-allow fix (design.md tip-sync) |
| `src/PatchPersistence.cpp` | `desktop-v2/External/Sheaf/src/PatchPersistence.cpp` | Patch load/save used by `PatchManager` (Engine dependency) |
| `src/DspWavetable.cpp` | `desktop-v2/External/Sheaf/src/DspWavetable.cpp` | Wavetable DSP backing `Modules.hpp` |
| `src/Modules.cpp` | `desktop-v2/External/Sheaf/src/Modules.cpp` | Module registration glue |
| `src/MidiReconcile.cpp` | `desktop-v2/External/Sheaf/src/MidiReconcile.cpp` | MIDI device reconciliation |
| `src/MidiDevicePoller.cpp` | `desktop-v2/External/Sheaf/src/MidiDevicePoller.cpp` | MIDI device polling |
| `src/MidiConfigViewModel.cpp` | `desktop-v2/External/Sheaf/src/MidiConfigViewModel.cpp` | MIDI config view-model |
| `src/MidiConfigBlocks.cpp` | `desktop-v2/External/Sheaf/src/MidiConfigBlocks.cpp` | MIDI config block parsing |
| `runtime/HostDataPaths.cpp` | `desktop-v2/External/Sheaf/runtime/HostDataPaths.cpp` | Only vendored source that touches JUCE (`juce_core`) directly |

## Headers — Engine / app contract / grid / runtime-UI-state

| Sheaf source path | FroggersTiga destination | Notes |
|---|---|---|
| `include/synth/Engine.hpp` | `desktop-v2/External/Sheaf/include/synth/Engine.hpp` | `synth::Engine<App>`; unconditionally constructs `GridManager` + `RuntimeUIState` regardless of `App` (verified: `SheafVendorSmoke_test` instantiates `Engine<StubApp>`) |
| `include/synth/AppConcepts.hpp` | `.../include/synth/AppConcepts.hpp` | `SynthApplicationCore`/`SynthApplication` concepts (C++20) |
| `include/synth/AppContext.hpp` | `.../include/synth/AppContext.hpp` | `AppContext`, `RuntimeConfig`, `AudioBlock` |
| `include/synth/AppRegistry.hpp` | `.../include/synth/AppRegistry.hpp` | App registry support used by `HostDataPaths` |
| `include/synth/RuntimeUIState.hpp` | `.../include/synth/RuntimeUIState.hpp` | `{parameters, grids}` facade (design.md tip-sync: AppContext/Engine wiring changed) |
| `include/synth/ButtonGrid.hpp` | `.../include/synth/ButtonGrid.hpp` | `GridManager`/`ButtonGrid` (out of scope for MIDI mapping; must compile per 2.2) |
| `include/synth/ParameterModulation.hpp` | `.../include/synth/ParameterModulation.hpp` | Phase1/2 param processing, `MessageInBus`, buses |
| `include/synth/MidiController.hpp` | `.../include/synth/MidiController.hpp` | MIDI controller/instrument config |
| `include/synth/MidiConfigBlocks.hpp` | `.../include/synth/MidiConfigBlocks.hpp` | Header for `src/MidiConfigBlocks.cpp`; transitive compile dep (MIDI mapping deferred, must compile per 2.2) |
| `include/synth/MidiConfigViewModel.hpp` | `.../include/synth/MidiConfigViewModel.hpp` | Header for `src/MidiConfigViewModel.cpp`; transitive compile dep (MIDI mapping deferred, must compile per 2.2) |
| `include/synth/MidiDevicePoller.hpp` | `.../include/synth/MidiDevicePoller.hpp` | Header for `src/MidiDevicePoller.cpp`; transitive compile dep (MIDI mapping deferred, must compile per 2.2) |
| `include/synth/MidiReconcile.hpp` | `.../include/synth/MidiReconcile.hpp` | Header for `src/MidiReconcile.cpp`; transitive compile dep (MIDI mapping deferred, must compile per 2.2) |
| `include/synth/PatchPersistence.hpp` | `.../include/synth/PatchPersistence.hpp` | `PatchManager` |
| `include/synth/AsyncLogger.hpp` | `.../include/synth/AsyncLogger.hpp` | Async log queue used by `Engine::Initialize` |
| `include/synth/Json.hpp` | `.../include/synth/Json.hpp` | JSON support for patch/config persistence |
| `include/synth/CircularQueue.hpp` | `.../include/synth/CircularQueue.hpp` | Lock-free queue used by MIDI/parameter buses |
| `include/synth/ThreadId.hpp` | `.../include/synth/ThreadId.hpp` | Thread-role assertions (sar-7) |

## Headers — DSP (ADSR, oscillators, filters, scope, random LFO)

| Sheaf source path | FroggersTiga destination | Notes |
|---|---|---|
| `include/synth/DspAdsr.hpp` | `.../include/synth/DspAdsr.hpp` | Full ADSR DSP (design.md tip-sync); Froggers product Envelope stays ASR — DSP vendored as-is, unused knee left fixed (per design.md, not this packet's job to wire) |
| `include/synth/DspMath.hpp` | `.../include/synth/DspMath.hpp` | Shared math helpers |
| `include/synth/DspConstant.hpp` | `.../include/synth/DspConstant.hpp` | Constant-source DSP |
| `include/synth/DspNoise.hpp` | `.../include/synth/DspNoise.hpp` | Noise generators |
| `include/synth/DspFilters.hpp` | `.../include/synth/DspFilters.hpp` | Filter modules (braid filter caches, tip-sync) |
| `include/synth/DspOscillators.hpp` | `.../include/synth/DspOscillators.hpp` | VCO/oscillator DSP |
| `include/synth/DspWavetable.hpp` | `.../include/synth/DspWavetable.hpp` | Wavetable DSP header |
| `include/synth/DspRandomLfo.hpp` | `.../include/synth/DspRandomLfo.hpp` | `GangedRandomLfoUiState<N>`; backs `GangedRandomLfoVisualizer` |
| `include/synth/DspScope.hpp` | `.../include/synth/DspScope.hpp` | `ScopeWriter`/`ScopeReader` — verified constructible in `SheafVendorSmoke_test` |
| `include/synth/DspTransferFunction.hpp` | `.../include/synth/DspTransferFunction.hpp` | Transfer-function DSP used by scope/waveform draw math |
| `include/synth/StandardModulators.hpp` | `.../include/synth/StandardModulators.hpp` | Standard modulator source set |
| `include/synth/Modules.hpp` | `.../include/synth/Modules.hpp` | Module type catalog (VCO/filter/LFO templates) |

## Headers — Portable UI / visualizers

| Sheaf source path | FroggersTiga destination | Notes |
|---|---|---|
| `include/synth/PortableUI.hpp` | `.../include/synth/PortableUI.hpp` | `synth::ui::Surface`, `DrawCommand`, `Bounds`, `Action`, `Visualizer` base |
| `include/synth/PortableUIBuilders.hpp` | `.../include/synth/PortableUIBuilders.hpp` | `ScopeVisualizer<LayerState>` — verified constructed + drawn in `SheafVendorSmoke_test` |
| `include/synth/GangedRandomLfoVisualizer.hpp` | `.../include/synth/GangedRandomLfoVisualizer.hpp` | `GangedRandomLfoVisualizer<VoiceCount>` — verified constructed + drawn (VoiceCount=3) in `SheafVendorSmoke_test`; target for Random S&H mod-depth cells (task 5.3) |
| `include/synth/ConstantBarVisualizer.hpp` | `.../include/synth/ConstantBarVisualizer.hpp` | Transitive dependency of the portable-UI/visualizer closure |
| `include/synth/NoiseWaveformVisualizer.hpp` | `.../include/synth/NoiseWaveformVisualizer.hpp` | Transitive dependency of the portable-UI/visualizer closure |
| `include/synth/EncoderDraw.hpp` | `.../include/synth/EncoderDraw.hpp` | Encoder draw-command geometry |
| `include/synth/AtomicColor.hpp` | `.../include/synth/AtomicColor.hpp` | Lock-free color storage read by `ScopeVisualizer`/visualizers |
| `include/synth/Color.hpp` | `.../include/synth/Color.hpp` | Color value type |

## Headers — Runtime shell (Engine host, JUCE-facing)

| Sheaf source path | FroggersTiga destination | Notes |
|---|---|---|
| `runtime/Runtime.hpp` | `desktop-v2/External/Sheaf/runtime/Runtime.hpp` | JUCE `AudioIODeviceCallback`/timer pump driving `Engine<App>`; not instantiated as a running app this packet (task 10, later) |
| `runtime/Shell.hpp` | `.../runtime/Shell.hpp` | `SYNTH_RUNTIME_MAIN(AppType)` macro + `ShellApplication<AppType>`; vendored + parses clean, **not invoked** (would create a second JUCE app entry point — out of scope until task 10 cutover) |
| `runtime/MainPane.hpp` | `.../runtime/MainPane.hpp` | File/Audio/Controllers shell pane |
| `runtime/AudioConfigPage.hpp` | `.../runtime/AudioConfigPage.hpp` | Runtime Audio page |
| `runtime/FilePage.hpp` | `.../runtime/FilePage.hpp` | Runtime File page |
| `runtime/MidiConnectionManager.hpp` | `.../runtime/MidiConnectionManager.hpp` | MIDI connection manager used by Runtime |
| `runtime/JuceRuntimeMainServices.hpp` | `.../runtime/JuceRuntimeMainServices.hpp` | Runtime main-services glue |
| `runtime/HostDataPaths.hpp` | `.../runtime/HostDataPaths.hpp` | Data-path resolution (paired with vendored `.cpp`) |
| `include/synth/RuntimePages.hpp` | `.../include/synth/RuntimePages.hpp` | JUCE-free runtime page contracts |
| `include/synth/RuntimeFileService.hpp` | `.../include/synth/RuntimeFileService.hpp` | File-service contract for File page |
| `include/synth/RuntimeMainComponent.hpp` | `.../include/synth/RuntimeMainComponent.hpp` | Runtime main-component contract |
| `include/synth/RuntimePagePolicy.hpp` | `.../include/synth/RuntimePagePolicy.hpp` | Runtime page policy helpers |
| `include/synth/ControllersPageUI.hpp` | `.../include/synth/ControllersPageUI.hpp` | Controllers shell page (present per 10.3; **no MIDI mapping** work this change) |
| `include/synth/PatchBrowser.hpp` | `.../include/synth/PatchBrowser.hpp` | Patch browser contract used by File page |
| `juce/PortableJuceBackend.hpp` | `desktop-v2/External/Sheaf/juce/PortableJuceBackend.hpp` | Carries the value-action **prefix append** (`:` + value) fix (design.md tip-sync) — required when hosting portable UI; do not reintroduce overwrite-only dispatch |
| `juce/RuntimePagesJuce.hpp` | `.../juce/RuntimePagesJuce.hpp` | JUCE adapters for runtime pages |
| `juce/MidiHandlers.hpp` | `.../juce/MidiHandlers.hpp` | JUCE MIDI input/output handler adapters |

## Explicitly NOT vendored

| Sheaf source path | FroggersTiga destination | Notes |
|---|---|---|
| `browser/**`, `apps/browser`-adjacent tooling, `include/synth/browser/**` | **DO NOT VENDOR** | Browser catalog / Pages / packages tree — out of scope for the desktop Runtime host (tasks.md 2.3, design.md tip-sync table) |
| `apps/miniapp/**`, `apps/braid-4/**`, other `apps/*` | **DO NOT VENDOR** | Sheaf's own application implementations of `SynthApplication`; `FroggersApp` is Froggers' own implementation (tasks.md 3.1, packet 3) — vendoring another app here would pre-empt that task |
| `runtime/juce_build.mk`, Sheaf `Makefile`s | **DO NOT VENDOR** | Sheaf's Make-based build; FroggersTiga uses its own CMake wiring (`desktop-v2/CMakeLists.txt`: `SheafRuntimeVendor` target) instead |
| `juce/*Tests.cpp`, `juce/ControllersPageHarness.hpp`, `juce/ControllersHarnessApp.cpp` | **DO NOT VENDOR** | Sheaf's own test/harness files; not part of the transitive include closure of tasks 2.1's named targets, and FroggersTiga has its own test suite under `desktop-v2/tests/` |

## Gate verification (task 2.3)

- `rg -i 'FetchContent.*[Ss]heaf|GIT_REPOSITORY.*[Ss]heaf' desktop-v2/` returns only two comment lines that state the absence of such a fetch (`desktop-v2/CMakeLists.txt`, `desktop-v2/tests/SheafVendorSmoke_test.cpp`) — no actual `FetchContent_Declare`/`GIT_REPOSITORY` call for Sheaf exists.
- `desktop-v2` configures and compiles clean (`FroggersTigaDesktopV2`, `SheafRuntimeVendor`, `SheafVendorSmoke_test`, and all pre-existing targets) via `nice -n 10 cmake --build build -j2`.
- `ctest` in `desktop-v2/build`: 15/15 tests pass, including the new `SheafVendorSmoke_test`.

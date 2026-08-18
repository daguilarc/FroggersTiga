# Proposal — `frogg3rs-browser-and-vst-hosts`

**Created 2026-08-18 at the operator's instruction**, superseding the queued
"wasm/web V2 host integration" plan: bring the Sheaf-hosted frogg3rs app to
the browser via Sheaf's wasm runtime (replacing the legacy sim/-based web
simulator as the deployed site), AND ship a new VST whose entire DAW-facing
surface — transport, MIDI mappings, automation — is external via the DAW,
while deleting the old, never-tested VST implementations.

## Why

- **Web:** the deployed simulator (web/ Vite UI + wasm/bindings.cpp over
  sim/) is a parallel implementation of a synth that now truly lives in
  `app/` on Sheaf's runtime. Every surface improvement (today: carousel
  arrows) reaches desktop only. Sheaf's browser side is built to make this
  simple: packaging is one declarative manifest record — no app-specific
  browser source, build recipe, or runtime branch (`sbap-1`) — and the
  production launcher trusts external catalog URLs (`sbac-10`). Two main
  specs in THIS repo already specify the intended end state:
  `froggers-browser-package` (self-hosted catalog identity, "SHALL NOT
  require a first-party build slot... inside the Sheaf repository") and
  `froggers-web-host` (the Sheaf-app build becomes the public site; web/ and
  wasm/ go dormant unmodified). This change implements them.
- **VST:** two old JUCE wrappers exist in `desktop/`/`desktop-v2/`, wrapping
  the RETIRED control cores, never wired into CI, and their actual
  `juce::AudioProcessor` classes have zero test coverage anywhere (v1's
  sources are not even in the public tree). The operator rules them dead.
  The replacement wraps the CURRENT app core with an inverted control
  contract: the DAW is the authority for everything a DAW touches —
  transport comes from the host playhead, not internal play/stop state;
  parameter access comes from a stable-ID automation surface the DAW maps
  MIDI to, not internal MIDI-learn (the current core has none anyway).

## What Changes

- **froggers-browser-package + froggers-web-host** (existing main specs,
  implemented; browser-package carries no delta, web-host gains a small
  ADDED delta recording the operator's 2026-08-18 site-presentation
  decisions — mobile viewport stacks around a full-width encoder grid,
  legacy site link roles carried forward under renamed references):
  browser build of `synth_froggers::FroggersApp`
  through Sheaf's generic pipeline (out-of-tree manifest +
  `--allowed-source-root`, the same mechanism `browser-fixture-app` uses);
  self-hosted catalog + package artifacts; pages.yml swapped to build and
  deploy the Sheaf-hosted app while web/ and wasm/ stay byte-identical and
  dormant. The repository-rename publication gate in `froggers-web-host`
  remains the operator's.
- **froggers-vst-host** (NEW capability): a JUCE plugin host (VST3/AU)
  under `app/` wrapping the JUCE-free core — DAW-external transport AND
  tempo (playhead → the existing `MessageIn` bus; host tempo → the
  core's existing external-clock slaving), recording left to the DAW
  (the Record control suppressed with Play/Stop; Freeze stays, labeled
  "FREEZE"), stable-ID automatable parameter inventory over the
  six-bank model, a stereo-out/no-input instrument with no MIDI-note
  wiring (the core has no note seam), plugin editor hosting the same
  portable `FroggersUiSurface`, and the core's `check_no_juce` gate
  preserved untouched.
- **juce-vst-cc-mod-gating, vst-v2-midi-modulation** (REMOVED capabilities):
  these specify the deleted wrappers' behavior; they are retired with the
  code.

## Impact

- Affected specs: `froggers-browser-package` (implemented as written);
  `froggers-web-host` (implemented as written + ADDED site-presentation
  delta, operator decisions 2026-08-18); `froggers-vst-host` (added);
  `juce-vst-cc-mod-gating`, `vst-v2-midi-modulation` (removed; each has
  its own REMOVED delta file per repo convention, added at the second
  2026-08-18 audit).
- Affected code: NEW `app/browser/` (manifest + build script + CI step),
  NEW `app/vst/` (plugin host + CMake), `.github/workflows/pages.yml`
  (swap), DELETED `desktop/CMakeLists.txt:136-313` (both `BUILD_VST*`
  option blocks; span and list audit-corrected 2026-08-18 to match
  Task-1 reality), DELETED `desktop-v2/Source/PluginProcessorV2.*` and
  `HostParameterRegistryV2.*` (plus tracked v1 `HostParameterRegistry.*`);
  KEPT, contrary to the original list: `PluginEditorV2.*`,
  `HostParameterInventoryV2.hpp`, and the `HostParameterProcessorV2`
  test — live, or text-asserted by live tests (see design amendment),
  doc sections
  (`desktop/PACKAGING.md:115-127`, `desktop-v2/PACKAGING.md:68-97`,
  `docs/CI.md:89` mention, README as applicable).
- Explicitly untouched: `web/`, `wasm/` (dormant, byte-identical, per
  froggers-web-host); `app/` core JUCE-freedom (`check_no_juce` in
  `make test`); `vcv/`/`Rack-SDK` (separate local-only surface, not a VST);
  `sim/` and the desktop app build (the deleted CMake blocks are
  default-OFF and CI never set them).
- Rename completion (added 2026-08-18, after the operator renamed the
  GitHub repo to `daguilarc/frogg3rs`): a dedicated task group, placed
  BEFORE publication, sweeps every tracked `FroggersTiga` mention with
  per-hit classification — renames land only where they don't break
  things (product/doc/publication-facing text); archived changes stay
  byte-identical and frozen-tree build targets keep their names.
  Workspace file renamed in-repo; branch rename deferred to the next
  branch cut; the machine-local folder rename is operator-coordinated at
  a session boundary. (The remote URL and the legacy site's base path
  were already fixed directly: commit `7beec7f`.)
- Delivery: one commit per task group on the current branch; operator
  gates for the browser smoke and the DAW smoke, both deferred to the
  end and run together in one sitting (group 9 — the operator tests
  nothing until every group lands, instruction 2026-08-18); the
  site-cutover step is user-gated per the existing spec.

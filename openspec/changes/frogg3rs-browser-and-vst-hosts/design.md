# Design — frogg3rs-browser-and-vst-hosts

Anchors verified 2026-08-18 by two exploration passes plus controller
spot-checks at FroggersTiga HEAD (`ce847c8`) and the checked-out Sheaf
submodule (`7bf1f223`, branch `fix-out-of-tree-app-gaps`). Anchors marked
UNVERIFIED must be read before the task relying on them executes. Sheaf
paths are `External/Sheaf/projects/synth/...` unless prefixed.
Re-audited 2026-08-18 (omni-rule preflight, three verification subagents,
HEAD `eb989a8`): corrections below are marked "audit-corrected".
Second audit 2026-08-18 (HEAD `a0f2f6c`): the Sheaf pin moved
`7bf1f223` → `c0bf7b48`, a docs-only delta (one sdd ledger file), so all
Sheaf anchors verified at `7bf1f223` remain valid; FroggersTiga code is
unchanged since `eb989a8` (artifact + pin commits only) and spot-checked
anchors held. A4, the Web-preflight classification in A3, and the
per-capability REMOVED delta files date from this second audit.

## Part A — Browser host

**Pipeline (all generic, Sheaf-side, already shipped):**
`browser/src/build-browser-apps.mjs` reads a manifest, generates one
`SYNTH_BROWSER_APP(cppType)` binding per app (template
`app-build-manifest.mjs:101-108` `generateBrowserBinding`, invoked at
`build-browser-apps.mjs:161-167` — anchor corrected in preflight; the
explorer had cited the right lines in the wrong file), compiles with
uniform emscripten flags (`:46-68`) against the fixed `CORE_SOURCES`
(`:10-23`), emits `dist/wasm/apps/<appId>/{appId.js,.wasm}` atomically
(`:138-197`). CLI accepts `--manifest`, repeatable
`--allowed-source-root`, `--output-root` (`:203-231`, audit-corrected). Default allowed
source root is only `projects/synth/apps` (`app-build-manifest.mjs:113`);
out-of-tree dirs are rejected by `requireAllowedDirectory`
(`:58-74`) unless allowlisted. The fixture target
(`browser/Makefile:40-41`) is the exact precedent for an external
manifest + allowed root: frogg3rs invokes the node script directly the
same way — NO Sheaf-side change required, per
`froggers-browser-package`'s own "no first-party build slot" requirement
(`openspec/specs/froggers-browser-package/spec.md:75-81`).

- **A1. Manifest + build script** (`app/browser/`): a frogg3rs manifest
  (schema per `app-build-manifest.mjs:5-9`: header, cppType
  `synth_froggers::FroggersApp`, includeDirs → `app/`, appId `frogg3rs`
  per `froggers-browser-package/spec.md:27-49`) + a build script that
  runs Sheaf's `browser` build (`npm run build` → tsc) then
  `node dist/src/build-browser-apps.mjs --manifest app/browser/... 
  --allowed-source-root <repo>/app --output-root <repo>/app/browser/dist`.
  Memory-policy and ABI come from the generic pipeline (`sbap-2`).
  UNVERIFIED at write time: whether `FroggersApp`'s header set compiles
  under emscripten as-is (the core is JUCE-free and std-only by
  construction — `app/Makefile:110-116` `check_no_juce` — so the expected
  answer is yes; first build proves it).
- **A2. Package + catalog** (self-hosted identity): package artifacts and
  a catalog JSON per `froggers-browser-package/spec.md` (publisher
  identity, `sbac-3` global id `<publisher>/frogg3rs`), served with the
  CORS/media-type constraints `sbac-7` imposes
  (`synth-browser-app-catalog/spec.md:95-106`). Local smoke: Sheaf's
  launcher with a localhost catalog URL (`sbac-10` keeps a relative/local
  catalog source for development, `spec.md:140-157`).
- **A3. pages.yml swap** (`froggers-web-host/spec.md:7-45`): replace the
  legacy `wasm/` emcmake + `web/` vite steps (`pages.yml:41-59`; artifact
  upload at `:64-67`, deploy at `:69-71` — audit-corrected) with the A1/A2 build + site publication; `web/` and `wasm/`
  remain byte-identical (spec scenario asserts dormancy, not deletion).
  The `Web preflight` step (`pages.yml:26-38`) is NOT one of the legacy
  build steps — it builds nothing (sim/doc mirror checks + generated
  host-display parity checks, e.g. `generate-host-display.mjs` writes
  `web/src/hostDisplay.generated.ts` and `--check` only verifies);
  classify each of its five checks at execution — keep what gates
  sim/doc parity that stays live, drop only what exists solely to serve
  the legacy site, and report kept vs dropped.
  The rename/publication gate stays with the operator — the workflow lands
  ready but the public cutover is their call per the spec.
- **Input capture:** frogg3rs currently requests ZERO input channels
  (`UPSTREAM-SHEAF-ASK.md:52`, audit-corrected; stale on upstream status but accurate on
  the app side), so the browser build needs no `getUserMedia` path
  (`sbw-4`: zero-input apps never call it). Re-enabling recording input
  in the browser is OUT OF SCOPE here; the sar-33 signal exists upstream
  when that day comes.
- **A4. Site shell: mobile stacking + legacy link roles** (operator
  decisions 2026-08-18; ADDED delta on `froggers-web-host` in this
  change): on mobile-width viewports the published page renders the
  sixteen-slot (4×4) encoder grid full-width, everything else stacked
  above/below — the legacy site's mobile stacking is the reference;
  small is acceptable, side-by-side is not. The page carries the legacy
  link roles — downloads/license/manual, enumerated at
  `web/index.html:39-62` — re-pointed at current references: manual →
  `MANUAL.md` (the current app's manual, `MANUAL.md:1-3`), release
  links → the release being published; old-name absence is already
  CI-gated by task 4.1's renamed-origin check. UNVERIFIED at write
  time: the mechanism for viewport-adaptive stacking of the
  Sheaf-rendered surface (host-page CSS/canvas sizing vs surface-side
  layout) — read Sheaf's browser runtime sizing path and the
  FroggersCellMap grid geometry before implementing; if it requires
  surface-side layout work beyond the browser host, that is in scope
  per the operator's instruction, but report the coupling trace.

## Part B — VST host

**Deletion first (risk-descending is wrong here; the corpse blocks
nothing, so it goes first as the cheapest group):** both option blocks in
`desktop/CMakeLists.txt:136-313` (audit-corrected span; closing `endif()`
at 313) (`BUILD_VST`, `BUILD_VST_V2`, default
OFF, never set by CI — `desktop-release.yml:27-28,60-61` carry no
`-DBUILD_VST*`, and `pages.yml` never configures `desktop/` at all
(audit-corrected: its `:44-45` cmake lines configure `wasm/`);
`desktop/PACKAGING.md:117`); tracked v2 sources
`desktop-v2/Source/PluginProcessorV2.*`, `PluginEditorV2.*`,
`HostParameterInventoryV2.hpp`, `HostParameterProcessorV2` test and its
CTest wiring (`desktop/CMakeLists.txt:279-296`); doc sections
(`desktop/PACKAGING.md:115-127`, `desktop-v2/PACKAGING.md:68-97`,
`docs/CI.md:89` clause); spec dirs `juce-vst-cc-mod-gating`,
`vst-v2-midi-modulation` (REMOVED deltas in this change; dirs deleted at
archive-time sync). v1 sources are local-only (`.git/info/exclude`) —
nothing tracked to delete beyond the CMake block that references them.
**Amended after Task 1 execution (2026-08-18) — the original deletion
list was wrong about four file groups, caught by
verify-before-delete:** `HostParameterInventoryV2.hpp` and its
Routing/PendingStore siblings are desktop-v2's live, compiled
host-parameter model (reached by the live app targets via
`FroggersV2ControlCore`/`FroggersV2HostBridge` includes,
`desktop-v2/CMakeLists.txt:99-105,193-198`) — not plugin-wrapper
corpses; kept untouched. Audit-corrected 2026-08-18:
`HostParameterStateEnvelopeV2.hpp` is NOT compiled into the live app
targets — its only includer repo-wide is
`HostParameterProcessorV2_test.cpp`; it is kept as test-only support for
that always-on CTest target.
`HostParameterProcessorV2_test.cpp` has its own always-on CTest target
(`desktop-v2/CMakeLists.txt:290-302`); the deleted desktop/ copy was
stale (hardcoded count 142 vs live 119). `PluginEditorV2.*` and
`HostedMainComponentV2.*` are compilation-dead but text-asserted by live
projection-validator tests; kept under the corpse-removal-only rule.
Actually deleted (commit `0be9ab0`): `PluginProcessorV2.*`,
`HostParameterRegistryV2.*`, the tracked v1 `HostParameterRegistry.*`,
both CMake option blocks, doc sections, and the v1 `BUILD_VST` lines in
`scripts/verify_clean_rebuild.sh`. Audit-corrected 2026-08-18: the v1
`PluginProcessor.*`/`PluginEditor.*` were never tracked
(`.git/info/exclude`) and remain on disk, untracked and untouched; the
script's guarded `BUILD_VST_V2` block
(`scripts/verify_clean_rebuild.sh:147-161`) was NOT deleted — its
`-LAH` option probe can never pass again, so it permanently SKIPs; its
removal is task 1.4. B3 builds an
INDEPENDENT dual-ID inventory over the unrelated `app/`-side six-bank
model — a parallel construction, not a recreation of deleted code. The
dual-ID requirement was never uniquely owned by `vst-v2-midi-modulation`:
`froggers-v2-app-manifest` (untouched) independently specifies and
enforces it, so removing `vst-v2-midi-modulation` in its entirety orphans
nothing.

- **B1. Plugin skeleton** (`app/vst/`): a JUCE `AudioProcessor` (VST3 +
  AU, IS_SYNTH) via CMake modeled on the deleted blocks' `juce_add_plugin`
  idiom. Durable source (the deletion has landed): the Task-1 report's
  capture, or authoritatively
  `git show 0be9ab0~1:desktop/CMakeLists.txt` lines 136-313
  (audit-corrected span) — git history
  is the reference, not the tmp scratchpad. It owns ALL JUCE; the core stays
  JUCE-free and `check_no_juce` (`app/Makefile:110-116,185-189,245`)
  stays green and untouched. `processBlock` drives
  `FroggersAppCore::ProcessFrame`; `prepareToPlay` maps to the core's
  prepare path (exact seam: read how `app/FroggersMain.cpp`'s launcher
  session drives the core — UNVERIFIED which init calls are needed,
  enumerate at implementation). Bus/MIDI posture (audit-traced
  2026-08-18): stereo output, NO audio input bus — the core requests
  zero input channels (`UPSTREAM-SHEAF-ASK.md:52`, same fact Part A's
  input-capture note rests on) — and NO MIDI-note wiring: the app core
  exposes no note-input seam (zero note-handling hits across all of
  `app/`, audit grep 2026-08-18), so the MIDI buffer is accepted and
  ignored; DAW MIDI reaches the instrument only via host parameters
  (B3).
- **B2. DAW-external transport:** the ONLY transport producer today is
  `HandleAction` off UI clicks (`app/FroggersUiSurface.hpp:1826-1900`,
  `kPlay/kStop/kFreeze` → `SetFreezeLatched`/
  `SetDesiredTransportRunning`/`PushMessage(MessageIn::Start/Stop)`).
  The plugin adds a second producer: `AudioProcessor::getPlayHead()`
  position info → the SAME `MessageIn::Start/Stop` messages + 
  `SetDesiredTransportRunning`, edge-triggered on host play-state change.
  In plugin mode the editor's internal transport controls are NOT
  rendered (DAW is the authority — the operator's explicit contract).
  The transport row is Play | Stop | Freeze | Record
  (`app/FroggersUiSurface.hpp:126`, audit-traced 2026-08-18): Play,
  Stop, AND Record are suppressed — recording is the DAW's job, the
  host records the plugin's output natively. Freeze is NOT transport —
  it is a musical control and becomes an automatable parameter (B3),
  preserving the stop-isolation semantics (`SetFreezeLatched`,
  `app/FroggersAppCore.hpp:447-450`, `TransportTeardownActive` `:471`);
  its button stays rendered and gains a "FREEZE" text label beside it
  in the row space the three suppressed controls free (operator
  instruction 2026-08-18; the transport plates are glyph-only today —
  the BPM label idiom `kBpmLabel`, `app/FroggersUiSurface.hpp:185`, is
  the nearest labeling precedent, though it sits below its control, not
  beside).
  **Host tempo (audit addition 2026-08-18):** the core already treats
  external clock as tempo authority — `MasterClock::SetTempoBpm` no-ops
  while slaved to external MIDI clock and the surface never enqueues a
  tempo request while slaved (comment chain
  `app/FroggersAppCore.hpp:569-577`, citing `src/MasterClock.cpp:963-965`
  UNVERIFIED at that path), with display-direction reads
  `DisplayTempoBpm()`/`TempoExternallyClocked()`
  (`app/FroggersAppCore.hpp:582-584`). In plugin mode the HOST is the
  external clock: host tempo reaches the master clock and the BPM slider
  behaves exactly as when MIDI-clock-slaved (display-only). Mechanism
  UNVERIFIED: playhead-BPM → `RequestTempoBpm` on change vs synthesizing
  external-origin `MessageIn::Clock` ticks (the message exists with an
  `Origin` distinction, Sheaf `ParameterModulation.hpp:990-996`) — read
  the slave-engage path in MasterClock before choosing.
  UNVERIFIED: whether the surface can suppress the transport row cleanly
  (a host-capability flag on the surface vs a build-time branch) — decide
  at implementation against the FroggersCellMap row-table structure
  (`app/FroggersUiSurface.hpp:429-449`), matching the "always reserve vs
  omit row" conventions.
- **B3. Stable-ID parameter surface:** a fresh inventory over the CURRENT
  model — `kFroggersBankCount = 6` banks × 16 slots incl. Crispy/Crunchy
  (`app/FroggersParameters.hpp:76`,
  `froggers-sheaf-parameter-model` spec) — exposing
  `juce::AudioProcessorParameter`s with dual identity (flat stableId for
  automation/MIDI-mapping + grouped display name), bridged bidirectionally
  to Sheaf's `ParameterManager` via the existing message bus
  (`MessageIn::ParamIncDec`/`MessageIn::ParamSetAbsolute` — the
  set-value message the first audit left to enumerate is
  `ParamSetAbsolute`, Sheaf `ParameterModulation.hpp:976`, traced at the
  second audit; verify its value semantics against the surface's usage
  at implementation. `app/FroggersUiSurface.hpp:1978-1986` shows the
  `ParamIncDec` producer idiom — audit-corrected; `:1954-1975` is the
  adjacent `SetSceneBlend` producer). MIDI mapping is thereby the DAW's job end to end;
  the plugin registers no internal MIDI-learn (the core has none —
  `app/FroggersModulation.hpp:1233`).
- **B4. Plugin editor:** hosts the SAME portable `FroggersUiSurface`
  through Sheaf's `PortableJuceBackend` renderer (the launcher's
  rendering path without the Sheaf runtime shell/sidebar — the DAW owns
  audio devices, so no Audio page; read `app/FroggersMain.cpp` +
  `runtime/LauncherWindow.hpp` for the render-host seam before building;
  UNVERIFIED how much of the session plumbing is separable — if the
  renderer cannot be hosted without the full runtime session, the fallback
  is a minimal editor (parameters visible in the DAW's generic view) and
  the full editor becomes its own follow-up change; that fallback decision
  is BLOCKED-report material, not silent descoping).

## Testing

- A: manifest validation + build emits `frogg3rs.js/.wasm` (build-level
  test in the new script, CI-runnable); catalog JSON schema-validates;
  pages workflow dry-run builds; legacy `web/`+`wasm/` byte-identity
  asserted (git-clean check in CI step); mobile-viewport stacking
  asserted (grid spans viewport width, nothing beside it) CI-runnably
  where the harness allows, else at the operator smoke; link roles
  present with no old-name target (folds into 4.1's old-name gate).
  Operator browser smoke incl. phone-width layout and links.
- B: deletion — desktop default build still configures/builds green
  (option blocks gone, nothing references them: grep gate); new plugin —
  parameter round-trip tests (host sets param → core value moves → host
  readback matches; stableIds stable across runs), transport edge tests
  (playhead run/stop transitions produce exactly one Start/Stop message
  each), tempo-follow test (host tempo change reaches the clock,
  `DisplayTempoBpm()` follows, `TempoExternallyClocked()` true, user
  tempo request rejected while slaved), editor-row assertions
  (Play/Stop/Record absent, Freeze present with its "FREEZE" label),
  `check_no_juce` untouched and green, plugin loads in
  `pluginval`-style host if available (else the DAW smoke is the
  operator's gate — say which in the report). Full app suite
  (`cd app && nice make -j2 test`, 279/279 baseline) green throughout.

## Risks

- Emscripten-compiling the app core for the first time (A1) may surface
  std-usage portability issues — bounded by the core's JUCE-free/std-only
  discipline; first build is the probe.
- B4's renderer-without-runtime separation is the largest unknown
  (explicitly gated above).
- The pages.yml swap changes the public site's content; the operator's
  rename/publication gate (froggers-web-host) is the protection.
- desktop-v2 is frozen: B's deletions there are corpse-removal only, no
  behavioral edits to the frozen tree.

## Gates

Per task group: `cd app && nice make -j2 test` green (279/279 + new);
NEVER above -j2. A-side additionally: the browser build script runs green
locally. Operator gates: browser smoke (A3/A4) and DAW smoke (B, in a
real DAW) — BOTH deferred to the end and run together in one sitting
(group 9); the operator tests nothing until every group has landed
(operator instruction 2026-08-18).

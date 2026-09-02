# Tasks — `frogg3rs-midi-mappings-for-froggers`

Task 1 was run once on 2026-09-01; the operator's audit of that pass on
2026-09-01/02 was the second preflight, and the third preflight of
2026-09-02 (task 1.8) revised the dispatch design. Execution is approved
on that revision. A postflight follows every task, and the commit and
push happen only when every postflight has passed. Dispatches are
sequential.

## 1. Preflight, before any code

- [x] 1.1 Hygiene sweep of every directory in the proposal's Impact list,
      named one by one in the report. Fix what it finds inside this change.
- [x] 1.2 Behavioural checks through `synth_rig::SynthRig<FroggersApp>`
      (`app/FroggersHeadlessTests.cpp:73`), each with its deciding quantity
      and a positive control: `ParamIncDec` moves a parameter; `ParamPush`
      on the MIDI bus versus `RequestEncoderPress`, reading both the library
      bank's `ShowingModulation()` and the app's `DrillLevel()`;
      `SceneSelect` and `SetSceneBlend`; `SetRandomMod` and `SetReset`
      followed by a tick and a push. `Start`/`Stop` on the MIDI bus are no
      longer a premise: Play and Stop are app actions.
- [x] 1.3 Reads that settle design points, each cited: the thread that
      drains `Engine::midiBus_`; what a failed `FromJSON` does to the saved
      runtime config; absolute-knob behaviour after a swallowed hold; which
      app path a MIDI encoder press must land on; the patch-file format and
      the thread that applies a loaded patch; the output-processor reset
      path that carries a connect-time message.
- [x] 1.4 Family enumeration: every `switch` on `MessageIn::Type` and on
      `UISystemMessage` outside tests, FOUND vs to-CHANGE per site; the new
      names grepped by operand in both trees, zeros reported.
- [x] 1.5 Deployed site: the Controllers page renders, the offered kinds
      are listed, a Generic controller can be added and removed.
- [x] 1.6 Both device manuals read: Twister User Guide (encoder and side
      button MIDI, Utility settings), APC40 Mk2 Communications Protocol
      v1.2 (inbound tables, generic-mode toggle list, mode introduction).
- [x] 1.7 Revise the proposal on the findings and stop for the go.
- [x] 1.8 Third preflight: traced the screen's action branches and the
      core's thread contracts; moved app actions and the encoder press to
      a message-thread dispatch through `PortableSurface().DispatchAction`;
      made Play and Stop app actions; gated the patch-file section on the
      catalog; sequenced the `FroggersEngine.hpp` citation fixes to the
      guitar change; grepped every new name in both trees (all zero).

## 2. Sheaf library, on branch `app-midi-catalog`

- [x] 2.1 Hygiene from 1.1: remove `PumpJuceMessages`
      (`juce/ControllersPageHarness.hpp:222`) and the three unused
      `SynthRig` helpers
      (`tests/support/SynthRig.hpp:186, :315, :324`); fix the citation at
      `include/synth/Engine.hpp:551`.
- [x] 2.2 `MidiAppCatalog.hpp` with `MidiAppAction` (action, value, label,
      `analogRange`), `MidiAppDeviceDefault`, `MidiAppCatalog` (actions,
      `libraryKinds`, `encoderPressAction`, `patchCarriesMappings`,
      `deviceDefaults`); `HasMidiCatalog<App>` in `AppConcepts.hpp`
      requiring `MidiCatalog()` only; the header added to
      `runtime/juce_build.mk`'s list.
- [x] 2.3 `MessageIn::Type::AppAction` and `HoldDrill`, appended; the
      `appActionIx` field; the two factories; name table and parse;
      `DescribeMessage`; `UISystemMessage::AppAction`/`HoldDrill` appended;
      `UISystemMessageChoice` action/value/index fields; every switch site in
      the proposal's disposition table; the pairing table in
      `viewmodel_tests.cpp` gains both kinds and compares by action and value.
- [x] 2.4 `ParameterMessageOut::Type::AppAction`/`AppEncoderPress` with their
      fields and factories; `MessageInBus::SetAppActionOut(out,
      forwardEncoderPress)`; the `Apply` cases; `Engine` sets both buses
      when the concept holds and `MessageThreadTick` dispatches the two
      types to `app_.PortableSurface().DispatchAction` as the proposal's
      section 4 formats them. Tests in `engine_tests.cpp` with a fake app
      whose surface records dispatched actions: an `AppAction` on the MIDI
      bus arrives after one tick as the catalog's action and value; an
      analog value maps through `analogRange`; with `encoderPressAction`
      set, a `ParamPush` arrives as that action with the position and
      `Bank::ShowingModulation()` stays 0; with it empty, `HandlePress`
      runs as today.
- [x] 2.5 Persistence: `appAction`/`appActionValue` on the association
      and on the new `AnalogAppActionMapping`, one address lookup shared
      with `FindGesture`, JSON both ways;
      `AnalogMidiInProcessor` pushes `AppAction` for a matching control;
      `Engine::RebuildMidiProcessors` resolves actions against the catalog on a
      copy and drops an unknown row with one status line. Tests: a saved
      file with one unknown action loads with the other rows intact and the
      file itself accepted; a known action resolves to the right index.
- [x] 2.6 Hold Drill: `HoldDrillState` owned by `MidiControllerProfileResult`,
      handed to both processors by `CreateMidiControllerProfileImpl`.
      Tests in `instrument_tests.cpp` driving `BasicMidi` through both
      processors on one bus: hold, turn, turn, release, turn gives exactly
      one `ParamPush` and then one `ParamIncDec`; two encoders turned during
      one hold each push once; an absolute encoder under hold emits nothing
      and its first turn after release emits `ParamSetAbsolute`.
- [x] 2.7 Catalog-driven page (2.7a the message list, 2.7b the analog
      app-action row group from the proposal's section 7): `ControllersPageCallbacks.messageCatalog` and
      `.layouts`; `MidiConfigViewModel::SetMessageCatalog`; the four readers
      of the static list read the view model's copy; row identity by
      `(message, appAction, appActionValue)`; the runtime services and
      the harness supply both vectors. Tests in `viewmodel_tests.cpp` with a
      fake catalog: offered kinds equal the catalog in order; an app-action
      row round-trips through edit and snapshot with its action and value.
- [x] 2.8 Layout dropdown: the "Layout" combo per controller row; choosing
      a layout generates through the descriptor's wizard and commits;
      "Custom" clears `wizardId`; a mapping edit clears `wizardId`;
      `kControllerReconfigure` and `OpenExistingFromSnapshot` removed with
      every mention of them; `ControllerWizardRegistry()` becomes the
      services' vector (app defaults when supplied, else the library's
      Twister descriptor), and its seven callers plus `MakeControllerWizard`
      take it. Tests in `controller_wizard_tests.cpp` and the controllers
      page tests: choosing a layout installs that descriptor's config and
      sets `wizardId`; editing a row after that shows Custom; discovery
      still classifies a device by the app descriptor's alias.
- [x] 2.9 `openSysEx` on `MidiControllerProfileConfig`, persisted; an
      output processor that sends each message exactly once after `Reset()`.
      Test: a profile with one `openSysEx` message enqueues it once after
      construction, not again on the next `Process()`, and once more after
      `Reset()`.
- [x] 2.10 Patch files: `ApplyPatchMessage` gains `carryInstrument`, passed
      from the catalog's `patchCarriesMappings`; with it `BuildPatchJSON`
      writes `schemaVersion` 2 with `midiInstrument`, without it version 1
      as today; `LoadPatchJSON` returns a present section through
      the new out-parameter and applies parameters only for a version-1
      file; `ApplyPatchMessage` passes it through; `Engine` stores it in a
      guarded pending slot and `MessageThreadTick` applies it through
      `EditInstrument`. Tests: a saved patch reloads its mappings; a
      version-1 patch leaves the live instrument untouched; a file without
      the section leaves it untouched; an app without the flag writes
      version 1 and ignores a version-2 file's section.
- [x] 2.11 Sheaf spec deltas: `synth-controller-wizards`, `synth-runtime-ui`,
      `synth-midi-instrument`, `synth-patch-persistence`.
- [ ] 2.12 Gates: `nice make -j2 -C External/Sheaf/projects/synth test` and
      the `miniapp` target, counts reported; the two braid-4 96 kHz deadline
      tests are a pre-existing machine-bound failure, reported as carried. Miniapp and braid-4 declare no
      catalog, see the full library list, suites unchanged. Branch pushed
      to the fork, upstream PR opened with the description the proposal's
      Delivery section specifies, including the testing instructions, the
      Launchpad SysEx procedure and the UNCONFIRMED marker.

## 3. frogg3rs

- [x] 3.1 Hygiene from 1.1: fix the `EncoderDraw.hpp` citation at
      `FroggersUiSurface.hpp:1827` and `FroggersSurfaceTests.cpp:1401`. The
      19 `FroggersEngine.hpp:LINE` citations are the guitar change's (its
      `tasks.md` carries the item); nothing under `src/` and no engine
      citation in `FroggersAppCore.hpp` is touched here.
- [x] 3.2 `FroggersMidiCatalog.hpp`: the action table in the proposal, the
      kept library kinds, `encoderPressAction = kEncoderPress`,
      `patchCarriesMappings`, and the three device defaults control for
      control as the proposal's tables give them, including the Ableton
      default's `openSysEx` message; `FroggersApp::MidiCatalog()` in
      `Froggers.hpp`; `kBpmMin`/`kBpmMax` replacing the slider's literals
      at `FroggersUiSurface.hpp:1490`. No method on the core.
- [x] 3.3 Tests, through `synth_rig::SynthRig<FroggersApp>`
      (`FroggersHeadlessTests.cpp:73`) pushing on `Engine::MidiBus()` and
      ticking the message thread: every catalog action moves the state the
      screen's branch moves, failing by action and value (the make check
      `check-catalog-covers-screen-actions` fails on a `FroggersActions`
      constant the catalog or `HandleAction` does not name, proven by
      breaking it once); a MIDI
      `ParamPush` raises `DrillLevel()` 1, 2, 3, 3 exactly like
      `RequestEncoderPress`, with the drill bank's `ShowingModulation()`
      equal at every step on both paths (the two never desync); the
      catalog names every front-screen action (the proposal's list) and
      nothing the screen does not route, so a control added to the screen
      without a catalog entry fails by name; `FroggersParameterModelTests.cpp`'s
      shared-Crunchy check presses the library manager directly, because a
      bus `ParamPush` is now this app's drill-in and never reaches the
      library's selection; each of the three device defaults validates
      against the library's kind support, addresses every control the
      proposal's table names and no other, and the Twister default's side
      buttons sit at ch 3 CC 8-13.
- [x] 3.4 `MANUAL.md` MIDI section as the proposal's section 10 lists it.
- [x] 3.5 Pin bump as its own commit; `nice make -j2 -C app test`; browser
      wasm build; push `main`; Pages and VST workflows green.

## 4. Operator, then archive

- [ ] 4.1 Desktop with the Twister: the Layout combo offers MIDI Fighter
      Twister, Akai APC40 mkII (Generic), Akai APC40 mkII (Ableton), Custom;
      choosing the Twister installs the layout; each side button does what
      the table says; a push drills in level by level and stops at the cap;
      pressing the selected cell again backs out one level; editing any
      row flips the combo to Custom.
- [ ] 4.2 Desktop with the APC40 in the Generic layout: each button and
      knob in the table does what it says; with Track 1 selected all 16
      knobs move encoders 1-16; SHIFT held plus a knob turn drills that
      knob and leaves its value alone; releasing SHIFT returns the knob to
      normal on its next turn.
- [ ] 4.3 UNCONFIRMED until this passes. Desktop with the APC40 in the
      Ableton layout: after choosing it, press any Track Select button and
      turn device knob 1; encoder 9 still moves. Buttons are dark, as the
      proposal states.
- [ ] 4.4 Save a patch, change a mapping, reload the patch: the mapping is
      back. Relaunch: the runtime config's map is what loads.
- [ ] 4.5 Deployed site with either controller: the Controllers page offers
      only frogg3rs targets; a mapped control moves the real thing.
- [ ] 4.6 The manual's MIDI section, followed on a Twister and on an APC40
      at factory settings with nothing else to go on.
- [ ] 4.7 Archive on confirmation.

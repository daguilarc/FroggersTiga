# Proposal — `frogg3rs-midi-mappings-for-froggers`

**Created 2026-09-01. Rewritten 2026-09-02 after the operator's audit
restored two rulings the earlier text had lost: two default devices, and
patch files carrying mappings. Revised again 2026-09-02 by the third
preflight (tasks 1.8): app actions and the encoder press now dispatch on
the message thread through the app's own surface (the audio-thread sink
would have called `ArmRecording`, which allocates); Play and Stop are app
actions (the screen's branches clear the Freeze latch before the push);
the patch-file section is gated on the catalog; the `FroggersEngine.hpp`
citation fixes are sequenced to the guitar change.** Execution is approved
on this revision.

All Sheaf paths are under `External/Sheaf/projects/synth/` at pin 350e3cf0
(branch `app-midi-catalog`, created off `browser-audio-activation`, tree
clean). frogg3rs paths are under `app/` except `MANUAL.md`, which is at the
repo root. Line numbers are 2026-09-01/02 reads.

## The defect, traced

The Controllers page offers one fixed list of mappable targets,
`UISystemMessageCatalog()` (`src/MidiConfigViewModel.cpp:421-446`), read at
four sites: the message-kind combo (`include/synth/ControllersPageUI.hpp:2518`),
`FindUISystemMessageChoice` (`src/MidiConfigViewModel.cpp:449`), the
row-index lookup (`:1626`) and a fourth reader at `:2404`. Observed on the
deployed site 2026-09-01: the page renders in the browser host, a Generic
controller can be added with CC or Note addressing, and the combo offers
exactly those 21 kinds. They are the library's parameter-modulation
vocabulary, handled by `ParameterManager` inside `MessageInBus::Apply`
(`src/ParameterModulation.cpp:4077-4199`), not by any sample app.

Measured 2026-09-01 through `synth_rig::SynthRig<FroggersApp>`, pushing on
`Engine::MidiBus()` and draining with `RunBlocks` (numbers are the app's
startup patch, slot 0 position 0):

| message on the MIDI bus | what moved |
|---|---|
| `ParamIncDec(+0.25)` | parameter 0.3108 -> 0.5492, same as the UI bus |
| `ParamPush` x6 | library `Bank::ShowingModulation` 0 -> 1 after the first push, then flat; app `DrillLevel()` 0 throughout |
| `RequestEncoderPress` x6 (screen path) | app `DrillLevel()` 1, 2, 3, 3, 3, 3 |
| `SceneSelect(1)` | nothing (scenes stay 0/1, blend 0) |
| `SetSceneBlend(1.0)` | blend 0 -> 1 |
| `SetRandomMod(true)` then `ParamIncDec` | held flag set; the tick is swallowed |
| `SetReset(true)` then `ParamPush` | parameter 0.3108 -> 0.3087; drill level unchanged |

So, for frogg3rs today:

1. **A MIDI encoder push is not the app's drill-in.** It reaches
   `Bank::HandlePress` (`src/ParameterModulation.cpp:2628-2650`) and opens
   the library's modulation view, one level, toggling. The app's own drill
   wrapper (`FroggersModulationDrillIn::PressEncoder`,
   `FroggersModulation.hpp:741`, cap 3, Back guard, level display) never
   sees it: `drillLevelDisplay_` is published from `drillIn_` only
   (`FroggersAppCore.hpp:1300`) and `drillIn_` is rebuilt only on a bank
   switch (`:695`; `:670` is its first-frame construction). The two paths
   desync from the first push. What the operator saw on the desk was the
   library view.
2. **The library's scene select is inert here.** frogg3rs's Scene 1/2 are
   app actions that push `SetSceneBlend` (`FroggersUiSurface.hpp:2289`,
   `:2310`; button wiring at `:1346`; `FroggersSurfaceTests.cpp:2030`).
3. **The held modifiers do act through MIDI** (they gate ticks at
   `src/ParameterModulation.cpp:4080` and reset on press). The operator's
   ruling is that they are not offered.
4. **The app's named actions are unreachable.** Randomize All/Page, Reset
   All/Page, Freeze, Record, BPM, bank select and prev/next go through
   `HandleAction` (`FroggersUiSurface.hpp:2157` onward) into request flags
   (`FroggersAppCore.hpp:577-598`) or, for Record, the core's arm call
   (`FroggersUiSurface.hpp:2264`, `ArmRecording` at `FroggersAppCore.hpp
   :503`); no `MessageIn` kind reaches them. Play and Stop are not
   exceptions: the screen's branches clear the Freeze latch and record the
   transport intent around the library push (`:2175-2183`, `:2210-2212`;
   Freeze's engage does the same at `:2246-2247`), and a bare `Start` on
   the MIDI bus does neither, so Play and Stop are app actions too. Bank
   selection must stay on the app path, because the library's
   `SelectParamBank` would switch the bank under `drillIn_` without the
   rebuild at `:695`. Encoder drag is the library's `ParamIncDec`, pushed
   by the screen (`:2338`); an on-screen encoder press is
   `RequestEncoderPress` (`:2346`), never a `ParamPush`.
   Every one of these paths is message-thread code: the request flags are
   "called from FroggersUiSurface::DispatchAction (UI/message thread)"
   (`FroggersAppCore.hpp:571-572`), `MessageInBus::Push` is one producer
   per bus (`src/ParameterModulation.cpp:4050-4060`), and `ArmRecording`
   assigns a buffer of up to thirty minutes of audio (`:503-516`; "All
   allocation happens once, in ArmRecording() (UI thread)" at `:2459`). No
   MIDI path may call them from the audio thread.
5. **Device defaults are library-baked.** The Twister wizard installs
   `MfTwisterDefaultProfileConfig` (`src/ControllerWizard.cpp:809-843`,
   builder `src/MidiController.cpp:3059-3077`) with the sample app's six
   side-button meanings (`src/ControllerWizard.cpp:398-405`: Hold Reset,
   Hold Random, Hold Random Mod, Next Bank, Start, Previous Bank). There is
   no APC40 layout anywhere.
6. **Patch files do not carry mappings.** `BuildPatchJSON` writes
   `schemaVersion` 1 with `parameterValues` only (`src/PatchPersistence.cpp
   :325-338`); `LoadPatchJSON` ignores a `midiInstrument` section by
   upstream decision (`include/synth/PatchPersistence.hpp:69-75`).

## Data flow (as it is)

```
device bytes
  -> MidiInProcessor chain per controller slot, built by
     CreateMidiControllerProfileImpl (src/MidiController.cpp:2778-2820):
       EncoderMidiInProcessor   turn -> ParamIncDec | ParamSetAbsolute
                                push -> ParamPush            (:695-737)
       AnalogMidiInProcessor    cc   -> SetGestureValue | SetSceneBlend
                                                             (:792-811)
       SystemButtonMidiInProcessor  press -> association.press,
                                    release -> association.release (:898-906)
  -> Engine::midiBus_, rebuilt from instrumentConfig_.controllers on every
     commit (include/synth/Engine.hpp:893-915, RebuildMidiProcessors,
     message thread)
  -> ProcessBlock, AUDIO THREAD: DrainMessageBus(uiBus_) then
     DrainMessageBus(midiBus_) (:399-400); realtime kinds to the clock
     lane (:1027-1056), the rest to MessageInBus::Apply; then
     app_.ProcessFrame() (:408)
  -> MessageInBus::Apply (src/ParameterModulation.cpp:4077-4199):
       param kinds  -> ParameterManager   (ParamPush -> HandlePress, :4095)
       scene kinds  -> ParameterManager scene state
       grid kinds   -> GridManager, the one non-parameter sink
                       (SetGridManager, include/synth/ParameterModulation.hpp
                       :1035, wired at Engine.hpp:161-163)
       held kinds   -> ParameterManager held flags

screen press -> kEncoderPress -> HandleAction (FroggersUiSurface.hpp:2345)
  -> RequestEncoderPress (FroggersAppCore.hpp:580) -> ProcessFrame drain
  (:719-722) -> DrillIn::PressEncoder (FroggersModulation.hpp:741)
screen action -> HandleAction -> Request* flags (atomics,
  FroggersAppCore.hpp:2414-2421) -> ProcessFrame drain (audio thread)

Controllers page: ControllersPageSurface (ControllersPageUI.hpp:914) owns
  MidiConfigViewModel and ControllersPageCallbacks (:865-873), made by
  runtime/JuceRuntimeMainServices.hpp:47-58, include/synth/browser/
  BrowserRuntimeMainServices.hpp:48-59 and juce/ControllersPageHarness.hpp
  :75-90, all holding the Engine.
  UI kind -> association: MakeUISystemMessageAssociation
  (src/MidiConfigViewModel.cpp:456-464) through PressForUISystemMessage
  (:179-226) and ReleaseForUISystemMessage (:228-258); reverse:
  UISystemMessageForAssociation (:131-174).
Wizard: ControllerWizardRegistry() static, Twister only
  (src/ControllerWizard.cpp:845-854), matched by device-name alias
  (:224-228); MakeControllerWizard(id) walks it (:895). Callers outside
  tests: ControllerWizardDiscoveryCache.hpp:42, ControllersPageUI.hpp
  :1008, :1155, :1516, :1547, MidiConfigViewModel.cpp:705, :2566.
  Reconfigure: ControllersPageUI.hpp:1098 names the action, :1101-1134
  reopens the wizard form seeded from the slot, :1147-1158 dispatches,
  :1163-1170 reports "Reconfigured".
Persistence: kind written by name (src/MidiController.cpp:2253-2255),
  parsed by name (:2307-2314; ParseMessageType :232, fails closed at
  :281-283). One unparseable message fails the association (:2477-2481),
  the profile, the instrument and the whole runtime config file
  (src/PatchPersistence.cpp:187); the engine then keeps its defaults for
  controllers, audio device and sync (include/synth/Engine.hpp:621-634).
Patch files: BuildPatchJSON (src/PatchPersistence.cpp:325) and
  LoadPatchJSON (:340), driven by ApplyPatchMessage (:539-586), which the
  engine calls with its live instrumentConfig_ from the audio-thread drain
  (Engine.hpp:1091, :1192, :1199) and once pre-audio (:378). The audio
  thread never mutates instrumentConfig_ (:682); the message thread
  rebuilds processors through EditInstrument (:727-735).
Absolute knobs: HandleSetAbsolute applies the value directly when no
  modifier is held; there is no pickup, so an absolute knob jumps on its
  first turn whenever the parameter moved by another route.
```

## Operator rulings this design implements

- Two default devices: MIDI Fighter Twister and Akai APC40 mkII. The APC40
  ships as two selectable defaults, "(Generic)" and "(Ableton)".
- Offered targets are every front-screen control and nothing from the
  configuration sidebar.
- The Controllers page gets a layout dropdown: the stored device defaults
  plus "Custom". It replaces the wizard's Reconfigure path.
- Saving a patch file saves the MIDI mappings, whether a modified default
  or a custom map. The runtime config still persists for relaunch. Both
  snapshot the live instrument; whichever loads last wins.
- An unknown app-action id in a saved file drops that row only.
- `ownsEncoderPress` is approved; it is the catalog's `encoderPressAction`.
  Record rides the same dispatch as every other action.
- Hold Drill behaves like Shift: while the button is down, a knob turn
  drills that knob and its value does not move; on release the knob is a
  plain knob again.
- The Ableton default is unconfirmed on hardware until the operator's
  check; it is marked so here, in the tasks and in the PR description.
- No PR numbers in artifacts. No parallel dispatches.

## Design

Additive on the library side. No baked table moves, no message kind is
removed, no saved profile stops loading, and apps that declare no catalog
see exactly what they see today.

### 1. What frogg3rs offers

Every front-screen control, by the operator's rule:

| front-screen control | offered as | route |
|---|---|---|
| encoder turn (16) | library `Param Inc/Dec`, `Param Set Absolute` | `ParameterManager`, as the screen's own drag (`FroggersUiSurface.hpp:2338`) |
| encoder push (16) | library `Param Push` | forwarded as `Action{kEncoderPress, position}` to `RequestEncoderPress` (`:2346`), the screen's own press (item 1 above) |
| Play, Stop, Freeze, Record | app actions | the screen's own branches (`:2175-2183`, `:2210-2212`, `:2246-2247`, `:2264`) |
| Randomize All, Randomize Page, Reset All, Reset Page | app actions | request flags, as the screen (`FroggersAppCore.hpp:577-598`) |
| Bank 1..6, Bank Previous, Bank Next | app actions | `RequestBankSelect`, the arrows behind their drill gate |
| Scene 1, Scene 2 | app actions | `SetSceneBlend` on the UI bus, as the screen (`:2289`) |
| Scene blend | library `Scene Blend` (analog) | `ParameterManager` scene state |
| BPM | app action with `analogRange` | `RequestTempoBpm` behind the screen's external-clock guard (`:2399`) |
| any button | library `Hold Drill` | the profile's input chain, never the bus |

Not offered: the held modifiers, every gesture kind, library Scene Select,
bank, `Start` and `Stop` kinds (items 2 to 4; a bare `Start` skips the
Freeze latch), `Continue` (no such control on the screen),
`Clock` (a timing tick from a clock source, meaningless from a mapped
control), the plugin host's input-channel button
(`FroggersUiSurface.hpp:1306`, rendered only in plugin-host mode in place
of Play/Stop/Record; a DAW bus picker), viewport narrow, and encoder drag
(the screen's mouse route into the same `ParamIncDec`).

### 2. App catalog hook

`include/synth/MidiAppCatalog.hpp` (new; includes `MidiController.hpp` for
`MidiControllerProfileConfig` and `MidiConfigViewModel.hpp` for
`UISystemMessage`) declares:

```cpp
struct MidiAppAction {
    std::string action;      // a ui::Action name the app's surface routes
    std::string value;       // that action's value ("3" for Bank 4); empty when none
    std::string label;       // what the Controllers page shows
    std::optional<std::pair<float, float>> analogRange;
                             // set: an analog control drives it, and the
                             // dispatched value is min + v * (max - min)
};
struct MidiAppDeviceDefault {
    std::string id;          // wizard id, stored in MidiControllerSlot::wizardId
    std::string displayName; // dropdown label
    MidiProfileKind kind;
    std::vector<std::string> inputAliases, outputAliases;
    MidiControllerProfileConfig config;
};
struct MidiAppCatalog {
    std::vector<MidiAppAction> actions;
    std::vector<UISystemMessage> libraryKinds;
    std::string encoderPressAction;   // ui::Action a ParamPush dispatches, with the
                                      // position as its value; empty = the library's
                                      // HandlePress, as today
    bool patchCarriesMappings = false;
    std::vector<MidiAppDeviceDefault> deviceDefaults;
};
```

`include/synth/AppConcepts.hpp` gains, beside `HasRestoreStartupState`
(`:68`, the accepted out-of-tree extension pattern):

```cpp
template <typename T>
concept HasMidiCatalog = requires(const T app) {
    { app.MidiCatalog() } -> std::same_as<MidiAppCatalog>;
};
```

The catalog is data; the app declares no method beyond it. Every
`SynthApplication` already exposes `PortableSurface()` returning
`ui::Surface&` (`AppConcepts.hpp:23-26`), and `ui::Surface::DispatchAction`
is the authoritative route for backend-originated UI actions
(`include/synth/PortableUI.hpp:285-289`); the engine dispatches app
actions there (section 4). The include is acyclic: `AppConcepts.hpp` is
included by `Engine.hpp:14`, `RuntimePages.hpp:6`, `AppRegistry.hpp:3`,
`RuntimeMainComponent.hpp:3`, `browser/BrowserAppEntry.hpp:3`,
`runtime/Runtime.hpp:75`, `runtime/MainPane.hpp:3`, `runtime/Shell.hpp`
and `runtime/LauncherWindow.hpp`, none of which
`MidiController.hpp` (`:3-5`: MasterClock, ParameterModulation,
RuntimeUIState) or `MidiConfigViewModel.hpp` (`:18-20`: MidiConfigBlocks,
MidiController, MidiReconcile) reaches. The new header joins the list at
`runtime/juce_build.mk:51`.

The catalog is read once, in the engine constructor and by the runtime
services at startup.

### 3. Two new message kinds

`MessageIn::Type` (`include/synth/ParameterModulation.hpp:942-967`) gains
`AppAction` and `HoldDrill`, appended after `ParamSetAbsoluteOnBank` so
every existing ordinal holds (the comment at `:961-963` and
`SystemMessageSortKey::typeOrder`, `include/synth/MidiConfigBlocks.hpp
:63-114`, depend on declaration order). `MessageIn` gains one field,
`std::size_t appActionIx = 0`, and two factories beside `:1027`:
`AppAction(timestamp, appActionIx, value)` and `HoldDrill(timestamp, held)`
(held in `boolValue`, `hasBoolValue = true`, the `SetReset` shape at
`:999`). No string travels on the bus: `MessageIn` sits in a lock-free ring
(`:1030-1053`) and stays trivially copyable.

`UISystemMessage` (`include/synth/MidiConfigViewModel.hpp:194-216`) gains
`AppAction` and `HoldDrill`, appended. `UISystemMessageChoice` (`:218-221`)
gains `std::string appAction`, `std::string appActionValue` and
`std::size_t appActionIx`; a library kind leaves them empty and zero. The
offered list is a vector of these choices, one per kept library kind plus
one per app action, in that order.

### 4. Dispatch: the audio thread forwards, the message thread acts

`ParameterMessageOut` (`include/synth/ParameterModulation.hpp`, the
audio-to-message ring `MessageThreadTick` already drains, `Engine.hpp
:488-503`) gains `Type::AppAction` and `Type::AppEncoderPress`, fields
`std::size_t appActionIx`, `float value`, `std::size_t slotIx`,
`std::size_t position`, and two factories beside
`ParameterStorageBatchNeeded`. `MessageInBus` gains
`SetAppActionOut(ParameterMessageOutBus* out, bool forwardEncoderPress)`.
In `Apply` (`src/ParameterModulation.cpp:4077-4199`): the `AppAction` case
pushes `AppAction` on the out bus under an `out != nullptr` guard, the
shape the `ParamSetAbsoluteOnBank` case documents (`:4089-4094`); the
`ParamPush` case (`:4095-4099`) pushes `AppEncoderPress` instead of
calling `HandlePress` when forwarding is on; `HoldDrill` is a `break`
with a comment saying the processors consume it before the bus (section
6). `Engine`'s constructor sets both on `uiBus_` and `midiBus_` beside
`:161-163` when `HasMidiCatalog<App>` holds, forwarding presses when
`encoderPressAction` is non-empty.

`MessageThreadTick`'s drain loop (`Engine.hpp:488-503`) handles the two
new types under `if constexpr (HasMidiCatalog<App>)`: `AppAction` becomes
`ui::Action{catalog.actions[ix].action, value}`, the value string being
the action's own `value` or, for an `analogRange` action,
`std::to_string(min + v * (max - min))`; `AppEncoderPress` becomes
`ui::Action{catalog.encoderPressAction, std::to_string(position)}`; both
go to `app_.PortableSurface().DispatchAction(action)`. The out bus holds
64 messages (`ParameterModulation.hpp:722`); the tick runs at
`uiFrameHz`, 30 by default (`runtime/Runtime.hpp:335`; the browser and the
plugin pump the same tick, `include/synth/browser/BrowserRuntime.hpp:717`,
`app/vst/FroggersPluginProcessor.cpp:290`). A MIDI action or press lands
within one frame plus one block; a drill-in is a view change, not audio,
so that latency is accepted, and a full fader sweep between two ticks is
about 33 CCs, under the ring's capacity.

frogg3rs: `FroggersUiSurface::DispatchAction` (`FroggersUiSurface.hpp
:985`) is `HandleAction` plus the outer handler, so a MIDI action runs
the screen's own branch, Play's latch clear, Record's refusal callback,
BPM's external-clock guard (`:2399`) and the arrows' drill gate included;
an encoder press is `Action{kEncoderPress, position}` to
`RequestEncoderPress` (`:2346`), the app's drill path. There is one
dispatch table, `HandleAction`, and the catalog is a list of its names.

### 5. Persistence

**System-message rows.** `MidiControllerSystemMessageAssociation`
(`include/synth/MidiController.hpp:865-873`) gains `std::string appAction`
and `std::string appActionValue`. `ToJSON` (`src/MidiController.cpp
:2449`) writes them as `"appAction"` and `"appActionValue"` whenever
`press.type == AppAction`; `FromJSON` (`:2477`) reads them. The press's
`appActionIx` is not persisted: it is a runtime index resolved from
`(appAction, appActionValue)` against the catalog.

**Analog rows.** `AnalogMidiInConfig` (`:266-274`) gains
`std::vector<AnalogAppActionMapping> appActions` with
`{MidiControlAddress control; std::string appAction; std::string
appActionValue; std::size_t appActionIx;}`, found by the one address
lookup `FindGesture` uses (`:813`), shared, not copied; JSON beside
`AnalogMidiMapping`'s (`:2156-2173`). `AnalogMidiInProcessor::Process`
(`:792-811`) gains one branch: a matching control pushes
`AppAction(ix, value / 127)`.

**Resolution and the drop-one-row rule.** `Engine::RebuildMidiProcessors`
(`Engine.hpp:893-915`, message thread) resolves every `AppAction`
association and analog mapping of a COPY of each controller config against
the catalog before `CreateMidiControllerProfile`; an action and value the
catalog does not have drops that row from the copy and logs one status
line naming the action. The persisted config keeps the row, so a later app
version that knows the id gets it back. Nothing here changes
`ParseMessageType`'s fail-closed policy (`:281-283`); the two policies sit
side by side and the PR names both.

**Patch files.** Only for an app whose catalog sets
`patchCarriesMappings` (frogg3rs does). `ApplyPatchMessage`
(`include/synth/PatchPersistence.hpp:194-198`, the only caller of
`BuildPatchJSON` and `LoadPatchJSON` outside their own file) gains a
`bool carryInstrument` the engine passes from the catalog. With it,
`BuildPatchJSON` (`src/PatchPersistence.cpp:325`) writes `schemaVersion`
2 and a `midiInstrument` section, `ToJSON(instrument)`
(`include/synth/MidiController.hpp:1054`); without it the file stays
version 1 and a loaded section is ignored, the policy
`include/synth/PatchPersistence.hpp:69-75` states, which therefore stays
true for miniapp, braid-4 and every app without a catalog. The ruling
that patches carry mappings is frogg3rs's, so frogg3rs's catalog says
so. `LoadPatchJSON` (`:340`) parses a present, valid section into a new
`std::optional<MidiInstrumentConfig>*` out-parameter and leaves the
engine's live instrument alone; a version-1 file, or a file without the
section, loads parameters only, as today. `ApplyPatchMessage` passes that out-parameter
through; `Engine` stores the result in a mutex-guarded pending slot beside
`audioDeviceStateMutex_` and `MessageThreadTick` (`Engine.hpp:488`)
applies it through `EditInstrument` (`:727-735`), which rebuilds and saves
the runtime config. The audio thread never writes `instrumentConfig_`
(`:682`). The load path already parses JSON on the thread that drains
(`:1192`); parsing one more section there is the same property, not a new
one. The Sheaf spec `synth-patch-persistence` (its scenario at `:24`
lists the root keys) gets the delta.

### 6. Hold Drill

`MessageIn::Type::HoldDrill` names the button's press (`held = true`) and
release (`held = false`) in the association, exactly as Hold Reset uses
`SetReset(true/false)` (`src/MidiConfigViewModel.cpp:179-258`). The state
lives in the profile's input chain, not on the bus:

```cpp
struct HoldDrillState {            // include/synth/MidiController.hpp
    bool held = false;
    std::vector<bool> drilled;     // one flag per encoder turn mapping
};
```

`CreateMidiControllerProfileImpl` (`src/MidiController.cpp:2778-2820`)
creates one per profile, owned by `MidiControllerProfileResult`
(`include/synth/MidiController.hpp:883-887`, new `std::unique_ptr` member),
and hands the pointer to the encoder processor (`:244-263`) and the
system-button processor (`:359-372`) through new trailing constructor
arguments. `SystemButtonMidiInProcessor::Process` (`:898-906`): an
association whose press is `HoldDrill` sets `held`, clears `drilled`, and
does not push; its release clears `held` and does not push.
`EncoderMidiInProcessor::Process` (`:695-737`): before the absolute branch
at `:698`, while `held`, a turn on mapping `ix` pushes
`ParamPush(slotIx, position)` if `drilled[ix]` is clear, sets it, and
returns; a further turn on that mapping returns without pushing. So while
the button is down the knob's value never moves and the absolute-feedback
bookkeeping (`:712-726`) is never reached; each knob turned during one hold
drills once; on release the knob is a plain knob again, and an absolute
knob jumps on its next turn, as it does today after any other change. The
push mapping of an encoder is unaffected by hold.

### 7. Catalog-driven page

`ControllersPageCallbacks` (`include/synth/ControllersPageUI.hpp:865-873`)
gains `std::vector<UISystemMessageChoice> messageCatalog` and
`std::vector<ControllerWizardDescriptor> layouts`. `MidiConfigViewModel`
gains `SetMessageCatalog(std::vector<UISystemMessageChoice>)`, defaulting
to the full library list, and the four readers of `UISystemMessageCatalog()`
(`ControllersPageUI.hpp:2518`; `src/MidiConfigViewModel.cpp:449`, `:1626`,
`:2404`) read the view model's copy. The runtime services build both
vectors from the app catalog (`runtime/JuceRuntimeMainServices.hpp:47-58`,
`include/synth/browser/BrowserRuntimeMainServices.hpp:48-59`) and the
harness supplies its own (`juce/ControllersPageHarness.hpp:75-90`). Row
identity for an app-action row is `(message, appAction, appActionValue)`,
never the index. `MakeUISystemMessageAssociation` (`:456-464`) takes the
choice, so an app-action association is created with its action, value
and index in one place.

Analog rows. The analog section shows gesture rows and the scene-blend
row (`detail::PresentationRowData`, `include/synth/MidiConfigViewModel.hpp
:330-336`; reconstruction at `src/MidiConfigViewModel.cpp:1085-1100`, field
read `:1509`, field commit `:2300`, flush `:2060`, add-row `:3110`,
duplicate-address check `:1962`); an `analogRange` action (BPM) installed
by a default would otherwise be invisible to a Custom edit. So
`AnalogAppActionMapping` joins the variant as its own row group beside
the gesture rows, with the same address fields and one target combo over
the catalog's analog actions, which reach the page as
`ControllersPageCallbacks::analogActionCatalog` from
`MakeAnalogAppActionChoices(catalog)` (the `analogRange` actions, as
`UISystemMessageChoice`s). Row identity is `(appAction, appActionValue)`.

### 8. The layout dropdown

Every controller row on the Controllers page gets a combo labelled
"Layout" whose options are the `layouts` display names followed by
"Custom". Its current value is the layout whose id equals the slot's
`wizardId` (`include/synth/MidiController.hpp:912-921`), else "Custom".

- Choosing a layout runs that descriptor's wizard `GenerateProfile` with a
  `WizardGenerationContext` (`include/synth/ControllerWizard.hpp:79-89`)
  built from the slot's name, input and output, replaces the slot's `kind`
  and `config`, sets `wizardId`, commits the instrument and saves the
  runtime config, the same commit path `SaveCommittedWizardAction` uses
  (`ControllersPageUI.hpp:1163`).
- Choosing "Custom" clears `wizardId` and changes nothing else.
- Any mapping edit committed on a slot (`Actions::kMappingFieldCommit`,
  `:2506` onward) clears `wizardId`, so the combo shows "Custom" from
  then on.
- `Actions::kControllerReconfigure` (`:1098`, `:1147-1158`,
  `:1163-1170`) is removed. `OpenExistingFromSnapshot` (`:1101-1134`)
  stays: the three-click wizard for a newly discovered device
  (`Actions::kControllerConfigure`, requirement sru-32) seeds a
  blacklisted record's form from it, so it has a live consumer after
  Reconfigure goes.

`ControllerWizardRegistry()` (`src/ControllerWizard.cpp:845-854`) stops
being a static: the runtime services own the registry vector, and it is
the app's `deviceDefaults` when the app supplies any, else the library's
Twister descriptor. Each app default becomes a `ControllerWizardDescriptor`
(`include/synth/ControllerWizard.hpp:154-161`) whose wizard has an empty
form (`Validate` true, no fields) and whose `GenerateProfile` returns the
default's `config` with the context's name and endpoints. The seven
callers (`ControllerWizardDiscoveryCache.hpp:42`, `ControllersPageUI.hpp
:1008`, `:1155`, `:1516`, `:1547`, `src/MidiConfigViewModel.cpp:705`,
`:2566`) and `MakeControllerWizard` (`:895`) take the vector. Discovery
(scw-2) keeps working through the descriptors' aliases, so plugging in a
Twister or an APC40 still offers the wizard. `DiscoverControllerWizards`
walks the registry in order and gives a device to the first descriptor
whose aliases match (`src/ControllerWizard.cpp:857-880`); the two APC40
defaults share their aliases, so the Generic one, listed first, is what
discovery offers, and the Ableton one is reached through the Layout combo.

### 9. The device defaults, control by control

All three are `MidiAppDeviceDefault`s frogg3rs registers. Channels are
0-based, as the Sheaf builders and both manufacturer documents write them.
"Slot 0" is the app's one encoder slot; positions 0-15 are the bank's 16
encoders in screen order.

**Twister: id `froggers.twister`, "MIDI Fighter Twister", kind MfTwister.**
Source: Midi Fighter Twister User Guide, Appendix 1, Bank 1; the library's
`EncoderMidiInConfig::TwisterDefault` (`src/MidiController.cpp:521`, via
`RowMajorInputDefault` `:49-58`) already encodes the encoder half.

| control | MIDI | target |
|---|---|---|
| encoder 1-16 turn | ch 0, CC 0-15, relative (3Fh/41h) | Param Inc/Dec slot 0 pos 0-15 |
| encoder 1-16 push | ch 1, CC 0-15 | Param Push slot 0 pos 0-15 (drills in, app-owned) |
| encoder LED rings | library `EncoderMidiOutConfig::TwisterDefault` | value feedback, as the library builder does |
| LH side 1 | ch 3, CC 8 | Bank Previous |
| LH side 2 | ch 3, CC 9 | Bank Next |
| LH side 3 | ch 3, CC 10 | Randomize Page |
| RH side 1 | ch 3, CC 11 | Randomize All |
| RH side 2 | ch 3, CC 12 | Reset Page |
| RH side 3 | ch 3, CC 13 | Reset All |

Not in this default (no controls left; mapped by hand): Play, Stop,
Freeze, Record, Scene 1/2, Scene blend, BPM, Hold Drill (a Twister's
encoders push, so it has no need of it). The side buttons carry no output
feedback (`outputFeedback = false`, as the builder sets at `:3069`).

Utility settings the manual states, from the guide: every encoder set to
"Enc 3FH/41H" (the factory setting is absolute CC); all six side buttons
set to "CC Hold" (127 on press, 0 on release; the factory setting has the
middle pair switching the Twister's own banks and sends nothing for them);
"Bank Side Buttons" unchecked, so the side buttons keep CC 8-13 whatever
Twister bank is lit. The library addresses exactly ch 3, CC 8 + index
(`src/MidiController.cpp:3071`).

**APC40 mkII (Generic): id `froggers.apc40.generic`, "Akai APC40 mkII
(Generic)", kind Generic.** Source: Akai APC40 Mk2 Communications Protocol
v1.2, inbound tables pp. 30-37 and the generic-mode notes p. 11. The unit
powers up in generic mode; nothing is sent to it.

| control | MIDI | target |
|---|---|---|
| track knob 1-8 (top row) | ch 0, CC 48-55 (0x30-0x37), absolute | Param Set Absolute slot 0 pos 0-7 |
| device knob 1-8 | ch 0, CC 16-23 (0x10-0x17), absolute, on the SELECTED TRACK's channel | Param Set Absolute slot 0 pos 8-15 |
| SHIFT | note 98 (0x62), momentary | Hold Drill |
| PLAY | note 91 (0x5B), momentary | Play |
| STOP | note 92 (0x5C), momentary | Stop |
| RECORD | note 93 (0x5D), momentary | Record |
| SCENE LAUNCH 1 | note 82 (0x52), momentary | Scene 1 |
| SCENE LAUNCH 2 | note 83 (0x53), momentary | Scene 2 |
| LEFT arrow | note 97 (0x61), momentary | Bank Previous |
| RIGHT arrow | note 96 (0x60), momentary | Bank Next |
| DEVICE ON/OFF | note 62 (0x3E), momentary | Randomize Page |
| DEVICE LOCK | note 63 (0x3F), momentary | Randomize All |
| CLIP/DEVICE VIEW | note 64 (0x40), momentary | Reset Page |
| DETAIL VIEW | note 65 (0x41), momentary | Reset All |
| STOP ALL CLIPS | note 81 (0x51), momentary | Freeze |
| CLIP STOP under track 1-6 | note 52 (0x34) on ch 0-5, momentary | Bank 1-6 |
| CROSSFADER | ch 0, CC 15 (0x0F) | Scene Blend |
| MASTER FADER | ch 0, CC 14 (0x0E) | BPM |

Buttons the protocol marks TOGGLE in generic mode (BANK LEFT/RIGHT, DEVICE
LEFT/RIGHT, ACTIVATOR, SOLO, RECORD ARM, PAN/SENDS/USER) are deliberately
unused: a toggle sends note-off on its second press, which the library
reads as a release, so a one-shot action would fire every other press.
Track Select buttons send no MIDI in this mode and cannot be mapped. The
eight track faders, cue level and tempo knob (relative) are unmapped.
Encoder pushes: none (the APC40 knobs have no switch). Output: none; the
unit lights its own rings and buttons in generic mode. Caveat the manual
states: the device knobs follow the Track Select buttons (track 1 = ch 0,
master = ch 8), so leave Track 1 selected; pressing another track moves
those eight knobs to another channel and they stop responding until Track
1 is pressed again. That caveat is the reason the Ableton default exists.

**APC40 mkII (Ableton): id `froggers.apc40.ableton`, "Akai APC40 mkII
(Ableton)", kind Generic.** Same table as Generic, with two differences:

1. The profile carries a connect-time message. `MidiControllerProfileConfig`
   (`include/synth/MidiController.hpp:875-881`) gains
   `std::vector<std::vector<std::uint8_t>> openSysEx`, persisted as an
   array of byte arrays; `CreateMidiControllerProfileImpl` adds an output
   processor for it that sends each message once after every `Reset()`
   (`MidiOutputProcessor`, `:393-398`; `Reset` runs when an output opens or
   reopens, `Engine.hpp:766-772`; sending goes through the same `Enqueue`
   the Launchpad colour output uses, `src/MidiController.cpp:1960-1979`).
   Exactly once, because the introduction message re-initialises the unit.
   Any device that needs a mode message on connect can use the field; it
   is not APC-specific.
2. The message is the protocol's Type 0 introduction with mode 0x41
   (pp. 10-12): `F0 47 7F 29 60 00 04 41 09 07 01 F7`, the host-version
   bytes being the ones Ableton Live 9.7.1 sends, which is the host the
   firmware ships against.

In Ableton mode the device knobs are not banked (p. 12), so all 16 knobs
stay on ch 0 whatever is pressed, and every button is momentary. The cost,
also on p. 12: every button LED is host-controlled, so the buttons stay
dark unless lit by the host. This default sends no LED feedback (the app
owns Freeze/Record state, the library has no query for it), so its buttons
are dark. Generic mode lights them itself.

**UNCONFIRMED.** No APC40 is reachable from the session that wrote this.
The bytes and the mechanism are from the protocol and from hosts that send
it (QLC+, Daslight, Live), and Sheaf's SysEx output path is the one the
Launchpad uses today, but nothing here has been seen to switch a real unit.
The operator check (task 4.3) is: choose the Ableton layout, press any
Track Select button, turn device knob 1; encoder 9 still moves. The PR
description says the same, and gives a Launchpad procedure that proves the
connect-time SysEx path on hardware the upstream author has.

**Custom.** The fourth dropdown entry. Nothing is installed; the slot's
current mappings stay and are editable row by row as today, with every
offered target from section 1 available for a Generic controller
(encoders, system messages and analogs, `src/MidiController.cpp:3181`).

### 10. frogg3rs

`FroggersApp::MidiCatalog()` in a new `app/FroggersMidiCatalog.hpp`, called
from `Froggers.hpp` (the class that owns `ui_`, `:52`), built from the
`FroggersActions` constants (`FroggersUiSurface.hpp:229-262`) and
`kFroggersBankCount` (`FroggersParameters.hpp:66`):

| action | value | label | analogRange |
|---|---|---|---|
| `kPlay` | | Play | |
| `kStop` | | Stop | |
| `kFreeze` | | Freeze | |
| `kRecord` | | Record | |
| `kRandomizeAll` | | Randomize All | |
| `kRandomizePage` | | Randomize Page | |
| `kResetAll` | | Reset All | |
| `kResetPage` | | Reset Page | |
| `kBankPrevious` | | Bank Previous | |
| `kBankNext` | | Bank Next | |
| `kBankSelect` | "0" .. "5" | Bank 1 .. Bank 6 | |
| `kSceneSelect` | "0", "1" | Scene 1, Scene 2 | |
| `kBpm` | | BPM | `kBpmMin` .. `kBpmMax` |

`libraryKinds` = Param Inc/Dec, Param Set Absolute, Param Push, Scene
Blend, Hold Drill. `encoderPressAction` = `kEncoderPress`.
`patchCarriesMappings` = true. `deviceDefaults` = the three above.

`kBpmMin` (30) and `kBpmMax` (300) are new constants beside
`FroggersActions`, replacing the two literals in the BPM slider
(`FroggersUiSurface.hpp:1490`), so the slider and the catalog share one
range; the dispatched string is parsed by the branch's own
`FroggersParseFloat` (`:2407`). No method is added to `FroggersAppCore`:
every action and the encoder press arrive through `ui_.DispatchAction`
(section 4).

`MANUAL.md` gains a MIDI section: the layout dropdown and Custom; the
Twister Utility settings above; the APC40's two layouts, the Track 1
caveat for Generic, the dark buttons for Ableton; what Hold Drill is (hold
the button, turn a knob, that knob drills in and does not move; let go and
it is a knob again); that mappings are saved with a patch and with the
runtime config.

## Switch families and other dispositions

Switches over `MessageIn::Type` or `UISystemMessage` outside tests, 19
sites, FOUND 19, CHANGE 19, each disposition for the two new kinds:

| site | AppAction | HoldDrill |
|---|---|---|
| `src/MidiController.cpp:183` MessageTypeName | "appAction" | "holdDrill" |
| `:232` ParseMessageType | parse | parse |
| `:1721` SystemMessageOutputInfo::Evaluate | `{}` (no feedback) | `{}` |
| `:2253` ToJSON(MessageIn) | type name only (the index is not persisted) | type, held |
| `:2307` FromJSON(MessageIn) | type only | type, held |
| `src/MidiConfigViewModel.cpp:17` PrimaryMessageArg | nullopt | nullopt |
| `:51` SetPrimaryMessageArg | false | false |
| `:90` UISystemMessageHasArg | false | false |
| `:131` UISystemMessageForAssociation | `AppAction` | `HoldDrill` |
| `:179` PressForUISystemMessage | `AppAction(0, choice.ix, 0)` | `HoldDrill(0, true)` |
| `:228` ReleaseForUISystemMessage | nullopt | `HoldDrill(0, false)` |
| `:505` DescribeMessage | "app action <action> <value>" | "hold drill on/off" |
| `src/MidiConfigBlocks.cpp:57` ComputeSystemMessageSortKey | arg1 = appActionIx | hasBoolValue/boolValue |
| `:992` ClassifyBlockable | default nullopt, unchanged | unchanged |
| `src/ControllerWizard.cpp:156` TwisterArgumentEnabled | false | false |
| `:341` TwisterArgument | 0 | 0 |
| `:363` TwisterMessageForAssociation | fallback; `TwisterMessageAllowed` false | same |
| `src/ParameterModulation.cpp:4077` Apply | forward to the out bus | break |
| `include/synth/Engine.hpp:977` IsRealtimeMessage | false | false |

`Engine.hpp:1047` (realtime routing) needs nothing: its `default: continue`
covers both. Five further switches (`src/MidiConfigViewModel.cpp:950`,
`:2735`; `src/MidiConfigBlocks.cpp:333`, `:1005`, `:1056`) are over
`BlockableMessage` (`include/synth/MidiConfigBlocks.hpp:151`) and are
untouched. The pairing table at `tests/viewmodel_tests.cpp:3818-3854` is
the existing drift check and gains both kinds; its loop at `:3855`
compares the catalog by index, and gains the id comparison.

New names, grepped by operand in both trees 2026-09-01/02, all 0:
`HoldDrill`, `holdDrill`, `HoldDrillState`, `appAction`, `appActionIx`,
`appActionValue`, `MidiAppCatalog`, `MidiAppAction`,
`MidiAppDeviceDefault`, `HasMidiCatalog`, `MidiCatalog`, `libraryKinds`,
`deviceDefaults`, `encoderPressAction`, `analogRange`,
`patchCarriesMappings`, `carryInstrument`, `AppEncoderPress`,
`SetAppActionOut`, `openSysEx`, `messageCatalog`,
`AnalogAppActionMapping`, `kBpmMin`, `kBpmMax`. `AppAction` has 41 substring
hits, all `MiniAppActions` and a test name; no type of that name exists.

The three baked profile builders (WrldBldr `src/MidiController.cpp:2974`,
Twister `:3059`, Launchpad `include/synth/MidiController.hpp:1004`) and
their consumers stay untouched; the library Twister wizard and its form
keep serving apps without a catalog.

## Preflight findings (first pass 2026-09-01, audited 2026-09-01/02)

- **Hygiene sweep**, directories named: Sheaf `include/synth`, `src`,
  `runtime` + `juce/ControllersPageHarness.hpp`, `tests` (the five files
  in Impact), `apps/miniapp`, `apps/braid-4`; frogg3rs `app/` top level,
  `openspec/`. Findings and dispositions:
  - Dead: `juce/ControllersPageHarness.hpp:222` `PumpJuceMessages`;
    `tests/support/SynthRig.hpp:186` `ExternalStartAt`, `:315`
    `ClockTimeAtTimestamp`, `:324` `ClearScheduledMidiEvents`. Each has
    one hit in both trees, its definition. Removed in packet 2.
    `TwisterWizardHarness::MakeCallbacks` (`:191-210`), listed as dead by
    the first pass, is not: its constructor calls it (`:135`) and
    `ControllersPageSimulationTests.cpp:891-925` uses the harness; it
    closes over the cache-driven state, not `MakeSurface`'s (`:75-90`).
    Left as is.
  - Stale citation: `include/synth/Engine.hpp:551` cites `:415/:418` for
    null checks that sit at `:424` and `:427`. Fixed in packet 2.
  - Stale citation: `FroggersUiSurface.hpp:1827` and
    `FroggersSurfaceTests.cpp:1401` cite `EncoderDraw.hpp:690-694`; the
    gated block is `:691-698`. Fixed in packet 3.
  - Stale citations: 19 references to `FroggersEngine.hpp:LINE` in
    `FroggersAppCore.hpp` (7) and `FroggersDspParityTests.cpp` (12), stale
    only against the guitar change's UNCOMMITTED engine; against `main`
    they resolve. Sequenced, not fixed here: this change commits to `main`
    and must not cite line numbers of a file that is not there yet. The
    guitar change moved the lines, so its `tasks.md` carries the re-sweep
    and its postflight lands it.
  - `FroggersDspParityTests.cpp:1664` contains the words "TEMP-BREAK" in
    prose describing how a number was measured. Not a marker; left.
  - Everything else clean: no junk, no unresolved symbols, no markers.
- **Behaviour**: the table above. **Deployed site**: as described above,
  no console errors, probe controller removed afterwards.
- **Citation audit** of the first pass: 90 checked, 78 exact, 5 off by a
  line, 7 wrong; all corrected in this text.

## Postflight notes (2026-09-02, packets 2 and 3)

- Every task had an independent postflight; three divergences were found
  and closed before the next task: the `typeOrder` comments in
  `MidiConfigBlocks` were stale after the two appended kinds; the Launchpad
  variant-select commit did not clear `wizardId`; and the patch-file LOAD
  path was not gated (an app without `patchCarriesMappings` would have
  applied a version-2 file's section). The last has its own engine test.
- `OpenExistingFromSnapshot` stays (section 8): the discovered-device
  configure flow seeds a blacklisted record's form from it.
- `tests/portable_ui_tests.cpp` pins the Controllers page's form-control
  count; the per-row Layout combo raised it by one per controller (buttons
  are not counted, so the removed Reconfigure button changed nothing). The
  pin is 63 for the fixture's twelve controllers. That binary prints
  nothing on success and stood in no executor's gate; the gate script runs
  every binary after the braid-4 deadline one, which stops the `test`
  recipe on this machine.
- `FroggersParameterModelTests.cpp`'s shared-Crunchy check pressed through
  the bus and asserted the library's selection in a bank it had switched
  with the library's bank select; with presses forwarded to the app's
  drill that selection never happens there, so the check presses the
  manager directly (its purpose is model-level).
- Gates: Sheaf green except the two carried 96 kHz deadline tests; app
  green (ten binaries, four checks); browser wasm build, package
  determinism and local smoke pass (the smoke's own server could not bind
  port 8787 because a `serve-site.mjs` started 2026-08-31 still serves the
  same `dist/site` directory, so the validator read the freshly packaged
  site through it); the desktop launcher builds and signs.

## Other active changes, enumerated

- Sheaf `rework-controllers-block-editing` (upstream author, 22 of 28
  tasks done). Owns the message-kind rows this change feeds; its D8 split
  kind from argument. Disposition: keep that shape, only replace the static
  list behind it and add the layout combo beside its rows, say so in the
  PR.
- Sheaf `bank-addressed-absolute-write`: supplied `ParamSetAbsoluteOnBank`,
  present at the pin. Its `Apply` case is the guard shape the `AppAction`
  case follows. Nothing else.
- Sheaf `fix-out-of-tree-app-gaps`, `ui-state-before-audio`,
  `browser-slider-value-readout`, `shorten-deadline-readout-window`,
  `fix-task-analyzer-plan-derived-tasks`: no MIDI surface. Nothing.
- frogg3rs `frogg3rs-guitar-and-solo-variants`: `src/` and `test/firmware/`,
  uncommitted, excluded. Executors stage nothing under those paths or
  `DAISY_MANUAL.md`.

## Impact

- Sheaf `include/synth/`: `MidiAppCatalog.hpp` (new), `AppConcepts.hpp`,
  `ParameterModulation.hpp` (kinds, field, factories, out-bus types,
  forwarding),
  `MidiController.hpp` (association fields, analog app actions, Hold Drill
  state, `openSysEx`, result member, processor constructors),
  `MidiConfigViewModel.hpp` (kinds, choice fields, view-model catalog),
  `MidiConfigBlocks.hpp` (sort-key comment), `ControllersPageUI.hpp`
  (callbacks, layout combo, Reconfigure removal, readers),
  `ControllerWizard.hpp` (registry as a value, app-default wizard),
  `ControllerWizardDiscoveryCache.hpp`, `Engine.hpp` (forwarding set-up, tick
  dispatch, catalog resolution in the rebuild, pending patch instrument;
  citation fix at `:551`), `PatchPersistence.hpp`.
- Sheaf `src/`: `MidiController.cpp`, `ParameterModulation.cpp`,
  `MidiConfigViewModel.cpp`, `MidiConfigBlocks.cpp`, `ControllerWizard.cpp`,
  `PatchPersistence.cpp`.
- Sheaf `runtime/JuceRuntimeMainServices.hpp`, `runtime/juce_build.mk`,
  `include/synth/browser/BrowserRuntimeMainServices.hpp`,
  `juce/ControllersPageHarness.hpp`.
- Sheaf `tests/`: viewmodel, controller_wizard, instrument, miniapp_system,
  patch persistence, engine; `support/SynthRig.hpp`. `apps/miniapp` and
  `apps/braid-4` unchanged, suites unchanged.
- Sheaf `openspec/`: deltas to `synth-controller-wizards` (scw-2 registry
  is app-supplied when the app supplies one), `synth-runtime-ui` (layout
  dropdown replaces Reconfigure; the offered list is the app's),
  `synth-midi-instrument` (the two kinds, app-action persistence by id
  and argument, analog app actions, `openSysEx`, Hold Drill),
  `synth-patch-persistence` (schema 2 carries `midiInstrument`).
- frogg3rs: `Froggers.hpp` (`MidiCatalog()`), `FroggersMidiCatalog.hpp`
  (new), `FroggersUiSurface.hpp` (`kBpmMin`/`kBpmMax`, the EncoderDraw
  citation), `FroggersSurfaceTests.cpp` (that citation), tests,
  `MANUAL.md` at the repo root, this delta; one task line in the guitar
  change's `tasks.md`.

## Delivery

Sheaf branch `app-midi-catalog` off `browser-audio-activation`, pushed to
the fork, one upstream PR. frogg3rs pin bump as its own commit on `main`.
Gates: `make -C External/Sheaf/projects/synth test` and the `miniapp`
target, `make -C app test`, the browser wasm build, then the operator on
the deployed site and on the desktop app with the Twister and the APC40.
Builds run `nice make -j2`. The two braid-4 96 kHz deadline tests fail on
this machine before this change (deterministic, machine-bound); a gate
report names them as carried, never as new.

The upstream PR description carries: what changed and why, in the
library's terms; the two persistence policies side by side; the
`openSysEx` field as the connect-time mechanism; the Ableton default
marked UNCONFIRMED; step-by-step testing instructions for the whole change
(add a Generic controller, map a button to an app action, switch layouts,
save and reload a patch, hold-drill with a Generic controller), for the
APC40 Ableton default (the Track Select check), and a Launchpad procedure
that proves the connect-time SysEx path with `openSysEx` set to the
Launchpad's own programmer-mode message on hardware the upstream author
has; and a note that testing by the upstream author himself is ideal,
with frogg3rs `main` being where the whole change can be seen.

## Dispatch

Sequential, one dispatch at a time, no parallel agents. Implementers on
Sonnet; the docs section and the name-table edits on Haiku; reviewers on
Sonnet, fresh context per task. One gate report per task with counts,
never logs. A postflight follows every task; the commit and the push
happen only when every postflight has passed.

# Proposal — `frogg3rs-midi-mappings-for-froggers`

**Created 2026-09-01, operator-commissioned. PLANNED ONLY — a fresh session
preflights and executes this. Upstream PR #13 is reserved for the Sheaf
half.**

## The defect, traced

Sheaf's MIDI configuration page offers mappings hardcoded to the Sheaf
sample app's control surface, none of which fit frogg3rs:

- The device default profiles are baked library functions:
  `WrldBldrDefaultProfileConfig` (`External/Sheaf/projects/synth/src/
  MidiController.cpp:2974-3010+`) and `MfTwisterDefaultProfileConfig`
  (declared `include/synth/MidiController.hpp:~985`). The WrldBldr grid is
  literally "Source-derived from TheNonagonSquiggleBoyWrldBldr.hpp"
  (comment at the system-position block, `MidiController.cpp:~3010`) — the
  SAMPLE APP's aux grid: reset / random / random-mod toggles, gesture
  selectors, its sceneBlend and 16 gesture analogs.
- The message vocabulary those profiles emit
  (`MidiController.cpp:185-255`): partly generic (`paramIncDec`,
  `paramSetAbsolute`, `paramSetAbsoluteOnBank`, `paramPush`,
  `selectParamBank`, `next/prevParamBank`, transport `start`/`continue`),
  partly sample-app-specific (`toggleRandomMod`, `toggleGestureSelect`,
  `setGestureSelect` — frogg3rs has no gestures at all, and its
  randomize/reset surface is Randomize Page/All + Reset Page/All, not the
  sample app's toggles).
- The Configuration Wizard installs these baked profiles
  (`ControllerWizard.cpp:812-839` builds
  `MfTwisterDefaultProfileConfig`; `:754-755` pins it in tests), and the
  runtime-ui spec's wizard requirement
  (`External/Sheaf/openspec/specs/synth-runtime-ui/spec.md:797`) governs
  the flow but not the CATALOG, which is the gap.

So a frogg3rs user opening the MIDI page is offered "random mod hold"-class
targets that dispatch into nothing, while the app's real controls —
Randomize All/Page, Reset All/Page, Scene 1/2, Scene blend, BPM, Freeze,
bank buttons, 16 encoders per bank — are unmappable.

## The design — app-supplied mapping catalog (the `Has`-hook precedent)

Sheaf gains an app-supplied MIDI catalog, following the exact extension
pattern already accepted upstream for `HasRestoreStartupState`
(jvictor0/Sheaf#10) and consumed through the `EXTRA_APP` registration the
sheaf-patch shell already carries:

1. A concept (working name `HasMidiMappingCatalog`, final name per
   `AppConcepts.hpp` idiom) an app type may satisfy, supplying:
   - the app's MAPPABLE TARGETS: named actions (each with a display name
     and the `MessageIn` it emits) plus the generic parameter-addressing
     space (banks × slots) the library already handles;
   - the app's own DEFAULT PROFILES per device kind (the app decides what
     a fresh WrldBldr/Twister maps to for THIS instrument).
2. The library's baked WrldBldr/Twister tables MOVE to the sample app,
   becoming its own catalog — the library keeps zero app-specific targets;
   the sample app's behavior is preserved byte-for-byte through its own
   registration (its tests prove it).
3. The MIDI configuration page and Configuration Wizard read the active
   app's catalog: targets offered = app targets, defaults installed = app
   defaults, and a "reset mappings to app defaults" action re-applies the
   catalog's profile — the operator's stated need.
4. frogg3rs registers its catalog in `app/`: generic param ops for the
   encoder grid (already covered by `paramSetAbsoluteOnBank` etc.), plus
   named targets for Randomize All/Page, Reset All/Page, Scene 1/2 select,
   Scene blend (analog), BPM (analog), Freeze, transport, and bank
   selection — every target verified against `FroggersActions`/the routing
   it dispatches into (trace at execution: a target that renders and
   dispatches into nothing is the disease this repo already knows).

## Sheaf delivery

Standalone branch off the current pin lineage, upstream PR
**jvictor0/Sheaf#13** (head `daguilarc:<branch>`), Sheaf's own openspec
gets the delta for the wizard/config-page requirement (catalog-driven, not
baked). frogg3rs pin bump as its own commit; all frogg3rs commits on
`main`. The fork carries the branch either way.

## Impact (anticipated)

- Sheaf: `MidiController.{hpp,cpp}` (profile construction becomes
  catalog-driven), `ControllerWizard.cpp` (+ its tests), `AppConcepts.hpp`
  (the hook), the sample app (receives its own tables), synth-runtime-ui
  spec delta, MidiConfig UI files as the trace demands.
- frogg3rs: the catalog registration (likely beside
  `FroggersRegistration.hpp`), tests proving every offered target
  dispatches into a real action, and a delta to
  `froggers-sheaf-runtime-app` (carried here).
- Gates: Sheaf synth gate + miniapp target (runtime shell — the MIDI
  config UI lives there), app suite, browser wasm, operator on the
  deployed MIDI page.

## For the preflighting session

Verify every file:line above (they are 2026-09-01 reads); enumerate ALL
consumers of the two DefaultProfileConfig functions and of each
sample-app-specific MessageIn kind (zeros included); check whether the
persistence of installed mappings is name- or index-addressed before
moving anything; and read `ControllersPageUI.hpp`/`MidiConfigViewModel`
for where the offered-target list is actually rendered — the catalog must
feed THAT list, not just the profile constructors.

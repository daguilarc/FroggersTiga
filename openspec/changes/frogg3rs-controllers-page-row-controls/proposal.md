# Proposal — `frogg3rs-controllers-page-row-controls`

**Created 2026-09-03. Supersedes `frogg3rs-controllers-page-name-in-the-editor`
(delivered: Sheaf c81727b9, frogg3rs 950e009). That change's operator checks
8.1-8.9 carry into task 6 here. PLANNED ONLY: nothing executes before the
operator's go.**

Sheaf paths are under `External/Sheaf/projects/synth/`. Branch
`app-midi-catalog`, base commit c81727b9. Line numbers are 2026-09-03
working-tree reads, re-verified in the preflight recorded at the end of this
document.

## Why this change exists

The delivered page puts two same-width combos captioned "Preset" seventy pixels
apart, offering different lists, beside a permanently disabled button called
"Blacklist". The operator met all three on first use. Nothing in the superseded
change's spec, tests or rendered states asserts that the two combos read
differently, because the change treated the caption rename as cosmetic.

The defects are not unrelated slips. Each is a control serving more than one
job, or a job nobody has.

## What a preset is

The configuration is mappings. No mapping names a device. A preset is a way to
speed up setting up a specific device: a baseline template, never a limit on
what a row can become or on which rows can be added. The four `custom.<kind>`
entries stay on the add row unconditionally, so adding presets never narrows
what is configurable. Everything below follows from this.

## The jobs this page exists to do

- **J1** Plug a controller in and set it up. — Available controllers, Configure.
- **J2** Set one up before plugging it in. — the add row.
- **J3** Add a second of the same controller. — the add row again.
- **J4** Change what a control does. — the expanded editor.
- **J5** Undo a mess and get the factory mapping back.
- **J6** See whether a row is still factory or has been edited.
- **J7** Decide how an APC40 should behave: whether the app puts it in Ableton
  mode on connect.
- **J8** Finish with a controller. — Delete.
- **J9** Let another application have a plugged-in controller tonight, keeping
  the mappings.

J1-J4 and J8 are served correctly today. J5, J6, J7 and J9 are the four this
change is about.

## The findings

**1. The row's preset combo offers other hardware's presets.**
`BuildLayoutOptions` (`ControllersPageUI.hpp:898-918`) iterates every descriptor
with no filter on the row's kind. frogg3rs supplies three
(`app/FroggersMidiCatalog.hpp:74, 150, 161`), so a MIDI Fighter Twister row's
combo offers "Akai APC40 mkII (Generic)" and "(Ableton)". In use: the control on
your Twister offers to turn it into an APC40. There is no job for that; a second
device is J3, the add row.

**2. The Variant menu has no state behind it.** `LaunchpadVariantIndex`
(`src/MidiConfigViewModel.cpp:3867-3891`) stores nothing. It reads an open
editor's presentation if there is one, otherwise it scans the slot's system
messages for the first association carrying a `launchpadPosition` and returns
that association's controller, otherwise it returns index 0 — Launchpad X.
`SetLaunchpadVariant` (`:3893-3969`) correspondingly records no choice: it
rewrites the controller on each existing `launchpadPosition` (`:3955-3959`). On a
row with no mappings its rewrite loop iterates zero times, so choosing Launchpad
Pro MK3 writes nothing and the menu reads Launchpad X again. A second menu
asking which Launchpad you own is asking a question the first menu should have
asked, then failing to remember the answer.

**3. "Custom" is a state on one combo and an action on the other.**
On a row it is the single entry `kLayoutCustomOptionLabel` (`:893`, `:915`),
keyed by array index, and it MEANS the row's config no longer matches its preset
— J6, a readout. On the add row it is four entries `custom.<kind>`
(`BuildAddPresetOptions`, `:972-989`), and it MEANS create an empty row of that
kind — J2. Same word, two meanings, two id schemes, one caption.

**4. Blacklist is the right capability under a wrong name, shown where it cannot
act.** It closes the open handler and releases the device while retaining the
stored refs (`src/MidiReconcile.cpp:64-86`) — that is J9, and it is real. It is
emitted for every row with a resolved preset (`:3350`) and enabled only when both
endpoints are bound (`:3354`, from `hasCompleteEndpointPair`,
`src/MidiConfigViewModel.cpp:831`), so a freshly added row shows it dead with
nothing saying why. "Blacklist" also reads as punitive and permanent for an act
that is neither.

**5. The Launchpad cannot be added in one press, and a delivered artifact says
the path it needs is testable.** frogg3rs ships descriptors for the Twister and
the two APC40 modes only (`app/FroggersMidiCatalog.hpp:74, 150, 161`), so every
Launchpad row is a Custom add with every mapping made by hand. `openSysEx` — the
field that switches a device's mode on connect — is reachable from no user
interface. It appears in exactly three files: declared at
`include/synth/MidiController.hpp:930`, serialized and parsed at
`src/MidiController.cpp:2689` and `:2754`, installed into an output processor at
`:3108-3110`, and exercised in `tests/instrument_tests.cpp`. `ControllersPageUI.hpp`,
`MidiConfigViewModel.hpp` and `src/MidiConfigViewModel.cpp` contain zero
occurrences. PR #13's description nonetheless tells the upstream author that a
Launchpad proves the `openSysEx` path by setting a profile's `openSysEx` and
reopening its output. That is possible only by hand-editing the saved runtime
configuration, which nothing documents. The check was published without
verifying that the state it asks for is reachable.

**6. `wizardId` carries two facts, and editing destroys both.** This is the
finding that decides the shape of Restore, and it was found in preflight.
`hasResolvedWizard` is `slot.wizardId.has_value()` and the id resolving to a
descriptor (`src/MidiConfigViewModel.cpp:825-829`). Every edit clears the field:
`ApplyMappingEdit` (`:2714`), `DeleteRow` (`:2934`), `AddSingle` (`:3529`),
`AddBlock` (`:3734`). The comment at `:2714` says why — "a committed mapping edit
no longer matches whatever layout generated it, so the Layout combo reads
'Custom' from here on". So one optional field carries both "which preset made
this row" and "is this row still pristine", and divergence is recorded by
destroying the provenance. Two consequences that are live defects today:

- Restore cannot be built on "has a resolved preset AND has diverged". Those
  name disjoint sets; the control would never render.
- `hasResolvedWizard` gates Configure on a blacklisted row (`:3199`) and
  Blacklist on an active one (`:3350`). Edit a mapping and both vanish. A user
  who remaps one encoder loses the ability to release that controller, and a
  blacklisted row that was ever edited cannot be reconfigured.

**7. The page simulation asserts a row for every controller after every action,
and the wizard legitimately has none.** Found in execution, proven pre-existing.
`VerifyTreeAndRenderer` (`juce/ControllersPageSimulationTests.cpp:259-278`)
requires `ControllerRow(ix)` for every controller, and `kAddRow`, after every
step. `BuildTree` returns `BuildWizardFormTree()` whenever a wizard session is
open (`ControllersPageUI.hpp:1033-1037`), and that tree emits neither. So any
successful Configure on an existing row breaks the invariant on the next check.
The invariant is wrong; the product is right. It reproduces on unmodified
c81727b9 with a different walk seed, so it is not introduced here — but finding
6's fix makes `OpenExistingFromSnapshot` succeed on edited rows, which turns a
rare walk outcome into a routine one. It is repaired here rather than deferred,
because it is a gate this change must leave green and it sits in a file this
change already touches.

## Design

**Finding 6 — provenance and divergence become separate facts.**
`wizardId` becomes provenance only: which descriptor created this row. The four
sites above stop clearing it. Divergence is computed where it is displayed, by
regenerating the row's descriptor profile through `InstallDescriptorProfile`
(`ControllersPageUI.hpp:928`, the existing single definition site) into a scratch
slot and comparing configs. The comparison is on the serialized form —
`ToJSON(JsonArena&, const MidiControllerProfileConfig&)`, declared
`include/synth/MidiController.hpp:1099`, defined `src/MidiController.cpp:2666` —
rather than on a new `operator==`, because seven structs in `MidiController.hpp`
lack one (`EncoderMidiInConfig`, `EncoderMidiOutConfig`, `AnalogMidiInConfig`,
`PolyphonicPressureMidiInConfig`, `WrldBldrSystemPosition`,
`MidiControllerSystemMessageAssociation`, `MidiControllerProfileConfig`) and
adding them spreads an equality chain across a header this change otherwise does
not touch. Serialized comparison also defines divergence as differing in what is
persisted, which is what "the same row" means across a relaunch. It is O(n) and
reused per render, so per §7 it is computed once while the row VM is built and
carried as a bool beside `hasResolvedWizard`.

No new persisted field and no migration: `wizardId` is already stored and parsed
(`src/MidiController.cpp:2878-2882`). Existing saved rows whose id was cleared by
a past edit simply offer no Restore, which is correct — their template is not
recorded. `MidiController.cpp:3498` requires blacklisted records to carry a
wizard id; preserving the id satisfies that in more cases, never fewer.

**Findings 1 and 2 — every distinct setup is its own preset, and Variant goes.**
A Launchpad X, a Launchpad Pro MK3 and a Launchpad Mini MK3 are three different
devices. You own one. Which one you own is a creation-time fact, which is what a
preset already is, so each becomes its own descriptor: `froggers.launchpad.x`,
`froggers.launchpad.promk3`, `froggers.launchpad.minimk3`.

The Variant menu is removed. It is a control with no backing state, and three
presets replace it exactly. Removing it also removes the only path that rewrites
an existing row's pads to a different model. That is the right trade: a row is a
device, and changing which device you own is J3, the add row.

The APC40 keeps its two presets, Generic and Ableton. They were never a mapping
choice a player switches between — they are two ways to set the unit up, decided
once, which is what a creation-time template is. Nothing about them moves.

**Findings 1 and 3 — the row carries no preset control at all.**
A preset is how the row got its initial mappings; after that the row is a device
bound to ports with a set of mappings, and the template is provenance. The menu
never read correctly because it offered to re-answer, permanently, a question
already answered when the row was created.

So the row's combo goes and nothing replaces it. No dropdown and no readout: the
row already names its device, and the readout's only real content is whether the
config has diverged from its template, which the presence of **Restore** already
says. Restore reinstalls that row's own template and is offered while the row has
one and differs from it. `BuildLayoutOptions` and `kLayoutCustomOptionLabel` go
with the combo, and presets are named in exactly one place on the page: the add
row, at creation.

**Finding 4 — Release, everywhere the word appears.**
"Blacklist" becomes **Release** and "Remove" on a released row becomes
**Reclaim**. The control is emitted when it will actually work rather than
emitted dead: `blacklist.enabled` goes, and the emission condition becomes the
model's own two preconditions — a resolved wizard id AND both endpoints bound.
`BlacklistController` (`src/MidiConfigViewModel.cpp:2867-2876`) refuses anything
else, so gating on endpoints alone would show a button that refuses on click,
which is finding 4's defect wearing a new name.

The `hasResolvedWizard` half of that gate is kept, not removed. Finding 6's fix
is what makes it correct: because edits no longer clear `wizardId`, the gate no
longer hides Release from an edited row, so the root cause is repaired at the
data model rather than by loosening the control's condition. The same gate on
Configure (`:3199`) stays for the same reason and starts working.

The wording is not page-local, and the preflight enumeration says where it
reaches. Nine user-visible strings change:

| file:line | string |
|---|---|
| `ControllersPageUI.hpp:3357` | `"Blacklist"` — the button |
| `ControllersPageUI.hpp:3182` | `"Blacklisted"` — the row badge |
| `ControllersPageUI.hpp:3210` | `"Remove"` — the reclaim button |
| `ControllersPageUI.hpp:1866` | `"Blacklisted controller"` — status |
| `ControllersPageUI.hpp:1876` | `"Removed controller from blacklist"` — status |
| `src/MidiConfigViewModel.cpp:2827` | `"blacklisting requires both input and output endpoint references"` |
| `src/MidiConfigViewModel.cpp:2838` | `"only registry-supported active controllers can be blacklisted"` |
| `src/MidiConfigViewModel.cpp:2850` | `"controller could not be blacklisted"` |
| `src/MidiConfigViewModel.cpp:2868` | `"only blacklisted controllers can be removed from blacklist"` |

The four viewmodel strings are refusal reasons the page renders as
"Refused: <reason>". Leaving them would put "Release" on the button and
"blacklisting requires..." in the status line, which is the split vocabulary this
change exists to remove. `src/MidiController.cpp:3498, 3501` are validation
failures for malformed saved records, reached only by a corrupt file; they keep
the stored vocabulary and are listed here as read and deliberately unchanged.

The persisted disposition token `"blacklisted"` (`src/MidiController.cpp:2814,
2824`) does not change. Records already stored stay readable, reclaimable and
unbroken. Action names `kControllerBlacklist` and `kControllerRemoveBlacklist`,
and the node ids at `:309` and `:324`, do not change either: they are wire
identifiers, not words anyone reads.

**Finding 5 — the Launchpad presets carry a real mapping and the message that
proves the path.** Each descriptor ships frogg3rs's own actions laid out on that
model's grid, plus its programmer-mode `openSysEx`. Adding a Launchpad and
connecting it switches the unit into programmer mode, and the operator sees
`openSysEx` fire. That is the check PR #13 has been asking for, on hardware the
upstream author has.

Every association a Launchpad preset installs carries a `launchpadPosition` at
that model's controller, so finding 2's derivation reads the correct model from
creation and keeps reading it after edits. This is what makes removing the
Variant menu safe: the model lives in the mappings, where the editor already
looks for it (`AddSingle` at `:3425`, `AddBlock` at `:3633`, both through
`CurrentLaunchpadVariant` at `:3162`). It is a starting point and not a
constraint — a pad's coordinate is validated against that pad's own carried
controller (`:2606`), never against a row-wide model.

The library's `LaunchpadDefaultProfileConfig`
(`include/synth/MidiController.hpp:1056`, `src/MidiController.cpp:3269`) already
generates a per-model grid and is the reason the geometry needs no new code. It
is not used wholesale: it emits library `MessageIn::SceneSelect` and
`SelectParamBank` messages, and frogg3rs drives scenes through its own app action
`froggers.scene.select` (`app/FroggersUiSurface.hpp:245`, handled at `:2279`),
which is what its APC40 descriptor maps (`app/FroggersMidiCatalog.hpp:121-122`).
Shipping the library default unchanged would give the operator a row of scene
pads wired to a mechanism this app does not drive through that path. The three
descriptors therefore use one shared frogg3rs helper differing only in model and
`openSysEx`, mapping actions frogg3rs already declares in `catalog.actions`.

The three messages, verified against Novation's programmer references and
cross-checked in-repo against `LaunchpadProductByte`
(`src/MidiController.cpp:3645-3655`, X `0x0C`, Pro MK3 `0x0E`, Mini MK3 `0x0D`)
and the header `LaunchpadColorSysex` already sends (`:3706-3711`):

| preset | device id | programmer-mode message | source |
|---|---|---|---|
| Launchpad X | `02 0C` | `F0 00 20 29 02 0C 0E 01 F7` | X programmer reference, mode select `0Eh <mode>`, mode 1 |
| Launchpad Pro MK3 | `02 0E` | `F0 00 20 29 02 0E 00 11 00 00 F7` | Pro MK3 programmer reference p.7, select layout `00h <layout> <page> 00h`, Programmer = layout `11h`, page `00h` |
| Launchpad Mini MK3 | `02 0D` | `F0 00 20 29 02 0D 0E 01 F7` | Mini MK3 programmer reference pp.7-8, mode select `0Eh <mode>`; the readback form `02h 0Dh 0Eh F7h` fixes the command byte |

The three are not one message with a changed id, which is a second reason the
Variant menu could not have worked. The X and the Mini MK3 share a dedicated
Programmer/Live switch, `0Eh <mode>` with mode 1. The Pro MK3 has no such
command: it selects Programmer Mode as a layout. Different command byte,
different length. Three presets express that directly.

Scope limit: `Ignore` on Available controllers is untouched. It answers "stop
offering me this device I have never configured", which is a different job from
J9 and already reads correctly.

## §5 forward enumeration

Counts are per file at c81727b9, from the preflight.

| concept | found | disposition |
|---|---|---|
| `BuildLayoutOptions` | page 2 (def `:898`, call `:3242`) | removed with the row combo |
| `kLayoutCustomOptionLabel` | page 2 (`:893`, `:915`) | removed with it |
| `HandleControllerLayout` | page 2 (def `:2188`, call `:1476`) | removed |
| `Actions::kControllerLayout` | page 3 (`:441`, `:1306`, `:1474`) | removed |
| `NodeIds::ControllerLayout` | page 2 (`:312`, `:3247`); tests 8 in `controllers_page_ui_tests.cpp`, 7 in `juce/ControllersPageSimulationTests.cpp`, 1 in `browser_runtime_contract_tests.cpp` | node id goes; 16 test references rewritten or removed, count reported per file in 2.6 |
| `BuildLaunchpadVariantOptions` | page 2 (def `:873`, call `:3262`) | removed |
| `LaunchpadVariantIndex` | viewmodel 2, page 1, tests 5 | removed |
| `SetLaunchpadVariant` | viewmodel 2, page 1 (`:2174`), tests 5 | removed |
| `LaunchpadVariantCatalog` | viewmodel 3, page 2, tests 2 | removed |
| `Actions::kVariantSelect` / `NodeIds::ControllerVariant` | page 4 total | removed |
| `RewritePresentationLaunchpadVariant` | viewmodel 2 | removed with `SetLaunchpadVariant`, its only caller |
| `CurrentLaunchpadVariant` | viewmodel 7 (`:3162`, `:3230`, `:3425`, `:3480`, `:3633`, `:3687`, `:3878`) | KEPT. `:3878` goes with `LaunchpadVariantIndex`; the four in `AddSingle`/`AddBlock` are the derivation the presets now feed |
| `LaunchpadShapeSupports` | viewmodel 6 | 2 removed with the variant setter (`:3265`, `:3944`); 4 kept (`:2606`, `:3184`, `:3219`, `:3221`) |
| `BuildAddPresetOptions` / `EffectiveAddPresetId` | page 2, 2 | unchanged; the add row is the one device chooser |
| `froggers.apc40.generic` / `.ableton` | catalog 1 each (`:150`, `:161`) | unchanged, both kept |
| `openSysEx` | header 1, `MidiController.cpp` 13, `instrument_tests.cpp` 7; zero in page and viewmodel | mechanism unchanged; three new writers in the frogg3rs catalog |
| `kControllerBlacklist` / `kControllerRemoveBlacklist` | page 2 | action names and node ids unchanged; 9 user-visible strings relabelled per finding 4 |
| `hasResolvedWizard` | viewmodel 1 (`:825`), page 2 (`:3199`, `:3350`) | `:3350` gate removed; `:3199` kept and now reached by edited rows |
| `wizardId` clear sites | viewmodel 6 | 4 removed (`:2714`, `:2934`, `:3529`, `:3734`); 2 go with `SetLaunchpadVariant` (`:3930`, `:3967`) |
| Restore (new) | 0 everywhere: `"Restore"` 0, `kControllerRestore` 0, `ControllerRestore` 0 | created; one action, one node id, one call site |
| Release / Reclaim (new labels) | `"Reclaim"` 0; `"Release"` 5, all the ADSR envelope parameter (`Modules.hpp:1180`, `miniapp_system_tests.cpp` 4) | no collision on this page; the envelope parameter is a different surface |
| `CurrentLaunchpadVariant(const SectionPresentation&)` | viewmodel 1 (`:3262`), callers 0 after 2.1 | dead once `LaunchpadVariantIndex` goes; deleted in 2.1b |
| `LaunchpadDefaultProfileConfig` | header 1, `MidiController.cpp` 1, callers 0 in frogg3rs | unchanged and still uncalled by frogg3rs; named in the design as the reason no geometry code is written |

## Impact

- Sheaf `projects/synth/`: `include/synth/ControllersPageUI.hpp`,
  `include/synth/MidiConfigViewModel.hpp`, `src/MidiConfigViewModel.cpp`;
  `tests/controllers_page_ui_tests.cpp`, `tests/portable_ui_tests.cpp`,
  `tests/browser_runtime_contract_tests.cpp`, `tests/viewmodel_tests.cpp`;
  `juce/ControllersPageSimulationTests.cpp`.
- Sheaf repo root: `openspec/changes/app-midi-catalog/` — sru-4, sru-60, sru-62.
- frogg3rs: `app/FroggersMidiCatalog.hpp`, `app/FroggersMidiCatalogTests.cpp`;
  `MANUAL.md` (the Controllers subsection, rewritten again); `README.md` if its
  wording moves; pin bump; this change's artifacts.

## Preflight record

Run inline 2026-09-03 against c81727b9. Citations: all resolve except `:3243`,
which task 1.1 named as the Variant combo and which is a continuation line of the
`BuildLayoutOptions` call; the Variant combo is `if (hasVariant)` at `:3254`,
gated by `hasVariant = rowVm.kind == MidiProfileKind::Launchpad` (`:3220`), and
emits at `:3260`. Corrected above.

Behavioural premises checked before execution, per §9:

- Finding 2 confirmed by reading both functions end to end rather than from the
  manual's sentence about them.
- Finding 6 found by tracing every `wizardId` write. It refutes the first draft of
  Restore and is why the design now separates the two facts.
- The `openSysEx` processor is already proven:
  `OpenSysExMidiOutProcessorSendsOnceThenWaitsForReset` (`tests/instrument_tests.cpp:1438`)
  and `...SendsMultipleMessagesInOrder` (`:1464`), with the JSON round trip at
  `:1403-1427`. Operator check 6.1b's "once and not again on the next tick" rests
  on a passing test. What is untested is the wiring at `:3108-3110`, which task
  2.6 covers.
- The three SysEx messages were read from Novation's references rather than
  recalled, and their device ids cross-check against in-repo `LaunchpadProductByte`.

§8.0 hygiene, every Impact directory named and swept: the eight Sheaf Impact
files carry zero planning-doc references (`sru-N`, `Finding N`, `Task N.N`,
`design.md`, `packet N`) — swept and clean.
`External/Sheaf/openspec/changes/app-midi-catalog/` clean. `app/` clean for this
change's files; `app/FroggersAppCore.hpp` and `app/FroggersDspParityTests.cpp`
carry stale `FroggersEngine.hpp:LINE` citations already owned and flagged by
`frogg3rs-guitar-and-solo-variants`, outside this change's Impact. The wider Sheaf
tree carries 131 such references across 30 files in directories this change does
not touch; pre-existing, not dragged in here.

Other active changes: `frogg3rs-controllers-page-name-in-the-editor` shares this
change's Impact files and is superseded by it, with disjoint findings.
`frogg3rs-controllers-page-user-story` marks itself superseded two generations
back and is dead pending archive. `frogg3rs-drilled-in-randomize-floor` and
`frogg3rs-guitar-and-solo-variants` are disjoint subsystems; the latter owns all
uncommitted work in the frogg3rs tree and does not collide.

## Delivery

One Sheaf commit on `app-midi-catalog` after the postflight, pushed to the fork;
PR #13 description updated. frogg3rs: pin bump as its own commit, then docs and
artifacts, push `main`, Pages and VST workflows green. Builds `nice make -j2`,
never more.

## Dispatch

Preflight inline, done. Implementers on Sonnet; the postflight reviewer on Sonnet
with fresh context. No fixes outside this text: a defect this text does not name
stops execution and supersedes the change.

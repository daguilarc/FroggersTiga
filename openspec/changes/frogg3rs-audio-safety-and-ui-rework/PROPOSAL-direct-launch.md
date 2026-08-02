# Proposal — launch straight into Frogg3rs, no app picker

Operator request, 2026-08-01: *"make the build launch just the frogg3rs app, not the rest of the
sheaf stuff that i have to click through to get to it."*

Written before any edit (OMNI §7/§13). Small and self-contained, so this is a proposal section
rather than a new OpenSpec change (OMNI §16.5, proportionality) — but it is a real capability
change and gets a real trace.

---

## §1 trace — what exists today

Every claim cites `file:line` read at pin `77a3019e`. Sheaf paths are relative to
`External/Sheaf/projects/synth/`.

**The picker is unconditional.** `apps/sheaf-patch/Main.cpp:35-51` builds a three-entry vector —
MiniApp (`:38`), Braid4 (`:41`), and our out-of-tree app behind `#ifdef
SHEAF_PATCH_EXTRA_APP_TYPE` (`:44-48`) — then always constructs `LauncherComponent` and shows it at
720×420 (`:50-51`). `LauncherComponent` renders a "Select an app" list (`apps/sheaf-patch/
Launcher.hpp:30`) whose row `onClick` calls `ActivateApp(appId)` (`:45-46`), which calls
`registration.launch(SheafPatchDataPathsForApp(dataRoot_, appId))` (`:127`).

**There is no existing bypass.** No command-line argument, no environment variable, no
"single registered app skips the picker" branch. `initialise(const juce::String&)`
(`Main.cpp:29`) discards its command-line parameter entirely. Verified by reading the whole of
`Main.cpp` and `Launcher.hpp`'s selection path — this is a read, not an inference.

**Launching is already a reusable one-liner.** `LaunchRegisteredApp<App>(paths)`
(`Main.cpp:87-99`) does the whole job: builds a `RuntimeSessionOwner`, reads `App::Config()` for
`appName`/`uiWidth`/`uiHeight`, and shows the session's component at that size. The picker adds
nothing to it but the click.

**The build is overridable without patching Sheaf.** `runtime/juce_build.mk` consumes `APP_NAME`
(binary + `.app` bundle name, `:25-27`), `APP_SOURCES` (the main entry point, `:149-150`), and
`APP_BUILD_DIR` (`:24`, documented at `:8` as *"must be distinct per app"*). sheaf-patch's Makefile
sets these with `:=` (`Makefile:3-6`), but a **make command-line override beats `:=`** — which is
exactly the mechanism `build-launcher.sh` already relies on for the five `EXTRA_APP_*` variables.
No `override` directive blocks it (checked).

**Data paths must not move.** The operator's runtime data root is
`~/Library/Sheaf/synth/sheaf-patch/`. Patches live under the per-app subdirectory that
`SheafPatchDataPathsForApp(dataRoot, "frogg3rs")` (`include/synth/AppRegistry.hpp:55`) derives from
the `appId` in `FroggersManifest()` (`app/FroggersRegistration.hpp:23`). Passing that same helper
with that same `appId` keeps every existing patch exactly where it is. **This is the one
correctness constraint that matters; getting it wrong silently orphans the operator's saved work.**

---

## What changes

**D1 — new `app/FroggersMain.cpp`.** Our tree, not Sheaf's. A JUCE application that does what
`SheafPatchApplication` does minus the picker: on `initialise`, resolve the data root, create the
window, and launch `FroggersApp` directly via the same `SheafPatchDataPathsForApp(dataRoot,
"frogg3rs")` call the picker would have made. Reports its name as `Frogg3rs`.

It is a near-copy of `Main.cpp`'s structure by necessity — `MainWindow` and `LaunchRegisteredApp`
are private members of Sheaf's application class and cannot be reused from outside it. That
duplication is **deliberate and bounded**: it is the price of not patching Sheaf, it is ~60 lines,
and it is the smallest thing that satisfies the request. Per OMNI §8 this is flagged, not hidden —
if Sheaf later exposes a reusable direct-launch entry point, this file collapses into it, and that
becomes upstream ask 12.

**D2 — `app/build-launcher.sh` builds Frogg3rs directly.** Add four command-line overrides to the
existing `make` invocation:
- `APP_NAME=Frogg3rs` — produces `Frogg3rs.app`, not `SheafPatch.app`
- `APP_SOURCES=$REPO_ROOT/app/FroggersMain.cpp`
- `APP_BUILD_DIR=$REPO_ROOT/app/build-launcher`
- `APP_INFO_PLIST=$REPO_ROOT/app/Frogg3rs-Info.plist` — **see the ERRATA below**

**ERRATA (2026-08-01, found by execution).** This section originally named only the first three
overrides, and that was a planning defect in this proposal, not an implementation error. The
implementer built exactly what was written, then correctly stopped and reported that the resulting
bundle was inconsistent rather than improvising a fourth override.

`juce_build.mk:152-156` copies `APP_INFO_PLIST` into `Contents/Info.plist` **verbatim, with no
templating**, while `APP_NAME` independently determines the binary's name at
`Contents/MacOS/$(APP_NAME)` (`:27`). Overriding `APP_NAME` without `APP_INFO_PLIST` therefore ships
a bundle whose `CFBundleExecutable` says `SheafPatch` while the only binary present is `Frogg3rs`.
**LaunchServices resolves the executable through the plist, so a Finder double-click fails** — which
is precisely the operator-facing action this whole proposal exists to make work.

Two properties make this defect nastier than its size suggests, and both are the reason it is
written up rather than silently patched:

1. **It hides from every automated check.** The build exits 0, the bundle exists, the binary is
   valid, and running it directly works. Only the double-click path breaks. Criterion 1 as
   originally written ("produces `Frogg3rs.app`") would have passed on a broken bundle.
2. **The two variables are coupled but neither documents the other.** `juce_build.mk:6-10`
   documents `APP_NAME` and `APP_INFO_PLIST` as independent inputs. They are not: overriding either
   alone yields an incoherent bundle. `build-launcher.sh` now carries a comment stating the
   coupling, and `app/Frogg3rs-Info.plist` carries the reciprocal one, so the trap is annotated
   from both ends. This is upstream ask 13 material — the plist wants templating from `APP_NAME`,
   or the makefile wants a guard that fails the build when they disagree.

`CFBundleIdentifier` is also changed to `dev.sheaf.Frogg3rs`, distinct from sheaf-patch's
`dev.sheaf.SheafPatch`: two installed bundles sharing one identifier let LaunchServices resolve the
wrong app.

The five `EXTRA_APP_*` variables and the header glob stay exactly as they are — the glob is load-
bearing (a hand-written list once tracked 4 of 18 headers and silently ignored every edit under
`app/dsp/`), and `FroggersMain.cpp` is a *source*, not a header, so it is tracked by `APP_SOURCES`
as a direct prerequisite of the link rule (`juce_build.mk:149`).

**Incidental fix, not scope creep.** `APP_BUILD_DIR` currently defaults to
`External/Sheaf/.../sheaf-patch/build`, so today our launcher build writes **into the submodule**.
Pointing it at `app/build-launcher` moves our artifacts into our own tree, which directly serves the
standing "`External/Sheaf` stays clean and unpatched" constraint. It also honours `juce_build.mk:8`'s
"must be distinct per app" requirement, which two apps sharing one build dir currently do not.

## What does NOT change

- **No file under `External/Sheaf` is edited.** The picker build still exists and still works by
  invoking `make -C External/Sheaf/projects/synth/apps/sheaf-patch` directly with no overrides.
- **No app behaviour, no UI, no DSP.** This changes only which entry point the binary starts at.
  §A audio safety and parameter-value randomization are untouched.
- **No data migration.** Same `appId`, same helper, same directory. Existing patches load.
- **The frozen trees** stay byte-identical.

## Success criteria

1. `./app/build-launcher.sh` produces `app/build-launcher/Frogg3rs.app` which, when opened, shows
   the Frogg3rs surface immediately — **no "Select an app" screen, no click**.
1b. **The bundle is internally coherent:** `Contents/Info.plist`'s `CFBundleExecutable` names a file
   that actually exists in `Contents/MacOS/`. Criterion 1 as first written did not pin this and
   would have passed on a bundle Finder could not launch (see D2's ERRATA). The assertion that
   catches it compares the plist key against the directory listing — not two app-side constants
   against each other, which is the shape of green-but-wrong guard this change has already hit
   three times.
2. The window opens at Frogg3rs's own `Config().uiWidth`/`uiHeight`, not the picker's 720×420.
3. An existing saved patch under `~/Library/Sheaf/synth/sheaf-patch/` still loads. **Operator
   confirms this one** — it is the failure mode a test cannot catch cheaply and the one that would
   hurt.
4. `External/Sheaf` remains clean and unpatched; `git status` in the submodule is empty.
5. The suite stays 156/156 across ten binaries.

## Interaction with the Sheaf migration

Independent. This touches the launcher entry point; Stage 2/3 touch `FroggersUiSurface.hpp`. No
shared file, so no ordering constraint — but it is executed **sequentially anyway**, per OMNI §4's
bar on parallel code changes, and it lands first because the operator asked for it first and
because it makes every subsequent Stage 2/3 walkthrough cheaper to reach.

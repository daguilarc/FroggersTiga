# Proposal — `frogg3rs-main-cutover-and-releases`

**Created 2026-08-21.** Picks up the gap the archived
`frogg3rs-external-audio-consent-repair` stated and did not close: that change
ended at the branch and said nothing concrete about how the work reaches
anyone.

## Why

Nothing built in the last several changes has shipped. The published site, the
downloadable app and the plugin all still describe or deliver a product line
that is frozen.

**The site serves the retired build.** Pages is configured at
`https://daguilarc.github.io/frogg3rs/` with `build_type: workflow`, and
`main`'s `.github/workflows/pages.yml:65-67` uploads `web/dist` — the v1 web
tree. `main` has no `app/` directory at all. The live page therefore renders
as unstyled HTML with a broken image: its assets are referenced absolutely as
`/FroggersTiga/assets/index-DsWz8uGT.css` and `/FroggersTiga/froggerstiga.png`
(`docs/index.html`), a base path that stopped existing when the repository was
renamed. The JavaScript 404s with them, so the simulator does not run.

This branch already fixes that and cannot deliver it: its
`pages.yml:160-163` uploads `app/browser/dist/site`, whose
`dist/site/index.html:59-60,152-153` references assets relatively
(`./sheaf-public/synth-browser.css`, `./site.css`, `./coi-serviceworker.js`)
and so survives any base path. The deploy job is
gated on `refs/heads/main`, so the fix is inert until the merge.

**The desktop release cannot be produced.** `desktop-release.yml` triggers only
on `froggerstiga-v*` tags, its macOS job is further gated
`if: github.ref_name == 'froggerstiga-v1'`, and it builds `./desktop/scripts/`
— the frozen v1 tree, not `app/`. The current latest release
(`froggerstiga-v1`, 2026-06-13) ships `FroggersTiga.dmg` and
`FroggersTiga-Setup.exe`, both under the retired product name.

**The plugin cannot be released at all.** `vst-plugin.yml` runs on every push
and pull request to `main` and declares `permissions: contents: read`
(`:11`). It builds and tests; it has no upload or release step and could not
publish one if it tried.

**And the app only builds on one platform.** v1 shipped macOS and Windows. The
plugin's CMake path is already cross-platform —
`app/vst/CMakeLists.txt:99-105` uses `juce_add_plugin(... FORMATS VST3 AU)`
with FetchContent'd JUCE, which selects platform sources itself. The
standalone does not use that path: `app/build-launcher.sh` drives Sheaf's
`sheaf-patch` Makefile, whose `runtime/juce_build.mk:76-85` hardcodes eight
Objective-C++ unity files (`juce_audio_devices.mm`, `juce_gui_basics.mm`, and
six more). That is what makes the standalone macOS-only.

**The obvious shortcut is wrong and this proposal rejects it.** JUCE's
`juce_add_plugin` accepts a `Standalone` format, and adding it would build on
Windows immediately. It would also wrap the AudioProcessor in JUCE's own
standalone shell: `juce_StandaloneFilterWindow.h:855` hosts the processor's
editor inside a `DocumentWindow`, and device choice comes from
`showAudioSettingsDialog()` (`:284`) backed by `AudioDeviceSelectorComponent`
(`:546`), reached from that window's own menu (`:793`, `:989`). That replaces
Sheaf's runtime shell — including the Audio page and the no-input selector the
consent repair just built. Windows and macOS would ship visibly different
applications. The port belongs in `juce_build.mk`, not in a second app shell.

## What Changes

- **NEW capability `frogg3rs-distribution`**: what the published site, the
  desktop release and the plugin release each are, and the requirement that
  every host ships from one app and one shell.
- **External/Sheaf**: `runtime/juce_build.mk` selects JUCE's platform-neutral
  unity sources per platform instead of hardcoding the Objective-C++ set, so
  the runtime shell builds on Windows. This benefits every Sheaf app.
- **`.github/workflows/desktop-release.yml`**: triggers on `frogg3rs_v2`,
  builds `app/`, produces a macOS `.dmg` and a Windows installer.
- **`.github/workflows/vst-plugin.yml`**: gains `contents: write` and publishes
  VST3 and AU on `frogg3rs_vst`.
- **`app/browser/site/index.html:65-67`**: desktop and plugin downloads side by side,
  with `app/browser/e2e/link-roles.spec.mjs` asserting both.
- **Hygiene**: `docs/` is deleted. No workflow or Makefile references it, and
  `main` deploys `web/dist` rather than `docs/`, so it is dead by two
  independent routes.

## Impact

- Affected specs: `frogg3rs-distribution` (new), `froggers-browser-package`.
- Upstream: the `juce_build.mk` port lands in Sheaf and is pushed before the
  superproject pin, as every Sheaf-touching change here does.
- Ordering is load-bearing: the Windows port gates the desktop release, and the
  merge to `main` gates the site and both releases, because every publishing
  job is `refs/heads/main`-gated.
- The operator performs the merge and pushes the tags. This change makes those
  actions produce the intended artifacts; it does not perform them.

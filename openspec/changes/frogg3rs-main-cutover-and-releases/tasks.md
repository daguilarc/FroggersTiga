# Tasks — `frogg3rs-main-cutover-and-releases`

Hygiene is step zero (§13.0). This change's sweep found `docs/` dead by two
independent routes, so it is deleted here rather than noted for later.

Gates: `cd app && nice make -j2 test` (301/301); plugin targets from
`app/vst/build` — `FroggersVstHostTests` 46/46, smoke 1/1, editor 3/3; Sheaf
`nice make -C projects/synth -j2 test` (923 passed / 2 failed, the two known
braid-4 96kHz deadline tests, NEVER a zero exit code — the recipe aborts there
and sixteen later binaries do not run, so anything after braid-4 must be run
directly); and `nice make -C projects/synth/apps/miniapp -j2 test`, the only
target that builds the JUCE runtime shell. Never above `-j2`, always `nice`.

## 0. Hygiene

- [ ] 0.1 Delete `docs/`. Nothing in `.github/workflows/` or any Makefile
      references it, and `main`'s Pages workflow uploads `web/dist`
      (`pages.yml:65-67`), not `docs/`. Confirm both again at execution time
      by searching for the INVOCATION, by bare name as well as by path, before
      deleting anything.
- [ ] 0.2 Report what else the sweep of the release surface turns up —
      `desktop/scripts/verify-release-metadata.sh` and
      `verify-tag-version.sh` are invoked only by the v1-gated job and may be
      dead once that job stops building `desktop/`. Trace their consumers
      before removing; a cleanup that breaks a shipping path is an outage.

## 1. Sheaf: the runtime shell builds on Windows

This gates the desktop release. Nothing in group 2 can ship without it.

- [ ] 1.1 Trace and report, before changing anything:
      `External/Sheaf/projects/synth/runtime/juce_build.mk:76-85` lists eight
      Objective-C++ unity sources. State for each whether JUCE ships a
      platform-neutral `.cpp` counterpart, and what else in that makefile
      (frameworks, link flags, tool selection) is macOS-specific.
- [ ] 1.2 Select JUCE's unity sources per platform rather than hardcoding the
      Objective-C++ set, keeping macOS byte-identical in what it compiles.
      Do NOT add a `Standalone` format to `app/vst/CMakeLists.txt` as a
      shortcut — `juce_StandaloneFilterWindow.h:855,284,546` shows it hosts
      the processor editor in JUCE's own window with JUCE's
      `AudioDeviceSelectorComponent`, in place of Sheaf's runtime shell, so Windows and macOS would be different
      applications and the no-input selector would not exist on Windows.
- [ ] 1.3 The gate must actually run on Windows. Report what it takes to build
      the miniapp runtime shell tests on `windows-latest`, and wire it, or
      state plainly that Windows ships unverified and why that is acceptable.
- [ ] 1.4 macOS is unchanged: same gates, same counts, reported.

## 2. The desktop release builds the app that exists

- [ ] 2.1 `desktop-release.yml` triggers on `froggerstiga-v*` and its macOS job
      is gated `if: github.ref_name == 'froggerstiga-v1'`. Retarget the trigger
      to `frogg3rs_v2` and remove the v1 ref gate. Verify the tag glob matches
      the tag exactly — a mismatched glob is a tag that silently does nothing.
- [ ] 2.2 Build `app/`, not `desktop/`. The macOS artifact is a `.dmg`; the
      Windows artifact matches whatever 1.2 produces. Both carry the current
      product name, not the retired one.
- [ ] 2.3 Retire the v1 release once v2 is downloadable, not before. The
      existing `froggerstiga-v1` release is currently marked Latest and ships
      `FroggersTiga.dmg` and `FroggersTiga-Setup.exe` from the frozen tree
      under the retired name; the `desktop-v1.0.4` draft goes with it. It is
      load-bearing until `frogg3rs_v2` publishes — it is the only download
      that exists — so it is retired at that cutover point and not earlier,
      or there is a window with nothing to download at all.
      Sequence: publish `frogg3rs_v2`, confirm both platform artifacts
      actually download and run, then retire v1. Removing a published release
      is destructive and outward-facing: the operator does it, this change
      only states when and why.

## 3. The plugin can be released

- [ ] 3.1 `vst-plugin.yml` declares `permissions: contents: read` (`:11`) and
      has no publish step. Add a release job triggered on `frogg3rs_vst` with
      `contents: write`, publishing VST3 and AU from
      `app/vst/build/FroggersVst_artefacts/`.
- [ ] 3.2 Keep the existing build-and-test job running on push and pull
      request unchanged — releasing must not cost the per-commit check.
- [ ] 3.3 AU is macOS-only. State what the plugin release contains per
      platform rather than letting a missing artifact read as a failure.

## 4. The site says what it ships

- [ ] 4.1 `app/browser/site/index.html:65-67` carries one link, "Download the
      desktop app", pointing at `releases/latest`. Make it two, side by side:
      desktop app and audio plugin. `releases/latest` resolves to whichever
      release was published most recently, so both links point at their own
      tag rather than at `latest`.
- [ ] 4.2 Update `app/browser/e2e/link-roles.spec.mjs` to assert both links,
      and report whether any other e2e or catalog check pins the old single
      link.
- [ ] 4.3 Confirm the built site still uses relative asset paths
      (`dist/site/index.html:59-60` for stylesheets and `:152-153` for scripts
      today) so the page survives its base path.
      This is what the live site gets wrong.

## 5. Verify against the published result, not the workflow file

- [ ] 5.1 A workflow that parses is not a workflow that ships. Before the
      operator merges, dry-run what can be dry-run (`workflow_dispatch` on the
      branch runs `pages.yml`'s build-and-test job without deploying, per its
      own main-gated deploy job) and report what could not be exercised.
- [ ] 5.2 State explicitly which steps remain unverifiable until `main` has the
      change, and therefore what the operator is being asked to trust.

## 6. Close

- [ ] 6.1 All gates green, counts reported, including the miniapp target.
- [ ] 6.2 Postflight: implementation versus proposal, plus a duplication pass
      over the whole diff for every new named concept.
- [ ] 6.3 Push Sheaf before the superproject pin.
- [ ] 6.4 OPERATOR: merge to `main`, then push `frogg3rs_v2` and
      `frogg3rs_vst`. Confirm the site renders styled and named correctly, and
      that both downloads resolve.

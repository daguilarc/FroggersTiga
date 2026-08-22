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

## 5b. Carried from `frogg3rs-automation-view-and-musical-ranges`

That change's implementation is complete and operator-tested; its group 10 was
the cutover and never ran. It is carried here verbatim rather than restated, so
nothing traced on 2026-08-20 is lost to a rewrite. Two notes where it and this
change meet:

- 10.2/10.3 already decided the tag and asset spelling. Group 2 above follows
  them; where the two disagree, these govern.
- 10.5's "four generated mirrors under `docs/`" and task 0.1's `docs/` deletion
  are the same work. Do it once, from 10.5's traced consumer list.

Every "traced 2026-08-20" list is re-verified before anything is deleted — a
consumer added since would make it wrong, and 10.5 says so itself.

- [ ] 10.0 BEFORE ANY OTHER ITEM IN THIS GROUP: declare the spec deltas this
      group needs. Retiring `SIM_MANUAL.md` touches three capabilities that
      name it — `sim-operator-doc-parity`, `froggers-host-master`,
      `global-strip-marbles-label` — and this change declares deltas for
      neither of them today. Deltas are what preflight validates, so they are
      written before the group runs, not during it.
      `sim-operator-doc-parity` RETIRES rather than being rewritten — traced,
      not assumed. It exists to hold one manual and four generated mirrors in
      sync, and after the merge nothing reads a mirror: the current app links
      `MANUAL.md` on GitHub directly (`app/browser/site/index.html`), the
      browser build copies no markdown at all, and `pages.yml` already
      publishes `app/browser/dist/site` instead of the legacy web tree. Its
      three remaining consumers — the v1 site's help modal, the built v1
      Pages site, and the frozen desktop app's embedded Help — all go with
      this group. One document with no copies has no parity to keep.
- [ ] 10.1 Merge v2 into main.
- [ ] 10.2 DECIDED — the new desktop app is `frogg3rs_v2`, and the old
      naming convention is dropped rather than carried forward. Assets become
      `frogg3rs.dmg` / `frogg3rs-Setup.exe` (confirm the exact Windows form
      when building). The filenames come from the build, not from GitHub, so
      there is no rename step as such: the product name in the packaging
      produces them, the workflow uploads those exact paths, and
      `web/index.html`'s two download links must change in the SAME commit or
      both downloads 404 the moment the release is replaced. Trace all four
      before touching any.
- [ ] 10.3 DECIDED — the release tag is `frogg3rs_v2`, replacing
      `froggerstiga-v1`. Everything carries the one spelling: app
      `frogg3rs_v2`, tag `frogg3rs_v2`, assets `frogg3rs.*`. Amend
      `AGENTS.md` in the same commit — it names `froggerstiga-v1` as the one
      permitted desktop channel and forbids creating other tags, so the rule
      moves with the tag or the next release violates it.
- [ ] 10.4 Move the release-notes source and the release-metadata version
      heading off `SIM_MANUAL.md` and onto `MANUAL.md`
      (`desktop/scripts/render-release-notes.sh`,
      `desktop/scripts/verify-release-metadata.sh`).
- [ ] 10.5 Tear down the mirror apparatus wholesale rather than re-pointing
      it, since it has no consumer left: `scripts/sync-help-docs.sh`,
      `sim/check_operator_docs_sync.sh`, the re-sync step in
      `scripts/hooks/pre-commit`, its invocation in `.github/workflows/
      pages.yml`, the four generated mirrors under `docs/` and
      `web/public/`, and the two CMake resource embeds. `QUICK_DICT.md`
      itself STAYS — it is the terse counterpart to `MANUAL.md`; only its
      copies go. Then delete `SIM_MANUAL.md`, against the deltas from 10.0.
      Verify no remaining reader before deleting each: this list was traced
      2026-08-20 and a consumer added since would make it wrong.
- [ ] 10.6 Retire the frozen trees the merge makes redundant — `desktop-v2/`
      alone is 165 tracked files with no consumer. Trace each tree's
      consumers first and report found versus changed; "frozen" is not
      "removable" until nothing builds from it.
- [ ] 10.7 Fix `sim/Fuegoize.hpp`'s divide-by-zero at full fuego (design G):
      move the cast off the divisor so it matches the firmware's form, and
      add a test that drives fuego to maximum. Nothing exercises that path
      today, so it needs its own coverage.
- [ ] 10.8 Update the desktop and wasm trees as part of the merge, per
      design G. Re-run group 0's sweep afterward: a merge that opens frozen
      trees is exactly when new orphans appear.

## 6. Close

- [ ] 6.1 All gates green, counts reported, including the miniapp target.
- [ ] 6.2 Postflight: implementation versus proposal, plus a duplication pass
      over the whole diff for every new named concept.
- [ ] 6.3 Push Sheaf before the superproject pin.
- [ ] 6.4 OPERATOR: merge to `main`, then push `frogg3rs_v2` and
      `frogg3rs_vst`. Confirm the site renders styled and named correctly, and
      that both downloads resolve.

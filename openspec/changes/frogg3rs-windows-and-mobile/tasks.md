# Tasks — `frogg3rs-windows-and-mobile`

Supersedes `frogg3rs-desktop-reaches-downloaders` (sections 0, 1 and 2 of that
change are delivered and committed; its Windows section, site logo and closing
sweep move here) and absorbs `frogg3rs-web-mobile-ux` (its section 1 was
already executed — see 4.1/4.2 below, carried with their evidence intact).

Gates: `cd app && nice make -j2 test` (301/301 at `de64f44`);
`cmake -S test/firmware -B test/firmware/build && ctest` (3/3);
`app/vst` ctest (3/3) — this change edits a header the plugin compiles, so the
plugin suite is a gate here, not a formality; the Daisy firmware ARM build
(`nice make -C src/FroggersTiga -j2`, `dec=278412` unchanged); the browser e2e
suite (`app/browser/e2e`); and the Windows job itself. Never above `-j2`,
always `nice`.

**Every Windows fix below is written from the audit in the proposal, not
discovered by a CI round-trip.** If a Windows run fails on something the audit
did not name, that is the audit being wrong — record what it missed and why
before fixing it, so the next platform port does not repeat the discovery.

## 0. Hygiene

- [ ] 0.1 Sweep `app/standalone/` and the Windows half of
      `desktop-release.yml`, both written by the predecessor and neither ever
      run to completion. Report anything the CMake target carries because
      `app/vst/CMakeLists.txt` carries it rather than because this target
      needs it — the `nice`/`-j2` wrapper in particular is a macOS
      machine-specific cap with no meaning on a runner.
- [ ] 0.2 `app/FroggersMain.cpp:80,247` and `app/FroggersBundledDocs.hpp`'s
      header comment both describe the macOS main menu as THE mechanism. Once
      it is one of two, those comments describe half the code. Fix them with
      the change, not after it.
- [ ] 0.3 Sweep `app/browser/site/` and `app/browser/e2e/`. Establish that
      every e2e spec is reachable from a Playwright project in the config — by
      project name as well as by file glob. A spec no project selects is a
      test that cannot fail, and this change adds three more assertions to
      that suite. Report found versus run.

## 1. The app builds and runs on Windows

- [ ] 1.1 Attach the Help menu per platform in `app/FroggersMain.cpp`:
      `setMacMainMenu` under `#if JUCE_MAC` (the existing call, byte for
      byte), `window_->setMenuBar(&helpMenu_)` otherwise
      (`juce_DocumentWindow.h:169`). Detach symmetrically in `shutdown()`,
      before `window_.reset()` — the existing ordering comment at `:247`
      explains why that order matters and stays true for both branches.
- [ ] 1.2 Locate the documents per platform in
      `app/FroggersBundledDocs.hpp`: `Contents/Resources` under
      `#if JUCE_MAC` (the existing expression unchanged), the executable's own
      directory otherwise.
      THIS HEADER IS COMPILED INTO THE SHIPPING macOS VST3 AND AU EDITORS
      (`app/vst/FroggersPluginEditor.{hpp,cpp}`). The macOS branch must be the
      same code it is today, and 3.1 verifies that against the plugin suite
      rather than assuming it.
- [ ] 1.3 Copy `MANUAL.md` and `QUICK_DICT.md` in
      `app/standalone/CMakeLists.txt` to wherever 1.2 looks on the platform
      being built, from the repository's single copy. No second checked-in
      copy. On macOS this target is not the shipping path, so the copy exists
      there to keep both branches testable, not to ship.
- [ ] 1.4 The Windows job builds. Report what it takes. If it fails on
      something outside the audit's list, record the gap in this file before
      fixing it.

## 2. The Windows build is signed, or says why not

- [ ] 2.1 `codesign` and `spctl` are macOS tools with no Windows equivalent
      this project can use: Authenticode signing needs a certificate that does
      not exist here, exactly as notarization does not on macOS. State that as
      the Windows counterpart of the predecessor's section 1 rather than
      leaving the asymmetry unexplained, and say what a Windows downloader
      sees instead — SmartScreen's unrecognised-app prompt — in the same
      `MANUAL.md` section that already describes the macOS step.
- [ ] 2.2 The macOS signing gate does not regress: `app/build-launcher.sh` and
      `app/vst/CMakeLists.txt` still sign and verify, and the `if(APPLE)`
      guard in `app/standalone/CMakeLists.txt` still means the Windows target
      never invokes a tool it does not have.

## 3. Nothing else moved

- [ ] 3.1 The plugin is unaffected by 1.2. Build `app/vst` and run its ctest
      (3/3), and confirm the macOS document path is byte-identical by reading
      the preprocessed macOS branch, not by reasoning about the `#if`.
      POSITIVE CONTROL: the assertion must be able to fail — show the check
      catching a deliberately wrong macOS path before trusting it green.
- [ ] 3.2 The macOS standalone is unaffected: `app/build-launcher.sh` builds,
      signs, verifies, and the bundle's file inventory is the same six paths
      as at `de64f44`. App suite 301/301, firmware target 3/3, Daisy ARM build
      `dec=278412`.

## 4. Mobile touch/drag

- [x] 4.1 Add `touch-action: none` to Draw nodes in
      `app/browser/site/site.css`, scoped to
      `#synth-root [data-synth-node-kind="draw"]`. Do not set it on container
      rows or on the whole mount, or page scrolling by touch on the surface
      stops working.
      (Executed under `frogg3rs-web-mobile-ux` before it was absorbed; the
      edit is in the working tree. `data-synth-node-kind` is correct —
      `ui.ts:106` writes `element.dataset.synthNodeKind` and `kindAttribute`
      (`:519`) kebab-cases `NodeKind.Draw` to exactly `draw`.)
- [x] 4.2 Verify with a real CDP touch sequence that a drag on an encoder
      reaches `pointerup` without `pointercancel`. Manual verification passed
      against a local serve of the edited CSS: events were `pointerdown,
      pointermove ×8, pointerup, lostpointercapture` — `pointercancel` never
      fired, and `lostpointercapture` fired only AFTER `pointerup`, which is
      the normal end of a captured gesture rather than the browser taking it
      back.
      NOT YET GUARDED: a one-off manual observation, so nothing in the suite
      fails if the rule regresses. 7.2 carries the durable assertion and this
      change does not close until it exists.

## 5. Viewport signal and mobile topology

- [ ] 5.1 Add `FroggersActions::kViewportNarrow = "froggers.viewport.narrow"`
      to the `FroggersActions` namespace in `FroggersUiSurface.hpp`, alongside
      the existing action constants.
- [ ] 5.2 Add `bool narrowViewport_ = false;` to `FroggersUiSurface`'s private
      members alongside `pluginHostMode_`, and a `SetNarrowViewport(bool)`
      setter for testability, following the `SetPluginHostMode` pattern.
- [ ] 5.3 Add a `HandleAction` branch for `kViewportNarrow` setting
      `narrowViewport_ = (action.value == "1")`. It does NOT push a message to
      the audio thread — it is a UI-only flag, like `pluginHostMode_`.
- [ ] 5.4 In `mobile-stack.mjs`'s patched `renderFrame`, dispatch the viewport
      action via `this.dispatchBrowserAction`. Dispatch only when the value
      CHANGES from the last dispatched value, tracked in a module-level
      variable, so the wasm app is not flooded every frame.
- [ ] 5.5 Add `FroggersCellMap::kRightRowsNarrow` — a second
      `constexpr std::array<RightRow, 8>` ordered BankTabs, Header, Randomize,
      Reset, EncoderRow 0, 4, 8, 12. Keep `kRightRows` unchanged.
- [ ] 5.6 In `AppendRightBlock`, select the table:
      `narrowViewport_ ? kRightRowsNarrow : kRightRows`. The method is `const`
      and reads the member — the same pattern `pluginHostMode_` already uses.
- [ ] 5.7 Run the app suite. `reset_row_sits_below_randomize_with_two_equal_halves`
      and `modulation_header_sits_below_bank_row_and_above_parameter_cells`
      both still pass in the wide default. Report any other surface test that
      assumed absolute row order.
- [ ] 5.8 Add a surface test `randomize_reset_above_encoders_in_narrow_viewport`
      calling `SetNarrowViewport(true)`, following the existing
      `reset_row_sits_below_randomize_with_two_equal_halves` pattern.

## 6. Site header logo

- [ ] 6.1 Source it from `app/Resources/Icon.png` (800x800, 313 KB) rather
      than adding a second logo file. That is NOT the file the bundle carries:
      `build-launcher.sh:63` copies `Icon.icns`, its sibling, and nothing in
      the tree derives either from the other — no `iconutil` or `sips` step
      exists. Establish which is the source before calling them one image, or
      say plainly that they are two files kept in step by hand. Either way the
      header needs a web-sized copy, so say where the derivation happens: a
      build step keeps one source, a committed derivative is simpler but is a
      second copy that can drift. Pick one and record why.
- [ ] 6.2 Placement is constrained, not free. `stageSiteShell`
      (`app/browser/package-catalog.mjs`) copies every FILE in
      `app/browser/site/` flat into the staged site, so an image placed there
      ships with no pipeline change.
- [ ] 6.3 The logo renders INSIDE `.site-header`. `blank-frame.spec.mjs:33-36`
      takes that element's bounding box and samples only the band beneath it,
      deliberately excluding header chrome so the title's own colour cannot be
      mistaken for a rendering app surface. A logo outside the header leaks
      colour into the sampled band and lets that guard pass over a blank app.
      Confirm the guard still fails on a blank surface after the change — it
      is the test that already missed one blank deployment.
- [ ] 6.4 Style it against the existing header: `.site-header` is centred with
      `padding: 12px 16px 0`, `.site-title` is 15px/1.2 (`site.css:28-38`).
      The logo sits inline beside the title at a matching size, and the header
      stays legible at the mobile viewport the `mobile` e2e project uses —
      including under section 5's stacked layout.
- [ ] 6.5 Assert it actually loads. A header `<img>` whose file did not ship
      renders as a broken-image icon and every existing test still passes.
      The assertion checks the image resolved (natural dimensions non-zero),
      not merely that the element exists.

## 7. Ship and close

- [ ] 7.1 The desktop release ships both platforms, and `MANUAL.md` stops
      saying the Windows build is in progress — the `frogg3rs-distribution`
      delta's own scenario.
- [ ] 7.2 ONE republish: rebuild the wasm package
      (`app/browser/Makefile` / `build-browser.sh`) and stage the catalog,
      carrying section 5's surface change and section 6's logo together. The
      browser e2e `mobile` project must include the touch-drag regression from
      4.2, the placement assertion for section 5, and 6.5's load assertion.
- [ ] 7.3 All gates green with counts, a duplication pass over the whole diff
      for every new named concept, and a check that no surviving script,
      workflow, spec or manifest names a path that no longer resolves.
- [ ] 7.4 OPERATOR: download the published Windows artifact on a real Windows
      machine and confirm it opens, recording what SmartScreen shows. No CI
      check substitutes for this.
- [ ] 7.5 OPERATOR: spot-check the live site on a real phone — drag an
      encoder, confirm Randomize/Reset are near the top, confirm the page
      still scrolls on the header, footer and gaps.

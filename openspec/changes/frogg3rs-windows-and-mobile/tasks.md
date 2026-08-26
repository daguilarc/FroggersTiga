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

Preflight already found it wrong once. The audit called its three compilation
gaps "the complete set", having read attempt 7's stop as a whole-toolchain
result when it was a compile-stage result; the link stage was never traced.
1.5 carries what that trace found. The lesson is the audit's own: a stage that
has never run is not covered by an observation of the stage before it.

## 0. Hygiene

- [x] 0.1 Sweep `app/standalone/` and the Windows half of
      `desktop-release.yml`, both written by the predecessor and neither ever
      run to completion. Report anything the CMake target carries because
      `app/vst/CMakeLists.txt` carries it rather than because this target
      needs it — the `nice`/`-j2` wrapper in particular (`:21`, `:49-53`,
      carried from `app/vst/CMakeLists.txt`) is a macOS machine-specific cap
      with no meaning on a runner, where `find_program(nice)` fails and the
      `-j2` survives anyway.
      Two defects preflight already found in that file, fixed here:
      `:62-63` says `ICON_BIG` takes "the same PNG build-launcher.sh's icon
      derives from (see that script's own header comment on Icon.png vs
      Icon.icns)". `app/build-launcher.sh` never mentions `Icon.png` at all,
      and no `iconutil`, `sips` or `magick` step exists anywhere in the tree —
      the comment asserts a derivation that does not exist and cites a comment
      that does not exist. 6.1 decides what replaces it.
      `:64-66` claims `juce_add_gui_app` converts the PNG per platform. That
      is the right mechanism (`JUCEUtils.cmake:635`, `_juce_generate_icon`),
      but nothing in this repo has ever run it: `app/vst` passes no
      `ICON_BIG` and its own comment at `:120-121` says so. Mark it a first
      attempt rather than a settled fact.
- [x] 0.2 `app/FroggersMain.cpp:80,247` describe the macOS main menu as THE
      mechanism, and `app/FroggersBundledDocs.hpp`'s header comment (`:7-17`)
      describes the macOS BUNDLE layout as THE mechanism — `dladdr`,
      `juce_Files_mac.mm`, walking up from `Contents/MacOS`. Two different
      macOS assumptions, not one. Once each is one of two branches, both
      comments describe half the code. Fix them with the change, not after it.
- [x] 0.3 Sweep `app/browser/site/` and `app/browser/e2e/`. The spec
      reachability question is already answered by preflight and needs no
      re-running: `playwright.config.mjs:38-40` defines `MOBILE_SPECS`,
      `DESKTOP_SPECS` and `PAGES_SPECS` as explicit regex allow-lists; five
      specs exist (`blank-frame`, `desktop-layout`, `link-roles`,
      `mobile-stacking`, `visibility`) and all five are selected by at least
      one project. FOUND 5, SELECTED 5, none orphaned. What matters forward is
      that `testMatch` is an allow-list, so a NEW spec file is silently unrun
      until it is added to one of those three constants — see 5.8, 6.5 and
      7.2, which each name their file and its registration.
      Stale citations to fix in the two files this change edits, all of the
      same class the predecessor's own 0.1 fixed in `build-launcher.sh` —
      line numbers into a moving pin, and paths into a retired tree:
      `mobile-stack.mjs:32-33` cites `web/src/main.ts:300` and
      `web/src/style.css:321,507` for the 720px breakpoint; `web/` no longer
      exists. `playwright.config.mjs:2,19,22,74` cite
      `web/playwright.config.ts` and `web/e2e/helpers.ts`; same.
      `mobile-stack.mjs` also cites `ui.ts:41` for `renderFrame` (actually
      `:63`), `ui.ts:94` for the transform clear (actually `:116`) and
      `ui.ts:308-319` for `fitSurface` (actually `:420`). Cite the symbol by
      name, not the line: these point into a submodule and drift on every pin
      move.
- [x] 0.4 Archive `openspec/changes/frogg3rs-desktop-reaches-downloaders`.
      It is superseded but still an ACTIVE change with eleven open tasks
      (3.4, 3.5, 3b.1-3b.5, 4.1-4.4) that duplicate this change's 1.4, 6.x and
      7.x — two changes planning the same work. Archiving is not bookkeeping:
      its delta ADDs two requirements to `frogg3rs-distribution` — "A
      downloadable build carries a signature matching its contents" and "A
      release states what opening it requires" — which its sections 1 and 2
      DELIVERED in code, and which the live
      `openspec/specs/frogg3rs-distribution/spec.md` has never contained.
      Superseding it without archiving it drops delivered requirements.
      Archive it BEFORE this change's own delta is applied, so 2.1's
      modification has a requirement to modify. Its open operator tasks 4.3
      and 4.4 are carried to 7.6 and 7.7 here, not dropped.

## 1. The app builds and runs on Windows

- [x] 1.1 Attach the Help menu per platform in `app/FroggersMain.cpp`:
      `setMacMainMenu` under `#if JUCE_MAC` (the existing call, byte for
      byte), `window_->setMenuBar(&helpMenu_)` otherwise
      (`juce_DocumentWindow.h:169`). Detach symmetrically in `shutdown()`,
      before `window_.reset()` — the existing ordering comment at `:247`
      explains why that order matters and stays true for both branches.
- [x] 1.2 Locate the documents per platform in
      `app/FroggersBundledDocs.hpp:24-25` (`:26` is `startAsProcess` and does
      not move): `Contents/Resources` under
      `#if JUCE_MAC` (the existing expression unchanged), the executable's own
      directory otherwise.
      THIS HEADER IS COMPILED INTO THE SHIPPING macOS VST3 AND AU EDITORS
      (`app/vst/FroggersPluginEditor.{hpp,cpp}`). The macOS branch must be the
      same code it is today, and 3.1 verifies that against the plugin suite
      rather than assuming it.
- [x] 1.3 Copy `MANUAL.md` and `QUICK_DICT.md` in
      `app/standalone/CMakeLists.txt` to wherever 1.2 looks on the platform
      being built, from the repository's single copy. No second checked-in
      copy. On macOS this target is not the shipping path, so the copy exists
      there to keep both branches testable, not to ship.
- [x] 1.4 The Windows job builds. Report what it takes. If it fails on
      something outside the audit's list, record the gap in this file before
      fixing it.
      **RUN 33003588859 (dispatch, 2026-08-26).** `build-macos` success,
      `build-windows` failure, `release` skipped — the coupled `needs` behaved
      as intended. Every compile fix in 1.1-1.3 held: JUCE, all twelve Sheaf
      sources and this repository's own code compiled. It failed at the LINK,
      which is where 1.5 said it would.
      38 x `LNK2019 unresolved external symbol`, every one of them a Sheaf
      symbol from `libsynth.a` — `MidiConfigViewModel::*`, `MidiDevicePoller`,
      `ControllerWizard`, `EncoderModeCatalog`, `FieldIsInteger`.
      NOT a missing source list, which was the first reading and was wrong:
      `ar t` shows the archive holds all twelve objects, `MidiConfigViewModel.o`
      included, and Sheaf's `Makefile:37` builds exactly `src/*.cpp`. The
      symbols are present and the linker still cannot use them.
      The cause is the toolchain seam, read off the runner rather than
      inferred: the libsynth step emitted `-Wmissing-field-initializers` and
      `-Wswitch` warnings, which are GCC/Clang diagnostics. `make` built that
      archive with the Makefile's `CXX ?= clang++` and its GCC-style flags,
      outside CMake's compiler selection, and MSVC's linker cannot resolve
      MSVC-mangled references against objects that driver produced.
      Only `app/FroggersMain.cpp` needs those symbols — it hosts Sheaf's
      runtime shell. `app/vst` links the same archive on macOS and never hits
      this because it compiles two files plus `HostDataPaths.cpp` and never
      references the runtime shell, so it is not the working analogue it
      looks like.
- [x] 1.5 FIRST ATTEMPT — the link stage. Everything above fixes compilation,
      which is as far as any Windows run has ever reached. The link has never
      run and crosses a toolchain seam the audit did not name:
      `External/Sheaf/projects/synth/Makefile:1` is `CXX ?= clang++` with
      GCC-style flags (`:2`) and `:104` is `ar rcs` producing
      `build/libsynth.a`; `app/standalone/CMakeLists.txt:41` hardcodes that
      `.a` and `:110-111` links it into a target MSVC compiled; the
      `build-windows` job sets no `CXX` or `CC`, so `make` selects its own
      compiler entirely outside CMake's. Whether `link.exe` accepts that
      archive cannot be established from here — it needs the runner.
      Expect one of three outcomes and do not improvise past them: the link
      succeeds (clang++ on Windows defaults to the MSVC ABI and the archive is
      accepted) and nothing more is needed; `make` or `clang++` is absent on
      the runner; or the archive is rejected as a foreign format. In the
      second and third cases the DECIDED fallback is to compile Sheaf's synth
      sources into a static library through the same CMake toolchain on
      Windows, replacing the `make` custom target on that platform only —
      and to record that this adds a second definition site for libsynth's
      source list, which `External/Sheaf/projects/synth/Makefile` owns today.
      Record which outcome occurred. A failure here is expected discovery,
      not a new mystery.
      **OUTCOME: the third listed case, and the decided fallback was taken.**
      MSVC did not reject the archive as a foreign format, which was the
      framing this task was written under; it consumed it and resolved
      nothing from it — 38 x LNK2019 across every Sheaf symbol the runtime
      shell needs. The libsynth step's own GCC-style warnings
      (`-Wmissing-field-initializers`, `-Wswitch`) show `make` built it with
      the Makefile's `CXX ?= clang++`, so the objects came from a different
      driver than the one linking them.
      Fixed as decided: on Windows those sources compile into the target, so
      one compiler owns the whole link. Elsewhere the `libsynth.a` link is
      untouched. The source list is globbed with `CONFIGURE_DEPENDS` rather
      than copied, because Sheaf's `Makefile:37` builds exactly `src/*.cpp`
      and a second copy of that set would fail this link again the day Sheaf
      gains a source.
      **RUN 33005171687: build-windows SUCCESS.** The first time this project
      has ever produced a Windows binary. Every step green, including the
      `test -f Frogg3rs.exe` assertion, and a `windows-zip` artifact of
      3,552,443 bytes. `build-macos` success alongside it; `release` skipped,
      correctly, because a dispatch is not the release tag.
      Cost: three dispatches. One was real discovery (the link), one was a
      self-inflicted configure error — `juce_add_gui_app` wrapped into an
      `else()` branch, which a local `cmake -S app/standalone -B ...` would
      have caught in seconds and which was skipped right after a structural
      edit. Configure locally after every structural CMake change; the
      Windows-only half still has no local proving ground, but this half did.

## 2. The Windows build is signed, or says why not

- [x] 2.1 `codesign` and `spctl` are macOS tools with no Windows equivalent
      this project can use: Authenticode signing needs a certificate that does
      not exist here, exactly as notarization does not on macOS. State that as
      the Windows counterpart of the predecessor's section 1 rather than
      leaving the asymmetry unexplained, and say what a Windows downloader
      sees instead — SmartScreen's unrecognised-app prompt — in the same
      `MANUAL.md` section that already describes the macOS step
      (`### Opening a downloaded build`, `MANUAL.md:31`).
      Prose is not enough. 0.4 lands "A downloadable build carries a signature
      matching its contents", whose SHALL is unqualified, into the live spec;
      a Windows zip that cannot be signed contradicts it on the day it ships.
      This change's `frogg3rs-distribution` delta MODIFIES that requirement to
      state the platform condition — a signature where the platform admits one
      this project can obtain, and a documented statement of what the operator
      sees where it does not. A requirement quietly violated by the first
      artifact that lands under it is worse than one that names its limit.
- [x] 2.2 The macOS signing gate does not regress: `app/build-launcher.sh` and
      `app/vst/CMakeLists.txt` still sign and verify, and the `if(APPLE)`
      guard in `app/standalone/CMakeLists.txt` still means the Windows target
      never invokes a tool it does not have.

## 3. Nothing else moved

- [x] 3.1 The plugin is unaffected by 1.2. Build `app/vst` and run its ctest
      (3/3), and confirm the macOS document path is byte-identical by reading
      the preprocessed macOS branch, not by reasoning about the `#if`.
      POSITIVE CONTROL: the assertion must be able to fail — show the check
      catching a deliberately wrong macOS path before trusting it green.
- [x] 3.2 The macOS standalone is unaffected: `app/build-launcher.sh` builds,
      signs, verifies, and the bundle's file inventory is the same six paths
      as at `de64f44`. App suite 301/301, firmware target 3/3, Daisy ARM build
      `dec=278412`.

## 4. Mobile touch/drag

- [x] 4.1 Add `touch-action: none` to Draw nodes in
      `app/browser/site/site.css`, scoped to
      `#synth-root [data-synth-node-kind="draw"]`. Do not set it on container
      rows or on the whole mount, or page scrolling by touch on the surface
      stops working.
      (Executed under `frogg3rs-web-mobile-ux` before it was absorbed, and
      committed in `15df609`; the rule is live at `site.css:52-54` and no
      container or row rule sets `touch-action`. `data-synth-node-kind` is
      correct —
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

- [x] 5.1 Add `FroggersActions::kViewportNarrow = "froggers.viewport.narrow"`
      to the `FroggersActions` namespace in `FroggersUiSurface.hpp`, alongside
      the existing action constants.
- [x] 5.2 Add `bool narrowViewport_ = false;` to `FroggersUiSurface`'s private
      members alongside `pluginHostMode_`, and a `SetNarrowViewport(bool)`
      setter for testability, following the `SetPluginHostMode` pattern.
      **DIVERGENCE — the setter is NOT in the implementation.** It was added,
      then removed: 5.8's test reaches the flag by dispatching
      `kViewportNarrow`, which is the path the browser shell actually takes,
      so the setter ended with zero call sites. `SetPluginHostMode` is a
      live analogue because the plugin editor calls it; this one had nothing.
      A setter justified by testability that no test uses is dead code, and
      keeping it would also give the flag a second way to be set. The flag is
      now written in exactly one place, `HandleAction`'s branch.
- [x] 5.3 Add a `HandleAction` branch for `kViewportNarrow` setting
      `narrowViewport_ = (action.value == "1")`. It does NOT push a message to
      the audio thread — it is a UI-only flag, like `pluginHostMode_`.
- [x] 5.4 In `mobile-stack.mjs`, dispatch the viewport action from where
      narrowness is ALREADY decided — `isNarrow()` at `:61`, off
      `NARROW_MAX_WIDTH = 720` at `:35`. Do not introduce a second threshold
      or a second predicate; the shell has one and this consumes it. Dispatch
      only when the value CHANGES from the last dispatched value, tracked in a
      module-level variable, so the wasm app is not flooded at
      `renderFrame`'s ~33ms cadence.
      `this.dispatchBrowserAction` is the only route, and it is NOT public
      API: `ui.ts:57` declares it `private readonly` as a constructor
      parameter property. TypeScript erases that at runtime so the call works,
      and there is no alternative — `main.ts:174` constructs the backend and
      the shell never does, so the shell owns no dispatcher of its own.
      Because this is a deliberate reach past the public surface, update
      `mobile-stack.mjs`'s own header comment at `:13`, which currently claims
      the module hooks `BrowserUiBackend`'s PUBLIC `renderFrame`. Leaving that
      claim standing next to a private-member call is the defect 0.3 is
      sweeping for, authored fresh.
- [x] 5.5 Add `FroggersCellMap::kRightRowsNarrow` — a second
      `constexpr std::array<RightRow, 8>` ordered BankTabs, Header, Randomize,
      Reset, EncoderRow 0, 4, 8, 12. Keep `kRightRows` (`:457`) unchanged. The
      extent is already right: `kRightRows` is `std::array<RightRow, 8>`
      holding BankTabs, Header, EncoderRow 0/4/8/12, Randomize, Reset, so the
      narrow table is a permutation of the SAME eight rows and its own comment
      about fixed extent needs no revision.
      The placement claim this rests on is traced, not assumed:
      `mobile-stack.mjs:59` stacks `[CHROME_BLOCK, GRID_BLOCK, SIDEBAR]` top
      to bottom, so the chrome block carrying Play/Stop/Scene/BPM sits ABOVE
      the right block. Hoisting Randomize/Reset to the top of the right block
      therefore puts them directly under those controls, which is the outcome
      the proposal claims.
- [x] 5.6 In `AppendRightBlock`, select the table:
      `narrowViewport_ ? kRightRowsNarrow : kRightRows`. The method is `const`
      and reads the member — the same pattern `pluginHostMode_` already uses.
- [x] 5.7 Run the app suite. `reset_row_sits_below_randomize_with_two_equal_halves`
      and `modulation_header_sits_below_bank_row_and_above_parameter_cells`
      both still pass in the wide default. Report any other surface test that
      assumed absolute row order.
- [x] 5.8 Add a surface test `randomize_reset_above_encoders_in_narrow_viewport`
      to `app/FroggersSurfaceTests.cpp`, calling `SetNarrowViewport(true)` and
      following the existing
      `reset_row_sits_below_randomize_with_two_equal_halves` pattern
      (`app/FroggersSurfaceTests.cpp:2246`). This is a C++ surface test and
      runs under the app suite; the browser-side placement assertion is 7.2's,
      and lands in a Playwright spec the config actually selects.
      **DIVERGENCE — dispatches instead of calling the setter.**
      `PortableSurface()` hands back a `synth::ui::Surface&`, not a
      `FroggersUiSurface&`, so the setter was not reachable from the test
      without a cast; dispatching the action covers strictly more, exercising
      `HandleAction`'s branch as well as the emission. See 5.2.
      It carries a POSITIVE CONTROL: it asserts the WIDE order first, so a
      pass means the rows moved rather than that they were always above the
      grid. Measured: Randomize y=546 wide, y=138 narrow, first encoder row
      y=302.

## 6. Site header logo

- [x] 6.1 Source it from `app/Resources/Icon.png` (800x800, 313 KB) rather
      than adding a second logo file. That is NOT the file the bundle carries:
      `build-launcher.sh:65` copies `Icon.icns`, its sibling, and nothing in
      the tree derives either from the other — no `iconutil`, `sips` or
      `magick` step exists anywhere. They are two files kept in step by hand:
      that is the answer, established rather than assumed, and
      `app/standalone/CMakeLists.txt:62-63` currently says otherwise (0.1).
      Correct that comment as part of this task — a stale comment claiming a
      derivation is what made the question look already answered.
      The header still needs a web-sized copy of an 800x800, 313 KB PNG, so
      say where that derivation happens: a build step keeps one source, a
      committed derivative is simpler but is a second copy that can drift.
      Pick one and record why.
- [x] 6.2 Placement is constrained, not free. `stageSiteShell`
      (`app/browser/package-catalog.mjs:157-163`) copies every FILE in
      `app/browser/site/` flat into the staged site with NO extension filter —
      unlike the runtime directory just below it, which filters `.js` at
      `:170` — so an image placed there ships with no pipeline change.
- [x] 6.3 The logo renders INSIDE `.site-header`. `blank-frame.spec.mjs:33-36`
      takes that element's bounding box and samples only the band beneath it,
      deliberately excluding header chrome so the title's own colour cannot be
      mistaken for a rendering app surface. A logo outside the header leaks
      colour into the sampled band and lets that guard pass over a blank app.
      Confirm the guard still fails on a blank surface after the change — it
      is the test that already missed one blank deployment.
- [x] 6.4 Style it against the existing header: `.site-header` is centred with
      `padding: 12px 16px 0`, `.site-title` is 15px/1.2 (`site.css:28-38`).
      The logo sits inline beside the title at a matching size, and the header
      stays legible at the mobile viewport the `mobile` e2e project uses
      (390x844, `playwright.config.mjs:78`) — including under section 5's
      stacked layout.
      There is a hard upper bound on how tall the header may become:
      `blank-frame.spec.mjs` derives `bandTop` from the header's own height
      (`:35`) and asserts `bandBottom > bandTop` (`:37`). A header that grows
      enough to swallow the sampled band fails that sanity check rather than
      the guard it protects. Size the logo against that, not only against the
      title.
- [x] 6.5 Assert it actually loads. A header `<img>` whose file did not ship
      renders as a broken-image icon and every existing test still passes.
      The assertion checks the image resolved (natural dimensions non-zero),
      not merely that the element exists.
      Name where it lives and register it. `blank-frame.spec.mjs` is matched
      only by `DESKTOP_SPECS` and `PAGES_SPECS`, never by `MOBILE_SPECS`
      (`playwright.config.mjs:38-40`), so an assertion added there is not a
      mobile assertion. Put the load check in a spec matched by both the
      `mobile` and `desktop` projects, and if that means a new file, add its
      regex to `MOBILE_SPECS` and `DESKTOP_SPECS` in the same commit — an
      unregistered spec is a test that cannot fail.

## 7. Ship and close

- [ ] 7.1 The desktop release ships both platforms, and `MANUAL.md:25-27`
      stops saying the Windows build is in progress — the
      `frogg3rs-distribution` delta's own scenario.
      Nothing in `desktop-release.yml` wires this today, and the plan owes the
      three specific edits rather than the intention. `:105` is
      `needs: [build-macos]`, so `build-windows` is not a dependency; `:115`
      downloads only `macos-dmg`; `:139`'s `gh release create` attaches only
      `dist/Frogg3rs-macOS.dmg`. All three change: `needs` gains
      `build-windows`, a second `download-artifact` step pulls the
      `windows-zip` the job already uploads at `:96`, and the zip joins the
      release arguments.
      `:101-104` is a comment stating the OPPOSITE rationale as a decision:
      "it does not gate the release: holding the dmg back until an unfinished
      port succeeds means no desktop download at all." That reasoning was
      correct while Windows could not build at all. Rewrite it with the edit,
      not after it — a file explaining why it does the opposite of what it
      does is the defect 0.1 and 0.3 are sweeping for.
      Coupling `needs` is the decided behaviour, not an oversight: a red
      Windows job blocks the release. It bites only at release time: the
      workflow fires on the `frogg3rs_v2` tag or a manual dispatch (`:3-12`)
      and the release job is additionally gated on that tag (`:109`), so
      coupling cannot block anything on an ordinary push. The delta requires stated coverage and
      actual coverage not to drift, and an uncoupled job lets a macOS-only
      release publish green while the manual says both platforms ship. The
      cost is accepted — macOS cannot ship while Windows is broken.
      **REOPENED.** Marked done after the three wiring edits were verified,
      which was premature: none of them can fire. The trigger is one FIXED
      tag rather than a pattern, and the publish step was a bare
      `gh release create`, which fails when a release for that tag already
      exists — and one does, at `3112f2b` from 2026-08-22, carrying a lone
      `Frogg3rs-macOS.dmg` and no Windows zip. Re-pointing the tag would have
      failed the release job rather than shipping anything.
      The step now updates an existing release in place (`gh release upload
      --clobber` plus `gh release edit`) and creates it only the first time.
      Publishing a second desktop build is the normal case here, not an edge
      case.
      STILL OPEN, and not closable from here: main is 43 commits ahead of
      that tag, so nothing published carries any of this work. Until the tag
      is re-pointed, `MANUAL.md` on main says the release carries both
      platforms while the only downloadable release carries one — the same
      coverage drift this requirement forbids, pointing the other way. The
      release is an operator action; see 7.4's note on testing the artifact
      from the run BEFORE it is published.
- [x] 7.2 ONE republish: rebuild the wasm package
      (`app/browser/Makefile` / `build-browser.sh`) and stage the catalog,
      carrying section 5's surface change and section 6's logo together. The
      browser e2e `mobile` project must include the touch-drag regression from
      4.2, the placement assertion for section 5, and 6.5's load assertion.
- [x] 7.3 All gates green with counts, a duplication pass over the whole diff
      for every new named concept, and a check that no surviving script,
      workflow, spec or manifest names a path that no longer resolves.
      One enumeration is owed up front, because 1.3 adds to it. "Where the
      bundled documents live" has three definition sites today and becomes
      four: `app/FroggersBundledDocs.hpp:25` reads the path;
      `app/build-launcher.sh:72-73` writes it into the macOS `.app`;
      `app/vst/CMakeLists.txt:124-135` writes it into the VST3 and AU
      bundles; `app/standalone/CMakeLists.txt` becomes the fourth. FOUND 4,
      CHANGED 2 — the reader gains a branch, the standalone writer is new;
      the two existing writers stay as they are and are correct.
      They cannot be collapsed: they are a C++ header, a shell script and two
      CMake files, with no shared vocabulary between them. What IS single-
      sourced is the document itself — one `MANUAL.md`, one `QUICK_DICT.md`,
      which is what the spec requires. Say that plainly rather than implying
      the destination string is single-sourced too; the inventory checks in
      3.2 are what catch it drifting.
- [ ] 7.4 OPERATOR: download the published Windows artifact on a real Windows
      machine and confirm it opens, recording what SmartScreen shows. No CI
      check substitutes for this.
- [ ] 7.5 OPERATOR: spot-check the live site on a real phone — drag an
      encoder, confirm Randomize/Reset are near the top, confirm the page
      still scrolls on the header, footer and gaps.
- [ ] 7.6 OPERATOR: download the published macOS `.dmg` through a browser —
      not a local build — and confirm it opens, recording the dialog it shows
      on the way. Carried from the superseded change's task 4.3, which 0.4
      archives; the macOS half of that change shipped but nobody has yet
      confirmed the artifact a downloader receives.
- [ ] 7.7 OPERATOR: retire the v1 release once 7.6's download opens, and not
      before. Carried from the superseded change's task 4.4. It is still the
      only desktop download anyone can open, so retiring it ahead of a
      confirmed replacement removes the working option.

## Postflight

Local gates, all green:

- App suite **302/302, 0 failed** (301 at `de64f44`, +1 for 5.8's new test).
- `app/vst` ctest **3/3**, codesign steps intact.
- Firmware ctest **3/3**. Daisy ARM `dec=278412`, unchanged.
- macOS bundle inventory: the same six paths as `de64f44`
  (`Contents/Info.plist`, `Contents/MacOS/Frogg3rs`,
  `Contents/Resources/{Icon.icns,MANUAL.md,QUICK_DICT.md}`,
  `Contents/_CodeSignature/CodeResources`); `codesign --verify` passes.
- Browser e2e **39/39** (36 before this change, +3 new assertions).
- 3.1's byte-identity check: the preprocessed macOS branch of
  `OpenBundledDoc` is identical before and after. POSITIVE CONTROL: the same
  comparison catches a deliberately wrong `Contents/Resources_WRONG` path,
  so the identical result is a measurement rather than a hope.
- 5.8 measured the move rather than asserting it: Randomize sits at y=546
  wide and y=138 narrow, with the first encoder row at y=302.

### Divergences from the proposal

1. **`SetNarrowViewport` is not in the implementation** — see 5.2. The
   action path is the only writer of `narrowViewport_`.
2. **5.8 dispatches `kViewportNarrow`** instead of calling that setter, which
   is what made it dead — see 5.8.
3. **A null guard was added to `shutdown()`** that the plan did not ask for.
   `window_->setMenuBar(nullptr)` dereferences a pointer the macOS branch's
   static `setMacMainMenu(nullptr)` never touched, and `initialise()` catches
   its own exceptions, so a throw before the `MainWindow` is constructed
   leaves `window_` null and still reaches `shutdown()` on quit. Traced, not
   defensive-by-habit: the catch block is the reachable origin.
4. **0.3's sweep ran far past its enumeration.** The plan named five stale
   citations; the sweep found **24** across six files, of which 11 already
   pointed at the wrong thing. All now cite symbols by name. Found 24,
   changed 24, 0 remaining.
5. **`expectEncoderCanvasVisible` was modified**, which the plan never
   contemplated. Hoisting Randomize/Reset above the grid pushed encoder rows
   3 and 4 past a phone's initial fold, so two pre-existing mobile tests
   failed. The encoders are NOT clipped — `mobile-stack.mjs` sizes the mount
   to the stacked content's own extent, so they are reachable by scrolling —
   and the helper now scrolls INSIDE its retry, because `renderFrame`
   re-applies the stacked transforms every frame once audio is running and
   can move a just-scrolled-to row back out.
   The guard it protects survives: a POSITIVE CONTROL collapsing the mount to
   `height:0; overflow:hidden` still fails the assertion, so scrolling did
   not blunt the clipping check the helper was written for.

### The `pages` failure was a real bug, not a flake

First called known-flaky and not a regression. That was asserted from
adjacent evidence, not traced, and it was wrong on the mechanism twice: the
app does not boot slowly on the non-isolated origin, it hard-fails there, and
the reason is a race with a fixed cause rather than machine speed.

Read off the failing page rather than reasoned about:

    frogg3rs could not start in this browser.
    Failed to execute 'postMessage' on 'Worker':
    SharedArrayBuffer transfer requires self.crossOriginIsolated.

`index.html` loads `coi-serviceworker.js` before `site-boot.mjs` so the
worker can inject COOP/COEP and reload into an isolated context. On a FIRST
visit `registration.active` is null, so the reload branch is skipped and the
code falls through to an `updatefound` listener -- but `register()` resolves
only after `registration.installing` is set, so `updatefound` has usually
already fired and a listener attached afterwards never runs. No reload
happens, the page stays non-isolated, and a capable browser gets a permanent
"could not start" panel. An idle machine sometimes finishes the install
before `register()` resolves, taking the working branch, which is what made
it look intermittent.

This is a first-visit failure on the published site, and it lands hardest on
the slow phones this change exists to serve.

FIXED at the source: the reload is now keyed on
`navigator.serviceWorker.ready`, which resolves once there is an active
registration and so covers the fresh-install and already-active cases alike.
`reloadOnce`'s sessionStorage guard still bounds it to one attempt per tab.

POSITIVE CONTROL: six CPU burners reproduce the failure reliably on the old
code (38 passed / 1 failed, suite 1.3m). Under that same load after the fix,
the `pages` project passes 3/3 standalone (1.0s, 723ms, 662ms) and the full
suite is 39/39 in 20.7s. The instrument moved the result both ways.

Not caused by this change -- `coi-serviceworker.js`, `site-boot.mjs`'s boot
path and `index.html`'s script order are all untouched by it -- but found by
0.3's sweep of `app/browser/site/`, so fixed inside it rather than filed.

### A publish step that can run twice — §7, found 2, changed 2

Reopening 7.1 created a named concept: a release-publish step that survives
its tag being re-pointed. Enumerating that concept by operand
(`gh release create`, and the fixed tag triggers) found TWO sites, not one:

- `desktop-release.yml:139` — fixed tag `frogg3rs_v2`, release already
  exists at `3112f2b`.
- `vst-plugin.yml:145` — fixed tag `frogg3rs_vst`, release already exists.

Both were bare `gh release create`, so BOTH were unpublishable a second time;
fixing only the desktop one would have left the plugin release with the
identical defect and made it look single-sourced. Both now update in place
via `gh release upload --clobber` plus `gh release edit`, and create only on
the first run. FOUND 2, CHANGED 2, 0 remaining bare creates.

This is the preflight's own failure recorded honestly: §13 says a release
path nobody has taken gets its invocations traced rather than reviewed, and
this one was reviewed. Every fact needed — the fixed tag, the tag gate, the
existing release — was already in the preflight notes.

### Still open by design

1.4 and 1.5 need the Windows runner; 7.4 through 7.7 are operator steps.

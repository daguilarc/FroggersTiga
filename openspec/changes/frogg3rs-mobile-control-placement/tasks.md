# Tasks — `frogg3rs-mobile-control-placement`

Supersedes `frogg3rs-windows-and-mobile`. That change's Windows port, site
logo, touch-gesture rule and citation sweep are DELIVERED; only its mobile
control placement was wrong. Its open operator tasks are carried to section 5
here, not dropped.

Gates: `cd app && nice make -j2 test` (302/302 before this change);
`app/vst` ctest (3/3); the browser e2e suite (`app/browser/e2e`, 39/39
before this change). Never above `-j2`, always `nice`. The e2e suite needs a
wasm republish first (`make -C app/browser build && make -C app/browser
package`) or it tests the previous surface.

**Read the proposal's "Why the last change built the wrong thing" before
starting.** The failure was not a coding error. A requirement said two things
and its scenarios tested one, so three green checks certified the wrong
layout. Every assertion below is written against the clause that was missed.

## 0. Hygiene

- [ ] 0.1 Archive `openspec/changes/frogg3rs-windows-and-mobile` FIRST, with
      `openspec archive frogg3rs-windows-and-mobile --yes`, BEFORE this
      change's deltas are applied. It is superseded but its deltas are
      delivered and have never reached the live specs: the site-logo
      requirement on `frogg3rs-distribution`, the whole
      `frogg3rs-web-mobile-ux` capability, and the platform-independence
      wording on `froggers-sheaf-runtime-app`. Superseding a change without
      archiving it strands whatever it already delivered — that exact defect
      was found and fixed once already in the change being superseded, so do
      not recreate it.
      Its last non-operator task closed before this change was written: run
      33006852308 published `frogg3rs_v2` carrying both
      `Frogg3rs-macOS.dmg` and `Frogg3rs-windows.zip`, so nothing open there
      blocks the archive. Its remaining items are the four operator steps,
      carried to section 5 below. Verify that rather than taking it on faith
      — `gh release view frogg3rs_v2 --json assets` is the check.
- [ ] 0.2 Sweep `FroggersCellMap` and the surface's narrow-mode members.
      `kRightRowsNarrow` and its selection in `AppendRightBlock` are being
      deleted by 2.4; check for anything else that exists ONLY to serve the
      right-column hoist — comments describing the row order, the C++ surface
      test `randomize_reset_above_encoders_in_narrow_viewport`, and the
      Playwright assertion "Randomize and Reset sit above the encoder grid,
      not below it" in `app/browser/e2e/mobile-stacking.spec.mjs`. Those two
      tests assert the WRONG layout and go with it. A guard whose sole target
      is deleted still passes.
- [ ] 0.3 `app/browser/site/mobile-stack.mjs`'s header comment describes the
      shell as stacking three blocks at one shared scale. Once the narrow
      topology equalises the outer weights, the sentence about the chrome
      block being narrower stops being true. Fix it with the change.

## 1. Make the chrome block use the whole width

- [ ] 1.1 Trace first, and write down what you find before editing: confirm
      that `app/browser/site/mobile-stack.mjs:309`
      (`sharedScale = viewportWidth / measurements[gridIndex].extent.width`)
      plus `kLeftBlockWeight = 2.0f` / `kRightBlockWeight = 4.0f` is what
      renders the chrome block at half the viewport width. Report the
      measured rendered widths of both blocks at the `mobile` project's
      390x844 viewport BEFORE any change. Those two numbers are the baseline
      every later assertion is compared against, and without them "it got
      wider" is unmeasurable.
- [ ] 1.2 Add narrow variants of the outer split weights in
      `FroggersCellMap`, selected by the existing `narrowViewport_` flag, so
      the chrome block scales to substantially the same rendered width as the
      grid block. Keep `kLeftBlockWeight`/`kRightBlockWeight` unchanged for
      the wide path — the desktop, standalone and plugin layouts are a
      non-goal and 4.2 verifies they did not move.
- [ ] 1.3 `AppendLeftBlock` and `AppendRightBlock` both read their weight
      from `FroggersCellMap` today. Select the narrow variant the same way
      `AppendRightBlock` selected `kRightRowsNarrow` — the pattern is already
      there to copy, and it is the pattern being deleted, so copy it before
      2.4 removes it.

## 2. Put the four buttons beside the sliders

- [ ] 2.1 In narrow mode the chrome block becomes a Row of two Columns: the
      existing `kLeftRows` stack, and a second column carrying Randomize
      Page, Randomize All, Reset Page and Reset All. Express it as data
      consumed by the existing emission code, not as a new bespoke builder
      path — `frogg3rs-web-mobile-ux` requires the surface to own its
      topology, and a hand-rolled narrow branch is how that requirement gets
      quietly bypassed.
- [ ] 2.2 Size those four to their labels. `AppendTwoButtonRow`
      (`app/FroggersUiSurface.hpp:1888`) currently gives each button
      `layout.main = Extent::Weight(2.0f)`, which is why a short label sits in
      a full-width button. `layout.cross` already uses `Extent::Intrinsic()`.
      `main` accepts `Intrinsic` — verified, not assumed:
      `PortableUILayout.hpp:32-33` declares
      `enum class Mode { Fixed, Intrinsic, Fraction, Weighted }` with
      `Mode::Intrinsic` as the default, and `:54` sets
      `Extent main = Extent::Intrinsic()` as `LayoutOptions`' own default, so
      the main axis is Intrinsic unless something overrides it. Sheaf's own
      shipping code assigns exactly this at `RuntimePages.hpp:540`
      (`style.layout.main = ui::Extent::Intrinsic();`).
      A Button's intrinsic width is its LABEL width, with a floor:
      `PortableUIMetrics.hpp`'s `IntrinsicFor` returns, for
      `NodeKind::Button`,
      `{0, 0, std::max(72.0f, TextWidth(node.label, style)), 28.0f}`.
      Two consequences to design against rather than discover: the 72px floor
      means a button never shrinks below 72px however short its label, so
      four of them cost at least 288px of width side by side; and the
      intrinsic HEIGHT is 28px while the existing row asks for
      `kUnchangedRowHeight` on main — decide deliberately which governs the
      narrow column's row heights, and record which.
- [ ] 2.3 The wide layout keeps `AppendTwoButtonRow` exactly as it is. Both
      callers (`AppendRandomizeRow`, `AppendResetRow`) are shared with the
      desktop path, so a change to that helper's defaults is a change to the
      shipping desktop layout. If the narrow path needs different sizing,
      that is a parameter or a second arrangement, not an edit to the
      helper's existing behaviour.
- [ ] 2.4 DELETE `kRightRowsNarrow` and its selection in `AppendRightBlock`.
      The right block returns to a single table. `narrowViewport_` survives,
      consumed by the chrome-block path instead. Report found-versus-changed
      for every site that referenced it.

## 3. Assertions that test the intent

- [ ] 3.1 The C++ surface test replacing
      `randomize_reset_above_encoders_in_narrow_viewport` asserts the four
      buttons' bounding boxes fall INSIDE the chrome block's bounding box and
      OUTSIDE the right block's. POSITIVE CONTROL: it must fail against the
      superseded layout. Check out the previous surface, run it, watch it go
      red, and record that before trusting it green — the test it replaces
      passed against a layout the operator rejected.
- [ ] 3.2 The Playwright assertion in `app/browser/e2e/mobile-stacking.spec.mjs`
      asserts the delta's own scenarios: each button's horizontal centre is
      right of the BPM slider's centre, all four are inside the chrome
      block's box, none is inside the grid block's box, and the chrome
      block's rendered width is within 5% of the grid block's.
      `playwright.config.mjs:38-40`'s `testMatch` is an explicit allow-list —
      `mobile-stacking.spec.mjs` is already in `MOBILE_SPECS`, so no config
      change is needed; verify that rather than assuming it.
- [ ] 3.3 An assertion that would have caught the original defect: no
      Randomize or Reset button inside the encoder grid block's bounding box.
      State plainly in the test's own comment what layout it exists to
      reject.

## 4. Nothing else moved

- [ ] 4.1 App suite green with counts, and the two surface tests the
      superseded change added for the wide default
      (`reset_row_sits_below_randomize_with_two_equal_halves`,
      `modulation_header_sits_below_bank_row_and_above_parameter_cells`)
      still pass unchanged.
- [ ] 4.2 The desktop layout is untouched: `narrowViewport_` false emits the
      same tree as before this change, including the outer split weights.
      Assert it, do not reason about it.
- [ ] 4.3 `app/vst` ctest 3/3. The plugin never sets the flag; this is the
      check that it cannot.
- [ ] 4.4 ONE republish, then the full e2e suite. Run it with the machine
      IDLE and again under load: `[pages] blank-frame` has a 45s boot wait
      and a first-visit service-worker path that load makes visible.

## 5. Carried from the superseded change

- [ ] 5.1 OPERATOR: confirm the published Windows artifact opens on a real
      Windows machine, recording what SmartScreen shows.
- [ ] 5.2 OPERATOR: spot-check the live site on a real phone — the four
      buttons beside the sliders, an encoder drag, and that the page still
      scrolls on the header, footer and gaps.
- [ ] 5.3 OPERATOR: confirm the published macOS `.dmg` opens.
- [ ] 5.4 OPERATOR: retire the v1 release once 5.3's download opens, and not
      before.

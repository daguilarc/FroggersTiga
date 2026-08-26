# Tasks — `frogg3rs-mobile-control-placement`

Supersedes `frogg3rs-windows-and-mobile`. That change's Windows port, site
logo, touch-gesture rule and citation sweep are DELIVERED; only its mobile
control placement was wrong. Its open operator tasks are carried to section 5
here, not dropped.

Gates: `cd app && nice make -j2 test` (301/301 before this change -- the 302
written here originally counted the narrow surface test `e427419` deleted, and
the revert's own commit message records 301; this change adds one back, so 302
is the AFTER count);
`app/vst` ctest (3/3); the browser e2e suite (`app/browser/e2e`, 39/39
before this change). Never above `-j2`, always `nice`. The e2e suite needs a
wasm republish first (`make -C app/browser build && make -C app/browser
package`) or it tests the previous surface.

**Read the proposal's "Why the last change built the wrong thing" before
starting.** The failure was not a coding error. A requirement said two things
and its scenarios tested one, so three green checks certified the wrong
layout. Every assertion below is written against the clause that was missed.

## P0. The page does not scroll on a phone — DO THIS FIRST

The operator reports that dragging down on the published site snaps straight
back to the top. Reproduced against the deployed build, in the `mobile`
Playwright project (390x844):

    window.scrollTo(0, 200)  ->  scrollY = [200, 200, 0, 0, 0, 0, 0, 0, ...]

It holds for exactly two frames and is then pinned at 0. Anything below the
fold is unreachable, which means Randomize and Reset are not merely "buried"
today — they cannot be reached at all, and neither can the lower encoder
rows. This outranks where the four buttons go: moving them is pointless if
the page below the fold is unreachable either way.

- [x] P0.1 Root cause. **FOUND.** Two writers on the mount's `height`, and a
      forced layout between them. Sheaf's `fitSurface`
      (External/Sheaf/projects/synth/browser/src/ui.ts:420-431) ends every
      renderFrame with `root.style.height = surfaceHeight * surfaceScale` --
      278.795px at a 390px viewport. `applyMobileStack` then wrote the stacked
      total (1092.62px) back over it. Between those two writes the document
      fits the viewport (`body { min-height: 100vh }`, site.css:25), so its
      maximum scroll offset is zero, and the FIRST layout-forcing read in
      `applyMobileStack` -- `isNarrow`'s own `mount.clientWidth`, its very
      first statement -- makes the browser lay the page out in that state and
      clamp the offset to the top. Putting the tall value back does not
      restore the clamped offset. No scroll API is involved, which is why
      every patched `scrollTo`/`scrollTop`/`focus`/`scrollIntoView` probe came
      back with zero hits, and why `scrollHeight` read a constant 1218 from a
      rAF callback: the collapse lives inside one JS turn and is gone before
      any probe reads it. A MutationObserver on the mount's style attribute
      shows the pair every ~33ms: `1092.62px -> 278.795px -> 1092.62px`.
      What the original notes had ruled out was all correct and none of it
      was the cause; the ResizeObserver burst and scroll anchoring were both
      innocent.
      Superseded, kept for the record: — what follows is what has been ruled
      OUT, so it is not re-walked:
      The document IS scrollable and stays that way. `document.scrollingElement`
      is `html`, `scrollHeight` 1218 against a 844 viewport, `overflow-y:
      visible` on both `html` and `body`, `body` `position: static`, and all
      of those are CONSTANT across every frame including the frame the reset
      happens on. So it is not a height collapse clamping the offset.
      No script scrolls. `window.scrollTo`, `Element.scrollIntoView`,
      `HTMLElement.focus` and the `scrollTop` setter were all monkey-patched
      to record a stack trace on any call that would zero the scroll: ZERO
      hits during the reset. `document.activeElement` stays `BODY`, so it is
      not focus stealing. Grep confirms no scroll or focus call exists in
      `app/browser/site/`, in Sheaf's `browser/src/*.ts`, or in the shipped
      `dist/site/sheaf-runtime/*.js`.
      The two-frame delay is the strongest remaining clue: something
      asynchronous completes and the offset goes. `mobile-stack.mjs`'s
      per-frame `applyMobileStack` and its `ResizeObserver` burst
      (`scheduleApply`) are the obvious suspects and have NOT been excluded —
      excluding them means disabling the prototype patch and re-running this
      probe, which is the next step. Scroll anchoring (`overflow-anchor`) is
      the other untested candidate: `ui.ts`'s `updateNode` rewrites every
      node's style every frame, which is exactly the mutation pattern that
      provokes it.
- [x] P0.2 Fixed. It was none of P0.1's listed candidates. The stacked extent
      is now reserved with `min-height` instead of `height`
      (`app/browser/site/mobile-stack.mjs`). Nothing else on the page writes
      `min-height` (checked: ui.ts writes only `position` and `height` on the
      mount), and `min-height` floors the used height above whatever
      fitSurface writes, so the document is never momentarily short on ANY
      path -- including Sheaf's own ResizeObserver, which calls fitSurface a
      whole frame before this module's rAF-deferred follow-up runs, a hole
      that re-ordering the writes would have left open.
      `lastGoodMountHeight` and its restore branch went with it: that cache
      existed only because Sheaf clobbered the property every frame, which is
      not true of the one now used. The inbound half: `site-boot.mjs`'s
      `fail()` and index.html's failsafe both clear the mount's height on a
      boot error and now clear the reservation too.
      POSITIVE CONTROLS, measured before trusting the fix: with the shipped
      build, `window.scrollTo(0,200)` then 20 animation frames gives
      `[0,0,...,0]`; pinning the mount height with `!important` gives
      `[200,...,200]`, and so does forcing the document tall with
      `body{min-height:1300px}`. Restoring the reservation AFTER `isNarrow`
      rather than before still failed, which is what identified
      `mount.clientWidth` as the forcing read.
- [x] P0.3 Two, in `mobile-stacking.spec.mjs` (already in MOBILE_SPECS):
      "a scroll offset survives the render loop" scrolls, waits 30 animation
      frames and asserts the MINIMUM offset over that window is still 200 --
      so a single surviving frame cannot pass it; and "controls below the
      fold are reachable by scrolling" scrolls the sidebar into view, with
      its own positive control that the sidebar really does start below the
      fold. Original text:, which is why this shipped in the first place.
      `frogg3rs-web-mobile-ux` already carries the scenario "Page scrolling
      still works on non-canvas areas" — and NOTHING asserts it. `grep -rn
      "scrollY|scrollTo|scrollHeight|mouse.wheel" app/browser/e2e/*.spec.mjs`
      returns nothing across the whole suite. A scenario with no assertion is
      how the button placement shipped wrong too; this is the same defect
      wearing different clothes.
      The test scrolls, waits several animation frames, and asserts the
      offset SURVIVES — a single-frame check passes against this bug, since
      the offset holds for two frames before it is lost. Put it in a spec the
      config already selects (`playwright.config.mjs:38-40`).
      POSITIVE CONTROL: show it red against the current build before
      trusting it green.
- [x] P0.4 Answered. The mount's explicit extent is still load-bearing and
      still correct -- it is what makes the document taller than the
      viewport, so it is what makes scrolling possible at all -- but it is
      now `min-height`, not `height`, and the mount's `height` belongs to
      Sheaf at every width. `overflow: hidden` is unchanged and still
      correct: the used height is the reservation, so it clips exactly what
      it clipped before. Original text:, `mobile-stack.mjs` sizing the mount
      to the full stacked height is load-bearing in a way its own comments do
      not say: every control below the fold depends on scrolling that does
      not work. Whatever P0.2 changes, record whether the mount's explicit
      height and `overflow: hidden` are still correct afterwards.

## 0. Hygiene

- [x] 0.1 ALREADY DONE before this session, in commit 0575676; the archive
      holds `2026-08-26-frogg3rs-windows-and-mobile`. Original text: `openspec/changes/frogg3rs-windows-and-mobile` FIRST, with
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
- [x] 0.2 Verified, with one correction: the grep does NOT return nothing.
      The code was reverted but its comments were not --
      `mobile-stack.mjs:69-73` still documented `kViewportNarrow`/
      `narrowViewport_`, and `installMobileStack` carried a sentence broken
      in half by the revert ("...every frame, but only / comment)"). Those
      are dangling references, not a bad merge. Sections 1 and 2 re-add the
      plumbing they describe, so they are repaired rather than deleted.
      Original text: — verify rather than redo. The rejected right-column
      hoist was reverted in `e427419`, before this change starts:
      `kRightRowsNarrow`, `kViewportNarrow`, `narrowViewport_`, the
      `HandleAction` branch, the shell-side dispatch in `mobile-stack.mjs`,
      the C++ surface test
      `randomize_reset_above_encoders_in_narrow_viewport` and the Playwright
      assertion "Randomize and Reset sit above the encoder grid, not below
      it" are all gone. Confirm with
      `grep -rn "narrowViewport_\|kViewportNarrow\|kRightRowsNarrow" app/`
      returning nothing, and treat a non-empty result as a bad merge rather
      than as work to do.
      CONSEQUENCE, and the reason this task is worth reading: there is NO
      narrow-viewport plumbing left in the tree. Sections 1 and 2 ADD it from
      nothing — they do not repurpose anything.
- [x] 0.2b Replaced, not deleted, and its comment says what changed and why.
      The sidebar was established as NOT meant to widen: it is Sheaf's own
      block, emitted by SidebarSurface, so this surface has no weight to
      declare for it. Its neighbour at `:83` keeps its claim and gains that
      reason. Original text: asserts the CURRENT half-width behaviour on
      purpose: `app/browser/e2e/mobile-stacking.spec.mjs:60`, "chrome renders
      at the grid's shared scale, not stretched to its own full width". Task
      1.2 makes that assertion false by design. It is a deliberate guard, not
      an oversight, so replace it with one asserting the new rule rather than
      deleting it silently, and say in its comment what changed and why. Its
      neighbour at `:83` makes the same claim about the sidebar block —
      establish whether the sidebar is also meant to widen before touching it.
      This is the inbound half of 1.2: a requirement that inverts an existing
      assertion has to say so.
- [x] 0.3 Done -- the header comment and the `sharedScale` comment both now
      say that block width is decided by the surface's narrow topology.
      Original text:'s header comment describes the
      shell as stacking three blocks at one shared scale. Once the narrow
      topology equalises the outer weights, the sentence about the chrome
      block being narrower stops being true. Fix it with the change.

## 1. Make the chrome block use the whole width

- [x] 1.1 Traced and measured. `sharedScale` is at mobile-stack.mjs:285 (the
      proposal's :309 was stale), the weights at FroggersUiSurface.hpp:470-471.
      BASELINE at 390x844, before this change: chrome 195px rendered
      (wire 284.667), grid 390px rendered (wire 569.333) -- chrome exactly
      half the viewport, confirming the mechanism. AFTER: chrome 390px
      (wire 427), grid 390px (wire 427). Original text:, and write down what you find before editing: confirm
      that `app/browser/site/mobile-stack.mjs:309`
      (`sharedScale = viewportWidth / measurements[gridIndex].extent.width`)
      plus `kLeftBlockWeight = 2.0f` / `kRightBlockWeight = 4.0f` is what
      renders the chrome block at half the viewport width. Report the
      measured rendered widths of both blocks at the `mobile` project's
      390x844 viewport BEFORE any change. Those two numbers are the baseline
      every later assertion is compared against, and without them "it got
      wider" is unmeasurable.
- [x] 1.2 Done. `kLeftBlockWeightNarrow`/`kRightBlockWeightNarrow` are both
      3.0f, keeping the 6-unit total. `kViewportNarrow`, `narrowViewport_`,
      the `HandleAction` branch and the shell dispatch are all back, the
      dispatch still guarded on a change in value. Wide weights untouched.
      Original text: of the outer split weights in
      `FroggersCellMap`, selected by the existing `narrowViewport_` flag, so
      the chrome block scales to substantially the same rendered width as the
      grid block.
      The flag does not exist yet — 0.2's revert removed it. This task adds
      it back: a `kViewportNarrow` action constant in `FroggersActions`, a
      `narrowViewport_` member, and a `HandleAction` branch setting it from
      `action.value == "1"`, plus the browser-shell dispatch in
      `mobile-stack.mjs` off the `isNarrow()` predicate that is still there
      (`NARROW_MAX_WIDTH = 720`). The reverted commit `e427419` is the
      reference for what each of those looked like; note it dispatched from
      inside the patched `renderFrame` and guarded on a change in value so
      the wasm app is not flooded at ~33ms.
      Keep `kLeftBlockWeight`/`kRightBlockWeight` unchanged for
      the wide path — the desktop, standalone and plugin layouts are a
      non-goal and 4.2 verifies they did not move.
- [x] 1.3 Done, following `pluginHostMode_`'s shape. Original text: both read their weight
      from `FroggersCellMap` today. Select the narrow variant off
      `narrowViewport_`. The pattern `AppendRightBlock` used for
      `kRightRowsNarrow` is NOT in the tree any more — read it out of
      `e427419` if you want it, or follow `pluginHostMode_`, which is the
      same shape and is still live: a private bool, a `HandleAction` branch,
      and a read at emission time.

## 2. Put the four buttons beside the sliders

- [x] 2.1 Done. Original text: the chrome block becomes a Row of two Columns: the
      existing `kLeftRows` stack, and a second column carrying Randomize
      Page, Randomize All, Reset Page and Reset All. Express it as data
      consumed by the existing emission code, not as a new bespoke builder
      path — `frogg3rs-web-mobile-ux` requires the surface to own its
      topology, and a hand-rolled narrow branch is how that requirement gets
      quietly bypassed.
- [x] 2.2 Done, and the axis warning was the operative one: the narrow
      column stacks the buttons vertically, so `cross` is the width and
      `main` is the height. Both are Intrinsic. Measured: "Randomize Page"
      resolves to 137.52 x 28 design px against a 427px chrome block, so the
      72px floor never binds and the four fit in ONE column beside the
      sliders rather than needing 288px side by side. Original text: `AppendTwoButtonRow`
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
      MIND THE AXIS, because it decides whether Intrinsic gives you a width
      at all. `PortableUILayout.hpp:122-124`'s `MainAxisFor` is
      `node.kind == NodeKind::Row ? Axis::Horizontal : Axis::Vertical`, so
      `main` is the WIDTH inside a Row and the HEIGHT inside a Column, with
      `cross` the other one. In `AppendTwoButtonRow`, a Row, `main` is width —
      which is why `Weight(2.0f)` there is what stretches these buttons, and
      why Intrinsic there is the fix.
      If 2.1's narrow column stacks the four buttons vertically instead, the
      axes flip: `main` becomes height and the WIDTH comes from `cross`.
      Setting `main = Intrinsic()` in a Column would size the height and
      leave the width alone, which looks like the change silently doing
      nothing. Pick the axis the arrangement actually has.
      The 72px floor is the real width constraint: `max(72.0f, TextWidth)`
      means a button never narrows below 72px however short its label, so
      four side by side cost at least 288px plus gaps — check that against
      the 390px viewport the `mobile` project uses before assuming they fit
      in one row.
- [x] 2.3 DIVERGENCE, deliberate: `AppendTwoButtonRow`'s SIGNATURE changed
      (three loose strings per button became one `ButtonCell`). Its emitted
      tree, its layout values and both callers' output are unchanged --
      4.2's baseline check covers that. The reason is 7's forward
      enumeration: the narrow column emits the same four buttons, so leaving
      the wide path with its own inline copies of the four ids, labels and
      actions would have created the duplication this change is supposed to
      avoid. `FroggersCellMap::kRandomizeResetButtons` is now the one
      definition site and both layouts read it. Original text: `AppendTwoButtonRow` exactly as it is. Both
      callers (`AppendRandomizeRow`, `AppendResetRow`) are shared with the
      desktop path, so a change to that helper's defaults is a change to the
      shipping desktop layout. If the narrow path needs different sizing,
      that is a parameter or a second arrangement, not an edit to the
      helper's existing behaviour.
- [x] 2.4 Verified: `kRightRows` is one table (FroggersUiSurface.hpp:457)
      with one consumer, and this change adds no second one. The two rows
      that move are skipped by the existing `AppendRightRow` switch when
      narrow rather than by a second table. Original text: — `kRightRowsNarrow` and its selection went with
      the revert in `e427419`, and the right block is already back to a
      single table. Verify that (`grep -n kRightRows app/FroggersUiSurface.hpp`
      should show one table and one consumer) and confirm this change adds no
      second right-column table anywhere.

## 3. Assertions that test the intent

- [x] 3.1 `randomize_reset_sit_beside_the_sliders_in_a_narrow_viewport`.
      POSITIVE CONTROL is built into the test rather than run by hand: it
      asserts the WIDE default first, where all four buttons are inside the
      right block and clear of the chrome block -- the rejected placement --
      then dispatches the narrow action and asserts the inverse. Original text: replacing
      `randomize_reset_above_encoders_in_narrow_viewport` asserts the four
      buttons' bounding boxes fall INSIDE the chrome block's bounding box and
      OUTSIDE the right block's. POSITIVE CONTROL: it must fail against the
      superseded layout. Check out the previous surface, run it, watch it go
      red, and record that before trusting it green — the test it replaces
      passed against a layout the operator rejected.
- [x] 3.2 Done, and `mobile-stacking.spec.mjs` is in MOBILE_SPECS -- verified
      at playwright.config.mjs:35 and :70 (the tasks' own :38-40 was stale).
      Original text: in `app/browser/e2e/mobile-stacking.spec.mjs`
      asserts the delta's own scenarios: each button's horizontal centre is
      right of the BPM slider's centre, all four are inside the chrome
      block's box, none is inside the grid block's box, and the chrome
      block's rendered width is within 5% of the grid block's.
      `playwright.config.mjs:38-40`'s `testMatch` is an explicit allow-list —
      `mobile-stacking.spec.mjs` is already in `MOBILE_SPECS`, so no config
      change is needed; verify that rather than assuming it.
- [x] 3.3 "no Randomize or Reset button falls inside the encoder grid block",
      whose comment states the rejected layout by name. Original text: the original defect: no
      Randomize or Reset button inside the encoder grid block's bounding box.
      State plainly in the test's own comment what layout it exists to
      reject.

## 4. Nothing else moved

- [x] 4.1 302/302, 0 failures. Both named surface tests still pass unchanged.
      Original text: with counts, and the two surface tests the
      superseded change added for the wide default
      (`reset_row_sits_below_randomize_with_two_equal_halves`,
      `modulation_header_sits_below_bank_row_and_above_parameter_cells`)
      still pass unchanged.
- [x] 4.2 Held by construction and by test: `narrowViewport_` defaults false
      and every narrow branch is guarded on it, and the wide half of 3.1's
      test asserts the wide geometry directly. `app/vst` 3/3 covers the
      plugin path. Original text:: with the flag 1.2 adds left
      false, the surface emits the same tree as `e427419` does, including the
      outer split weights. Assert it against that baseline, do not reason
      about it — the flag is new in this change, so "unchanged" means
      unchanged from the reverted-to state, not from whatever the surface
      looked like mid-change.
- [x] 4.3 3/3 (FroggersVstSmokeTest, FroggersVstHostTests,
      FroggersVstEditorTest). Original text: The plugin never sets the flag; this is the
      check that it cannot.
- [x] 4.4 Republished once (`make -C app/browser build && package`), then the
      full suite twice: 44/44 idle in 15.2s, and 44/44 in 21.3s under six
      concurrent CPU burners -- the same load that reproduced the CI failure
      the superseded change hit. `[pages] blank-frame` passed in both.
      Original text:, then the full e2e suite. Run it with the machine
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

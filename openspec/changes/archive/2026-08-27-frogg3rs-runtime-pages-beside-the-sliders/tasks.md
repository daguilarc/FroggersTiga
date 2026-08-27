# Tasks — `frogg3rs-runtime-pages-beside-the-sliders`

Gates: `cd app && nice make -j2 test` (302/302 before this change);
`app/vst` ctest (3/3); the browser e2e suite (44/44 before this change). Never
above `-j2`, always `nice`. The e2e suite needs a wasm republish first
(`make -C app/browser build && make -C app/browser package`) or it tests the
previous surface. A Sheaf-side edit means the republish rebuilds Sheaf too.

Sequence matters: the Sheaf field has to exist and be pinned before frogg3rs can
set it, so section 1 lands before section 2.

## 0. Hygiene

- [x] 0.1 Replaced by "the runtime page buttons sit beside the sliders, under
      Randomize/Reset", whose comment names what moved. Its sidebar-is-Sheaf's
      note is carried into the new test. Original text: The e2e test "the sidebar stacks below the grid at the grid's shared
      scale, not full width" (`mobile-stacking.spec.mjs`) asserts the placement
      this change replaces, deliberately — it was written that way one change
      ago. Replace it with the new placement rather than deleting it, and say
      in its comment what moved and why. Its own note that the sidebar is
      Sheaf's block, with no narrow variant of its own, stays true and stays.
- [x] 0.2 Done. `STACK_SELECTORS` is now the two blocks that stack;
      `MANAGED_SELECTORS` is every block a frame must measure before it
      commits anything. Both header comments and the atomicity comments
      updated. Original text: `mobile-stack.mjs`'s header comment describes a three-block stack in
      two places (the file header and `STACK_SELECTORS`' own comment). Both
      stop being true. So does the header's "stacked here as a third block,
      below the grid" on SIDEBAR_SELECTOR.
- [x] 0.3 MANUAL.md updated. `QUICK_DICT.md` and `README.md` never named the
      sidebar page, checked rather than assumed. Original text: `MANUAL.md` names the sidebar page "Audio" under "Audio and MIDI
      configuration" ("An **Audio** page (reached from the app's sidebar)").
      That is the page being renamed. Check `QUICK_DICT.md` and `README.md` for
      the same name rather than assuming the manual is the only place.

## 1. Sheaf: let an app rename a runtime page

Branch `fix-out-of-tree-app-gaps`, which is what `External/Sheaf` already points
at and what `jvictor0/Sheaf#9` tracks. Push to `fork`, which updates that PR.

- [x] 1.1 `RuntimeConfig::audioPageTitle`, with `<optional>` added to that
      header's includes. Original text: `RuntimeConfig::audioPageTitle`, a `std::optional<std::string>`
      (`projects/synth/include/synth/AppContext.hpp:31-40`). Unset means the
      built-in name, so every existing app is unaffected. Say in its comment
      what the field is FOR — a host whose own vocabulary already uses a
      runtime page's name — not just what it does.
- [x] 1.2 Both added beside their precedents. Original text: `SidebarSnapshot::audioPageTitle` plus
      `SidebarSurface::SetAudioPageTitle`, following
      `registeredPageTitle`/`SetRegisteredPageTitle` exactly
      (`RuntimePages.hpp:186-190,1527-1530`). Do not invent a second shape for
      the same idea.
- [x] 1.3 One site, confirmed by grep before editing: `"Audio"` as a sidebar
      label appeared exactly once in RuntimePages.hpp. Original text: `BuildSidebarTree` reads `snapshot.audioPageTitle.value_or("Audio")`
      at `RuntimePages.hpp:695`. One call site; confirm by grep that "Audio"
      as a sidebar label appears nowhere else before changing it.
- [x] 1.4 One wiring point covers both hosts: the browser runtime reaches the
      sidebar through the same `RuntimeMainComponent`
      (`include/synth/browser/BrowserRuntime.hpp`), verified by grep rather
      than wired twice. Original text: `RuntimeMainComponent` passes `config.audioPageTitle` into the
      sidebar surface where it already reads `App::Config()`
      (`RuntimeMainComponent.hpp:70`). Both the JUCE runtime and the browser
      runtime reach the sidebar through this component
      (`include/synth/browser/BrowserRuntime.hpp`), so one wiring point covers
      both — verify that rather than wiring two.
- [x] 1.5 Two tests. The unset half asserts the literal "Audio", so changing the
      default fails there instead of moving both sides together. The set half
      also asserts what must NOT change: the action dispatched, the button's
      position, the entry count and the sidebar's height. Original text: A Sheaf test: unset renders "Audio", set renders the given label.
      POSITIVE CONTROL: the unset half must fail if the default is changed, so
      assert the literal rather than comparing two snapshots that would move
      together.
- [x] 1.6 ANSWERED, and my earlier note was wrong: `runtime_main_component_tests`
      DOES build `RuntimeMainComponent.hpp` (Makefile:194 lists it as a
      prerequisite), so 1.4's wiring is covered by Sheaf's own gate rather
      than only by the frogg3rs build. 26/26 in that suite. Full
      `projects/synth` gate: 923 pass, 2 fail, both
      `braid4_*_96000hz_*_deadline` -- pre-existing timing failures on this
      machine, in DSP this change does not touch. Original text: Sheaf's own gate for the files touched. Note which suites actually
      build `RuntimeMainComponent.hpp` — `projects/synth test` does not build
      the runtime shell, so a green run there is not evidence 1.4 compiles.
      Find the target that does, or say plainly that the wiring is covered
      only by the frogg3rs build.
- [x] 1.7 Commit `ef07e3da`, pushed to `fork`. PR #9's head is that commit,
      confirmed with `gh pr view` rather than assumed. Original text: Commit on the branch and push to `fork`. Confirm PR #9 shows the new
      commit before moving on; a pin bump against an unpushed commit is a
      broken submodule for anyone else.

## 2. frogg3rs: use it, and move the sidebar

- [x] 2.1 Pin moved to `ef07e3da`. Original text: Bump the `External/Sheaf` pin to 1.7's commit, as its own step. The
      pin and the code that depends on it are separate failures if either is
      missed.
- [x] 2.2 Set for every host. Original text: `FroggersAppCore::Config()` sets `config.audioPageTitle = "Audio
      I/O"`. Not browser-only: the Audio bank is called Audio in the
      standalone too, so the collision is the same there.
- [x] 2.3 Done, and asserted in the surface test rather than eyeballed: the
      column resolves to 154 design px, which is 4 x 28 plus 3 x kGap exactly,
      and its box now ends at its last button's bottom. It leaves 299.33 px
      under it inside the chrome block -- the sidebar needs 200, or 240 if an
      app ever registers a page. The wide tree is unchanged: the column only
      exists when narrow. Original text: `AppendNarrowButtonColumn`'s column declares
      `cross = Extent::Intrinsic()`, so `froggers.layout.left.buttons`' own box
      ends where its last button ends instead of filling the block. Confirm
      the resolved height against the arithmetic (4 x 28 plus 3 x kGap) rather
      than eyeballing it, and confirm the WIDE layout is untouched — the
      column exists only when narrow, so this should be provable by the wide
      tree being unchanged.
- [x] 2.4 Done. The anchor rect is read AFTER the stacking loop applies the
      chrome block's transform, as required. The fit is checked at runtime,
      not assumed: a sidebar that would land past the chrome block's bottom
      falls back to being stacked under the grid, so it is never silently
      clipped. Measured: column ends at y=185, sidebar occupies 197-380,
      chrome block ends at 458. Original text: `mobile-stack.mjs` stacks chrome and grid, then places the sidebar at
      the button column's left edge and just below its bottom, both read live
      from that element's rect. The mount's reservation covers two blocks, not
      three.
      MIND THE CLIPPING: the mount is `overflow: hidden`, so a sidebar placed
      past the chrome block's bottom edge would be cut off rather than
      overflow visibly. Assert it fits (measured: 154 + gap + 200 against 453
      design px) rather than assuming.
- [x] 2.5 Held: `desktop-layout.spec.mjs` passes untouched, including its
      narrow round-trip. Original text: The wide path is untouched: at wide viewports the shell does nothing
      at all, and 3.4 checks the emitted tree is unchanged.

## 3. Assertions

- [x] 3.1 Done, including the below-the-column clause that separates this
      placement from the old one. Original text: e2e: each of the four sidebar buttons has its horizontal centre right
      of the BPM slider's centre, sits inside the chrome block's box, and sits
      below the last Randomize/Reset button. That last clause is what
      distinguishes the new placement from the old one — "inside the chrome
      block" alone would also pass for a sidebar overlapping the sliders.
- [x] 3.2 Done. Original text: e2e: no sidebar button falls inside the encoder grid block's box, and
      the sidebar no longer sits below the grid. State in the comment that
      this replaces the assertion 0.1 removed.
- [x] 3.3 Done -- `toHaveText(/Audio I\/O/)` on the first sidebar button. Original text: e2e: the first sidebar button's label reads "Audio I/O". This is the
      only assertion that the Sheaf field is wired all the way through to a
      rendered page, so it is the one that fails if the pin bump is missed.
- [x] 3.4 Passes untouched. Original text: The wide layout is unchanged: `desktop-layout.spec.mjs` still passes
      untouched, including its narrow round-trip.
- [x] 3.5 MEASURED at 390x844. Document height 1367 -> 1173, so the page is
      194px shorter -- and shorter than the 1218 it was before the chrome
      block was widened at all. The mount's reservation went 1448.82px ->
      1047.13px. Nothing else moved: the first encoder row is still at y=606,
      the BPM slider still 201px wide.
      One test had to be retargeted rather than kept: "controls below the
      fold are reachable by scrolling" used the sidebar as its
      below-the-fold subject, and the sidebar is now above the fold. Its
      positive control would have failed, correctly. It uses the last encoder
      row now. Original text: The page got shorter, asserted rather than claimed: record the
      document height before and after in the task notes.

## 4. Nothing else moved

- [x] 4.1 302/302, 0 failures. Original text: App suite green with counts.
- [x] 4.2 3/3. Original text: `app/vst` ctest 3/3. The plugin has no runtime sidebar; this is the
      check that the RuntimeConfig field costs it nothing.
- [x] 4.3 One republish, then 46/46 idle in 17.8s and 46/46 in 23.5s under six
      concurrent CPU burners. `[pages] blank-frame` passed in both. Original text: ONE republish, then the full e2e suite, idle and under load.
- [x] 4.4 Both clean and pushed: Sheaf `ef07e3da` on the PR #9 branch,
      frogg3rs `c39e666` with the pin moved. Original text: Both trees clean and both pushed: the Sheaf branch, and frogg3rs with
      the moved pin.

## 5. Operator

- [x] 5.1 CONFIRMED in conversation: the operator approved the placement and
      reported mobile scrolling working. The Audio I/O page was not
      separately opened on a phone. Original text: OPERATOR: spot-check the live site on a real phone — the four runtime
      page buttons beside the sliders, the Audio I/O page opening from the
      first one, and the page still scrolling.

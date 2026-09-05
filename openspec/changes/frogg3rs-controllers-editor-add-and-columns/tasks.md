# Tasks — `frogg3rs-controllers-editor-add-and-columns`

Task 1 is the preflight and is complete; its results are recorded here and in
`evidence.md`, and they rewrote the proposal. No fix outside this text: a defect
this text does not name stops execution and supersedes the change.

## 1. Preflight — complete

- [x] 1.1 Every citation verified by reading it. `kMaxBlockDomain`
      `src/MidiConfigBlocks.cpp:233`; `StartPlusCountExceedsDomain` `:238-243`;
      the refusal string `:287`; its three callers `:286`, `:320`, `:415` inside
      `ExpandEncoderBlock` `:269`, `ExpandAnalogBlock` `:307`,
      `ExpandSystemBlock` `:399`; `ExpandGridBlock` `:629` with its own
      `std::uint64_t` guard `:639`. No `MEMORY64` / `-sWASM64` in
      `app/browser/build-browser.sh` or `browser/Makefile`. Header row:
      `FieldEditorWidth` `include/synth/ControllersPageUI.hpp:552-603` returning
      58 at `:592`; `kAddButtonWidth = 62.0f` `:480`; the `0.0f` gap `:2906-2909`
      in `emitGroupHeader` `:2891-2952`; `kControllerHeaderMinWidth` `:548-549`.
      One correction: `ControllersPageSpacing()` spans `:196-211`, not `:196-202`.
- [x] 1.2 Truncation confirmed by running it, not by arithmetic on paper. One
      source file compiled twice: host `sizeof(size_t)=8`, constant 2^53, all
      cases false; wasm32 `sizeof(size_t)=4`, constant **0**, `exceeds(0,2)` and
      `exceeds(0,16)` true, `exceeds(0,0)` false. The third is the control that
      reproduces the observed empty-row/populated-row asymmetry. `evidence.md` C.
- [x] 1.3 A frogg3rs-side real-catalog test needs **no build change**.
      `app/Makefile:24` already sets `-I$(SHEAF_SYNTH_DIR)/include` and links
      `libsynth.a`, whose `SRC` list (`External/Sheaf/projects/synth/Makefile`)
      includes `MidiConfigViewModel.cpp` and `MidiConfigBlocks.cpp`;
      `nm` confirms `MidiConfigViewModel::AddBlock` and `ExpandEncoderBlock` are
      in the archive, and `ControllersPageUI.hpp` is header-only. Proven by
      compiling and linking a probe that built the real catalog: 6 device
      defaults, 6 wizard registry entries. The premise that frogg3rs "does not
      currently link the page" was wrong.
- [x] 1.4 Open question settled: the 2^53 bound has exactly one genuine
      consumer and it already holds it correctly. Searched by operand across
      both repos — `kMaxBlockDomain`, `9007199254740992`, "safe integer",
      `isSafeInteger`. C++ hits: `src/MidiConfigBlocks.cpp:233` (broken) and
      `src/MidiConfigViewModel.cpp:1816` (`kMaxSafeInteger`, a `double`,
      clamped by `std::min(kMaxSafeInteger, maxSizeT)` with the 32-bit case
      named in its comment). JS hits are a different domain — catalog file sizes
      and MIDI deadlines — and are unrelated to block positions. So the cap is
      **removed**, not retyped.
- [x] 1.5 The suite can be built and run for wasm32 today, in about 15 seconds
      per binary, with an emsdk install and node v26.7.0. Two separate
      installs exist here -- `~/Desktop/frogg3rs/.emsdk` and `~/.local/emsdk`,
      distinct directories confirmed with `realpath`; the gate uses the latter,
      which is what `browser/Makefile:1` already expects. Both `blocks_tests` and `viewmodel_tests` compile unmodified and
      already fail there. `evidence.md` D.
- [x] 1.6 §5 forward enumeration on every concept, FOUND vs CHANGED:
      - overflow guard idiom `numeric_limits<std::size_t>::max()` — FOUND 5
        (`MidiConfigBlocks.cpp:251`, `:639`; `ButtonGrid.cpp:31`;
        `ParameterModulation.cpp:443`, `:1597`), CHANGED 0. The fix joins this
        family rather than adding a sixth spelling.
      - `kMaxBlockDomain` — FOUND 4 (all `MidiConfigBlocks.cpp`: `:233` decl,
        `:235` comment, `:239`, `:242`), CHANGED 4 (all removed).
      - `StartPlusCountExceedsDomain` — FOUND 4 (def `:238` + 3 calls),
        CHANGED 1 (the body; the three call sites keep their signature).
      - 2^53 as a value — FOUND 2 in C++, CHANGED 1; the survivor is correct.
      - text-fit criterion — FOUND 1, already named
        (`visual-criteria.spec.ts:55`, implemented `:412` and `:766`), with an
        existing positive control at `:901` and an existing driven-state pattern
        at `:1022`. CHANGED 1. No second text-measurement criterion exists under
        another name; none is added.
      - gap criterion — FOUND 1 (`SpacingConformance`, called
        `juce/ControllersPageSimulationTests.cpp:247`), CHANGED 0; only its
        allowance list `ControllersPageSpacing()` changes.
      - `static_assert` in `src/` — FOUND 0. None is added; 1.9 says why.
      - real-catalog page test — FOUND 0. Sheaf's page tests pass
        `synth::MidiAppCatalog{}`, an empty catalog
        (`tests/controllers_page_ui_tests.cpp:259`).
- [x] 1.7 Sequencing settled across all five other open changes. No other change
      touches `src/MidiConfigBlocks.cpp` or `browser/tests/visual-criteria.spec.ts`.
      Three name `ControllersPageUI.hpp` but only in the per-controller-row
      header; this change edits `emitGroupHeader` (`:2891-2952`) and
      `FieldEditorWidth`'s `BlockStartPos` case (`:592`), which none of them
      cite. No line conflict, so no sequencing constraint beyond confirming
      `kControllerHeaderMinWidth` does not move. Corrections: `row-controls`
      reads **27/42**, not 8/42, and its code is delivered;
      `frogg3rs-guitar-and-solo-variants` is delivered but **uncommitted** in
      the frogg3rs working tree and must stay out of this change's commit.
- [x] 1.8 §8.0 hygiene sweep, seven directories, each named: Sheaf `src/`,
      `include/synth/`, `tests/`, `juce/`, `browser/tests/`; frogg3rs `app/` and
      this change's own directory. No dead scripts, gates or specs: all 12 `src/`
      TUs are in `SRC`, all 30 test binaries are in `make test`, all 7 juce tests
      are in the miniapp target, and every spec file is matched by a Playwright
      or node glob. Fixed inside this change: 11 stale file:line citations
      (`Engine.hpp:602`, `PortableUIBuilders.hpp:139`,
      `RuntimeMainComponent.hpp:301` and `:302`, `portable_ui_tests.cpp:1681`,
      `engine_tests.cpp:1496-1498` and `:1625`,
      `RuntimeShellSessionTests.cpp:1172` and `:1192`,
      `ui-state-before-audio.spec.ts:9-11`, `pages.yml:35`); two orphaned
      screenshot PNGs and the `.gitignore` sentence claiming they were
      referenced by name (nothing loads them: no `toHaveScreenshot`,
      `toMatchSnapshot` or name match anywhere in the tree); `app/README.md`'s
      paragraph describing a double-click action hook and two functions
      (`SetNodeAction`, `SetNodeActionAndLabel`) that exist nowhere; and
      `docs/coverage.md:279`'s clause about screenshots written by a spec file
      that no longer exists. One item reported, not changed: `app/browser/Makefile`'s
      targets are never invoked by name — CI calls the underlying scripts
      directly, so the validation runs and the wrapper is documentation.
- [x] 1.9 A `static_assert` was the first draft's guard. It is dropped, because
      it was measured and does not hold: compiled for wasm32,
      `constexpr std::size_t k = 9007199254740992ULL;` produces no diagnostic
      under `-Wall -Wextra -Wpedantic` or `-Wconversion`. Positive control: the
      list-initialised form is a hard error naming the value. `evidence.md` E.
      Task 4 replaces it with a gate that runs, not one that compiles.

## 2. Sheaf — the defect

- [x] 2.1 Remove `kMaxBlockDomain` and make `StartPlusCountExceedsDomain`
      overflow-safe on its own type, using the idiom the same file already uses
      at `:251` and `:639`. Rewrite the comment above it to say what the guard
      does and where the 2^53 bound actually lives; no history, no change names.
- [x] 2.2 No new host tests for the `Expand*` paths. `blocks_tests` and
      `viewmodel_tests` already cover all three and already name the defect on
      the target where it occurs; adding host duplicates of checks that cannot
      fail before the fix is the redundancy §5 forbids. Task 4 is what pins
      finding 1. Confirm rather than assume this: after 2.1, both binaries reach
      their host counts on wasm32 (102/0 and 147/0), which is also what proves
      no second width-dependent defect was hiding behind the 49 failures.
      Confirmed: wasm32 reaches 102/0 and 147/0, its host counts exactly.

## 3. Sheaf — the header row

- [x] 3.1 Done, and §5 changed the shape of it. `BlockStartPos` is NOT the only
      label that overruns: measuring every label the 58px case renders, at the
      header's own `600 13px system-ui`, "Bank Slot" (`BlockBankSlotIx`) needs
      **60.0px** and "Start Pos" **58.3px**, with "Start Arg" at 57.5 and
      "Grid Slot" at 55.2 within 3px of the limit. Splitting out one case would
      have fixed the instance and left its sibling broken, so the shared value
      is sized to the widest label the group renders: **58 -> 66**.
      The gap: a new `kEditorColumnGap = 4.0f`, drawn by the group header row
      AND by the mapping rows below it — both were a literal `0.0f`, and they
      must match or a header label stops sitting over the field it names.
      `fieldsWidth` and `desiredSectionWidth` now reserve the gaps they draw.
      `kControllerHeaderMinWidth` stays **724**, established rather than
      assumed: it derives from `kActiveControllerHeaderWidth` /
      `kBlacklistedControllerHeaderWidth`, whose whole input chain is the
      per-controller-row constants, and the diff touches none of them (0 lines).
- [x] 3.2 Done. Do NOT add a criterion. Extend the one that exists, in both the ways
      the preflight showed it needs.
      (a) State coverage: `openSurface()` visits page roots only, so a row
      expanded with Encoders open is never presented. Add that state, following
      the existing driven-state pattern at `:1022`.
      (b) Measurement: `scrollWidth > clientWidth + TOLERANCE` cannot see this
      defect — both read 58 for a 0.3px overflow. Measure text width in
      sub-pixel terms instead. PROVE it fails on the current 58px allocation
      before 3.1 corrects it. The measured blast radius is two nodes across four
      surfaces (`evidence.md` B2), so confirm nothing else lights up.
      Keep it a Playwright criterion: text fit needs real rendering, and the
      headless `Node` (`include/synth/PortableUI.hpp:235-274`) carries no
      text-width field. Do not write a headless proxy.
      Delivered: the driven state (row expanded, Encoders open, both "Start Pos"
      labels on screen) and a sub-pixel measurement replacing the integer
      comparison at both implementation sites, tolerance 0.1 (the existing 0.5
      would itself hide a 0.3px shortfall). The measurement that pins this is
      the lead's own, against the deployed build and recorded in `evidence.md`
      B2: for both "Start Pos" labels `scrollWidth = clientWidth = 58`, so the
      integer comparison reports no violation while the text needs 58.3px. The
      committed positive control for the criterion is `visual-criteria.spec.ts`'s
      own "the text-fit and contrast checks can actually fail".
      Two false-positive sources found by running it and
      fixed: a native button centres its caption over padding, so subtracting
      padding computed 0 available for the 22px disclosure; and a combo's
      `textContent` concatenates every option rather than the selected one.
- [x] 3.3 Answered empirically, and the answer rules the planned fix out.
      Removing `0.0f` and running the fixture produced 84 violations of the form
      "gap 0.00 between X and its leading edge" — container PADDING, legitimately
      zero — and only 5 distinct sibling-gap pairs, all deliberate
      (`row.N.line1`/`line2`, the two-line controller row, and
      `available.heading`/`available.empty`). `SpacingConformance` measures
      padding and sibling gaps against ONE permitted set
      (`tests/support/VisualCriteria.hpp:331`, leading edges recorded at
      `:383-384`), so it cannot express "zero padding yes, zero gap between a
      label and a button no" without splitting that set — and even split, the
      Controllers page still needs zero permitted for those 5 sibling pairs.
      A per-position allowance is a wider change than this one, with its own
      Impact across every page that uses the criterion. Recorded as a finding;
      NOT attempted here. What covers the defect instead: the browser-side
      criterion (3.2) and the targeted assertion in 3.5.
- [x] 3.5 Assert directly, in `tests/controllers_page_ui_tests.cpp`, that the
      Turn and Push group headers separate their last column from `add_single`,
      and `add_single` from `add_block`, by `kEditorColumnGap`. Prove it goes
      red at a zero gap. Delivered as
      `TestEncoderGroupHeaderSeparatesLastColumnFromAddButton`, asserting against
      the named constant, never a literal. Red proven twice (the harness is
      fail-fast, so one broken expectation at a time): "Turn header: last column
      to add_single keeps kEditorColumnGap" and "Turn header: add_single to
      add_block keeps kEditorColumnGap", both exit 134; green after restoring.
- [x] 3.4 Width sweep `[480 .. 1280 step 80]`, chosen so 480 sits inside the
      shrink-transform zone and the step crosses `COMPOSITE_SURFACE_WIDTH = 736`
      (where `fitSurface` engages) between 720 and 800; 1280 is the file's
      standard viewport. Adds under a second of wall clock.

## 4. Sheaf — build the suite for the target that ships

- [x] 4.1 Added a wasm32 gate to `External/Sheaf/projects/synth/Makefile`: build
      `blocks_tests` and `viewmodel_tests` with `emcc` and run them under node,
      inside `make test`. It must FAIL LOUDLY when `emcc` is absent, not skip —
      a silently skipping gate is the shape of the hole this change closes.
      It is a PREREQUISITE of `test`, not a line in its recipe, and deliberately:
      the recipe aborts partway on this machine (the two carried 96kHz deadline
      tests fail at position 14 of 30, and everything after them is skipped), so
      a gate placed at the end of the recipe would never run here. As a
      prerequisite it runs every time. The cost is that its ~31s precedes the
      host tests' output.
- [x] 4.2 Proven live in both directions, by restoring the pre-fix guard and
      running the gate: **exit 2, 79 pass / 23 fail** (it stops at
      `blocks_tests`, so `viewmodel_tests`' own 26 do not get to run). With the
      fix: **exit 0, 249 pass / 0 fail**, 31 seconds for both binaries including
      their builds. `em++` rather than `emcc`: `emcc` does not link the C++
      runtime, so a harness that throws fails at link time — found by running
      it. The vendored toolchain is preferred over one on
      PATH, the reverse of `browser/Makefile`'s order and deliberate: another
      emscripten on PATH links against its own sysroot and fails here.

## 5. frogg3rs — the catalog gap

- [x] 5.1 Added a Controllers page test under `app/` that installs a row from
      frogg3rs's REAL catalog — `FroggersMidiCatalog()`, not Sheaf's empty
      `synth::MidiAppCatalog{}` — and drives Add and Block. Per 1.3 this needs
      no build-system change beyond a new target in `app/Makefile`, wired into
      `app/Makefile`'s `test`. It runs on the host, where it cannot fail before
      the fix; its value is that nothing in frogg3rs exercises the shipping
      Delivered as `app/FroggersControllersPageTests.cpp`, wired into
      `app/Makefile`'s `test`. Control run by the lead rather than relayed:
      replacing both `FroggersMidiCatalog()` calls with `synth::MidiAppCatalog{}`
      gives exit 1, both tests failing at `:98` and `:124`; restoring gives exit
      0, both passing. The restore needed the binary removed first — `make`
      no-opped on same-second mtimes and re-reported the break, which is the
      trap that makes a break/restore pair lie.
      Running it surfaced four genuine refusals, each encoded as an expectation
      only after reading the source line that produces it: Twister's six factory
      side buttons leave none free for a new System row; Launchpad has no
      encoders and Twister/Launchpad no analog input, so `KindSupport` refuses
      both; and a Launchpad System block always collides with the shared pad
      map's transport row. One self-inflicted bug found the same way: sharing a
      view model between the Add and Block checks let an accepted Add starve the
      Block of address space, so each now rebuilds from the untouched instrument.

## 5b. §8.0 — the browser suite was already red

- [x] 5b.1 `browser/tests/visual-criteria.spec.ts` was **8 failed / 8 passed** at
      448dee5d, before this change touched it.
      `frogg3rs-controllers-page-name-in-the-editor` removed
      `runtime.controllers.add_name` and `add_kind` from the C++ at **c81727b9**
      (both traced with `git log -S`) — the add row is now a preset combo and an
      Add button — and left the inbound half unenumerated. Found by the §8.0 sweep of `browser/tests/`, which this
      change's Impact names. Repaired, each verified against the C++ first:
      `seedControllers` filled a name field that no longer exists; two caption
      exceptions named both removed ids; a per-row `rename_draft` exception named
      a control that now lives in a row's expanded editor and carries its own
      "Name" caption; and the form-control count was pinned at
      `FIXTURE_CONTROLLER_COUNT * 3 + 2` describing the old add row, against an
      actual 25 (`* 2 + 1`). Separately, `SPACING_METRIC_VALUES` was missing
      `16` although `ControllersLayout::kStatusLegendPairGap = 16.0f` names it
      and the headless allowance already permits it — the cross-language
      permitted-spacing pair had drifted, which is the §5 family this change's
      own spec requirement is about. Now **16 passed / 0 failed**.
- [ ] 5b.2 REPORTED, NOT REPAIRED — and it is bigger than it first looked.
      `browser/tests/fake-app.e2e.spec.ts` is **10 failed / 7 passed**, red since
      c81727b9. Seven of the ten are `controller wizard ...` tests, whose repair
      means re-deriving the wizard flow under the one-control-per-job row. That
      is a design ruling belonging to
      `frogg3rs-controllers-page-name-in-the-editor` and
      `frogg3rs-controllers-page-row-controls`, not to this change: §8.0 says a
      broken consumer presents a fork — restore it or remove it — and that repair
      gets chosen without the fork being noticed because it feels like care.
      Attribution established rather than assumed: this change's diff touches
      that file 0 times, its own last edit is 8e84eadb, and `git log -S` puts
      both removals at c81727b9.
      The narrow part, for whoever takes it: `:541-542` drives `add_name` and
      `add_kind`; under the new add row the nearest equivalent is a
      "Custom (<kind>)" preset (`include/synth/ControllersPageUI.hpp:919,937`).

## 6. Gates

- [x] 6.1 All gates run 2026-09-04, after every source change; none carried
      forward stale.
      - Sheaf `nice make -j2 test`: **1194 pass / 2 fail**, exit 2. The two are
        the carried 96kHz Braid 4 deadline tests, which fail deterministically on
        this machine and are not chased.
      - That recipe aborts at those tests, at position 14 of 30, so the 16
        binaries after them never ran under it. Run by path instead, all exit 0:
        `reconcile`, `reconcile_executor`, `poller`, `midi_sender`,
        `viewmodel` (147), `blocks` (102), `portable_ui`, `portable_ui_layout`,
        `runtime_main_component`, `runtime_file_service`, `controllers_page_ui`,
        `browser_runtime_contract`, `browser_command_buffer`,
        `browser_audio_device`, `browser_midi_bridge`, `controller_wizard` (27).
      - wasm32 gate: **249 pass / 0 fail** (it is a prerequisite, so it ran
        before the recipe aborted).
      - miniapp JUCE `nice make -j2 -C apps/miniapp test`: exit 0, every
        simulation passed including `ControllersPageSimulationTests`.
      - frogg3rs `nice make -j2 -C app test`: exit 0, **347 pass / 0 fail**,
        including the new `froggers_controllers_page_tests`.
      - Browser `npx playwright test tests/visual-criteria.spec.ts`:
        **16 passed / 0 failed** (was 8 failed / 8 passed before this change).
      - `npx tsc -p tsconfig.json --noEmit`: clean.
      - Not green and not this change's: `browser/tests/fake-app.e2e.spec.ts`,
        10 failed / 7 passed, red since c81727b9 — see 5b.2.

## 7. Postflight

- [x] 7.1 Fresh-context reviewer run over the whole diff. One real defect and
      several imprecisions; all fixed:
      - **The spec delta promoted a requirement this change does not deliver** --
        "A page's permitted spacing values distinguish design from defect", whose
        Check cited a narrowed allowance, while 3.3 had already ruled that change
        out and `ControllersPageSpacing()` is untouched. REMOVED from the delta;
        the proposal's matching Design sentence and Impact entry corrected. This
        is precisely the defect §9 names: grammar asserting behaviour nobody
        checks.
      - Scenario 1's Check credited the wasm32 gate with 26 `viewmodel_tests`
        failures. The gate is fail-fast and stops at `blocks_tests`; that figure
        comes from building the binary for the target directly. Reworded.
      - Two claims were relayed from subagents rather than run by the lead. Both
        are now the lead's own: 5.1's control re-run, 3.2 rewritten onto the
        lead's own measurement.
      - The 64-bit acceptance band this fix widens was implicit. Now stated in
        the proposal, including that no test covers it.
      - The Makefile comment claimed em++ is resolved "the same way
        browser/Makefile resolves it". It is the reverse order, deliberately:
        `browser/Makefile:8` prefers PATH, this gate prefers the vendored
        toolchain, because another emscripten on PATH links against its own
        sysroot and fails here. Corrected to say what it does and why.
      - 1.5 called the two emsdk directories "the same install". They are not.
        Corrected.
      - Reported, NOT fixed: **208 planning labels** in comments across the tree
        (`src` 6, `include/synth` 75, `tests` 62, `juce` 27, `browser/tests` 38)
        -- "Task N", "sprs-N", "sru-N", "Finding N". Two sit beside lines this
        diff touched, but fixing two of 208 makes a file look swept when it is
        not. The 8 in the two files this change edits substantively were
        stripped; the rest is its own change.
      - Confirmed clean: no other divergence between diff and artifacts;
        `kEditorColumnGap` FOUND 12, all in-diff; `WASM32_*` FOUND only in
        Sheaf's Makefile; no new comment-hygiene violations; the scope boundary
        against `guitar-and-solo-variants` exact.

## 8. Operator

- [ ] 8.1 On the deployed build: Add and Block work on a Twister row, on a
      Generic row and on an empty Custom row.
- [ ] 8.2 The Encoders header reads in full — "Start Pos" is not ellipsised —
      and the Add and Block buttons stand clear of it.
- [ ] 8.3 Anything that was already mapped still works: the refusal blocked
      edits, so confirm existing mappings survive the fix.

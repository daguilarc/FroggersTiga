# Tasks — `frogg3rs-gui-and-dsp-robustness`

## §0 Standing constraints — apply to every task below

- **Subagents: Sonnet or Haiku, never Opus.** Set the model explicitly on every dispatch; an
  omitted model inherits the session's expensive one (OMNI §4, §15).
- **Builds capped at `-j2` with `nice`** (8-core / 16 GB). Use `./app/build-launcher.sh` for the
  launcher — never a hand-written `make` line. It globs all 18 headers; a hand-written list once
  tracked 4 and silently ignored edits to all of `app/dsp/`.
- **Build/test runs go through a subagent** reporting pass/fail counts and failure tails only —
  never full logs (OMNI §16.1).
- **`External/Sheaf` stays pinned at `1940ddcb` and clean.** No task here may modify it. Anything
  needing a Sheaf change goes to `UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees stay byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No `Co-Authored-By` or AI attribution on any commit.**
- **Parallel subagents are permitted for ANALYSIS only.** Code changes are sequential unless the
  lead has fully mapped dependencies AND the operator has reviewed and accepted that map (OMNI §4).
- **No task is complete on unit tests alone** (design E5). User-visible tasks require GUI
  confirmation.

## §0-bis Audit correction, 2026-07-28 — READ THIS BEFORE PICKING A TASK

The checkbox state in this file was **stale**: §1 and all of §3 had landed in the tree while every
box still read `[ ]`. An agent reading top-down would have re-implemented finished work and fought
§6's reversals. Boxes below are now set from the tree, each with the evidence that closed it
(design E0 has the same table). Two of them are marked `[x]→` because §6 deliberately reverses
them; do not "restore" the `[x]` version.

**Execution order is NOT file order.** It is:

1. **§6** — current work, in its own order (6.1 → 6.2 → 6.3/6.4 → 6.5 → 6.6 → 6.7 → 6.8 → 6.9)
2. **§1.2, §1.4** — the two §1 tasks that did not land (1.4 is executed *as* 6.1)
3. **§2** — DSP recovery, except 2.2 which 6.5 pulls forward (see 6.5)
4. **§4, §5**

§6 is placed first in this file for that reason.

---

## §6 2026-07-28 GUI-test remediation (design E6) — **CURRENT WORK**, takes priority over §1–§5

Ordering constraint: 6.1 (sweep) and 6.2 (defaults) first — they gate everything audible. No task
in this section is closeable without pixels/ears on the running app (E6-F6; no exceptions).

- [x] 6.1 **RUN task 1.4's call-site sweep NOW, before any further feature work.** Deliverable: the
  full unit→method→called-in-production table, every "no" triaged. This task **is** 1.4 — closing
  it closes 1.4.
  **DONE 2026-07-28.** Two reports, both spot-checked against the code by the lead before
  acceptance:
  - `reports/6.1-call-site-sweep-dsp.md` — 45 methods across `app/dsp/`: **44 OK, 1 flagged.**
    Only flag is `StereoDelay::ClearBuffers` (test-only), triaged benign: its sole production
    sibling `SetSampleRate` (`app/FroggersAppCore.hpp:237`) already zeros both lines, `writePos`
    and `lfoPhase` at the one point in the lifecycle that matters. **The ported DSP is clean** —
    including the `scoopNotch` setters, re-verified fixed.
  - `reports/6.1-call-site-sweep-sheaf.md` — differential vs Braid 4 / Sheaf reference:
    **3 MISSING**, listed in 6.11. Everything else OK or justified N/A.
  **Two results that change other tasks:**
  1. **F5 is REFUTED** — the S&H visualizer attachment is fully intact. See 6.8.
  2. **A new real defect was found** that no operator report had surfaced:
     `ParameterGroup::ConfigureProcessingTiming` is never called. See 6.11.
  **Scope limit, recorded so nobody over-trusts this sweep:** it answers *"is the method called"*,
  not *"is it called with a sensible value."* F1 (pitch knobs left at 0.0 because
  `ApplyFroggersDefaultPatch` never assigns those three slots) is the same defect class in a shape
  no call-site table can catch — `HandleSetAbsolute` **is** called, just never for slots 0-2.
  **6.2 must cover the value side on its own; it cannot lean on this sweep.**
- [x] 6.2 **Defaults/ranges audibility audit + fix (operator directive, E6-F1).**
  **DONE 2026-07-28**, pending the operator hearing it (6.9). Production change is one line:
  `.defaultValue` 0.2468/0.3471/0.4058 on the three Audio pitch specs
  (`app/FroggersParameters.hpp:155`), mirroring the Sustain precedent at `:160-162`. `.defaultValue`
  was traced through to the live knob before relying on it, not assumed.
  Test-first honoured: `default_patch_has_audible_band_energy_above_150hz`
  (`app/FroggersAudioRoutingTests.cpp:231`) measured band energy **2.0e4 → 5.1e5** across the fix,
  over the real engine path with the transport started via `MessageIn::Start`. Full suite green:
  128 tests across 10 binaries, `nice make -j2 test` exit 0.
  Audit table for all six banks: `reports/6.2-defaults-audit.md`.
  **Answers to the two semantics questions, read from the DSP:** Delay "Wet mix" = 0 →
  `ToReverbMono` (`app/dsp/Delay.hpp:151-156`) returns `(1-mix)*dry + mix*wet`, so 0 **passes dry
  through**, it does not mute. Reverb "Wet/dry" = 0 → `Reverb::Process`
  (`app/dsp/Reverb.hpp:198-199`), same shape, so 0 is **fully dry**. Neither is inverted.
  **⚠️ FOR THE OPERATOR (6.9) — a second instance of F1's shape, found by the audit, NOT fixed:**
  the default patch authors Drive at 20% (`app/FroggersModulation.hpp:961`), but Drive's **Blend**
  (slot 7) defaults to 0.0 and `DriveBlendPhase::Process` returns `dry*(1-blend) + phased*blend`
  (`app/dsp/Drive.hpp:294`) — at blend 0 the wet path is crossfaded out entirely, so **the authored
  Drive setting is currently inaudible.** Same defect shape as the pitch bug: authored intent that
  never reaches the audio. Left unfixed deliberately — what Blend should default to is a voicing
  decision, not a correctness one. Operator decides.

- [x] 6.12 **Harden 6.2's audibility assertion (review finding, not a defect).**
  **DONE 2026-07-28** — the magic threshold is gone; the test now sums `GoertzelPower` at the
  expected fundamentals (110/220/330 Hz) against the broken ones (20/40/60 Hz) and asserts a 10×
  ratio. `BandEnergy()` lost its only caller and was removed.
  **Why it was needed:** the pre-fix figure was **not** near zero — a 20 Hz saw/square has harmonics
  reaching well into the 150-2000 Hz band — so the original assertion discriminated by a ~25x ratio
  against a calibrated constant with only ~5x margin, rather than by presence/absence. The bug was
  never "no energy above 150 Hz", it was "the fundamentals are in the wrong place", and the
  assertion now says exactly that.

  *(Housekeeping note: an earlier revision of this file accidentally spliced task 6.2's original
  body onto this entry while marking 6.2 done. Removed 2026-07-28 — 6.2's own entry above is
  complete and authoritative.)*
- [x]· 6.3 **Restore single-click bank switching (E6-F3.1, reverses part of 3.1).**
  **IMPLEMENTED 2026-07-28, NOT CLOSED** — `·` means code landed, pixels unseen. Only the operator
  or a screenshot closes it (E6-F6).
  `AppendBankHeader` (`app/FroggersUiSurface.hpp:603`) emits `builder.Button` with the action wired
  directly; `MarkSelectedBank()` (~`:714`, after `builder.Build()`) sets `node.selected`. No marker
  character; labels still from `FroggersBankLayouts()`. Suite green 128/128. Bank buttons
  return to `Button` nodes; selection shown by post-`Build()` `node.selected = true` (background
  inversion via `ButtonColourForNode`, `PortableJuceBackend.hpp:1130-1149` at the pin). Labels still derived
  from `FroggersBankLayouts()`. No marker character. Full text inversion resumes when upstream
  plain-click + selected-text support land — **neither has**, see 6.4 and design E7. Update the
  3.1 test accordingly.
- [x]· 6.4 **Transport single-click — DECIDED by the audit; no upstream check needed.**
  **IMPLEMENTED 2026-07-28, NOT CLOSED** — pixels unseen. Play/Stop are labelled `Button` nodes
  (`app/FroggersUiSurface.hpp:506-507`). `BuildPlayDrawCommands`/`BuildStopDrawCommands` kept
  unreferenced for the day upstream lands plain-click (they are `inline`, not `static`, so no
  unused-function warning). `SetNodeActionAndLabel` lost its last caller and was removed;
  `WireDrawNodeActions` now wires only the encoder grid. Suite green 128/128.

- [x]· 6.13 **Layout fallout from 6.3/6.4 — REOPENS 3.7.**
  **IMPLEMENTED 2026-07-28, NOT CLOSED** — needs a human to see the window.
  Computed extent: **2 rows, 64 px**. Row 1 fills to exactly 876 px (= 900 − 2×12) with Play 72,
  Stop 72, Audio 72, Envelope 76, Filter 72, Drive 72, Delay 72, Reverb 72, Randomize Page 115,
  Randomize All 109 + 9 gaps; row 2 takes Scene 1/2, both blend nodes, both BPM nodes. Reserved for
  the **stopped** BPM label (201 px), the wider of the two states.
  `RequiredHeight()` 596 → **632** (`app/FroggersAppCore.hpp:184`), now computed by
  `FroggersAutoFlowedChromeModel` (`app/FroggersUiSurface.hpp:152`), which replicates the backend's
  sizing/wrap rules with per-line citations and reads bank labels from `FroggersBankLayouts()`
  rather than a second list.
  The two dead strips are gone; scope gained 54 px and grid 96 px — which also fixed a
  **pre-existing** scope/grid height mismatch (grid had been losing both strips, scope only one).
  Test strengthened to assert row count and total height, not two app-side numbers agreeing with
  each other, plus a case that fails if the 1-row assumption returns. Suite green 130/130.
  Verification note: the agent checked its arithmetic with a real C++ program rather than Python,
  because banker's rounding disagrees with `std::round` on the 108.5/200.5 boundary cases — the
  two widths that decide where row 1 breaks.

- [x] 6.14 **Cache the auto-flow model (review finding, not a defect).**
  **DONE 2026-07-28** — `FlowedControls()` returns `const std::vector<ControlSpec>&` to a
  function-local `static const` built once; `ComputeFlowExtent` stays per-call because it is
  width-dependent.
  **Why it was needed:** `ContentArea` calls `ComputeFlowExtent`, and `BuildTree` calls
  `ContentArea` once per UI frame at `uiFrameHz = 30`, so the app was heap-allocating ~16 strings
  30×/second to recompute a value that cannot change — the control set is static. OMNI §10: an
  O(n) result reused every frame must be cached.

  *(Housekeeping: an earlier revision spliced 6.13's body onto this entry; removed 2026-07-28.
  6.13's own entry above is complete.)*
  1. **Two reserved strips are now blank.** `kTransportHeight` (40) and `kBankHeaderHeight` (28),
     plus gaps, still push the scope/grid areas down (`app/FroggersUiSurface.hpp:236-239, 284-287`),
     but nothing draws in them any more — the controls that used to occupy them are now auto-flowed
     elsewhere. Collapse the dead strips, or give them back to the scope/grid.
  2. **The auto-flowed chrome went from 6 controls to 14** — Play, Stop, 6 bank buttons, Randomize
     Page, Randomize All, Scene 1, Scene 2, Scene blend, BPM. `Button` nodes carry no app-computed
     bounds, so the runtime flows them below the lowest explicitly-bounded `Draw` node
     (`PortableJuceBackend.hpp:771-783` computes the cursor start from `maxDrawBottom`), i.e. below
     the encoder grid. **Mechanism verified; the resulting height is NOT measured.** With 14
     controls wrapping at 900 px it very likely exceeds `uiHeight = 596` and re-triggers the
     clipping 3.7 fixed. **Measure before assuming either way.**
  3. **6.6 adds two more** (`Label` nodes for BPM and Scene blend) → 16. That is why this task waits
     for 6.6: derive the height once against the final control set.
  Re-run 3.7's derivation and its `uiHeight >= computed extent` assertion. **GUI confirmation
  required** — this is a "look at it" task by construction. design E7:
  `origin/main` is now `1dd4d275` and `RetainedDrawComponent` still dispatches only from
  `mouseDoubleClick`. **Plain-click has NOT landed.** Therefore take the "not landed" branch:
  Play/Stop revert to labelled `Button` nodes (function over cosmetics, operator's priority stated
  2026-07-28). **No pin bump, so no operator approval is required.** Keep
  `BuildPlayDrawCommands`/`BuildStopDrawCommands` (`app/FroggersUiSurface.hpp:344-382`) in the file
  unused-but-present — they are what gets restored when upstream lands it. Exit criterion:
  single-click Play and single-click Stop both work in the running app.
- [x]· 6.5 **Stop kills tails (E6-F3.2).** **IMPLEMENTED 2026-07-28, PARTIAL — see 6.15.**
  `Reverb::Reset()` added (`app/dsp/Reverb.hpp:115`); `StereoDelay::ClearBuffers()` reused, not
  duplicated. Single call site on the running→stopped edge **inside `ProcessBlock`, on the audio
  thread** (`app/FroggersAppCore.hpp:451-456`), reusing the already-computed
  `transportQuarterNotes.has_value()` rather than querying the clock twice — deliberately not in
  the UI-thread `kStop` handler, which would race the audio thread on those buffers.
  Test `stopping_transport_silences_self_sustaining_delay_and_reverb` fails before, passes after;
  measured ~−73 dBFS against a −60 dBFS floor. Suite green 131/131.

- [x]· 6.15 **Stop is incomplete for long-release patches — and that is the operator's own repro.**
  **IMPLEMENTED 2026-07-28.** `VcoAdsrState::AllIdle()` added; `ProcessBlock` now keeps clearing
  delay/reverb while stopped until every voice is Idle. Failing test first:
  `stopping_transport_silences_self_sustaining_delay_and_reverb_with_long_release` sets Release
  rows 2/5/8 = 1.0 (~10 s) plus 0.98 feedback with delay time ~96 ms, so the unaided decay is
  ~32.7 s — "tens of seconds", matching the operator's report. Failed pre-fix, passes post-fix.
  **Refined by 6.16** — the clearing frequency chosen here (every block) was my specification
  error; see below.

- [x]· 6.16 **Clear once at AllIdle, not every block (lead's specification error, corrected).**
  **DONE 2026-07-28.** 6.15's per-block clearing was wrong on two counts, both mine: ~1876 full
  `std::fill` wipes per 10 s release inside the audio callback (~100 MB/s of memset), and it erased
  the delay repeats *of the release itself*, so a note releasing after Stop played dry while the
  same note releasing with the transport running played wet.
  Now: clear exactly **once**, the instant `AllIdle()` first turns true while stopped. **1876 → 1.**
  **Edge case the implementer found that my spec missed:** if the transport stops during the closed
  half of the gate cycle with every voice already released, `AllIdle()` is *already* true at the
  edge, so a pure false→true transition would never fire and nothing would ever be cleared. The
  edge site therefore keeps its own conditional clear — immediate if already Idle, deferred
  otherwise (`app/FroggersAppCore.hpp:483-537`). Pending is cancelled if the transport resumes, so
  a legitimately playing delay is never wiped.
  Both existing stop tests pass **unmodified**; a new assertion proves the release still rings wet
  0.3 s after Stop, which is the difference between the two policies and was previously untested.
  6.5's reset is **one-shot on the transport edge**, which is correct only while the envelope
  release is short. Verified gap:
  - `VcoAdsrState::kMaxReleaseSeconds = 10.0f` (`app/dsp/VoiceEnvelope.hpp:36`), and closing the
    gate puts each voice into `Stage::Release` (`:71`) rather than silencing it.
  - So after Stop the voices keep sounding for **up to 10 seconds**, feeding the delay and reverb
    *after* the one-shot reset has already fired. At the randomizer's near-unity feedback (0.98)
    the re-excited delay then needs on the order of tens of seconds to reach −60 dBFS.
  - 6.5's test sets **zero** Envelope-bank parameters (`grep -c "BankId::Envelope"` in that test
    returns 0), so Release sits at its ~0 default and the re-excitation is the ~24-sample tail the
    implementer observed. **The passing test and the defect do not overlap.**
  This matters because the operator's original report was *Randomize All → cannot stop*, and
  Randomize All can set Release long. The scenario the fix does not cover is the scenario that was
  reported.
  **Decide the policy, then implement:** either (a) keep clearing delay/reverb while the transport
  is stopped until every ASR voice reaches `Stage::Idle`, or (b) force the voices to Idle on Stop
  and accept the cut-off (risks a click, and discards release as a musical feature). (a) preserves
  the release and still guarantees silence; prefer it unless tracing shows a reason not to.
  **Failing test first, and it must set a LONG release plus 0.98 feedback** — the combination 6.5's
  test omits. On Stop: close the gate AND reset StereoDelay + Reverb
  (not the VCOs/filters — no need). Failing test: randomize until delay/reverb self-sustain, Stop,
  assert output decays below -60 dBFS within ~250 ms and stays there. GUI confirmation: press
  Stop, hear it actually stop.
  **Dependency carve-out (audit):** this needs `Reset()` on `StereoDelay` and `Reverb`, which is
  part of task **2.2** — a §2 task, and §6 otherwise precedes §2. Resolve by pulling *only* the
  `StereoDelay`/`Reverb` half of 2.2 forward into 6.5; the rest of 2.2 stays in §2. Note
  `StereoDelay` already has `ClearBuffers()` (`app/dsp/Delay.hpp:224-231`) — reuse it rather than
  adding a second reset entry point. `Reverb` genuinely has none.
- [x]· 6.6 **Visible labels for BPM and Scene blend (E6-F2, reopens 3.6).**
  **IMPLEMENTED 2026-07-28, NOT CLOSED** — the whole point of this task is that text must be
  *rendered*, and no one has looked yet. `kSceneBlendLabel`/`kBpmLabel` `Label` nodes emitted
  immediately before their sliders in `AppendChromeBand` (`app/FroggersUiSurface.hpp`); the
  builder-order → flow-order adjacency claim was traced to `PortableJuceBackend.hpp:754-798`, not
  assumed. Slider `label` kept, since it still feeds the accessible name via `setName`.
  The new tests assert the Label nodes **exist and are adjacent** — they do **not** prove the text
  renders. Closing this on those assertions would re-commit exactly the F2 error. Suite green
  130/130. Slider-label gap filed as item 6 in `UPSTREAM-SHEAF-ASK.md`. Slider labels are never
  rendered (`setName`, `PortableJuceBackend.hpp:1231`; still true upstream — design E7). Add
  adjacent `Label` nodes ("BPM", "Scene blend"); move the "(no effect while stopped)" annotation
  into the Label. Test asserts the Label NODES exist; **closure requires a screenshot showing the
  text** — a field assertion re-commits F2's error. Add the Slider-label gap to
  `UPSTREAM-SHEAF-ASK.md`.
  **Cross-task consequence (audit):** the chrome band is auto-flowed, so two new Label nodes
  consume flow width and may push it onto another row. **Re-run 3.7's height derivation and its
  `uiHeight >= computed extent` assertion as part of this task** — do not assume `uiHeight = 596`
  (`app/FroggersAppCore.hpp:177`) still holds.
- [ ] 6.7 **Scope overhaul (E6-F4, remedy revised by the audit — read E6-F4-bis first).**
  **6.7a — the bottom band is a tap-point fix, not an open question.** (Revised after operator
  challenge: an earlier version of this task asked the operator to choose a different signal. Wrong
  diagnosis — the follower is fine, it is tapped in the wrong place, exactly like the top band.)
  The existing `VcoEnvelopeFollowers` read `vco1Raw`/`vco2Raw`/`vco3Raw`
  (`app/FroggersModulation.hpp:389-390`), i.e. the raw oscillator **before** the ASR — and
  `dsp::Vco::Process` has no amplitude term at all (`app/dsp/Vco.hpp:149-159`), so that input is
  constant-amplitude by construction and the follower settles to a fixed level (≈8.7% ripple at
  110 Hz, ≈3% at 330 Hz). Feed the **display** followers the post-gate per-voice values that 6.7c
  already extracts for the top band; the bottom band then shows real ASR contours.
  **Do NOT re-tap the existing followers** — `vco1EfSource_`…`vco3EfSource_`
  (`app/FroggersModulation.hpp:391-393`) are registered modulation sources (D5 slots 9-11) and
  changing their input is a DSP parity break for a display change. Use separate instances;
  `dsp::SingleEnvelopeFollower` (`app/dsp/EnvelopeFollowers.hpp:66`) is already the one-channel
  form of the same ported formula, so this is three instances of existing code, not new DSP.
  **6.7b:** reconcile ONE built visualizer node (`app/FroggersUiSurface.hpp:526`) vs TWO visible
  panels. E6-F4 names the two code-side candidates (`DrawCommandsLookLocal` mis-classification;
  encoder-cell visualizer underlays) — check those against the running GUI rather than looking
  open-endedly.
  **6.7c:** implement via the **generic `ScopeVisualizer`, not hand-built Draw commands** — E6-F4-bis
  shows all four operator requirements are reachable without app-side rasterization, and hand-built
  commands would contradict this change's own `froggers-vco-topology` delta. Move the scope write
  to the post-gate per-voice values (extend `MixOscVoices` with an out-parameter rather than
  re-applying `adsr.apply` at a second call site — OMNI §8); add a second `ScopeWriter` + layer
  states + `builder.Visualizer` node for the bottom band; set per-layer `scopeColor` to
  blue/yellow/magenta; port the skipped marker recording (`app/dsp/Vco.hpp:47-52`) using
  `ScopeWriterHolder::RecordStart/RecordEnd` (`DspScope.hpp:242-251` — no Sheaf change).
  GUI confirmation with the operator: lines flat when silent, distinct bands, stable waveform when
  playing.
- [ ] 6.8 **S&H dice-roll motion on mod pages (E6-F5) — HYPOTHESIS REFUTED, RE-SCOPED, DEFERRED.**
  The 6.1 sweep traced the whole chain and it is **intact end to end**, so there is no attachment
  to fix. Verified by the lead, not just reported: the type is `RandomShLaneVisualizer` (not
  `RandomShVisualizer` — the earlier note had the name wrong); five instances are constructed bound
  to their UI states (`app/FroggersModulation.hpp:232-236`); they are handed to the slate
  registration with matching per-lane colours (`:504-511`); `PopulateUiState` runs **per block**
  for every lane (`:432`, inside `PublishUiState()`, called from `app/FroggersAppCore.hpp:494`);
  and `ParameterConfig::visualizer` is populated at registration
  (`app/FroggersParameters.hpp:329`). Nothing is dropped.
  **Therefore the symptom has a different cause. Do not write code for this task yet.** The
  leading hypothesis is that it is not an independent defect at all but a **consequence of F3.1**:
  the modulation detail page is reached by drilling in via encoder press, and encoder press is
  double-click-only at this pin (E7). An operator who reported "cannot switch pages by clicking"
  may never have reached the page the visualizer renders on.
  **Sequencing:** re-observe after 6.3/6.4 land and 6.10 has told the operator to double-click the
  encoders. If the dice roll animates then, close this as "was F3.1". If it still does not, open a
  fresh investigation with the chain above already excluded — start at whether the lane is being
  clocked at all, not at the wiring. GUI confirmation required either way.
- [ ] 6.9 **Postflight (OMNI §14) for this section: operator walkthrough.** Every E6 item
  demonstrated fixed in the running app, by the operator, before this section is called done.
  No self-certification.
- [ ] 6.10 **Disclose the encoder double-click limitation to the operator (audit, design E7).**
  Not a fix — a disclosure. Encoder press/drill-in is double-click-only for the same reason
  Play/Stop and the banks were (`SetNodeAction` sets `doubleClickAction`,
  `app/FroggersUiSurface.hpp:701-708`), but encoders **cannot** revert to `Button` nodes because
  they are custom-drawn and need bounds. It is unfixable app-side at pin `1940ddcb`. Tell the
  operator explicitly, confirm they accept it as a known limitation, and confirm
  `UPSTREAM-SHEAF-ASK.md` item 1 states that encoders — not just the transport — depend on it.
  **Also tell them to double-click the encoders when re-testing**, because 6.8 now depends on the
  operator actually reaching the modulation detail pages.

- [x] 6.11 **Fix the three dropped Sheaf call sites found by the 6.1 sweep.**
  **DONE 2026-07-28** (2a and 2b; 2c belongs to 6.7c and was deliberately excluded).
  **2a:** `ConfigureProcessingTiming` now called from `PrepareToPlay` against the **host** sample
  rate — traced that this app has no parameter-tier oversampling, unlike Braid 4, and that
  `modulation_.Init(parameters_.Group())` shares the same `ParameterGroup`, so one call covers
  both. Failing test at 96 kHz first: `processLiteAlpha` 0.122694 (the 48 kHz constant) vs
  0.063354 expected — roughly half, as a one-pole at double the rate should be.
  **2b:** `gangedRandomLfo6_.SetVoiceColor(0, LaneColor(5))` added in `RegisterSources()`. The test
  drives the LFO and compares a **rendered** `DrawCommand.color` against the registered
  `sourceColor` — a render assertion, not a field assertion, which is the F2 lesson applied. None of these came
  from an operator report — the sweep found them, which is the whole reason it exists. Do them
  together; all three are small.
  1. **`ParameterGroup::ConfigureProcessingTiming` is never called** — the real find, and a textbook
     instance of the class. Sheaf's parameter-smoothing constants are defined **at a 48 kHz
     reference** and are meant to be rescaled per host sample rate: `kDefaultProcessLiteAlpha`
     is commented "1 kHz one-pole cutoff at 48 kHz" and `kDefaultUiDisplayCenterAlpha` "about
     10 Hz at 48 kHz" (`External/Sheaf/…/ParameterModulation.hpp:170-173`). `ParameterConfig`
     defaults them to exactly those 48 kHz values (`:199-202`), and
     `ConfigureProcessingTiming` (`src/ParameterModulation.cpp:859-865`) is what replaces them.
     Braid 4 calls it on every group from its sample-rate hook, converting each constant with
     `ConvertOnePoleAlpha`/`ConvertSampleInterval` (`Braid4Core.hpp:205-220`). **Froggers never
     calls it at all** (`grep ConfigureProcessingTiming app/` → nothing). Consequence: knob glide,
     modulation-depth smoothing and UI-display slew run at the wrong real-time rate at any host
     rate other than 48 kHz — no audible effect at 48 k, ~9% off at 44.1 k, **2× off at 96 k**,
     4× at 192 k. Fix: mirror Braid 4's block in `PrepareToPlay`. Note Braid 4 converts against its
     *internal* (oversampled) rate; this app has no oversampling at the parameter tier, so convert
     against the host rate — state which you used and why.
     Failing test first: drive the group at 96 kHz and assert the configured alpha differs from the
     48 kHz default by the expected ratio.
  2. **`GangedRandomLfoProcessor::SetVoiceColor` is never called** for `gangedRandomLfo6_`. Sheaf's
     own `StandardModulators::Init` calls it for every such processor
     (`StandardModulators.hpp:128`). Consequence: the "Random S&H 6" lane's visualizer renders in
     the packed default **Grey** while lanes 1-5 carry `LaneColor(0..4)` — one mod source visibly
     the wrong colour. The registered `sourceColor` is already `LaneColor(5)`
     (`app/FroggersModulation.hpp:517`), so the metadata and the visualizer disagree today.
  3. **`ScopeWriter::RecordStart`/`RecordEnd` are never called** — already known and already owned
     by **6.7c** (cycle alignment). Listed here only so the sweep's output is complete; do not
     implement it twice.

---

## §1 Oscilloscopes + the defect class

- [x] 1.1 **Call `AdvanceIndex()` per sample.** Add `vcoScopeWriter_.AdvanceIndex()` once per
  sample in `ProcessBlock()`'s per-frame loop, mirroring
  Braid 4 (`Braid4Core.hpp:487`). Failing test first: assert `ScopeReader` is non-`Empty()` after
  rendering one block with the transport running. **GUI confirmation required** — launch and look
  at the scopes.
  **CLOSED** — `app/FroggersAppCore.hpp:480`; operator confirmed scopes draw. What they draw is
  wrong (E6-F4), which is 6.7's problem, not a reopening of this one.
- [ ] 1.2 **Reconcile `ScopeWriter` sizing** — separately from 1.1, so a regression is
  attributable. Froggers uses defaults (`maxChannels=16, maxFrames=4096`,
  `app/FroggersAppCore.hpp:777`); Braid 4 passes explicit `{kOscillatorCount*2, kScopeFrames}`
  (`Braid4Core.hpp:35,707`). Decide and document whether our frame count is right for our display
  width (`numSamples=512`, `app/FroggersAppCore.hpp:122`). Do NOT bundle with 1.1.
  **STILL OPEN** — no decision was recorded. Fold into 6.7c, which touches the same construction.
- [x] 1.3 **Regression test** that fails if `AdvanceIndex()` is ever removed again: assert the
  published index advances across successive blocks, not merely that output is non-empty.
  **CLOSED** — `app/FroggersScopeAdvanceIndexTests.cpp`.
- [x] 1.4 **CALL-SITE SWEEP — the class fix, not an instance fix.** Enumerate every Sheaf API and
  every frozen-firmware DSP unit the app consumes, and verify each required call site exists in
  production code (not merely in tests). Two instances of this class are already confirmed:
  `scoopNotch`'s setters (fixed) and `ScopeWriter::AdvanceIndex()` (task 1.1). Method: for each
  DSP unit in `app/dsp/` and each Sheaf type used in `app/`, list its mutating/lifecycle methods,
  then grep production code for each. **A method called only from `*Tests.cpp` is a red flag** —
  that is exactly the signature both known defects had. Produce a table of unit → method →
  called-in-production? Every "no" is triaged as defect or intentional, with a one-line reason.
  This task may be dispatched to parallel analysis subagents (§0 permits analysis parallelism).
  **Executed as 6.1** — one task, two numbers. Close both together.

## §2 DSP recovery architecture

Item numbering here is **not** monotonic: 2.8 was inserted between 2.4 and 2.5 on purpose, because
2.5's Tier-2 test asserts on output magnitude and reads differently once 2.8 has moved the output
clamp. Execute in listed order (2.1, 2.2, 2.3, 2.4, 2.8, 2.5, 2.6, 2.7), not numeric order.

- [ ] 2.1 **Rebuild the blowup repro against the REAL engine path.** The existing
  `app/FroggersRandomizeAllReproTests.cpp` and `app/FroggersCrunchyBlowupReproTests.cpp` drive a
  *shadow copy* of `RouteAudioSample()` — formulas copied into separate instances. That is the
  predecessor's trap #1 repeated; their numbers are indicative, not authoritative. Rebuild against
  the real `Engine`/`Runtime`, starting the transport with `MessageIn::Start` as the app does.
  **Then attempt to reproduce the operator's unrecoverable case** (Randomize All → Crunchy max →
  permanent silence, no recovery). Report honestly whether it reproduces; "did not reproduce" is a
  valid and important result.
- [ ] 2.2 **Add `Reset()` to every stateful unit** in design E2a's table that lacks one:
  `ResonantBump` ×2, `Comb`, `Reverb` (which has no reset method at all), `Oversampler2x`,
  `SampleRateReducer` ×2, `DriveBlendPhase`, `dsp::Vco` phases. Each zeros its own recursive state
  only. Pure addition — no behavior change yet, so the full suite must stay green unchanged.
  **Audit note (OMNI §12).** Confirmed still true: `grep "void Reset()" app/dsp/` returns nothing.
  But do not add a `Reset()` that nothing will ever call. E2a's own table marks `dsp::Vco` phases,
  `Oversampler2x`, `SampleRateReducer` and `DriveBlendPhase` **not reachable-unstable**, and no
  task in this change calls their reset: Tier 1/2 (2.3/2.5) reset only units that trip a guard,
  and 6.5 resets only `StereoDelay` + `Reverb`. Add `Reset()` where a caller exists — the units
  that can actually go unstable (`ResonantBump` ×2, `Comb`) plus `Reverb` and `StereoDelay` for
  6.5 — and for the rest, either give the Tier-1 guard a reason to cover them or record a one-line
  "no caller, not added" beside each. An unreachable reset method is the same defensive-dead-branch
  mistake 2.6 exists to prevent.
  **`StereoDelay` already has one:** `ClearBuffers()` (`app/dsp/Delay.hpp:224-231`). Reuse it; do
  not add a second entry point beside it.
  **Split note:** 6.5 pulls the `StereoDelay`/`Reverb` half of this task forward. Do that half
  there; leave the rest here.
- [ ] 2.3 **Tier 1 — finiteness recovery.** After each block, if a unit's state is non-finite,
  reset **that unit only** and continue. Per-unit, never global: a global reset would cut the
  reverb tail and delay repeats every time one filter misbehaved (design E2d). Failing test first:
  inject a non-finite value into one unit's state, assert audio recovers within N blocks AND that
  other units' state is untouched.
- [ ] 2.4 **Tier 2 threshold is DERIVED — do not measure, do not guess.** Ceiling = **100.0**.
  Derivation, from design E2c, all of it checkable in the code: filter-chain input is bounded to
  ±1.0 by `PadeSaturator::Saturate` (`app/dsp/FilterFx.hpp:94-99`); `ResonantBump`'s peak gain is
  `A² = height` with `height = ExpMapCompute(1,10,knob)`, so at most 10×; `scoopNotch`'s height is
  a dip ∈ [0.05,1] and adds no gain. Legitimate magnitude ≈10, ≈30 with ringing. 100 gives 10×
  margin over anything musical and sits 3.4e36 below float overflow, and because divergence is
  exponential a real fault crosses it within milliseconds. Encode the derivation as a comment
  beside the constant so nobody "tunes" it later without re-deriving.
- [ ] 2.8 **Fix the output stage: hard clamp at 1.0, not 8.0.** In `SanitizeOutputSample`
  (`app/FroggersAppCore.hpp:743-751` — re-verified 2026-07-28, still `8.0f`),
  change `kMaxOutputMagnitude` from `8.0f` to `1.0f`. That is the
  whole change. In float audio 1.0 is full scale (0 dBFS); 8.0 is +18 dBFS, so the app was handing
  the device 8× full scale and the device hard-clipped it into a square wave — part of the
  reported "buzz."
  **Operator decision 2026-07-28: hard clamp, NOT a saturator.** A soft limit via
  `PadeSaturator::Saturate` was offered and declined — it would impose a tonal character the frozen
  DSP never had on every loud patch. Do not add one. Do not "improve" this into a limiter.
  **Preserve exactly:** the non-finite → `0.0f` branch and the denormal flush. Only the constant
  changes.
  Update the function's own comment, which currently justifies the 8.0 figure.
  Failing test first: assert no emitted sample exceeds ±1.0 while a deliberately overdriven patch
  plays, and that a normal-level signal passes through bit-identical.
- [ ] 2.5 **Tier 2 — magnitude recovery**, using 2.4's threshold. **This is the tier that catches
  the actually-reproduced bug**: the Filter-stage excursion reached ≈8.7 with `sawNaN == 0`
  throughout, so a finiteness-only guard misses it entirely. Failing test first: drive the
  Randomize-All-then-Crunchy-max sequence and assert output returns to a sane magnitude and stays
  there.
- [ ] 2.6 **Do NOT add divisor clamps.** Recorded so it is not re-proposed: the predecessor's
  handoff proposed clamping `GetDelaySamples`'s `1/freq` and the bump's `sinw/(2q)`. Audited and
  refuted — all three divisors are exp-mapped between strictly positive endpoints and knob values
  cannot leave [0,1] (`ClampToRange` is an unconditional `std::clamp(v,0,1)`). Those branches would
  be unreachable (OMNI §12). This task is complete when confirmed still absent.
- [ ] 2.7 **Full-range endpoint sweep test.** Drive every parameter in every bank to 0.0, 1.0 and
  back through the real engine path, asserting output stays finite AND non-silent. No existing
  test drives parameters to their endpoints.

## §3 UI defects — **all landed; two are reversed by §6.** Do not start here.

- [x]→ 3.1 **Bank selection: inverted background + text**, replacing the `" *"` suffix.
  Rendered as `Draw` nodes with `FillRoundedRect` + `Text`, colours chosen app-side.
  **CLOSED, THEN REVERSED BY 6.3** — the Draw-node choice cost single-click dispatch. Implement
  6.3, not this. Keep the derived-labels part (`FroggersBankLayouts()`); drop the Draw nodes.
- [x] 3.2 **Signal-path page order**: Audio, Envelope, Filter, Drive, Delay, Reverb.
  **CLOSED** — enum at `app/FroggersParameters.hpp:67-74`; `kBankLabels` **deleted** (grep returns
  nothing) and labels now read from `FroggersBankLayouts()` at `app/FroggersUiSurface.hpp:741-746`,
  so the third parallel copy OMNI §8 objected to is gone. The `[UNVERIFIED]` persistence
  prerequisite resolved clean: persistence is name-keyed, not index-keyed.
- [x] 3.3 **ASR short labels** → `"Atk1"/"Sus1"/"Rel1"` (etc.).
  **CLOSED** — `app/FroggersParameters.hpp:153-155`. Exactly 4 chars, so the VCO digit survives
  `EncoderDraw`'s hard truncation (`EncoderDraw.hpp:571-582`).
- [x] 3.4 **Remove the canvas title** `"Frogg3rs Synth"`. Leave
  `config.appName` (`app/FroggersAppCore.hpp:135`) and `FroggersManifest().displayName`
  (`app/FroggersRegistration.hpp:24`) alone — they are launcher metadata, not canvas text.
  **CLOSED** — removal note at `app/FroggersUiSurface.hpp:479`. Logo still deferred pending
  upstream `DrawCommand::Image`.
- [x] 3.5 **Scene controls.** S1/S2 → "Scene 1"/"Scene 2", toggling the blend between extremes via
  `SetSceneBlend(0.0f/1.0f)` instead of `SetLessSelectedScene`. Slider labelled "Scene blend".
  **CLOSED** — `app/FroggersUiSurface.hpp:801` and handler `:870+`. **Carried limitation:** the
  float readout could NOT be removed — JUCE's Slider text box is unconditional upstream
  (`PortableJuceBackend.hpp:1228`), so the spec's "no raw numeric blend value" scenario is
  **not** met. Reported, not fixed; belongs in `UPSTREAM-SHEAF-ASK.md`.
- [x]→ 3.6 **Settle the BPM label conflict by LOOKING at the GUI.**
  **CLOSED WRONGLY, REOPENED AS 6.6.** It was closed on a node-field assertion; the label is a
  Slider label and Slider labels are never drawn (`PortableJuceBackend.hpp:1231`). This is F2, the
  named process failure of the last session. The discoverability annotation
  (`app/FroggersUiSurface.hpp:834-836`) exists in the field but renders nowhere.
- [x] 3.7 **Window sizing.** Height derived from the layout rather than hardcoded, with a test
  asserting `uiHeight >= computed extent`.
  **CLOSED** — `config.uiHeight = 596` (`app/FroggersAppCore.hpp:177`). **Re-verify under 6.6**,
  which adds Label nodes to the same auto-flowed chrome band and can change the computed extent.
- [x] 3.8 **Play/Stop geometry.** Square bounds (were ~110×50 wide rectangles), icon inset to
  ~55–60% of the plate with even padding, chrome-palette plate rather than stark white. Keep
  `FillPolygon` + `FillRoundedRect` — both render in the JUCE **and** browser backends. Matching
  the website's exact artwork is out of scope pending image support.
  **CLOSED** — `app/FroggersUiSurface.hpp:344-382`, side length = `transportArea.height` on both
  axes. **6.4 removes these Draw nodes** in favour of labelled Buttons until upstream plain-click
  lands; keep the builders in the file so this work is not lost.

## §4 Voicing judgement (by ear — operator decides)

- [ ] 4.1 **Is max Crunchy supposed to be near-silent chaos?** Not a defect: RMS 0.29 → ~0.01,
  never non-finite, never clipping — Fuegoize's scramble at `fuegKnob=1` collapsing the signal.
  Distinct from a blowup and from legitimate silence. May legitimately resolve as working as
  intended (design E4).
- [ ] 4.2 **(inherited 8.6)** Validate D8a's Random S&H character table by ear; #4's ~5-level
  quantisation is the flagged weak choice.
- [ ] 4.3 **(inherited 12.6)** Whether ~61 depth changes per Randomize All is the right reach, and
  whether Randomize Page's ~50% no-op rate reads as broken.

## §5 Inherited from `froggers-sheaf-app` (archived 2026-07-28)

- [ ] 5.1 **(6c.4)** Latent issue flagged during packet 6c, deliberately not patched around.
- [ ] 5.2 **(8.2)** External MIDI clock sync via `SyncConfig`/`HandleExternalClock`; verified only
  through internal `SetTempoBpm` so far.
- [ ] 5.3 **(11.6)** Website cutover: rewrite `pages.yml`; `VITE_BASE` must become `/frogg3rs/`.
- [ ] 5.4 **(11.7)** Clean or exclude `browser/dist` build artifacts.
- [ ] 5.5 **(11.8)** Re-verify the frozen-tree property after the workflow rewrite.
- [ ] 5.6 **(11.9–11.11)** Host with CORS + correct media types; self-certify the deployed catalog;
  cross-origin load test through the launcher.
- [ ] 5.7 **(11.12–11.13)** Site smoke test; registration submission for `catalog-sources.json`.
- [ ] 5.8 **(12.1)** Full suite + parity suites green, via subagent, pass/fail + tail only.
- [ ] 5.9 **(12.2) THE GATE THAT WAS SKIPPED.** Desktop smoke through `sheaf-patch`: app launches,
  **scopes live**, banks switch, drill-in reaches L2 and refuses L3, Marbles follow the clock,
  Randomize All audibly changes the patch without touching Crunchy, Randomize Page changes only
  its grid, and neither retired button appears. **This task was never run in the predecessor, and
  running it once would have caught the dead oscilloscopes.** It is not complete until someone has
  actually launched the app and checked each clause.
- [ ] 5.10 **(12.3)** Browser smoke: package loads in the launcher, audio runs, UI interactive.
- [ ] 5.11 **(12.4)** Frozen-tree proof — **source half only**. The output-hash half is invalid:
  the tracked `.bin` does not reproduce from baseline sources with the current toolchain
  (`6ca56ee8…`/88964 vs `be0dc826…`/89172, diverging at byte 5 in the ISR vector table). Do not
  treat a hash mismatch as evidence of frozen-tree damage.
- [ ] 5.12 **(12.5)** Docs closure: app README, DSP-fork note (D3), fuegoize-UB divergence (D6),
  pin policy (D1), resolved decisions.

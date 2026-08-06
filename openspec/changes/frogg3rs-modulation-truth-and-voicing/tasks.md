# Tasks — `frogg3rs-modulation-truth-and-voicing`

## §0 Standing constraints (carried verbatim from the predecessor; still binding)

- **Subagents: Sonnet or Haiku, never Opus.** Model set explicitly on every dispatch (OMNI §4, §15).
- **`nice make -j2`, never higher** (8-core/16 GB). Launcher only via `./app/build-launcher.sh`.
- **Build/test runs go through a subagent**, foreground inside the brief, counts + failure tails
  only (OMNI §16.1). Count that all ten binaries ran — `make test` has no `-k`.
- **`External/Sheaf` pinned and clean** (`77a3019e` until W4.1 lands `508d9d68`). We do not patch
  Sheaf; needs → `/UPSTREAM-SHEAF-ASK.md`.
- **Frozen trees byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`.
- **No AI attribution on commits.**
- **Code changes sequential; parallel dispatch only for read-only analysis.**
- **No unrequested user-visible behaviour. Propose first.**
- **An implementer may not close a task whose spec requires operator eyes or ears.**
- **A pin is rewritten, never deleted; a guard asserts the property that broke, not two app-side
  numbers against each other.** Six green-while-wrong guards to date; the list is long enough.
- **An instruction's rationale is part of the instruction** (F.6 lesson): when the rationale dies,
  re-derive rather than mechanically preserve — and when honouring an instruction requires adding
  a branch, suspect a literal reading.
- **Enumeration is complete only when every TU/site is compiled or swept** (F-PLAN ERRATA, twice).
- **Systematic debugging is binding for W1/W2:** no fix before the recorded root cause.
- **The operator's runtime data root is `~/Library/Sheaf/synth/sheaf-patch/`.**

### Execution order — this file is the single proposal layer

1. **W1** modulation truth — *blocked on investigation report (dispatched 2026-08-05)*
2. **W2** filter maxima — *blocked on investigation report (dispatched 2026-08-05)*; fixes are
   per-offender operator decisions
3. **W3** scene-2 opposite defaults — value sign-off then implement; independent of W1/W2 code
   paths until proven otherwise by the W1 trace (defaults touch `FroggersParameters.hpp`, W1
   likely touches `FroggersModulation.hpp` — re-check overlap when W1's fix is scoped)
4. **W4.1** pin bump `508d9d68` → **W4.2** external-audio unblock → **W4.3** G.2 decision
5. **W5** operator walkthrough: C.2 + G.3 + criteria 1–4 — last, never implementer-closed

W1 before W2's implementation (both touch the audible result; one variable at a time when the
operator is listening). W4 after W1/W2 so bump breakage stays attributable (F.1's lesson).

---

## W1 — Modulation truth (badges vs depths vs sound)

Symptoms (operator, 2026-08-05, F.6 build): S1 Randomize All badges nearly every parameter;
S2 drill-in depth knobs all read 12 o'clock; S3 Randomize All inside level-1 shows no icons.

- [x] W1.0 **Root-cause investigation — LANDED 2026-08-05.** All file:line-cited (Sheaf cites at
  pin `77a3019e`, `src/ParameterModulation.cpp` unless noted). Ruled OUT: scene mismatch (writes
  and reads use the same mono `ParameterManager::Scene()`), unipolar-midpoint misread (depths are
  `RangeKind::Bipolar`, 0.5 IS zero), and Sheaf-side overwrite (`Deselect()` recycling is gated on
  the same nonzero check as badges; fresh writes protected).

  **S2 CONFIRMED — the knobs are the lie; badges and audio are true.** Badges are genuinely
  nonzero-gated (`ModulatorsAffectingMask` requires `HasNonZeroState()`, `:2356-2400`), and
  `RandomizeVisibleValue` writes `sceneCenters_` directly and immediately (`:1549-1554`). But the
  drill-in knob draws from `uiDisplayCenters_`, which is populated only by `ProcessLitePhase1` —
  and that runs only for `topLevelParameters_`, which **depth parameters are never registered
  into** (`:867-869` vs `:712`). The one seed a depth's display ever gets is inside
  `RandomizeVisibleValue` itself: a single smoothing step at `targetCenterAlpha ≈ 0.0994`
  (`:2206-2213`) — one ~10 % nudge of a filter calibrated to converge over thousands of audio-rate
  calls, then hard-copied to the display (`:2324-2329`) and never ticked again. The commanded
  depth jumps; the displayed depth stays visually at center forever.

  **S1 CONFIRMED — accumulation, not overreach.** The median-3 draw works as designed, but nothing
  ever zeroes previously-randomized sources (`FroggersModulation.hpp:862-878` touches only the
  drawn subset). Repeated Randomize All presses monotonically saturate badge coverage. Decisive
  check for the operator: badge count after 1 press from a fresh patch vs after N.

  **S3 PLAUSIBLE, UNCONFIRMED — silent pool exhaustion.** `EnsureModulationDepth` returns nullptr
  when `!group_.CanAllocate()` (`:1823-1826`) and the app helper `break`s with zero writes,
  leaving `partial=true` un-surfaced (`FroggersModulation.hpp:872-876`). The L1 Randomize All
  fans out to 225 level-2 depths; a pool already consumed by S1-style accumulation could exhaust
  mid-loop. No rendering gap exists at L1 (same mask path at every level). **Must repro from a
  FRESH app state before any fix** — if S3 reproduces fresh, this hypothesis is wrong and the
  cause is still unidentified.

  **The sixth green-while-wrong guard, confirmed.** Both E.1 tests assert `depth->SceneCenter(0)`
  — the raw commanded value — never the UI-facing display
  (`FroggersModulationTests.cpp:524-624`). Their own comment (`:527-548`) documents the smoothed
  display path and works around it with `ComputeAllParameters()` between trials — the tests KNEW
  the display was stale and stepped over it rather than asserting it.

- [ ] W1.1 Fix design — constrained by **no-Sheaf-patch**: every implicated mechanism
  (`uiDisplayCenters_`, `ProcessLitePhase1`, the seed path) lives in Sheaf.
  - [x] W1.1a **LANDED 2026-08-05 — an app-callable fix exists; NO upstream ask needed.**

    **Patch load is NOT bugged, and is the working reference.**
    `ParameterManager::LoadParameterValuesFromJSON` writes values then calls
    `ComputeAllParameters()` (`ParameterModulation.cpp:3141-3155`, the call at `:3153`). That is
    exactly the reseed randomize is missing.

    **CORRECTION to this plan's earlier estimate.** W1.0 recorded an interim settle costing "~46
    iterations to converge at alpha 0.0994". **Wrong — one call converges exactly.**
    `ComputeAllParameters` (`:3165-3173`, public, `ParameterModulation.hpp:796`) calls
    `ComputeAtDepth(scene_, 0, /*smoothTargetCenter=*/false)`, and the smoothing branch is gated
    `if (smoothTargetCenter && recursionDepth == 0)` (`:2207-2213`) — so alpha is bypassed
    entirely, `targetCenter_ = rawCenter` instantly, and depth children (at `recursionDepth_ > 0`)
    take the unconditional snap-and-seed branch (`:2295-2304`), with `SnapCurrentToTarget`
    re-snapping recursively (`:2317-2321`). **Exact convergence, one call.**

    **THE PRECISE MECHANISM, which the drag path reveals.** Dragging a depth knob displays
    correctly not because `HandleTick` refreshes anything — it calls only `HandleIncDec`
    (`:2652-2658`) — but because the depth's **top-level ancestor** runs a periodic
    `Compute(scene)` in `ProcessSamplePhase1` (`:1486-1491`), whose recursion reaches each depth
    child at `recursionDepth_ > 0` and therefore always takes the *instant* branch, re-seeding the
    display every few milliseconds while audio runs.
    **`RandomizeVisibleValue` breaks precisely because it calls `Compute()` directly ON the depth
    parameter** (`:1727/:1731`), making that call `recursionDepth == 0` **for the depth itself** —
    which is the one path that takes the smoothed branch. The bug is a recursion-depth accident,
    not a missing feature.

    **Threading constraint — binding.** `ComputeAllParameters` is a full graph traversal and is
    explicitly NOT in the lock-free class that `ProcessLitePhase1` documents
    (`ParameterModulation.hpp:484-485`), and `ParameterManager` is audio-thread-owned once running
    (`app/FroggersAppCore.hpp:23`). **It must not be called from the UI thread.** Fortunately
    randomize already crosses correctly: Randomize All/Page go through
    `FroggersAppCore::Request*`, a pending-atomic bridge the audio thread drains in `ProcessFrame`
    (`app/FroggersAppCore.hpp:336-337, 1483-1484`) — so the reseed lands on the audio thread by
    construction if it is added at the end of that drain, not at the UI call site.

    **No lighter API exists:** `SnapCurrentToTarget`/`SeedCachedKnobAndUiDisplayState` are private
    (`ParameterModulation.hpp:548-549`); `ComputeAllParameters` is the only public, deterministic
    reseed. Broader than ideal (whole tree) but correct, and the same call load already makes.

- [ ] W1.1d **Fix design, derived from W1.1a/W1.1b (write the brief from this, not from symptoms):**
  1. Option A zeroing: before drawing, zero the depths in scope (all, for Randomize All; the page's
     own, for Randomize Page).
  2. Draw the median-3 per parameter exactly as today — the distribution is not the defect.
  3. **Reseed once, at the end of the audio-thread drain**, via `ComputeAllParameters()` — one
     call, whole tree, after all writes; not per-parameter, not from the UI thread.
  4. Do NOT touch `RandomizeVisibleValue`'s internals or anything in Sheaf.
  - [x] W1.1b **DECIDED 2026-08-05 — Option A, fresh sculpture.** Operator: *"option A is what it
    should have always been, the randomization should never have been additive (and it was still
    too much to start with)."* Randomize All zeroes ALL modulation depths first, then draws the
    fresh median-3 per parameter; Randomize Page zeroes only its own page's parameters' depths
    first. **Density flag:** "still too much to start with" may mean median-3 itself is too dense —
    but it was judged on accumulated state and cannot be read cleanly. Land Option A first; the
    operator judges single-press density in isolation at W1.3, and the count distribution moves
    only if they still say so.
  - [ ] W1.1c **S3 fresh-state repro** (failing-test-first): drive L1 Randomize All from a clean
    rig, assert visible depth changes; separately surface `partial=true` somewhere a user can see
    (silent partial success is its own defect regardless of S3's verdict).
- [ ] W1.2 Guard tests pin `UIDisplayCenter`/knob-facing state after a randomize press — the
  property that was actually wrong — replacing the E.1 pins' layer; record the green-while-wrong
  instance in the ledger.
- [ ] W1.3 Operator confirms: badges, drill-in knobs, and audible modulation agree (criterion 1);
  S3 either fixed or explicitly accepted as designed (criterion 2).

## W2 — Filter maxima, evidence-first

- [x] W2.0 **Gain-path audit — LANDED 2026-08-05.** All claims file:line-cited; condensed here as
  the binding §1 trace.

  **The pumping mechanism, found.** `OutputLimiter` (`FroggersAppCore.hpp:1107-1179`: threshold
  0.9, ceiling 1.0, attack 1 ms, release 100 ms, no lookahead) sits at the END of the entire
  chain: Filter → Delay → Reverb → limiter. A sustained comb/peak resonance is therefore NOT
  attenuated before it feeds Delay and Reverb; when the limiter engages continuously it ducks the
  whole downstream mix — echoes and reverb tail included. That whole-mix ducking IS the audible
  "sounds like shit," and **no per-stage headroom exists anywhere**: `ResonantBump` applies no
  Q-dependent trim (`FilterFx.hpp:179-201`), no feedback-dependent wet attenuation exists
  (grep-confirmed absent in the mapping block).

  **Why "fixed" passed every test:** §A's fix removed absolute blowout, and the tests only check
  that. The 200-draw storm test asserts finite / peak ≤ 1.001 / not-silent
  (`FroggersAudioRoutingTests.cpp:613-672`) — an absolute-ceiling check that passes while the
  limiter rides hard continuously. `FroggersRandomizeAllReproTests.cpp:107-113` watches RMS/peak/
  NaN over random combinations, never a single-parameter endpoint. The peak ceiling test pins
  height at **Q = 1.0 only** (`FroggersDspParityTests.cpp:574`). **No test sweeps any individual
  Filter parameter to its endpoint while measuring sustained RMS or limiter engagement.** The
  operator's symptom lives exactly in that untested region.

  **Ranked offenders (math in the audit, key cites):**
  1. **Comb feedback max = exactly 0.95** (`FilterFx.hpp:426-434`, `FroggersAppCore.hpp:1027`) —
     near-unity loop gain, ring measured in thousands of samples even in isolation
     (`FroggersDspParityTests.cpp:638-663`).
  2. **Comb LP max, alpha ≈ 0.927** (`FroggersAppCore.hpp:1028-1031`) — removes the loop's ONE
     damping mechanism, sustaining offender 1's ring across nearly the full band. Only ever tested
     as an isolated transfer function, never through the audible chain.
  3. **Peak Q max 10 × Peak gain max 2.0** (`FroggersAppCore.hpp:968,1002-1004`) — narrow +6 dB
     spike with Q-proportional ring time; this COMBINATION is untested (ceiling test fixes Q=1).
  4. **Comb/Peak blend = 1.0** (`:1032-1033`) — not independently harmful; the fader that routes
     100 % of offenders 1-2 into the mix.
  Tone-only (limiter can't help): Peak freq near Nyquist + high Q (thin whistle), comb delay at
  ~4.8 samples (metallic ~10 kHz teeth). Harmless: Comb offset, Scoop (a dip, not gain).

  **Latent defect found in passing (fix with W2.2, not before):** the Crunchy blowup repro's
  shadow chain is STALE — `FroggersCrunchyBlowupReproTests.cpp:193` still hardcodes
  `ExpMapCompute(1.0f, 10.0f, ...)` for peak height, predating §A's `kMaxResonantBumpHeight = 2.0`,
  so its diagnostic thresholds no longer describe the real ceiling. A second definition site that
  drifted, again.
- [ ] W2.1 Operator triage — **PARTIAL VERDICT 2026-08-05: "more pumping than piercing/ringing",
  reproduced by Filter-bank Crispy at max.** Consistent with offenders 1-2 (comb feedback + LP),
  not 3 (peak). Mechanism confirmed by reading `dsp/Fuegoize.hpp:52-79`: Crispy is the per-bank
  fuegoization knob, a bit-scramble of the normalized value — `outInt = (inputInt & ~mask) |
  scrambledLowerBits`, output always in [0,1] — so **Crispy exposes reachable extremes, it cannot
  create out-of-range values.** At max (mask=255) it scrambles all 8 bits, jumping filter params
  anywhere in range including the comb-feedback/LP maxima. Therefore the treatment target is the
  maxima/stage headroom, NOT Crispy: taming those tames Crispy's exposure of them for free. (Also
  noted: Crispy's discontinuous jumps into a ringing comb add transient bursts — the post-filter
  limiter candidate catches those too.) **Operator also asked to consider a cap on Drive's gain
  ("probably not... but maybe worth looking at") — added to the W2.2 evidence list, not assumed.**
  Per-offender decision: range trim / stage headroom / per-stage limiting / leave. **By ear.**
  Treatment candidates the trace supports (pick per offender, not wholesale):
  - **Post-filter, pre-Delay limiter/soft-knee** — addresses the mechanism directly: the comb's
    sustained ring gets caught BEFORE it smears through Delay/Reverb and forces whole-mix ducking.
    This is the operator's own suggestion ("a limiter on the output of that parameter") and the
    trace endorses its position.
  - **Feedback-dependent comb wet trim** — attenuate the comb's contribution as |feedback| → max,
    the standing DSP idiom for near-unity resonators; kills offenders 1-2's sustained level
    without touching their character at moderate settings.
  - **Q-dependent peak gain compensation** — offender 3; standard resonant-filter trim.
  - **Range trims** (feedback 0.95 → lower, LP max → lower, Q max → lower) — bluntest tool,
    changes reachable character; operator's call only.
- [x] W2.2-PREP **Treatment scoping — LANDED 2026-08-05.** File:line cited; binding for W2.2.

  **Insertion point exists and is clean.** `filterOut` is computed at `FroggersAppCore.hpp:
  1032-1033` and consumed at `:1050` by `delay_.Process(filterOut, ...)`. It is a **single mono
  `float`** — the whole path is mono here (`FilterFxChain::Process` returns `float`,
  `FilterFx.hpp:475-492`). A limiter inserted between those two lines sees exactly what Delay and
  Reverb see today, before their tails can be ducked.

  **`OutputLimiter` is state-safe to instantiate twice but NOT parametrically a drop-in.**
  `Process()` (`:1160-1166`) is a pure per-sample envelope follower with all state in instance
  fields, so a second instance is isolated and correct. **But `kThreshold` 0.9 / `kCeiling` 1.0 /
  `kHeadroom` / `kAttackSeconds` 1 ms / `kReleaseSeconds` 100 ms are `static constexpr`
  (`:1108-1112`)** and `Configure()` takes only `sampleRate` (`:1120-1124`) — every instance of the
  type shares one tuning. A second instance copied verbatim would duck **identically, one stage
  earlier**, which is not the goal. Making it independently tunable (template params or instance
  fields) is a real edit to the struct, and it must be counted as part of the work rather than
  discovered mid-implementation. Per-sample cost either way is trivial (`fabs`, compare,
  multiply-add; `std::exp` only when above threshold, `:1134-1139`).

  **DRIVE NEEDS NO CAP — operator's instinct confirmed with the math.** Drive runs BEFORE Filter
  (`:942-943` feeds `:1032`). `PolynomialDrive` is internally unbounded (`gain = ExpMap(1,5)`,
  `Drive.hpp:86`; shape coefficients to ~11, `:96-100`) **but its output never escapes**: the
  `FrogBlock` lambda always folds it through `Sine01(out/4)` — bounded to [-1,1] for any real input
  (`DspMath.hpp:28-33`) — convex-blended with a hard-clamped `PadeSaturator` (`FilterFx.hpp:97-102`)
  by weights summing to 1 (`Drive.hpp:283-289`). Downstream Drive stages add no gain: bit-crusher
  (`:239-267`), sample-and-hold (`:169-208`), convex blend through a stable `|a|<0.98` allpass
  (`:339-367`). **Drive is a bounded waveshaper, not a net-gain stage. Dropping the cap idea on
  evidence, not intuition.**

  **No headroom anywhere, confirmed and extended.** Nothing between VCO output and the limiter
  reduces level: `FilterFxChain` convex-blends only (`FilterFx.hpp:475-492`), `StereoDelay` clamps
  *parameters* but never scales signal (`Delay.hpp:170`), `Reverb` blends dry/wet only
  (`Reverb.hpp:262-263`).

  **The guard can assert the STRONG claim.** `TestOutputLimiter()` returns a live reference and
  `envelope` is a public field, already read per-block by an existing test
  (`FroggersAudioRoutingTests.cpp:567,571`). So W2.2's test can pin *"the limiter never engages
  continuously across a sustained run"* — the property that actually broke — rather than an
  output-RMS proxy. Single-parameter setup pattern exists too
  (`model.PageParameter(bank, slot).SceneCenter(0) = value`, `:559-563`) plus
  `rig.StartAt(0)`/`RunBlocks(n)`.

- [x] W2.1-MATH **Option B sizing — computed 2026-08-05, and it rules Option B OUT as a primary
  fix.** Operator asked for the optimal setting by arithmetic rather than by ear.

  **Model, from the code (not the summary).** The chain runs `useParallel = true`
  (`FroggersAppCore.hpp:1033`), so comb and peak are **parallel, convex-blended**
  (`FilterFx.hpp:475-484`) — the bump does NOT multiply the comb. Bounds, for filter input
  amplitude `A` (Drive output is bounded |A| ≤ 1):
  - Comb: `out = in + fb · Saturate(lp(delayed))` with `Saturate` clamped to ±1
    (`FilterFx.hpp` `PadeSaturator`), so **|comb| ≤ A + fb**. At fb = 0.95 the loop saturates for
    any input above ≈0.05, so this bound is the operative one, not the linear `A/(1−fb)`.
  - Peak: RBJ peaking biquad with `a = sqrt(height)` (`FilterFx.hpp:186`), so centre gain = height
    → **|peak| ≤ A · height**, height ≤ 2.0.
  - Blend and scoop are both convex, so **filterOut ≤ max(A + fb, A · height)**.

  **The decisive number.** Limiter threshold is 0.9. The comb's feedback term alone at fb = 0.95
  contributes **0.95 — already above 0.9 with ZERO input.** A decaying tail with the voice silent
  still holds the limiter engaged. That is the pumping, and no choice of input level avoids it.

  **What Option B would have to do.** Solving `A + fb ≤ 0.9`:

  | Signal condition | Required fb |
  |---|---|
  | Tail only, A → 0 | ≤ 0.90 |
  | Quiet voice, A = 0.2 | ≤ 0.70 |
  | Moderate, A = 0.3 | ≤ 0.60 |
  | Typical, A = 0.5 | ≤ 0.40 |

  And `A · height ≤ 0.9` → height ≤ 1.8 at A = 0.5, ≤ 1.2 at A = 0.75.

  **What that costs, quantified.** Comb resonant gain is `1/(1−fb)`; T60 is
  `ln(0.001)/ln(fb)` round trips, × delay period (up to 2400 samples ≈ 50 ms at the low end of
  `combFreq`):

  | fb | resonant gain | round trips to −60 dB | longest-delay ring |
  |---|---|---|---|
  | 0.95 (today) | 20× (+26 dB) | 135 | **6.7 s** |
  | 0.90 | 10× (+20 dB) | 66 | 3.3 s |
  | 0.80 | 5× (+14 dB) | 31 | 1.6 s |
  | 0.60 | 2.5× (+8 dB) | 13.5 | 0.68 s |
  | 0.40 | 1.7× (+4.4 dB) | 7.5 | **0.38 s** |

  **Verdict: Option B is not viable as the primary fix.** To stop sustained limiting at a typical
  playing level it needs fb ≈ 0.4 — surrendering ~22 dB of resonant gain and ~95 % of the ring
  time. That is a different instrument, not a tamed one. Even the most generous target (tail-only,
  fb ≤ 0.9) is a change so small it would not fix the operator's symptom.

  **What the arithmetic reveals instead: there is no gain-staging budget anywhere.** The limiter
  threshold (0.9) sits BELOW the level a correctly-working filter produces — even a modest fb = 0.5
  at A = 0.5 gives 1.0 > 0.9. That is why the limiter rides continuously regardless of settings,
  and it is a headroom defect, not a filter-range defect.

  **Therefore a THIRD option the earlier scoping missed — Option D, settings-derived output trim.**
  Scale each branch by the inverse of its own worst-case gain before blending: comb by
  `1/(1 + fb)`, peak by `1/height`. Then comb → `(A + fb)/(1 + fb)` = 1.0 at A = 1/fb = 0.95, and
  0.74 at A = 0.5; peak → `A`. **The filter's frequency response shape is unchanged** — a scalar
  trim moves level, not tone — so the resonant character the operator called "settled as wanted"
  survives exactly, while the limiter stops engaging. Cost: one multiply per sample per branch,
  plus smoothing of the trim so a moving `fb` knob does not zipper. **This preserves character
  strictly better than B and adds no compression artifacts at all, unlike A.**

- [ ] W2.2 Implement the agreed treatments, sequentially, each with a guard that pins the property
  that was wrong (sustained-tone limiter engagement at max, not merely peak ceiling), using the
  observable `envelope` above.
- [ ] W2.3 Extend the storm test if W2.0 shows it cannot catch the operator's symptom (per-param
  endpoint dwell + limiter-engagement assertion, not just >1.0 samples).

## W3 — Scene-2 opposite VCO defaults + shared light cross-coupling

- [ ] W3.0 Trace how per-scene defaults work today: `FroggersParamSpec.defaultValue` is
  single-valued (`FroggersParameters.hpp:145-186`) — establish where scene values initialize and
  whether a per-scene default exists in Sheaf's `ParameterConfig` or must be set post-init by the
  app. File:line before design.
- [ ] W3.1 Operator sign-off on exact values: which "shape" params invert (Shp1-3? PM1-3 too?),
  what "opposite" means numerically (1.0 − default? range-flipped?), and the cross-coupling
  matrix (which sources → which targets at what light depth, both scenes identical).
- [ ] W3.2 Implement + tests (fresh-patch scene 1 vs scene 2 defaults differ as signed off;
  blend midpoint audible check is W5's).
- [ ] W3.3 **Existing-patch impact stated before landing:** defaults apply to fresh patches; the
  operator's saved patches must not be rewritten. Verify load path leaves stored values alone.

## W4 — Carried mechanics (from predecessor F.4/F.5/G.2)

- [ ] W4.1 Pin bump `77a3019e` → `508d9d68` (27 commits, all audio-input + a demo app).
  Compile-first, zero behaviour change, full-TU error enumeration, suite green before W4.2.
- [ ] W4.2 Remove `kExternalAudioOptedIn` (`FroggersAppCore.hpp:507` area):
  `RuntimeConfig::numAudioInputs` requested nonzero; gate on
  `block.InputView().HasActiveChannel(0)`. The old comment promised a one-line change — verify,
  don't trust; written when the API didn't exist. Operator confirms source #6 behaves and no-device
  leaves it dark (recorded residual: device-selected-but-unpatched yields an active silent channel).
- [ ] W4.3 G.2: startup failure currently logs and leaves a blank window (`FroggersMain.cpp`
  inner catch). Operator decision: quit-with-code vs error surface. Then implement.

## W5 — Operator verification (closes the change)

- [ ] C.2 walkthrough on the final build; G.3 saved-patch load from
  `~/Library/Sheaf/synth/sheaf-patch/`; criteria 1–4 from `proposal.md`. **None of this is
  implementer-closable.**

## Deferred (carried)

- §H mobile-web UI layer (incl. slot-repack constraint: slot index == PhysicalEncoderId here;
  display-order repack is the viable route; hardware meaning UNCLEAR — ask first).
- §I VST layer (DAW owns audio/MIDI routing; W4.2's gating must stay swappable for it).
- D.4 publish pipeline.

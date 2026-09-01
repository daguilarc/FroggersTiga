# Tasks — `frogg3rs-vco-pitch-ceiling`

## 0. Tree and sweep

`git status --short -- app External/Sheaf` and the submodule's own status
clean before any evidence run. Sweep `app/dsp/` and the touched test files;
name what was swept.

## 1. The named range

`app/dsp/Vco.hpp`: `kPitchMinHz = 20.0f` and `kPitchMaxHz = 5000.0f`
declared beside `kRingModMinHz/MaxHz`, read by `PitchToPhaseIncrement`.
Comment states the ceiling clears the top of the piano with headroom and
deliberately excludes the shrill-to-inaudible top the audibility-limit
ceiling spent the knob on; extend the file's frozen-reference divergence
idiom for the range change. No change/task references.

## 1b. The recomputed defaults

`app/FroggersParameters.hpp:142`: the three pitch defaults become
`0.3087f/0.4343f/0.5077f`, each with the existing comment style updated to
state the Hz it decodes to under the named range. The
`kAudibleFundamentalsHz` tests stay untouched and must pass — they are the
drift check on this recomputation.

## 2. Enumerations, closed out in the same breath

- `PitchToPhaseIncrement` callers: FOUND vs CHANGED, zeros included.
- Tests pinning pitch-derived absolute values: floor pins must not move;
  mid-curve pins change, each named old vs new, expressed as the formula
  where the file's idiom allows.
- The `20000` family: confirm the three filter ceilings and the visualizer
  axis comment are untouched, with a one-line disposition each.
- The default-patch pin family: `kAudibleFundamentalsHz` and its 7 sites
  (`FroggersAudioRoutingTests.cpp:157,334,2593,2674,2773,2950,3265,3281`)
  UNCHANGED and green; the stale comment at `:281` refreshed.

## 3. Gates

- `rm -f` touched test binaries, `nice make -C app test` — full count (324
  baseline plus any new cases), zero FAIL.
- A pin proves the new ceiling: knob 1.0 maps to exactly kPitchMaxHz/sr and
  knob 0.0 to exactly kPitchMinHz/sr (bit-exact floor), in whichever suite
  already pins this mapping's shape.
- `rm` the wasm artifact, `nice make -C app/browser build` — fresh mtime.

## 4. Ship and confirm

PUSH IS HELD by operator instruction. When lifted: commit on `main`, push;
OPERATOR, deployed site, by ear: sweeping any VCO pitch knob to its top
lands on a high but pitched, audible note; launch chord unchanged
(110/220/330). Archive on confirmation.

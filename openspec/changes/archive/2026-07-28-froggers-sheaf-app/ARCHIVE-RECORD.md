# Archive record — `froggers-sheaf-app`, archived 2026-07-28

**Status at archive: 82/99 tasks complete. This change did NOT pass its own acceptance gate.**
It is archived as superseded, not as done. The 17 open tasks are carried forward into the
successor change rather than dropped — see "Carried forward" below.

## What genuinely shipped

A working out-of-tree Sheaf app. Frogg3rs registers into the `sheaf-patch` launcher beside
Braid 4 and the Miniapp, runs, and makes sound. 120 tests green across 9 binaries. The full
DSP port (VCOs, drive, filter chain, reverb, delay, Marbles modulator, fuegoization) is in
place with parity tests against the frozen firmware.

The frozen-tree property **holds**: `git diff b409106 --name-status` shows only
`M External/Sheaf`, and that is solely the sanctioned `dafa54b6 → 1940ddcb` pin bump from
task 1.1. All seven frozen trees are byte-identical.

## What did NOT pass, stated plainly

**The acceptance gate (12.1–12.6) never ran to completion.** Had 12.2's desktop smoke test —
which explicitly requires "app launches, **scopes live**, banks switch" — actually been
executed, it would have caught the dead oscilloscopes immediately. It was not run. The
change was treated as functionally complete on the strength of the unit suite alone.

Two defects escaped to the operator as a direct result, both found on 2026-07-27/28:

1. **`scoopNotch` was never configured in production code.** `FilterFxChain` kept the scoop
   blend, but the three setters at frozen `src/core/FroggersEngine.hpp:561-564` were dropped
   in the port, so the filter ran on `ResonantBump`'s default `freq = 1000.0f` — an
   unnormalized value in a cycles/sample convention. Its biquad state diverged to non-finite
   under a self-oscillating comb and `SanitizeOutputSample` masked every later sample to
   `0.0f`: permanent silence. **Fixed** at `app/FroggersAppCore.hpp:613-638`.

2. **`ScopeWriter::AdvanceIndex()` is never called anywhere in `app/`.** Braid 4 calls it
   per-sample (`Braid4Core.hpp:487`). Without it the ring-buffer write cursor stays at 0,
   the reader is permanently `Empty()`, and the scope renders only its background — the
   "black box" the operator reported. **Not fixed here**; carried forward.

**These two share a root cause worth recording:** the port faithfully copied every data
structure and every per-sample formula, then dropped required *call sites*. Unit tests
cannot catch this class, because a unit test constructs and configures the object itself —
`FroggersDspParityTests.cpp`'s scoop test calls all three `scoopNotch` setters by hand and
passed both before and after the fix. The successor change carries a task to sweep for the
rest of this class systematically rather than fixing known instances one at a time.

**Task 12.4's output-hash half is invalid**, not merely unfinished. The tracked
`FroggersTiga.bin` does not reproduce from baseline sources with the current toolchain
(baseline `6ca56ee8…` / 88964 bytes vs fresh out-of-tree `be0dc826…` / 89172 bytes,
diverging at byte 5 in the ISR vector table). The task's own "genuinely reproducible" claim
was false and has been retracted in place. The source half of the proof passes outright, and
that is what actually protects Daisy.

**The publish pipeline (11.6–11.13) was never started.** Steps 1–4 of §11 are done — emcc
installed, package built, catalog validated through Sheaf's own `parseCatalog`. Everything
from the `pages.yml` rewrite onward is untouched.

## Resolved during this change's lifetime, recorded so it is not re-litigated

- **Repository rename**: complete. `daguilarc/frogg3rs` (note: `frogg3rs`, not `froggers3`
  as originally planned). Remote confirmed. Task 11.5 closed.
- **The Sheaf fork**: tried and reverted the same day. For part of 2026-07-27 the submodule
  sat on local branch `froggers-fork` two commits ahead, adding plain-click dispatch for
  Draw nodes. Because those commits existed only on one machine the gitlink was
  unresolvable from any other checkout, blocking the browser publish. Reverted to upstream
  pin `1940ddcb`. The work is preserved on the local `froggers-fork` branch and has been
  written up for upstreaming (`UPSTREAM-SHEAF-ASK.md`). **Cost of the revert:** encoder
  press and Play/Stop are double-click until plain-click lands upstream.
- **Build dependency defect**: `EXTRA_APP_HEADERS` was documented with a 2-header example
  and used with a 4-header command, while the app has 18 headers. Since `APP_HEADERS` is a
  literal prerequisite list and no `-MMD` files are generated, 14 headers — all of
  `app/dsp/` — were untracked, so edits to them produced builds that succeeded while
  ignoring the change. Fixed by `app/build-launcher.sh`, which globs.

## Carried forward into the successor change

All 17 open tasks transfer. Grouped:

- **6c.4** — latent issue flagged during packet 6c, deliberately not patched around.
- **8.2** — external MIDI clock sync via `SyncConfig`/`HandleExternalClock`, verified only
  through internal `SetTempoBpm`.
- **8.6** — validate D8a's Random S&H character table by ear.
- **11.6–11.13** — the entire publish pipeline. Now gated on product readiness rather than
  infrastructure: both former blockers (rename, fork) are resolved, and nothing should
  deploy until the GUI defects are fixed.
- **12.1–12.6** — the entire acceptance gate. **12.2 must be re-run after the UI overhaul
  regardless**, which is the main reason these move rather than staying behind.

# Proposal — `frogg3rs-filter-page-review`

**Created 2026-09-01, operator-commissioned.** The Filter page gets the
full treatment: every knob's range and curve evaluated for whether its
travel is spent on audible change, the page's layout regrouped by stage,
and two named suspects investigated first. Everything here is HELD — no
commit or push until the operator lifts the hold.

## Suspect 1 — "did we cap Peak Q too low?" Answer: the cap was GAIN, not Q

- Peak Q (slot 3) is the peak's WIDTH: `ExpMapCompute(0.1, 10.0, knob)`
  (`FroggersAppCore.hpp:1745`), untouched by any blowout fix.
- The blowout-era cap is Peak GAIN (slot 2, height):
  `kMaxResonantBumpHeight` (`FilterFx.hpp:118`), value history recorded in
  its own comment — 10.0 (firmware parity) → 4.0 ("gross overload") → 2.0
  ("still too harsh when modulated … very close to blowout territory",
  operator 2026-07-29). The `peakLimiter` and `1/height` trim were sized BY
  MEASUREMENT for height ∈ [1, 2] (`FilterFx.hpp:120-132`).
- Width cannot reopen the blowout: the RBJ peak's worst-case gain equals
  height regardless of width — the trim and limiter bound on height alone
  (`FilterFx.hpp:746-758`).
- MATERIAL CHANGE SINCE THE CAP: the governing scenario was "a pinned
  self-oscillating comb through a 20 dB peak"
  (`FroggersAppCore.hpp:1758-1760`). The comb feedback now DEFAULTS TO ZERO
  (centred, `frogg3rs-comb-knob-travel`) and the always-routed bed sits at
  −22 dB — the pinned-comb launch state that fed the blowout no longer
  exists. DECIDED PROCEDURE (operator, 2026-09-01): re-run the limiter's
  measurement harness at candidate ceilings 3.0 and 4.0 under today's
  defaults and report the worst-case numbers; the constant MOVES IN THIS
  WAVE only if the measurement clears it AND the operator approves the
  number at postflight review — otherwise it ships unchanged with the
  numbers recorded for a later ear-gated change.

## Suspect 2 — Comb drive is useless until the end. Mechanism found

`out = input + fb·Sat(drive·lp(delayed))` (`FilterFx.hpp:414`), drive ∈
0.25..4 (`ExpMapCompute(0.25, 4.0)`), NO output compensation. So drive
plays two roles at once: below unity it simply WEAKENS the loop (0.25..1,
the knob's whole bottom half, quiets the tail — audibly "nothing"), and
saturation character only appears once drive·signal reaches the knee —
roughly drive > 2, i.e. knob > 0.75. The audible effect concentrating "at
the very end" is exactly this. DECIDED (operator, 2026-09-01): COMPENSATED
drive — `Sat(drive·x)/drive` — so the knob owns exactly one quantity,
saturation depth, across its whole travel, and the feedback knob's
just-promoted ring-time law stays true at every drive setting (an
uncompensated re-span would multiply effective loop gain and silently
distort that law above unity). The compensation preserves the unity
default and transport-stopped override bit-for-bit (`Sat(x)/1`).
PREFLIGHT CORRECTION on the bound: `|fb|/drive` would LOOSEN the absolute
ceiling for drive < 1, so the bound argument is REWRITTEN, not carried:
the saturator is compressive (`Sat(y) ≤ y` for `y ≥ 0`), so the
compensated fed-back term `Sat(drive·x)/drive` never exceeds `|x|` — the
delayed signal it is computed from — and the loop's per-pass decay factor
stays ≤ |fb| at EVERY drive. The old absolute `|Sat| ≤ 1` ceiling is
replaced by this relative bound in the comment at `FilterFx.hpp:386-392`,
and the finite-plot-frame bound (`FilterFx.hpp:63-65`, `:388`) is
re-derived on the same argument. The parameter-model delta carried by this
change rewrites slot 12's description accordingly.

Test thresholds, decided:

1. Unity identity: EXACT equality at knob 0.5 and the transport-stopped
   override; the per-sample bound assertion unchanged.
2. Feedback-law invariance: the geometric ring-time guard parameterized
   over drive knob {0, 0.5, 1}, the SAME 8% ratio tolerance holding at
   all three — the requirement's signature test.
3. Travel audibility: monotonic saturation depth across the knob at the
   suite's reference level; ≤0.5 dB compression at knob 0 (clean provably
   clean); ≥6 dB depth span across the top half with the knee by
   mid-knob. The exact fixture level and metric (compression vs
   harmonic-band energy) calibrate in the prototype, and the chosen
   constants land in the test with the measured old-curve values beside
   them.
4. Unchanged regression floor: the sweep-latch probe passes untouched.

The archived `frogg3rs-comb-knob-travel` classified this knob "already
geometric, untouched" — correct about the curve's shape, wrong about its
usefulness; superseded here.

## The layout

Current slot order interleaves the stages (comb at 0,4,5,6,12; peak at
1,2,3; scoop at 8,10,11,13; blend 7; topology 9). Operator's target
grouping: peak together, comb together, scoop together, blend and topology
at the end before Crispy/Crunchy. The blocker to trace FIRST: slot indices
are load-bearing identity — saved patches, `labels.md`,
`FroggersApprovedLabels()[bank][ix]`, `ApplyBankDefaultPatch`, modulation
state, and the detent machinery all address (bank, slot). Re-slotting
breaks saved patches unless the persistence layer maps by name, and a
display-permutation layer is a new mechanism the parameter model does not
have. Execution's first task on this axis is reading the persistence
format and the spec's "Bank-slate growth is safe for existing saved
patches by construction" requirement. SETTLED BY PREFLIGHT: persistence is
name-addressed — `ParameterValuesToJSON`/`LoadParameterValuesFromJSON` key
and look up by `Name()`, never by bank/slot
(`External/Sheaf/.../ParameterModulation.cpp:3139-3175`) — so RE-SLOT is
safe and chosen; no display-map. The parameter-model delta carried by this
change rewrites the Filter-bank scenario to the grouped order.

## The full sweep

Every Filter knob gets the same evaluation the comb feedback got: what
does the ear track, does the curve spend travel on it, where do the
defaults sit, and is any bound a leftover from a world that no longer
exists. The fourteen parameters, their current mappings, all read at the
routing block (`FroggersAppCore.hpp:1717-1856` — Comb offset maps at
`:1729-1732`, ahead of the rest) — the matrix lives in tasks.md.

## Impact (anticipated; the review refines it)

- `app/FroggersAppCore.hpp` (mappings), `app/dsp/FilterFx.hpp` (bounds,
  possibly the drive law), `app/FroggersParameters.hpp` (defaults/order if
  re-slotted), `labels.md` and surface tests if the layout moves, parity
  and routing tests per enumeration.
- SEQUENCING DECIDED: `frogg3rs-vco-pitch-ceiling` executes FIRST in the
  same wave (its own gates attributable), this review's execution follows
  on that baseline, and both changes commit and push TOGETHER once the
  joint postflight is approved.
- Gates: full app suite fresh, wasm build, the limiter re-measurement
  harness if the height ceiling moves, operator's ear on the deployed
  site, per-knob.

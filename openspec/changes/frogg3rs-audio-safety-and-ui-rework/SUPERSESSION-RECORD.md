# Supersession record — what happened to `frogg3rs-gui-and-dsp-robustness`

Written 2026-07-28. Read this before assuming any predecessor task still needs doing.

## Why it was superseded rather than continued

Two premises it was built on were invalidated by the operator on 2026-07-28:

1. **Parity with the frozen firmware is a constraint.** Operator: *"parity is stupid. we need
   feedback stability and a limiter."* The predecessor treated the ±1.1 comb feedback and the 10×
   resonant bump as untouchable because the frozen firmware had them. They are the direct cause of
   the blowout, and they are now changed.
2. **The output stage is a hard clamp, never a limiter.** The predecessor recorded this as an
   operator decision *and wrote it into a spec requirement* forbidding "saturation, soft-knee
   limiting, or any other tone-shaping". The operator reversed it the same day.

A third reason is process, not content: the predecessor sequenced GUI remediation (§6) **ahead of**
the DSP recovery architecture (§2), then invited the operator to test a build in which the entire
recovery system did not exist. They hit the blowout it existed to prevent. That is the single most
expensive mistake in the change's history and the reason this successor leads with audio safety.

## Landed and verified in the tree — DO NOT REDO

Confirmed by reading the code, not by trusting checkboxes:

| Area | Evidence |
|---|---|
| Scope `AdvanceIndex()` per sample + regression test | `app/FroggersAppCore.hpp`, `app/FroggersScopeAdvanceIndexTests.cpp` |
| Call-site sweep (the class fix) | `reports/6.1-call-site-sweep-{dsp,sheaf}.md` in the predecessor dir |
| `ConfigureProcessingTiming` wired at the host rate | found *by* the sweep; no bug report existed |
| `SetVoiceColor` on the Random S&H 6 lane | verified by a **rendered** draw command, not a field |
| Signal-path bank order; `kBankLabels` deleted | labels derive from `FroggersBankLayouts()` |
| ASR short labels keep their VCO digit | `app/FroggersParameters.hpp` |
| Canvas title removed; scene controls; window height derived (632) | `app/FroggersUiSurface.hpp` |
| Audible pitch defaults (110/220/330 Hz) | `app/FroggersParameters.hpp`, band-energy test |
| Single-click bank buttons; visible BPM/Scene-blend `Label` nodes | `app/FroggersUiSurface.hpp` |
| Stop clears delay/reverb tails, incl. the long-release case, clearing **once** at `AllIdle` | `app/FroggersAppCore.hpp` |
| Per-unit `Reset()` on eight DSP units | `app/dsp/*.hpp` |
| Tier 1 finiteness + Tier 2 magnitude recovery, ceiling 100.0 derived | `app/FroggersAppCore.hpp` |
| `DriveBlendPhase` marginal stability (`\|a\|` could equal 1 at the **default**) | `app/dsp/Drive.hpp:350` |
| Full-range endpoint sweep test | `app/FroggersAudioRoutingTests.cpp` |

## Superseded — actively replaced by this change

- **Task 2.8's hard clamp at 1.0** → replaced by the limiter (§A.3).
- **`frogg3rs-dsp-recovery`'s "Output never exceeds full scale"** requirement, which forbade
  limiting → rewritten as "Output is limited, not clipped".
- **The two-band scope requirement** (post-gate audio above envelope-follower band) → the operator
  looked at the running app and confirmed there is **one** panel; the second band is withdrawn.
- **Task 6.4's removal of the Play/Stop icons.** It traded icons for single click; the operator
  wants both. Resolved with glyph-labelled `Button` nodes.

## Corrected analysis carried forward

The predecessor's design (and this author's own statement to the operator) claimed the comb
feedback **diverges exponentially**. It does not — the saturator sits inside the feedback path, so
the comb is bounded by `|in| + 1.1`. `|fb| > 1` produces **sustained self-oscillation at the
saturator limit**, not divergence. See `design.md` §A1a. Anyone reading the predecessor's design
should treat its divergence claim as retracted.

## Still open, inherited into §D

S&H dice-roll re-observation; `ScopeWriter` sizing; the §4 voicing judgements; the entire §5 publish
pipeline and acceptance gate; and the **Drive Blend default**, where the authored 20% Drive is
currently inaudible and the operator has not ruled on a value.

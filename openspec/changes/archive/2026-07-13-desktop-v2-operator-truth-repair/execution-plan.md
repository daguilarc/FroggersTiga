# Execution Plan — `desktop-v2-operator-truth-repair`

**OpenSpec change:** `desktop-v2-operator-truth-repair`  
**Change root:** `openspec/changes/desktop-v2-operator-truth-repair/`  
**Status at plan write:** 0/90+ tasks complete; planning artifacts done; implementation not started  
**Supersedes:** archived `froggers-v2-sheaf-runtime-convergence` remainder + narrow operator-truth repair scope

## Goal

Ship desktop v2 at 1280×920 where layout, mod grid, randomization, host sync, sequencer, runtime pages, and hosted projection match the manifest product contract. Manual QA packet 14 is the archive gate.

## Guiding principle

Operator symptoms are convergence gaps, not isolated bugs. **Packets 1–2 (layout + mod grid) run before packet 4 (randomization wiring).** Do not patch Rand Mods until manifest mod routes and compact mod cells exist.

## Current baseline (already in tree)

| Area | State | Action |
|------|-------|--------|
| Manifest + validators (§1–2) | Done | Do not redo |
| Layout shell `DesktopV2ChromeLayout` (§3) | Ledger done; operator QA failed | Re-verify packet 1 |
| Boot path (§11) | Done | Use `./scripts/open-desktop-v2.sh` |
| `MidiCvSettingsComponent` data-driven | Partial code exists | Verify in packet 10 |
| `FroggersV2AppCoreFacade` | Exists with test; `MainComponent` routes through `m_facade` | Verify/complete packet 7 |
| `GlobalOscilloscopeDisplay` | Exists, wired visible in standalone + hosted shells, fixed-capacity bindings, test exists | Verify/complete packet 8 — do not reimplement |
| Runtime pages (`Source/runtime/`) | File/Patch, Audio, Controllers components + projections + `RuntimePages_test` exist | Verify/complete packet 9 — do not reimplement |
| Manifest 15-lane mod eligibility (`isModSourceEligibleForRow`, VCO pair-bus, external-audio availability) | Done in manifest | Packet 2 consumes — do not re-derive |
| `ModLanePicker` | Exists; fills expanded full-row area | Packet 2 makes compact |
| Write Seq arm/capture | Partial in `SequencerPanelComponent` | Verify/complete packet 5 |
| `CenterGlobalClusterV2` | Dead: permanently hidden duplicate of `GlobalStripV2` rand cluster, same broken `RandSequencerMods` wiring | Delete in packet 4 (task 4.9) |
| Rand Mods / host sync / mod width | Broken | Packets 1–4 |

## Packet execution sequence

```
0 gates ──► 1 layout ──► 2 mod grid ──► 3 host sync
                              │
         ┌────────────────────┼────────────────────┐
         ▼                    ▼                    ▼
    4 rand auth          5 sequencer           6 UI dedup
         │                    │                    │
         └────────────────────┴────────────────────┘
                              │
    7 facade ──► 8 scope ──► 9 runtime ──► 10 controller ──► 11 VST
                              │
                    12 docs ──► 13 OMNI closure ──► 14 manual QA
```

**Rule:** One packet at a time. No parallel subagents. Parent re-runs gates after every merge.

## Mandatory gates (every packet)

```bash
bash scripts/check_subagent_packet_gates.sh
bash scripts/check_desktop_v2_operator_truth.sh   # exists after packet 0.2
ctest --test-dir desktop-v2/build --output-on-failure
```

Exit nonzero → fix or revert before next packet. Attach stdout to task ledger when marking `[x]`.

**Authority:** `scripts/SUBAGENT_OMNI_CONTRACT.md` — include verbatim in every subagent dispatch.

---

## Phase A — Operator-visible fixes (packets 0–2)

### Packet 0 — Gates and evidence

| Task | Deliverable |
|------|-------------|
| 0.1 | `operator-qa-2026-07-07.md` — screenshot refs + pass/fail table stub |
| 0.2 | `scripts/check_desktop_v2_operator_truth.sh` — fail if `pushRandMods` → `RandSequencerMods` (must match **both** `GlobalStripV2` and `CenterGlobalClusterV2` until 4.9 deletes the latter); fail if `syncToHost` single-page-only; fail if `modW` expands past `kModCellW` (grep is advisory — `LayoutBounds_test` 1.4 is the authoritative width gate) |
| 0.3 | Wire script into `check_subagent_packet_gates.sh` or packet gate docs |

**Write scope:** `scripts/`, change folder docs only.

### Packet 1 — Layout and mod-cell authority

| Task | Primary files |
|------|---------------|
| 1.1 | `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` — `modW = kModCellW` |
| 1.2 | `desktop-v2/Source/MainComponent.cpp`, carousel/sequencer height constants |
| 1.3 | `desktop-v2/Source/ui/PerformanceBandV2.cpp` |
| 1.4 | `desktop-v2/tests/LayoutBounds_test.cpp` |
| 1.5 | Launch app; record failures in `operator-qa-2026-07-07.md` |

**Packet gate:** `ctest -R LayoutBounds_test --test-dir desktop-v2/build`  
**Operator preview:** mod columns stop eating the row; Audio page scroll reduced or eliminated.

### Packet 2 — Mod source grid convergence

| Task | Primary files |
|------|---------------|
| 2.1–2.3 | `ModLanePicker`, parameter-detail grid components, manifest mod eligibility consumers |
| 2.4–2.6 | `FroggersV2ControlCore`, host mod route projection, 15-lane rack |
| 2.7 | New/extended tests under `desktop-v2/tests/` |

**Packet gate:** mod eligibility / detail grid tests pass; compact pickers on module rows.  
**Dependency:** packet 1 layout geometry stable.

---

## Phase B — Operator behavior fixes (packets 3–6)

### Packet 3 — Full-engine host sync

| Task | Primary files |
|------|---------------|
| 3.1 | `desktop-v2/Source/control/FroggersV2HostBridge.cpp` — loop all pages in `syncToHost` |
| 3.2 | `desktop-v2/tests/ControlCoreBridge_test.cpp` |
| 3.3 | Audio callback audit (no new allocs) |

**Operator preview:** Drive/Filter audible without visiting carousel page.

### Packet 4 — Randomization authority

| Task | Primary files |
|------|---------------|
| 4.1–4.6 | `FroggersV2ControlCore.cpp`, `GlobalStripV2.cpp`, `DesktopV2HostCallbacks.cpp` |
| 4.7 | Delay overlay integration |
| 4.8 | `ControlCoreBridge_test`, scope tests |
| 4.9 | Delete `CenterGlobalClusterV2.{hpp,cpp}`; remove `m_centerCluster` from `PageCarouselComponent.{hpp,cpp}` (dead hidden duplicate — required for gate 0.2 to pass in scope) |

**Blocked until:** packet 2 mod authority complete.  
**Operator preview:** global Rand Mods changes live mod depths; scope radios change behavior.

### Packet 5 — Sequencer operator loop

| Task | Primary files |
|------|---------------|
| 5.1–5.3 | `SequencerPanelComponent`, `FroggersV2HostBridge` capture paths |
| 5.4–5.7 | Control core sequencer snapshots/locks |
| 5.8 | Sequencer UI tests |

### Packet 6 — UI deduplication

| Task | Primary files |
|------|---------------|
| 6.1 | `SubmodulePagePanel`, `AdsrPagePanel` — remove header Randomize/Randmod |
| 6.2 | `SequencerPanelComponent` — remove duplicate All Steps |
| 6.3–6.4 | Tooltips; `GlobalControlParity_test` |

**Milestone A:** After packet 6, run `./scripts/open-desktop-v2.sh` and smoke-test items 14.11–14.18 (expect partial pass until packets 7–11).

---

## Phase C — Convergence features (packets 7–11)

### Packet 7 — Facade boundary

Verify existing `FroggersV2AppCoreFacade` against tasks 7.1–7.4. Complete gaps only.

**Gate:** `ctest -R FroggersV2AppCoreFacade_test --test-dir desktop-v2/build`

### Packet 8 — Global oscilloscope

Verify existing `GlobalOscilloscopeDisplay` (wired in `MainComponent` + `HostedMainComponentV2`) against tasks 8.1–8.5. Complete gaps only — do not reimplement.

**Gate:** `ctest -R GlobalOscilloscopeDisplay_test --test-dir desktop-v2/build`

### Packet 9 — Runtime pages + audio

Verify existing `Source/runtime/` components (File/Patch, Audio, Controllers, projections, hosted status panel) against tasks 9.1–9.6. Complete gaps only — do not reimplement.

**Gate:** `ctest -R RuntimePages_test --test-dir desktop-v2/build`

### Packet 10 — Controller configuration

Verify `MidiCvSettingsComponent` data-driven from `buildTargetMappingRows()`; complete manifest target fan-out.

**Gate:** `ctest -R MidiCvAssignment_test --test-dir desktop-v2/build`

### Packet 11 — VST/AU hosted projection

Stable IDs, round-trip, hosted editor hiding standalone controls.

---

## Phase D — Closure (packets 12–14)

### Packet 12 — Operator docs

Update `SIM_MANUAL.md`, `QUICK_DICT.md`; sync mirrors; `bash sim/check_operator_docs_sync.sh`.

### Packet 13 — OMNI closure

Hedge grep, host-master audit, release-channel check, path review, Sheaf inventory, spec sync, `openspec validate desktop-v2-operator-truth-repair --strict`.

### Packet 14 — Manual QA at 1280×920

All items 14.1–14.18 pass. Record evidence in `operator-qa-2026-07-07.md`. Blocks archive.

---

## Subagent dispatch template

```text
Read and obey: scripts/SUBAGENT_OMNI_CONTRACT.md

CHANGE: desktop-v2-operator-truth-repair
PACKET: <N> — <title from tasks.md>
WRITE SCOPE: <explicit paths from execution-plan packet section>
FORBIDDEN: out-of-scope files; duplicate authority tables; wiring Rand Mods before packet 2 complete

OMNI FOCUS: <single authority | data flow | no repetition | contract honesty>

ACCEPTANCE:
- bash scripts/check_subagent_packet_gates.sh exits 0
- bash scripts/check_desktop_v2_operator_truth.sh exits 0
- <packet-specific ctest from plan>

Report using COMPLETION REPORT format in SUBAGENT_OMNI_CONTRACT.md
```

---

## Task ledger discipline

1. Mark `tasks.md` items `[x]` only after parent gate stdout attached.
2. Update `operator-qa-2026-07-07.md` as items move from fail → pass.
3. Do not archive this change until packet 14 complete and packet 13.7 validates.
4. Crosswalk reference: `convergence-remainder-crosswalk.md`.

## Estimated effort (order-of-magnitude)

| Phase | Packets | Relative size |
|-------|---------|---------------|
| A — visible UI | 0–2 | Medium (highest operator impact) |
| B — behavior | 3–6 | Medium |
| C — convergence | 7–11 | Large |
| D — closure | 12–14 | Small–medium |

## Start command

When ready to implement:

```
/opsx:apply desktop-v2-operator-truth-repair
```

Begin with **packet 0**, then **packet 1**.

---

## Addendum — Packets 15–19 (operator critique 2026-07-09, reopened)

The 2026-07-09 live QA reopened five deferred packets after packets 0–13 landed. These append to the sequence above (packet 14 manual QA moves to post-archive acceptance per the archive-timing decision).

**Order:** `15 ──► (16 · 17 · 18) ──► 19`. Couplings: **15** unblocks **19** and the follow-on unified-layout change; **18 before 19** (shared `rowKindForIndex`); **19** depends on **15.2** (`ModDrillIn`).

**Packet 15 is engine-gated, not a wiring rewire.** Task **15.0 is a PARENT engine-scope spike** — *does the audio engine sum N independent lanes per row, or is that new DSP capability?* The engine stores one (source, depth) per row today (`FroggersV2HostBridge`/`HostParameterRoutingV2`, `DelayState::Modulate`, sequencer snapshot). **15.0 blocks all 15.2–15.10 code.** If summation is new DSP, Packet 15 is re-scoped at the OpenSpec layer (`design.md` D11a + `tasks.md`) before dispatch — not patched inside execution (omni-rule §2). See `design.md` D11a.

| Packet | Content | Subagent model | Gate |
|--------|---------|----------------|------|
| 15 | Multi-depth mod routing + `ModDrillIn` (15.0 spike, 15.2a array collapse) | **parent** spike; sonnet transcribes | `desktop-v2-mod-source-grid` tests; drill-in; Rand-Mod all lanes; eligibility |
| 16 | Oscilloscope per-trace auto-scale | **parent** decides 16.1; haiku for 16.2–16.4 | `GlobalOscilloscopeDisplay_test` + visual QA |
| 17 | Top chrome honest grid | sonnet | `LayoutBounds_test`; reopen 1.3 |
| 18 | Random S&H naming + full Shift purge | haiku naming, sonnet purge | projection validators; `rg 'shift|HeldModifier' = 0` |
| 19 | Per-row MIDI encoder targets (19.0 retires parallel target table first) | sonnet | `MidiCvAssignment_test`; `RuntimePages_test` |

**Decisions belong to the parent (omni-rule §4):** 15.0 (engine scope) and 16.1 (tap product signal) are parent adjudications; subagents may gather evidence but do not decide.

**Archive gate (updated):** all three spikes (15.0 engine, unified Packet 1 fit, unified Packet 6 sustain) resolved favorably + all automated gates green + `openspec validate --strict`. Manual/visual items are marked `UNVALIDATED-AT-ARCHIVE — record on test`, not `[x]` (contract-honest, not a false "done").

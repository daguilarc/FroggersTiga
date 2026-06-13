## Context

**Current state (2026-06-12):**

```text
openspec/changes/          16 dirs (15 pre-existing + archive-cleanup)
openspec/specs/            (empty — no baseline)
Code landed                ~80% of planned sim-host work
Live bug                   RecordExportCluster setBounds(header) blocks transport
Two 0% task changes        desktop-header-hit-test, web-chrome-cohesion
```

OMNI audit conclusion: most changes are **archive candidates**; only header hit-test and web chrome cohesion need new implementation.

## Goals / Non-Goals

**Goals:**

- End state: **2 active changes**, **13 archived**, **1 meta change** (`archive-cleanup` itself archived last).
- No lost requirements — open manual tasks land in `MANUAL_VERIFY.md`.
- No lost deltas — CC queue and spec supersession captured before MIDI changes archive.
- `openspec archive` run in dependency-safe order so main specs merge cleanly.

**Non-Goals:**

- Re-implementing stereo delay, mutation queue, help menu, or VCO morph (already in tree).
- Deleting git history or application code.
- Merging desktop and web into one UI change.

## Decisions

### 1. Active vs archived (final inventory)

| Status | Change | Tasks | Action |
|--------|--------|-------|--------|
| **ACTIVE** | `desktop-header-hit-test` | 0/10 | Apply first |
| **ACTIVE** | `web-chrome-cohesion` | 0/14 | Apply after desktop transport works |
| Archive | `desktop-chrome-cohesion` | 29/29 | Archive **after** hit-test apply; footnote hit-test was missing slice |
| Archive | `desktop-compact-layout` | 14/14 | Superseded by chrome-cohesion 1440 default |
| Archive | `desktop-vco-morph-fix` | 15/15 | Absorbed into mutation-safety |
| Archive | `sim-hosts-multi-ui` | 45/47 | Umbrella complete; stale design footnote only |
| Archive | `desktop-sim-ux-polish` | 23/31 | Manual → MANUAL_VERIFY |
| Archive | `desktop-host-corrections` | 28/33 | Manual → MANUAL_VERIFY |
| Archive | `desktop-host-mutation-safety` | 21/28 | Manual → MANUAL_VERIFY |
| Archive | `stereo-delay-page` | 22/32 | Manual §A–I → MANUAL_VERIFY |
| Archive | `desktop-audio-export` | 18/23 | §6 verify → header-hit-test tasks |
| Archive | `app-header-help-menu` | 14/16 | Manual → MANUAL_VERIFY |
| Archive | `desktop-qwerty-midi-pitch-cv` | 12/18 | Add CC queue tasks §5; then archive |
| Archive | `desktop-midi-input-clarity` | 13/18 | Supersede velocity spec on archive |
| Archive | `web-sim-page-ux` | 19/24 | §7 verify → web-chrome-cohesion |
| Meta | `archive-cleanup` | — | Archive last after all above |

### 2. Archive order (dependency-safe)

Archive **foundational → dependent** so spec merges do not fight:

```text
1.  desktop-vco-morph-fix
2.  desktop-compact-layout
3.  sim-hosts-multi-ui
4.  desktop-sim-ux-polish
5.  desktop-host-corrections
6.  desktop-host-mutation-safety
7.  stereo-delay-page
8.  desktop-audio-export
9.  app-header-help-menu
10. desktop-midi-input-clarity      ← after pitch-cv spec supersedes noted
11. desktop-qwerty-midi-pitch-cv    ← after CC queue tasks land or waived
12. web-sim-page-ux                 ← after §7 merged into web-chrome-cohesion
13. desktop-chrome-cohesion         ← after desktop-header-hit-test apply
14. archive-cleanup                 ← this change
```

Use `openspec archive <name> -y` per change. Use `--skip-specs` only for pure doc-only archives (`desktop-compact-layout` if spec conflicts).

### 3. Tail-merge rules (before archive)

**Into `desktop-header-hit-test/tasks.md` §5:**

- From `desktop-audio-export` §6.1–6.5 (WAV/stereo export verify; RECORD UI order).

**Into `web-chrome-cohesion/tasks.md` §5:**

- From `web-sim-page-ux` §7.1–7.5 (mobile chrome + pills + Delay hints verify).

**Into `desktop-qwerty-midi-pitch-cv/tasks.md` §5 (new):**

```text
5.1 Add MidiCcEvent SPSC ring (mirror MidiNoteEvent, kCcQueueSize = 64)
5.2 PushMidiCc enqueues only; drainMidiIn drains CC queue before m_pendingIn removal
5.3 Remove std::vector<MidiCcEvent> m_pendingIn
```

**Spec supersession** — in `desktop-midi-input-clarity` archive message or delta:

- `desktop-qwerty-midi-input` requirement "max velocity" → **REMOVED**, migrated to `desktop-midi-pitch-cv` pitch × velocity.

### 4. `MANUAL_VERIFY.md` structure

Single file; sections mirror archived change IDs:

```text
# FroggersTiga Manual Verification

## Desktop transport + chrome (header-hit-test)
## Desktop audio export (audio-export)
## Desktop patch + randomize (host-mutation-safety)
## Desktop wave + external (host-corrections)
## Desktop UX polish (sim-ux-polish)
## Stereo delay (stereo-delay-page §A–I)
## MIDI pitch CV (qwerty-midi-pitch-cv)
## Help menu (app-header-help-menu)
## Web page UX + chrome (web-sim-page-ux + web-chrome-cohesion)
```

Copy checkbox text verbatim from source `tasks.md` manual sections — no paraphrase.

### 5. `sim-hosts-multi-ui` stale design

On archive, prepend to archived copy (or leave note in MANUAL_VERIFY header):

> Superseded facts: six panels (not five), rotary knobs (not vertical sliders), VCO level (not VCO feat), no XCPL strip, default 1440×720 (not 2016).

Do not edit live design before archive unless blocking.

### 6. Rejected: collapse all into one mega-change

Violates OMNI scope boundaries (DSP, MIDI, chrome, web are separate data flows). Two active implementation changes is the minimum honest split.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `openspec archive` spec merge conflicts | Archive in order §2; resolve conflicts in archive-cleanup tasks |
| Lost manual checklist items | MANUAL_VERIFY.md copy step before each archive |
| chrome-cohesion archived before hit-test | Hold step 13 until hit-test apply completes |
| CC queue deferred indefinitely | Explicit task in qwerty-midi §5; waive only with user sign-off in tasks |

## Migration Plan

1. Append tail tasks to active/target changes (§3).
2. Author `MANUAL_VERIFY.md` from open manual tasks.
3. Run archive sequence §2 (steps 1–13).
4. Apply `desktop-header-hit-test`, then `web-chrome-cohesion`.
5. Archive `archive-cleanup`.
6. README pointer to MANUAL_VERIFY + active changes.

## Open Questions

- None blocking. CC queue can ship in qwerty-midi apply or immediately after archive-cleanup task 3.11.

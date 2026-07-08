# Subagent OMNI Contract (mandatory)

Every subagent dispatched for `froggers-v2-sheaf-runtime-convergence` MUST receive this block verbatim in its prompt.

## Parent enforcement (parent agent — not subagent)

The parent agent MUST:

1. Dispatch **one packet at a time** — no parallel subagents into overlapping file families.
2. Include this entire file plus explicit **WRITE SCOPE** paths in every Task prompt.
3. Re-run `bash scripts/check_subagent_packet_gates.sh` on the merged tree after every subagent merge.
4. **Block the next packet** on exit nonzero — revert or fix; never mark tasks `[x]` without parent gate stdout.
5. Reject diffs containing forbidden tactics even when a slice test passes.
6. Require the **COMPLETION REPORT** format below; reject reports missing OMNI self-check lines.

## Sandbox

- Default sandbox only. No network. No installs. No permission escalation.
- Report `BLOCKED` with exact evidence if blocked. Do not self-escalate.

## Write scope

- Touch only files listed in the packet write-scope section of the dispatch prompt.
- Do not dispatch parallel edits into the same file family.

## OMNI rules (binding)

1. **Single authority** — Manifest (`FroggersV2AppManifest.hpp`) owns structural inventories. Consumers project from manifest or approved generated/test files. No parallel label tables, stable-ID tables, mod catalogs, sequencer inventories, or controller target enums in UI/control code.
2. **No validator/grep gaming** — Forbidden: `(void)` stubs, comment-string hacks to satisfy grep, duplicate enums mirroring manifest, literal stable IDs (e.g. `"midi_pitch"`) outside manifest, marking work complete while global gates fail.
3. **Repetition** — Same transform on multiple variables → loop, shared authority, or manifest projection. No copy-paste blocks differing only by name.
4. **Accumulate then apply** — Loops accumulate mutations; apply once after. No repeated shared-state mutation per iteration.
5. **Nesting** — Max 3 levels in touched decision-heavy functions.
6. **Defensive code** — Only where the condition is possible given actual data flow.
7. **Imports** — Global. No dead code.
8. **Contract honesty** — Do not mark tasks `[x]`. Do not claim completion without gate output attached.

## Mandatory gates before reporting complete

Run from repo root:

```bash
bash scripts/check_subagent_packet_gates.sh
```

Also run the packet-specific gate from `tasks.md` when the dispatch prompt lists one.

Attach full stdout to the completion report. Exit code must be 0.

## Completion report format

```
PACKET: <id>
GATES: check_subagent_packet_gates.sh exit <code>
EXTRA: <packet-specific gate command + exit code>
FILES CHANGED: <list>
OMNI SELF-CHECK:
  single authority: pass|fail — <one line>
  no gaming: pass|fail — <one line>
  repetition: pass|fail — <one line>
BLOCKED: none|<exact blocker>
```

If any OMNI self-check is fail, do not report complete.

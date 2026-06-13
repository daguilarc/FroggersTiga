# Archive cleanup — tasks

## 1. Tail merges (before any archive)

- [x] 1.1 Append `desktop-audio-export` §6.1–6.5 to `desktop-header-hit-test/tasks.md` §5 Verification
- [x] 1.2 Append `web-sim-page-ux` §7.1–7.5 to `web-chrome-cohesion/tasks.md` §5 Verification
- [x] 1.3 Add `desktop-qwerty-midi-pitch-cv/tasks.md` §5 CC SPSC queue (5.1–5.3 per design §3)
- [x] 1.4 Add supersession note to `desktop-midi-input-clarity/proposal.md` footer: velocity-only → pitch CV

## 2. MANUAL_VERIFY.md

- [x] 2.1 Create `MANUAL_VERIFY.md` at repo root with sections per design §4
- [x] 2.2 Copy open manual tasks from: stereo-delay-page, host-mutation-safety, host-corrections, sim-ux-polish, app-header-help-menu, qwerty-midi-pitch-cv, midi-input-clarity (with pitch-CV semantics)
- [x] 2.3 Add README one-liner linking to `MANUAL_VERIFY.md` and naming active changes

## 3. Apply active implementation (gate for chrome archive)

- [x] 3.1 `/opsx:apply desktop-header-hit-test` — union bounds, pass-through, transport toFront
- [ ] 3.2 Manual: cold launch Play/Audio/MIDI work; RECORD + format toggles work
- [x] 3.3 `/opsx:apply desktop-qwerty-midi-pitch-cv` §5 CC queue
- [x] 3.4 `/opsx:apply web-chrome-cohesion` — labels, meters, touch, DOM order (build passes; manual §5.1–5.3 open)

## 4. Archive sequence (openspec archive -y)

Consolidated specs written to `openspec/specs/` before archiving superseded changes (`--skip-specs`).

- [x] 4.0a `openspec archive sim-parameter-full-names -y --skip-specs`
- [x] 4.0b `openspec archive delay-grain-filter-row0 -y --skip-specs --no-validate`
- [x] 4.0c `openspec archive web-sim-core-fix -y --skip-specs`
- [x] 4.1 `openspec archive desktop-vco-morph-fix -y --skip-specs`
- [x] 4.2 `openspec archive desktop-compact-layout -y`
- [x] 4.3 `openspec archive sim-hosts-multi-ui -y --skip-specs --no-validate`
- [x] 4.4 `openspec archive desktop-sim-ux-polish -y --skip-specs --no-validate`
- [x] 4.5 `openspec archive desktop-host-corrections -y --skip-specs`
- [x] 4.6 `openspec archive desktop-host-mutation-safety -y --skip-specs`
- [x] 4.7 `openspec archive stereo-delay-page -y`
- [x] 4.8 `openspec archive desktop-audio-export -y`
- [x] 4.9 `openspec archive app-header-help-menu -y`
- [x] 4.10 `openspec archive desktop-midi-input-clarity -y --skip-specs`
- [x] 4.11 `openspec archive desktop-qwerty-midi-pitch-cv -y --skip-specs`
- [x] 4.12 `openspec archive web-sim-page-ux -y --skip-specs --no-validate`
- [x] 4.13 `openspec archive desktop-chrome-cohesion -y --skip-specs --no-validate`

## 5. Close meta change

- [ ] 5.1 Verify `openspec list` active implementation: `web-sim-bootstrap-repair`, `filter-precomb-dispersion`, `desktop-header-hit-test`, `web-chrome-cohesion` (+ any explicitly kept desktop tails)
- [ ] 5.2 `openspec archive archive-cleanup -y`
- [x] 5.3 `openspec/specs/` canonical: `sim-parameter-display-names`, `filter-comb-offset`, `reverb-stereo-diffusion`, `quick-dict-format` (+ prior: compact-layout, stereo-delay, audio-export, help-menu)

## 6. Sign-off

- [ ] 6.1 `openspec validate` passes
- [ ] 6.2 No stale change dirs under `openspec/changes/` except active two

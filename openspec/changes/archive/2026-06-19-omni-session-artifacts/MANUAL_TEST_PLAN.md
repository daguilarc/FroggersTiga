# Manual test plan (tasks 5.10 and 9.7)

> **Host differences (what to expect in each DAW):** [`openspec/specs/froggers-host-master/spec.md`](../../specs/froggers-host-master/spec.md)

Everything else in `omni-repository-harmonization` is implemented and covered by automated scripts/tests. **These two tasks are operator-only** — they need real DAWs and Apple/Steinberg validator tools on your Mac.

When you finish, check off items below, note pass/fail in the **Evidence** section, then mark **5.10** and **9.7** in `tasks.md` and archive the omni change.

---

## 0. Build the plugin once

From repo root:

```sh
cd desktop
cmake -B build -DBUILD_VST=ON
cmake --build build --config Release
```

Expected artefacts (macOS):

| Format | Path |
|--------|------|
| VST3 | `desktop/build/FroggersTigaPlugin_artefacts/Release/VST3/FroggersTiga.vst3` |
| AU | `desktop/build/FroggersTigaPlugin_artefacts/Release/AU/FroggersTiga.component` |

Copy or symlink into your host scan folders if needed:

```sh
# VST3
cp -R build/FroggersTigaPlugin_artefacts/Release/VST3/FroggersTiga.vst3 \
  ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -R build/FroggersTigaPlugin_artefacts/Release/AU/FroggersTiga.component \
  ~/Library/Audio/Plug-Ins/Components/
```

Rescan plugins in each DAW after copying.

**Identity to expect:** instrument/generator (`IS_SYNTH TRUE`), **107** automatable parameters, **no** hosted MIDI Settings / CC ingest UI, mod rack cells **4 / 5 / 6** only (VCO Envelope, Random 1, Random 2).

---

## 1. Validator tools (no DAW required)

Run these against the Release builds from §0. Record tool version and exit code.

### 1.1 Steinberg VST3 validator

Install [VST3 SDK validator](https://github.com/steinbergmedia/vst3sdk) or use the `validator` binary from a Steinberg SDK install.

```sh
validator ~/Library/Audio/Plug-Ins/VST3/FroggersTiga.vst3
```

- [ ] Exit code 0
- [ ] No failed tests in output

### 1.2 pluginval (strictness ≥ 5)

Install [Tracktion pluginval](https://github.com/Tracktion/pluginval), then:

```sh
pluginval --strictness-level 5 --validate-in-process \
  ~/Library/Audio/Plug-Ins/VST3/FroggersTiga.vst3
```

Also run on AU if your pluginval build supports it:

```sh
pluginval --strictness-level 5 --validate-in-process \
  ~/Library/Audio/Plug-Ins/Components/FroggersTiga.component
```

- [ ] VST3 pass at strictness 5+
- [ ] AU pass at strictness 5+ (if supported)

### 1.3 auval (AU only, macOS)

Plugin codes from `desktop/CMakeLists.txt`: manufacturer `FrTi`, code `FrTg`, synth type `aumu`.

```sh
auval -v aumu FrTg FrTi
```

- [ ] Exit code 0
- [ ] All required AU validation stages pass

---

## 2. VST3 host smoke — REAPER (or alternate VST3 host)

Use REAPER if available; any VST3 host is acceptable if REAPER is not installed. Create a **new empty project**, insert **FroggersTiga** as an instrument on a track.

### 2.1 Zero-input rendering

- [ ] Press Play with **no MIDI clips** and no audio input routed to the plugin
- [ ] Stereo output is produced (sim runs; no silence/crash)
- [ ] CPU stays stable over ~30 s

### 2.2 Hosted chrome / layout (task 5.7)

- [ ] **Record** and **Export** controls are **hidden**
- [ ] Hardware **Audio** / **MIDI** device controls are **hidden**
- [ ] **QWERTY** keyboard does not drive CC when plugin focus is in the editor
- [ ] Minimum editor size keeps labels and mod-rack routing readable (resize smaller → clamp at min, nothing clipped illegibly)

### 2.3 DAW parameter mapping (not plugin MIDI ingest)

Map **three or more** different parameters via the **DAW’s** MIDI learn / control surface (e.g. Filter cutoff, Delay time, a pair-AR attack). The plugin does **not** expose CC1/CC2 mod cells when hosted.

- [ ] Mapping 1: knob/automation moves audible DSP
- [ ] Mapping 2: different parameter, independent control
- [ ] Mapping 3: third parameter, independent control

### 2.4 Automation

- [ ] Write automation lane for one page knob (e.g. Reverb mix)
- [ ] Playback follows automation; bypass/automation touch modes behave as host expects

### 2.5 Stopped-transport UI mutations (task 5.6)

With transport **stopped**:

- [ ] Drag a page knob → value sticks and is audible on next Play
- [ ] Change a mod depth → persists
- [ ] **Rand Mods** (global strip) → mod assignments update; patch overlay resyncs
- [ ] Manual patch-cable connect/disconnect → overlay matches host routes after release
- [ ] Clear routes (if available) → overlay clears

### 2.6 State and editor recall

- [ ] Save project, close, reopen → parameter values and mod routes match
- [ ] Close plugin editor, reopen → UI reflects current state (no stale knobs)
- [ ] Save DAW preset/plugin preset (if host supports) → recall works

### 2.7 Optional mono input bus

If the host exposes the plugin’s **mono input** bus:

- [ ] Route mono test signal → processes without crash
- [ ] Leaving input disconnected still works (host-optional)

---

## 3. AU host smoke — Logic Pro

Repeat the **same checklist as §2** in Logic on an **Software Instrument** track using the **AU** build.

Pay extra attention to:

- [ ] Logic scan finds `FroggersTiga` under Audio Units → Instruments
- [ ] AU render parity with VST3: same patch/settings produce equivalent sound (informal A/B is fine)
- [ ] AU state recall across project save/load matches VST3 behavior from §2.6

---

## 4. VST3 vs AU parity spot-check

Using the **same musical settings** (pick 3–5 parameters + one mod route):

| Check | VST3 (REAPER) | AU (Logic) | Match? |
|-------|---------------|------------|--------|
| Zero-input Play | | | [ ] |
| Saved project reload | | | [ ] |
| Automation on one param | | | [ ] |
| Rand Mods + overlay | | | [ ] |

Note any intentional host differences in Evidence.

---

## 5. Optional but useful (9.7 remainder)

Not required to close 5.10, but completes the broader 9.7 matrix if you want full confidence:

### 5.1 Playwright E2E (web)

```sh
cd web
npm run test:e2e
```

- [ ] All specs pass locally (CI runs this on web/sim PRs)

### 5.2 VCV Rack (local SDK only)

If you have the VCV SDK and `vcv/` tree:

- [ ] Build/package plugin
- [ ] Confirm **no** MIDI In/Out widgets; mod rack shows VCO Envelope + Random 1/2
- [ ] CV jack + internal mod combination behaves (disconnect / positive / negative CV)

### 5.3 Desktop standalone sanity (contrast with hosted)

Open `FroggersTiga.app` (not the plugin):

- [ ] **Two** hardware CC mod pairs still work (indices 0/1 + 4/5/6 layout)
- [ ] Record/Export and Audio/MIDI settings **visible** (opposite of hosted)

---

## 6. Evidence (fill in when done)

| Item | Date | Tool/host version | Result | Notes |
|------|------|-------------------|--------|-------|
| Steinberg validator | | | pass / fail | |
| pluginval VST3 | | | pass / fail | |
| pluginval AU | | | pass / fail | |
| auval | | | pass / fail | |
| REAPER smoke | | REAPER x.x | pass / fail | |
| Logic smoke | | Logic x.x | pass / fail | |
| VST3/AU parity | | | pass / fail | |
| Playwright (optional) | | | pass / fail / skipped | |
| VCV (optional) | | | pass / fail / skipped | |

---

## 7. After manual tests pass

1. Paste or summarize Evidence into `final-audit.md` (or keep it in this file).
2. Mark **5.10** and **9.7** complete in `tasks.md`.
3. Run closure:

   ```sh
   scripts/check_openspec_hygiene.sh --post-closure
   openspec archive omni-repository-harmonization -y
   scripts/check_openspec_hygiene.sh --post-closure   # should still pass; zero active changes
   ```

4. Commit when ready (you drive git — nothing is committed automatically from this plan).

---

## Quick reference: what is already automated

You do **not** need to re-run these for 5.10/9.7 unless you change code:

```sh
scripts/verify_clean_rebuild.sh
scripts/check_host_artifact_hygiene.sh
scripts/check_openspec_hygiene.sh --post-closure
cd web && npm run verify:host-display && npm run verify:wasm-render-allocation
cd desktop/build && ctest -R 'HostParameter|AudioRecorder' --output-on-failure
cd sim/build && ctest --output-on-failure
```

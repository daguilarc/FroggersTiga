# Audit Remediation Implementation Plan — 2026-07-27

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Correct the falsified and unresolvable claims in `HANDOFF.md` and the `froggers-sheaf-app` OpenSpec artifacts, fix the stale-build defect that would silently invalidate any successor's DSP edit, and replace Defect A's refuted diagnosis with a traced diagnostic.

**Architecture:** Three phases. Phase 1 fixes the build-dependency defect (it gates everything else — until it lands, no source edit is reliably compiled). Phase 2 corrects the written record so the successor inherits facts instead of assertions. Phase 3 runs the one diagnostic that can actually localize Defect A. Phase 3's *fix* task is deliberately unspecified: the cause is unknown, and specifying a fix for an unproven cause is the exact error this plan exists to correct.

**Tech Stack:** GNU Make (Sheaf is Makefile-only, not CMake), C++17, Catch2-style test harness in `app/`, Markdown artifacts under `openspec/changes/froggers-sheaf-app/`.

**Scope note:** This plan does NOT regenerate OpenSpec. Per OMNI §2, OpenSpec is generated once per task scope and is never regenerated inside the execution loop. The `froggers-sheaf-app` change already owns this work — task 12.5 is "docs closure" and task 12.1 is the test gate. This plan is a derived proposal (OMNI §3) that amends those existing artifacts in place.

**Explicitly out of scope:** Addendum defects **B** (window size), **C** (Play/Stop icons), **D** (dropdown labels). They are untouched successor work. Also out of scope: pushing/upstreaming the Sheaf fork, and tasks 11.6–11.12.

## Global Constraints

- Subagents: **Sonnet or Haiku, never Opus.** Set the model explicitly on every dispatch — an omitted model inherits the session's expensive one (OMNI §4).
- Builds capped at `-j2` with `nice` (8-core / 16 GB machine). Never raise this.
- Never add `Co-Authored-By` or any AI attribution to any commit.
- OMNI §16.1: build and test runs go through a subagent, which reports pass/fail counts plus failure tail only — never the full log.
- Frozen trees `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/` must stay byte-identical. `git diff b409106 --name-status` must continue to show **only** `M External/Sheaf`. No task in this plan may touch them.
- `openspec/` is gitignored; edit it on disk.
- Nothing in the parent repo is committed. Commit steps below stage only untracked/new work and must never stage a frozen tree.

---

## Preflight Trace (OMNI §1, §14)

Every claim this plan acts on, cited to the line where it was read. A §14 preflight rejects a proposal whose trace is missing, partial, or asserts unread behavior.

### T1 — Header dependency tracking is absent (grounds Task 1)

- `EXTRA_APP_HEADERS ?=` declared — `External/Sheaf/projects/synth/apps/sheaf-patch/Makefile:20`
- Hard `$(error)` if unset while `EXTRA_APP_DIR` is set, with the rationale "list the headers so edits trigger a rebuild" — `Makefile:25`
- `APP_HEADERS := $(APP_DIR)/Launcher.hpp \` … — `Makefile:32`
- `APP_HEADERS += $(EXTRA_APP_HEADERS)`, placed after the `:=` assignment as upstream requires — `Makefile:47`
- `$(APP): $(APP_HEADERS)` — the list is a literal prerequisite list — `Makefile:48`
- **No `-MMD`, `-MP`, `DEPFLAGS`, or `-include *.d`** in `Makefile` or `runtime/juce_build.mk` (grep returned empty for both). Therefore header dependencies are **not** auto-generated; an unlisted header is untracked.
- Actual header count: 8 in `app/` (`Froggers.hpp`, `FroggersAppCore.hpp`, `FroggersModulation.hpp`, `FroggersParameters.hpp`, `FroggersRandomShVisualizer.hpp`, `FroggersRegistration.hpp`, `FroggersTransferFunctionVisualizer.hpp`, `FroggersUiSurface.hpp`) + 10 in `app/dsp/` (`Delay.hpp`, `Drive.hpp`, `DspMath.hpp`, `EnvelopeFollowers.hpp`, `FilterFx.hpp`, `Fuegoize.hpp`, `RandomShLane.hpp`, `Reverb.hpp`, `Vco.hpp`, `VoiceEnvelope.hpp`) = **18**.
- `HANDOFF.md:21-27` lists **4**. `openspec/.../tasks.md:32` and `design.md:196` list **2**.
- The trap is documented upstream and in our own design: `design.md:188` ("builds that succeed while silently ignoring changes to our headers").

**Consequence:** 14 of 18 headers are untracked, including every file in `app/dsp/`. A successor editing `app/dsp/FilterFx.hpp` and rebuilding gets a stale binary and concludes their fix did nothing.

### T2 — Defect A's stated entry points are unreachable (grounds Task 4)

- `ExpMapCompute(min, max, value) { return min * std::pow(max / min, value); }` — `app/dsp/DspMath.hpp:43-46`. With `min > 0`, the result is `> 0` for every finite `value`.
- Comb: `combFreq = dsp::ExpMapCompute(20.0f / sampleRate_, 10000.0f / sampleRate_, knob(FroggersBankId::Filter, 4))` — `app/FroggersAppCore.hpp:617-618`. Floor ≈ `4.17e-4` at 48 kHz, never `0`.
- The `1.0f / freq` result is clamped: `std::min<std::size_t>(kSize - 1, std::max<std::size_t>(1, (std::size_t)GetDelaySamples(combFreq)))` — `app/FroggersAppCore.hpp:619-621`.
- Peak Q: `filterChain_.peak.SetWidth(dsp::ExpMapCompute(0.1f, 10.0f, knob(FroggersBankId::Filter, 3)))` — `app/FroggersAppCore.hpp:616`. `q ∈ [0.1, 10]`, never `0`, so `alpha = sinw / (2.0f * q)` (`app/dsp/FilterFx.hpp:166`) stays finite.
- Height: `filterChain_.peak.SetHeight(dsp::ExpMapCompute(1.0f, 10.0f, knob(FroggersBankId::Filter, 2)))` — `app/FroggersAppCore.hpp:615`. `height ∈ [1, 10]`, so `std::sqrt(height)` (`app/dsp/FilterFx.hpp:164`) never sees a negative.
- **Every non-test call site enumerated** (OMNI §1 requires all definition sites, not the first found): `peak.SetFreq` `:613`, `peak.SetHeight` `:615`, `peak.SetWidth` `:616`, `comb.delaySamples` `:619`. No others outside `app/FroggersDspParityTests.cpp`.
- Naming error in the current text: "Comb offset" is Filter knob **0**, which feeds `pureDelay.SetDelaySeconds(combOffsetSeconds, sampleRate_)` — `app/FroggersAppCore.hpp:610-612` — and never reaches `GetDelaySamples`. Only knob 4 does.
- Checked and clean, not an entry point: `SampleRateReducer::Process` guards `freq >= 1.0f` and `freq <= 0.0f` explicitly with no division — `app/dsp/Drive.hpp:158-176`; its inputs are `1e-2f + ZeroedExpCompute(...)` — `app/FroggersAppCore.hpp:584-587`.

**Consequence:** Defect A's proposed fix 1 ("clamp the divisors at their source… fixes the actual cause") would add branches that can never be taken — an OMNI §12 violation ("verify real possibility, remove impossible branches") — and would close the defect while the real cause stays live. The **symptom is real and operator-observed**; only the named cause is refuted. Proposed fix 3 (endpoint sweep) remains correct and becomes Task 8.

### T3 — Citation roots are ambiguous and one is materially wrong (grounds Task 5)

- `HANDOFF.md:4` fixes repo root at `/Users/diegoaguilar-canabal/Desktop/FroggersTiga`; `HANDOFF.md:43` lists `src/` as a frozen tree of that repo.
- Resolving at that root **succeeds** for `src/core/RGen.hpp:12` (`static uint32_t s_state;` — verified) and `src/core/Parameter.hpp:143`.
- Resolving at that root **fails** for `src/MasterClock.cpp` and `src/ParameterModulation.cpp`; both live at `External/Sheaf/projects/synth/src/`.
- Two Sheaf copies exist: `External/Sheaf/projects/synth/` and `desktop-v2/External/Sheaf/`, and their line numbers **disagree**. `Runtime.hpp:502-511` spans `audioDeviceAboutToStart` including `engine_.Prepare(sampleRate, blockSize)` in the fork copy; the same range in the `desktop-v2` copy starts mid-function, offset ~3 lines.
- Same-line copies (ambiguous but harmless): `ParameterModulation.cpp:3069-3071`, `Shell.hpp:86-88`.

### T4 — Stale claims, verified stale (grounds Tasks 5–7)

- `app/README.md:12` — "**No file inside `External/Sheaf` is edited by this project.**" False since the fork.
- `openspec/.../tasks.md:18` (task 1.1) — same claim, still worded as a live constraint. `tasks.md:9` marks the *general* claim historical, but `:18` itself is unamended.
- `openspec/.../proposal.md:13` — "**no file inside the submodule is modified**", unamended.
- `openspec/.../tasks.md:32` and `design.md:196` — 2-header `EXTRA_APP_HEADERS` example.
- `openspec/.../tasks.md:215` (task 12.4) — "so `.bin` is genuinely reproducible". Refuted by `HANDOFF.md:49-53` (baseline `6ca56ee8…`/88964 bytes vs fresh out-of-tree `be0dc826…`/89172 bytes, diverging at byte 5 in the ISR vector table).
- `HANDOFF.md:99` — "Repo rename `daguilarc/FroggersTiga` → `froggers3` — operator's action, gates 11.5 onward." **Wrong name and wrong status.** `git remote -v` returns `git@github.com:daguilarc/frogg3rs.git`; `tasks.md:200` records 11.5 complete as `frogg3rs`; `tasks.md:193` records the gate passed and re-pointed at the fork.

### T5 — Verified accurate, do not "fix"

Confirmed correct and to be left alone: baseline `b409106` and the frozen-tree property (`git diff b409106 --name-status` → only `M External/Sheaf`); fork state (`froggers-fork`, two commits past `1940ddcb`, `04818deb` + `7fa9ce34`, one file, 40 insertions — "~40 lines in one file" is exact); `MasterClock.cpp:929` (`transportState_ = ClockTransportState::Stopped;`); `ParameterModulation.hpp:930-949` (the `MessageIn::Type` enum, which contains no note type); `MidiController.cpp:725-731` (note-on → `ParamPush`); `EncoderDraw.hpp:326,790`; `sim/DelayState.hpp:187-193`; `External/Sheaf/.gitignore:17`; `FroggersAppCore.hpp:158-159` and `:676`; `FilterFx.hpp:79-81`; `Vco.hpp:69`; and the grep-exhaustiveness claim about `isfinite` under `app/`.

**Unverified, and must not be restated as fact:** "120 tests green across 9 binaries." No suite was run during the audit. Task 2 establishes it.

---

## File Structure

| File | Responsibility in this plan |
|---|---|
| `External/Sheaf/projects/synth/apps/sheaf-patch/Makefile` | **Not modified.** The fix is at the call site, not in the fork — keeps the fork at 2 commits. |
| `app/build-launcher.sh` | **Created.** Single source of truth for the launcher build command; globs headers so the list cannot rot again. |
| `app/README.md` | Corrected fork claim; points at the new script. |
| `HANDOFF.md` | Defect A rewrite, citation-root disambiguation, rename status, header-list pointer. |
| `openspec/.../tasks.md` | Tasks 1.1, 2.2, 12.4 amended; 12.5 scope recorded. |
| `openspec/.../design.md` | D1a header example corrected. |
| `openspec/.../proposal.md` | Submodule claim amended. |
| `app/FroggersEndpointSweepTests.cpp` | **Created.** The Defect A diagnostic. |

---

## Phase 1 — Unblock the build (must land first)

### Task 1: Make the launcher build track every app header

**Files:**
- Create: `app/build-launcher.sh`
- Modify: `app/README.md` (append a "Build the launcher" section)

**Interfaces:**
- Produces: `app/build-launcher.sh`, invoked as `./app/build-launcher.sh` from the repo root. Every later task and all future rebuilds use it instead of a pasted command.

**Why a script rather than a longer pasted command:** listing 18 headers by hand re-rots the moment a 19th is added. Globbing fixes the class, not the instance. The Makefile's `$(error)` at `Makefile:25` still enforces that the variable is non-empty.

- [ ] **Step 1: Write the failing test**

Create `app/build-launcher.sh` is Step 3; first prove the defect exists. Run this check — it compares what the current 4-header command tracks against what exists on disk:

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga
comm -23 \
  <(ls app/*.hpp app/dsp/*.hpp | xargs -n1 basename | sort) \
  <(printf '%s\n' FroggersRegistration.hpp Froggers.hpp FroggersAppCore.hpp FroggersUiSurface.hpp | sort)
```

- [ ] **Step 2: Run it to verify the defect**

Expected: 14 filenames printed — `Delay.hpp`, `Drive.hpp`, `DspMath.hpp`, `EnvelopeFollowers.hpp`, `FilterFx.hpp`, `FroggersModulation.hpp`, `FroggersParameters.hpp`, `FroggersRandomShVisualizer.hpp`, `FroggersTransferFunctionVisualizer.hpp`, `Fuegoize.hpp`, `RandomShLane.hpp`, `Reverb.hpp`, `Vco.hpp`, `VoiceEnvelope.hpp`. Non-empty output IS the defect.

- [ ] **Step 3: Write the script**

```bash
#!/usr/bin/env bash
# Builds the sheaf-patch launcher with the Froggers app injected out-of-tree.
#
# EXTRA_APP_HEADERS must list EVERY header the app compiles: the sheaf-patch
# Makefile uses them as literal prerequisites (Makefile:47-48) and generates no
# -MMD/-MP dependency files, so an unlisted header is untracked and edits to it
# produce a build that succeeds while silently ignoring the change
# (design.md:188). Globbing keeps that list from rotting.
set -euo pipefail

cd "$(dirname "$0")/.."
REPO_ROOT="$PWD"

APP_HEADERS="$(ls "$REPO_ROOT"/app/*.hpp "$REPO_ROOT"/app/dsp/*.hpp | tr '\n' ' ')"

# -j2 + nice: 8-core / 16 GB machine, higher parallelism freezes it.
nice make -j2 -C External/Sheaf/projects/synth/apps/sheaf-patch \
  EXTRA_APP_DIR="$REPO_ROOT/app" \
  EXTRA_APP_HEADER=FroggersRegistration.hpp \
  EXTRA_APP_TYPE=synth_froggers::FroggersApp \
  EXTRA_APP_REGISTRAR=synth_froggers::MakeFroggersRegistration \
  EXTRA_APP_HEADERS="$APP_HEADERS" \
  "$@"
```

- [ ] **Step 4: Make it executable and verify it tracks all 18**

```bash
chmod +x app/build-launcher.sh
bash -c 'cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga && ls app/*.hpp app/dsp/*.hpp | wc -l'
```

Expected: `18`.

- [ ] **Step 5: Prove the stale-build defect is actually fixed**

This is the real gate — a touch on a previously-untracked header must now relink. Dispatch to a **Haiku** subagent (OMNI §16.1); it reports only the two `ls -la` timestamps and whether they differ.

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga
APP=External/Sheaf/projects/synth/apps/sheaf-patch/build/SheafPatch.app/Contents/MacOS/SheafPatch
./app/build-launcher.sh >/dev/null 2>&1
BEFORE=$(stat -f %m "$APP")
touch app/dsp/FilterFx.hpp
./app/build-launcher.sh >/dev/null 2>&1
AFTER=$(stat -f %m "$APP")
[ "$BEFORE" != "$AFTER" ] && echo "PASS: FilterFx.hpp edit now triggers a rebuild" || echo "FAIL: still stale"
```

Expected: `PASS`.

- [ ] **Step 6: Point the README at the script**

Append to `app/README.md`:

```markdown
## Build the launcher

```bash
./app/build-launcher.sh
```

Do not paste a hand-written `make` command. `EXTRA_APP_HEADERS` must list every
header the app compiles — the Makefile treats them as literal prerequisites
(`Makefile:47-48`) and generates no `-MMD` dependency files, so an unlisted
header is silently untracked. The script globs `app/*.hpp` and `app/dsp/*.hpp`
so the list cannot fall out of date.
```

- [ ] **Step 7: Commit**

```bash
git add app/build-launcher.sh app/README.md
git commit -m "build: track every app header in the launcher build

The sheaf-patch Makefile uses EXTRA_APP_HEADERS as literal prerequisites and
emits no -MMD dependency files, so the 4-header command left 14 headers --
all of app/dsp/ among them -- untracked. Edits to them produced builds that
succeeded while ignoring the change."
```

### Task 2: Establish the test baseline

**Files:** none modified. This task produces a number the rest of the plan is measured against.

**Interfaces:**
- Produces: a verified pass count and binary count, replacing the unverified "120 tests green across 9 binaries."

- [ ] **Step 1: Run the suite via subagent**

Dispatch a **Haiku** subagent (OMNI §16.1 — the parent must not ingest the log). Its entire report must be: the number of test binaries, total assertions/tests passed, total failed, and — only if non-zero — the last 20 lines of the first failure.

- [ ] **Step 2: Record the result**

If it matches 120/9, no edit needed. If it differs, correct `HANDOFF.md:11` to the observed numbers and note the date. Do not restate the old figure without having run it.

- [ ] **Step 3: Commit only if a doc changed**

```bash
git add HANDOFF.md
git commit -m "docs: record verified test baseline"
```

---

## Phase 2 — Correct the written record

Each task here is independently reviewable and independently revertible. Dispatch each to a **Haiku** subagent; these are bounded text edits with exact old/new strings.

### Task 3: Correct the rename status

**Files:**
- Modify: `HANDOFF.md:99`

- [ ] **Step 1: Replace the line**

Old:

```markdown
- **Repo rename** `daguilarc/FroggersTiga` → `froggers3` — operator's action, gates 11.5 onward.
```

New:

```markdown
- ~~**Repo rename**~~ **DONE 2026-07-27.** Renamed to **`frogg3rs`** — note: `frogg3rs`, *not*
  `froggers3` as this line previously said. `git remote -v` → `git@github.com:daguilarc/frogg3rs.git`.
  Task 11.5 is complete (`tasks.md:200`); the 11.5 gate has passed (`tasks.md:193`). Publication is
  now blocked on the Sheaf fork alone. Remaining for 11.6: `pages.yml` still has
  `VITE_BASE=/FroggersTiga/` and must become `/frogg3rs/`.
```

- [ ] **Step 2: Verify no other occurrence of the wrong name survives**

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga && grep -rn "froggers3" HANDOFF.md app/ openspec/changes/froggers-sheaf-app/ | grep -v "not \`froggers3\`\|not .froggers3"
```

Expected: no output.

- [ ] **Step 3: Commit**

```bash
git add HANDOFF.md
git commit -m "docs: correct rename status -- frogg3rs, and it is already done"
```

### Task 4: Replace Defect A's diagnosis with the traced finding

**Files:**
- Modify: `HANDOFF.md:133-175` (section "A. Audio dies permanently after turning encoders")

**Interfaces:**
- Consumes: trace T2 above.
- Produces: the diagnosis Task 8's test is written against.

- [ ] **Step 1: Replace the "Two unguarded divisions" block and the proposed fix list**

Replace everything from `**Two unguarded divisions are the likely entry points, both reachable by turning an encoder:**` through the end of the numbered proposed-fix list with:

```markdown
**The originally-proposed cause was WRONG — audited and refuted 2026-07-27. Do not implement it.**
The earlier draft named two divisions as entry points: `GetDelaySamples`'s `1.0f / freq`
(`app/dsp/FilterFx.hpp:297`) and `alpha = sinw / (2.0f * q)` (`app/dsp/FilterFx.hpp:166`), on the
theory that Comb delay or Peak Q could reach `0`. **Neither can.** Every non-test call site was
enumerated, and all three divisors are exponentially mapped between strictly positive endpoints:

- `ExpMapCompute(min, max, v) = min * pow(max/min, v)` (`app/dsp/DspMath.hpp:43-46`) — with
  `min > 0` the output is `> 0` for every finite `v`.
- Comb delay: `ExpMapCompute(20/sr, 10000/sr, knob)` (`app/FroggersAppCore.hpp:617-618`), floor
  ≈ 4.17e-4 at 48 kHz. The `1/freq` result is then clamped to `[1, kSize-1]` and cast to `size_t`
  (`:619-621`).
- Peak Q: `ExpMapCompute(0.1f, 10.0f, knob)` (`:616`) — `q ∈ [0.1, 10]`.
- Height: `ExpMapCompute(1.0f, 10.0f, knob)` (`:615`) — `sqrt(height)` never sees a negative, so the
  old item 4 is also a non-issue on this path.
- Also checked and clean: `SampleRateReducer` guards `freq <= 0` and `>= 1` with no division
  (`app/dsp/Drive.hpp:158-176`).

A terminology error fed the wrong theory: **"Comb offset" is Filter knob 0**, which feeds
`pureDelay.SetDelaySeconds` (`:610-612`) and never reaches `GetDelaySamples`. Only knob 4 does.

**What still stands:** the symptom is real and operator-observed, and NaN-latching in recursive
state remains a plausible *mechanism* — `SanitizeOutputSample` (`:676`) does run on the final
output only, and the sole `isfinite` uses under `app/dsp/` really are `FilterFx.hpp:79-81`
(visualizer math) and `Vco.hpp:69`. What is refuted is the entry point, not the mechanism.

**Live hypotheses, in the order worth testing:**
1. A knob value arriving NaN or out of `[0,1]` from upstream (`CachedKnobValue`,
   `External/Sheaf/projects/synth/include/synth/ParameterModulation.hpp:477`) — this would poison
   every mapping at once and is the only candidate consistent with "all audio, not one effect."
2. `StereoDelay`'s read index at endpoint exposure.
3. A **non**-NaN latch: state that simply stops updating (a stuck `output`, a zeroed feedback path),
   which would produce identical symptoms and which no `isfinite` guard would ever catch.

**Do NOT add clamps before the diagnostic runs.** Clamping an impossible branch is an OMNI §12
violation, and it would close this defect while the real cause stays live.

**The diagnostic is the endpoint sweep** — see the remediation plan's Task 8
(`openspec/changes/froggers-sheaf-app/remediation-audit-2026-07-27.md`). It is the one item from the
original proposal that survives audit intact: no current test drives a parameter to its range
endpoints, which is why 120 green tests missed this.
```

- [ ] **Step 2: Verify the refuted fix is gone**

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga && grep -n "Clamp the divisors\|fixes the actual cause" HANDOFF.md
```

Expected: no output.

- [ ] **Step 3: Commit**

```bash
git add HANDOFF.md
git commit -m "docs: refute Defect A's stated cause, keep the endpoint-sweep diagnostic

Both named entry points are exp-mapped between strictly positive endpoints and
cannot reach zero. Symptom and NaN mechanism stand; the cause does not."
```

### Task 5: Disambiguate every citation root

**Files:**
- Modify: `HANDOFF.md` — lines 70-92 (the traps list) and any other Sheaf-relative citation.

- [ ] **Step 1: Add a resolution key immediately after `HANDOFF.md:7`**

```markdown
**Citation roots.** Paths are relative to the repo root unless prefixed. Sheaf paths resolve at
`External/Sheaf/projects/synth/` and are written with the `Sheaf:` prefix below. This matters:
`desktop-v2/External/Sheaf/` is a **second, differently-laid-out copy**, and the line numbers do not
agree between them — `Runtime.hpp:502-511` spans `audioDeviceAboutToStart` in the fork copy but
starts mid-function in the desktop-v2 copy. A bare `src/` means this repo's frozen firmware tree
(`src/core/RGen.hpp`), never Sheaf's.
```

- [ ] **Step 2: Prefix the Sheaf citations**

Apply exactly these replacements in `HANDOFF.md`:

| Old | New |
|---|---|
| `` `src/MasterClock.cpp:929` `` | `` `Sheaf:src/MasterClock.cpp:929` `` |
| `` `Runtime.hpp:502-511` `` | `` `Sheaf:runtime/Runtime.hpp:502-511` `` |
| `` `AppContext.hpp:95,101-102` `` | `` `Sheaf:include/synth/AppContext.hpp:95,101-102` `` |
| `` `ParameterModulation.hpp:930-949` `` | `` `Sheaf:include/synth/ParameterModulation.hpp:930-949` `` |
| `` `MidiController.cpp:725-731,851-903` `` | `` `Sheaf:src/MidiController.cpp:725-731,851-903` `` |
| `` `src/ParameterModulation.cpp:3069-3071` `` | `` `Sheaf:src/ParameterModulation.cpp:3069-3071` `` |
| `` `EncoderDraw.hpp:326,790` `` | `` `Sheaf:include/synth/EncoderDraw.hpp:326,790` `` |
| `` `runtime/Shell.hpp:86-88` `` | `` `Sheaf:runtime/Shell.hpp:86-88` `` |

Leave `src/core/RGen.hpp:12`, `src/core/Parameter.hpp:143`, `sim/DelayState.hpp`, and all `app/`
paths unchanged — they already resolve at the repo root.

- [ ] **Step 3: Verify every citation now resolves**

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga
grep -o '`Sheaf:[^`]*`' HANDOFF.md | tr -d '`' | sed 's/^Sheaf://; s/:.*//' | sort -u | \
  while read -r f; do [ -f "External/Sheaf/projects/synth/$f" ] || echo "UNRESOLVED: $f"; done
```

Expected: no output.

- [ ] **Step 4: Commit**

```bash
git add HANDOFF.md
git commit -m "docs: disambiguate Sheaf vs frozen-tree citation roots"
```

### Task 6: Retire the "no file inside External/Sheaf is edited" claim at every site

**Files:**
- Modify: `app/README.md:12-14`
- Modify: `openspec/changes/froggers-sheaf-app/tasks.md:18`
- Modify: `openspec/changes/froggers-sheaf-app/proposal.md:13`

`tasks.md:8-9` and `design.md:179,199` are **already** amended — do not touch them. These three are the unamended stragglers.

- [ ] **Step 1: `app/README.md` — replace lines 12-14**

Old:

```markdown
**No file inside `External/Sheaf` is edited by this project.** The gitlink is
the only change to the submodule's presence in this repo — everything the
hook needs is already upstream at the pinned commit.
```

New:

```markdown
**SUPERSEDED 2026-07-27 — `External/Sheaf` is now a fork.** This held only through the
`1940ddcb` pin. The submodule sits on local branch **`froggers-fork`**, two commits ahead
(`04818deb`, `7fa9ce34`), both in `projects/synth/juce/PortableJuceBackend.hpp`. **Those commits
exist only on this machine**, so until the branch is pushed or upstreamed, no other checkout can
build and the browser publish is blocked. See design.md D1's 2026-07-27 amendment.
```

- [ ] **Step 2: `tasks.md:18` — append to task 1.1**

Append this sentence to the end of the existing 1.1 line (do not delete the historical text):

```markdown
 **[SUPERSEDED 2026-07-27: the "no file inside `External/Sheaf` is edited" constraint no longer holds — see the amendment at the top of this file and design D1. Retained as the record of what task 1.1 required at the time it was executed.]**
```

- [ ] **Step 3: `proposal.md:13` — append to the NEW APP bullet**

```markdown
 **[SUPERSEDED 2026-07-27: "no file inside the submodule is modified" held only through `1940ddcb`. Sheaf is now a fork on local branch `froggers-fork`, two commits ahead, unpushed — see design D1's amendment.]**
```

- [ ] **Step 4: Verify no unamended claim survives**

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga
grep -rn "no file inside\|No file inside" app/README.md openspec/changes/froggers-sheaf-app/ | grep -vi "superseded\|historical\|amend"
```

Expected: no output.

- [ ] **Step 5: Commit**

```bash
git add app/README.md
git commit -m "docs: retire the pristine-submodule claim in app/README"
```

(`openspec/` is gitignored; its edits are not staged.)

### Task 7: Correct the header-count examples and the 12.4 reproducibility claim

**Files:**
- Modify: `openspec/changes/froggers-sheaf-app/tasks.md:32`
- Modify: `openspec/changes/froggers-sheaf-app/design.md:196`
- Modify: `openspec/changes/froggers-sheaf-app/tasks.md:215`

- [ ] **Step 1: `tasks.md:32` — replace the example line**

Old (4-space indent):

```
    EXTRA_APP_HEADERS="$PWD/app/FroggersRegistration.hpp $PWD/app/Froggers.hpp"
```

New:

```
    EXTRA_APP_HEADERS="$(ls $PWD/app/*.hpp $PWD/app/dsp/*.hpp)"
```

- [ ] **Step 2: `design.md:196` — same replacement, 2-space indent**

Old:

```
  EXTRA_APP_HEADERS="$PWD/app/FroggersRegistration.hpp $PWD/app/Froggers.hpp"
```

New:

```
  EXTRA_APP_HEADERS="$(ls $PWD/app/*.hpp $PWD/app/dsp/*.hpp)"
```

- [ ] **Step 3: Append the rationale to `design.md:188`**

```markdown
 **Corrected 2026-07-27:** the two-header example above was not merely stale, it was actively
dangerous — the app now has 18 headers, and the Makefile emits no `-MMD` dependency files, so the
14 unlisted ones (all of `app/dsp/` included) were untracked. Use `app/build-launcher.sh`, which
globs, rather than any hand-written list.
```

- [ ] **Step 4: `tasks.md:215` — replace the reproducibility clause**

Old:

```
There is no `__DATE__`/`__TIME__` anywhere under `src/`, so `.bin` is genuinely reproducible;
```

New:

```
There is no `__DATE__`/`__TIME__` anywhere under `src/`, **but the "genuinely reproducible" claim
that followed is FALSE — audited 2026-07-27.** The tracked `.bin` does not reproduce from baseline
sources with the current toolchain: baseline sha256 `6ca56ee8…` / 88964 bytes vs a fresh
out-of-tree build `be0dc826…` / 89172 bytes, diverging from byte 5 in the ISR vector table, even
though our own build is internally deterministic. **Task 12.4's output-hash half is therefore
invalid and stays open.** The source half of the proof — zero modifications under the seven frozen
trees — passes outright, and that is what actually protects Daisy;
```

- [ ] **Step 5: Verify**

```bash
cd /Users/diegoaguilar-canabal/Desktop/FroggersTiga
grep -c "FroggersRegistration.hpp \$PWD/app/Froggers.hpp" openspec/changes/froggers-sheaf-app/tasks.md openspec/changes/froggers-sheaf-app/design.md
```

Expected: `0` for both files.

- [ ] **Step 6: No commit** — `openspec/` is gitignored. Report completion instead.

---

## Phase 3 — Localize Defect A

**Gate:** Task 1 MUST be complete. Before it lands, an edit to `app/dsp/` does not rebuild, so any result from this phase is untrustworthy.

### Task 8: Endpoint-sweep diagnostic

**Files:**
- Create: `app/FroggersEndpointSweepTests.cpp`

**Interfaces:**
- Consumes: the app's existing test harness. Match the harness style already used by `app/FroggersDspParityTests.cpp` — read that file's includes and main/registration pattern first and mirror it exactly; do not introduce a second harness.
- Produces: a reproduction, or a proof that single-parameter endpoints are not the trigger. Either outcome is a result.

**Why this and nothing else:** the cause is unknown. Writing a fix task now would repeat the exact error this plan corrects. The fix task gets specified after this reports.

- [ ] **Step 1: Write the failing test**

The test must drive the **real** `Engine`/`Runtime` path, not `SynthRig` — trap 1 in `HANDOFF.md:67-69` records that `SynthRig` + `rig.StartAt(0)` is why the app shipped silent with 119 green tests. Start the transport with `MessageIn::Start`.

```cpp
// For each parameter in each bank: drive it to 0.0, then 1.0, then back to 0.5,
// running audio blocks between each move, and assert after every block that
//   (a) every output sample is finite, and
//   (b) the block is not silent when it should not be.
// (b) is the half that catches a non-NaN latch, which is live hypothesis 3.
TEST_CASE("every parameter endpoint leaves audio finite and alive") {
    for (auto bank : AllBanks()) {
        for (std::size_t ix = 0; ix < BankSize(bank); ++ix) {
            auto app = MakeRunningApp();           // real Engine/Runtime, MessageIn::Start
            RenderBlocks(app, 8);                  // settle
            const float baseline = Rms(LastBlock(app));
            REQUIRE(baseline > 0.0f);

            for (float endpoint : {0.0f, 1.0f, 0.5f}) {
                SetParameterNormalized(app, bank, ix, endpoint);
                RenderBlocks(app, 8);
                INFO("bank=" << BankName(bank) << " ix=" << ix << " endpoint=" << endpoint);
                REQUIRE(AllFinite(LastBlock(app)));
            }

            RenderBlocks(app, 32);
            INFO("bank=" << BankName(bank) << " ix=" << ix << " went permanently silent");
            REQUIRE(Rms(LastBlock(app)) > 0.0f);
        }
    }
}
```

- [ ] **Step 2: Run it**

Dispatch to a **Sonnet** subagent (this one needs judgement about the harness API). Build with `./app/build-launcher.sh` per Task 1. Report: pass/fail, and for each failure the bank name, index, and endpoint from the `INFO` line — not the log.

- [ ] **Step 3: Branch on the result**

- **If it fails:** the reported `bank`/`ix` names the parameter. That is the localization this phase exists to produce. Stop here and report — the fix is a new task, specified against the actual failing parameter, not guessed now.
- **If it passes:** single-parameter endpoints are **not** the trigger. Record that in `HANDOFF.md` as a hypothesis eliminated, and move to live hypothesis 1 (a NaN/out-of-range knob value from upstream) by asserting `std::isfinite(knob)` and `0.0f <= knob <= 1.0f` at `app/FroggersAppCore.hpp:546-548` in a debug build. Note that the operator's repro involved **click-dragging**, i.e. rapid successive changes — if single-parameter sweeps pass, the next test is concurrent/rapid multi-parameter motion, which the above deliberately does not cover.

- [ ] **Step 4: Commit the test regardless of outcome**

```bash
git add app/FroggersEndpointSweepTests.cpp
git commit -m "test: sweep every parameter to its range endpoints on the real engine path

No existing test drives a parameter to 0.0 or 1.0, which is why 120 green
tests missed the audio-death defect. Asserts finite AND non-silent -- the
second half catches a latch that no isfinite guard would see."
```

---

## Self-Review

**Spec coverage.** Audit findings → tasks: F1 (refuted diagnosis) → Tasks 4, 8. F2 (citation roots) → Task 5. F3 (stale docs, OMNI §16.2) → Tasks 6, 7. F4 (rename) → Task 3. F5 (no proposal for the addendum) → this document. F6 (weak `AppContext` citation) → **deliberately not tasked**; the claim it supports is sound on its other two legs (T5), so editing it is churn. New finding (untracked headers) → Tasks 1, 7. Unverified test count → Task 2. Addendum B/C/D → out of scope, stated up front.

**Placeholder scan.** No TBDs. Task 8 Step 3's branch is a genuine decision point with both arms specified, not a placeholder; the deliberately-unwritten fix task is justified in that task's preamble rather than left implicit.

**Type consistency.** `app/build-launcher.sh` is referred to by that exact path in Tasks 1, 7, and 8. `Sheaf:` prefix is defined in Task 5 Step 1 before its use in Step 2. Header count 18 = 8 + 10 is consistent across the trace, Task 1, and Task 7.

**Known risk.** Task 8's test pseudocode names harness helpers (`MakeRunningApp`, `RenderBlocks`, `Rms`, `AllFinite`, `SetParameterNormalized`) whose real signatures live in the existing test files. The task instructs the implementer to mirror `app/FroggersDspParityTests.cpp` rather than invent them. This is the one place the plan cannot be fully literal without reading a file the audit did not open.

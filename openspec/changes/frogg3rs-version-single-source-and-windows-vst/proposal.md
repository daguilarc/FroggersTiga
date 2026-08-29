# Proposal — `frogg3rs-version-single-source-and-windows-vst`

**Created 2026-08-28. Successor to `frogg3rs-browser-audio-device-selection`.**

Two things that change found the hard way, plus the Windows VST3 nobody scoped.

## 1. The browser ABI version is a literal in dozens of places

`SUPPORTED_BROWSER_ABI_VERSION` exists (`protocol.ts:2`), and almost nothing
uses it. The same number is hand-written as a bare literal across the tree: the
C++ `synth_browser_abi_version()` return, the C++ contract test's assertion,
every `_synth_browser_abi_version: () => N` stub in `static-server.mjs` and in
the tests, the synthesized fixture catalog in `static-server.mjs`, and the
`toBe(N)` in `two-origin-package.spec.ts`. A bump means finding all of them.

This is not hypothetical drift. Bumping 4 to 5 during the predecessor missed the
fixture-catalog literal sitting fourteen lines below a stub that was updated in
the same edit, because the two express one concept in two syntactic forms and a
search by form finds one of them.

The version is one fact. It gets one definition that every other site reads,
including the fixtures. Where a language boundary makes literal sharing
impossible, the mirror is generated or asserted equal by a test that fails on
drift, not maintained by hand.

## 2. A test run can silently use a server that predates the code

`static-server.mjs` had been serving port 4174 for over 24 hours when the
predecessor's suite ran against it, because Playwright reuses an existing
server rather than starting its own. It serves files from disk, so app code was
fresh, but its synthesized fixture catalog is in-process and therefore frozen at
whatever the process was started with. One test failed against a fixture that
predated the ABI bump by a day, and the failure was indistinguishable from a
real defect until the process start time was read.

A reused server SHALL be proven current before a run trusts it, or not reused.

## 3. There is no Windows VST3, and no recorded reason

The desktop application ships for macOS and Windows. The plugin ships for macOS
only. `2026-08-26-frogg3rs-windows-and-mobile` listed "A Windows VST3" as an
explicit non-goal and nothing has been opened since, so the absence is a scoping
decision that was never revisited, not a technical finding.

Traced, so the work is known rather than guessed:

- `app/vst/CMakeLists.txt:98` is `FORMATS VST3 AU` with no platform
  conditional. AU is macOS-only, so that line cannot build on Windows as
  written. VST3 is cross-platform.
- The doc-bundling loop (`:127`) and the signing loop (`:152`) both iterate
  `VST3 AU` and use `$<TARGET_BUNDLE_CONTENT_DIR:...>` and `codesign`. Both are
  macOS-shaped: `codesign` does not exist on Windows, and a Windows VST3 is a
  directory laid out differently from a macOS bundle.
- `.github/workflows/vst-plugin.yml` builds only on `macos-14`; its
  `ubuntu-latest` job publishes. `desktop-release.yml` already builds on
  `windows-latest`, and `72700a9` fixed Sheaf submodule checkout there, so the
  runner, toolchain and submodule path are proven for this repo.
- The workflow's packaging step uses `ditto` and asserts `test -d` on
  `Frogg3rs.vst3` and `Frogg3rs.component` — both macOS-only assumptions.
- No Authenticode certificate exists, so a Windows VST3 ships unsigned, exactly
  as the Windows standalone already does.

## Non-goals

- Changing plugin behaviour on any platform. This is packaging and build
  configuration.
- An LV2 or a Linux build.
- Signing Windows artifacts. There is no certificate, and inventing one is a
  business decision, not an engineering one.

## Impact

- `app/vst/CMakeLists.txt` — format list, doc bundling, signing, all three
  currently unconditional.
- `.github/workflows/vst-plugin.yml` — a Windows build job and packaging that
  does not assume `ditto` or a macOS bundle.
- `MANUAL.md` "Release platforms", whose text is extracted verbatim into every
  published release body by both release workflows.
- `frogg3rs-distribution`, which records what ships where.
- The browser ABI version's definition sites, and the Playwright server reuse.

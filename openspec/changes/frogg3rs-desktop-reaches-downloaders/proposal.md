# Proposal — `frogg3rs-desktop-reaches-downloaders`

**Created 2026-08-22.** Picks up the gap the archived
`frogg3rs-main-cutover-and-releases` stated: the site is live and the plugin
installs, but the desktop application does not reach anyone who downloads it.

## Why

**The macOS download will not open.** The published `.dmg` produces "Frogg3rs is
damaged and can't be opened", offering only Eject and Cancel. The file is not
damaged. `codesign --verify --deep --strict` on the shipped bundle reports
`code has no resources but signature indicates they must be present`, and
`spctl --assess` errors rather than returning a verdict at all.

The mechanism, traced: the linker ad-hoc-signs the binary
(`flags=0x20002(adhoc,linker-signed)`, `Info.plist=not bound`), and
`app/build-launcher.sh` then assembles the bundle and copies `Info.plist` in
afterwards. The signature therefore does not cover the bundle's contents, and
there is no `_CodeSignature` directory. A download carries
`com.apple.quarantine`; a quarantined bundle whose signature does not match
itself is what macOS calls damaged.

**This is not a regression.** The v1 release has the identical signature state,
verified by mounting its `.dmg`: same ad-hoc linker signature, same
`Sealed Resources=none`, same verify failure. It only ever worked for someone
whose copy was built locally and so never carried the quarantine attribute —
the operator's own v1 image, created 2026-06-13, has no quarantine attribute at
all. Nobody had downloaded a build of this project through a browser until
today.

Re-signing the assembled bundle changes the verdict from an error to a clean
`rejected` — measured, not assumed. That is the unidentified-developer path,
which offers Open Anyway under Privacy & Security; the current state offers
nothing. It is an improvement, not a cure: only Developer ID plus notarization
makes a download open normally, and this machine has zero signing identities.

**The Windows standalone does not build.** Three attempts each reached one step
further — submodule checkout, the build invocation, path spelling — and none
reached a compiler. It builds through Sheaf's `juce_build.mk`, a Makefile that
has only ever run on macOS, so this is a port rather than a configuration.
Both working Windows precedents in this repository use CMake instead: the v1
job that produced the `.exe` still attached to the v1 release, and
`app/vst/CMakeLists.txt`, which is cross-platform by construction.

## What Changes

- **frogg3rs-distribution** (delta): a downloadable build carries a signature
  that matches its own contents, and a release states what a downloader must do
  to open it.
- **`app/build-launcher.sh`**: signs the bundle after assembling it, so the
  signature covers what is actually in it.
- **A gate**: the shipped bundle passes `codesign --verify --deep --strict`.
  This defect shipped because nothing checked, and the check is one command.
- **`MANUAL.md`**: what a downloader sees and does, for as long as the build is
  not notarized.
- **Windows**: the standalone gets a CMake build, mirroring the two precedents
  that work, rather than further porting of the Makefile.
- Carried from the archived automation change: `sim/Fuegoize.hpp`'s
  divide-by-zero at full fuego, still unfixed and still uncovered.

## Impact

- Affected specs: `frogg3rs-distribution`.
- Notarization is recorded as the thing that makes the manual step unnecessary,
  and is not attempted here: it needs an Apple Developer account and CI secrets
  that do not exist.
- Retiring the v1 release stays blocked until a downloaded v2 opens, since v1
  is still the only desktop download that anyone can open at all.

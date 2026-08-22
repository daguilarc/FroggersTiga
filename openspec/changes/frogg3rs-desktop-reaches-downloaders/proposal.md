# Proposal — `frogg3rs-desktop-reaches-downloaders`

**Created 2026-08-22.** Picks up the gap the archived
`frogg3rs-main-cutover-and-releases` stated: the site is live and the plugin
release publishes, but what a downloader gets does not open on the machine
they downloaded it to.

## Why

**The macOS download will not open.** The published `.dmg` produces "Frogg3rs is
damaged and can't be opened", offering only Eject and Cancel. The file is not
damaged. `codesign --verify --deep --strict` on the shipped bundle reports
`code has no resources but signature indicates they must be present`, and
`spctl --assess` errors rather than returning a verdict at all.

The mechanism, traced: the linker ad-hoc-signs the binary
(`flags=0x20002(adhoc,linker-signed)`, `Info.plist=not bound`), and the bundle
is assembled around that binary afterwards, in two places. `juce_build.mk`'s
`$(APP_BUNDLE)` rule copies `Info.plist` and the binary into `Contents/`, and
`app/build-launcher.sh:62-71` then copies `Icon.icns`, `MANUAL.md` and
`QUICK_DICT.md` into `Contents/Resources/`. The signature covers none of it,
and there is no `_CodeSignature` directory. A download carries
`com.apple.quarantine`; a quarantined bundle whose signature does not match
itself is what macOS calls damaged.

**The Audio Unit ships in the same state**, measured on the built bundle:
`Frogg3rs.component` is `flags=0x20002(adhoc,linker-signed)`,
`Info.plist=not bound`, `Sealed Resources=none`, and fails `codesign --verify`
with the identical message. `Frogg3rs.vst3` beside it is fine
(`flags=0x2(adhoc)`, `Sealed Resources version=2`), and the difference is not
luck: JUCE's `_juce_adhoc_sign` is attached to the VST3 and LV2 targets and to
the copy-plugin step, and to no AU target
(`JUCEUtils.cmake:962-980,1001,1237,1364`). Both bundles are published as
release assets by `.github/workflows/vst-plugin.yml`. "The plugin installs" was
true of a locally built copy and has never been true of a downloaded one.

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
makes a download open normally, and this machine has zero signing identities
(`security find-identity -v -p codesigning` reports `0 valid identities`).

**The signature is the last step of a build or it is not a signature.** Also
measured, on a copy of the shipped bundle: sign the assembled bundle and
`--verify --deep --strict` passes; add one file to `Contents/Resources/`
afterwards and the same command reports `a sealed resource is missing or
invalid`. Signing after the binary and plist but before the resource copies
therefore trades one broken verdict for another, and `MANUAL.md` — which this
change edits — is one of those copied resources.

**The Windows standalone does not build, and why is not yet known.** Three
attempts each reached one step further — submodule checkout, the build
invocation, path spelling — and none reached a compiler. All three failed in CI
plumbing, so the toolchain's portability is untested rather than disproven, and
`juce_build.mk` already carries a Windows port nobody has run: the
Objective-C++/C++ unity-source switch and the MinGW-versus-MSVC link-flag
branch. The choice between finishing that and moving the standalone to CMake is
made after the trace in task 3.1 and against this repository's own precedents,
not before: `app/vst/CMakeLists.txt` links Sheaf's `libsynth.a` by invoking
Sheaf's own Makefile rather than restating its source list, and the v1 Windows
job that produced the `.exe` still attached to the v1 release used
`juce_add_gui_app` — both deleted with `desktop/`, readable at
`git show b9a8199^:desktop/CMakeLists.txt` and
`git show b9a8199^:.github/workflows/desktop-release.yml`.

## What Changes

- **frogg3rs-distribution** (delta): every bundle a release ships — application
  and plug-in alike — carries a signature that matches its own contents, and a
  release states what a downloader must do to open it.
- **`app/build-launcher.sh`**: signs the bundle as its last step, after every
  file that goes into it is in place.
- **`app/vst/CMakeLists.txt`**: signs the AU bundle after JUCE assembles it,
  since JUCE's own re-signing step is wired to VST3 and LV2 and not to AU.
- **A gate**: every bundle a release ships passes
  `codesign --verify --deep --strict`. This defect shipped because nothing
  checked, and the check is one command.
- **`MANUAL.md`**: what a downloader sees and does, for as long as the builds
  are not notarized.
- **Windows**: task 3.1's trace decides between the existing Makefile port and
  a CMake build; either way the synth core is reused, not re-listed.
- **Hygiene**: the packaging surface, the leftovers of the retired `desktop/`
  tree inside `sim/`, the tree names in `README.md` that no longer resolve, the
  uninvoked `scripts/`, and `sim/Fuegoize.hpp`'s divide-by-zero — the one code
  task the predecessor change archived undone.

**The site header shows the title without the logo.** The application has one —
`app/Resources/Icon.png`, the same image the bundle carries as its icon — and
the published header does not use it. Adding it is constrained by the
blank-frame guard, which measures `.site-header`'s box and samples only the band
below it: a logo inside the header preserves that exclusion, and one outside it
would let the guard pass over a blank app surface.

## Impact

- Affected specs: `frogg3rs-distribution`.
- Notarization is recorded as the thing that makes the manual step unnecessary,
  and is not attempted here: it needs an Apple Developer account and CI secrets
  that do not exist.
- Retiring the v1 release stays blocked until a downloaded v2 opens, since v1
  is still the only desktop download that anyone can open at all.
- The plugin release is in scope for the signature work even though its
  headline problem is the desktop one. The same defect, in a bundle already
  being published, is not a separate change.

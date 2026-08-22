# Tasks — `frogg3rs-desktop-reaches-downloaders`

Gates: `cd app && nice make -j2 test` (301/301); plugin targets from
`app/vst/build` — `FroggersVstHostTests` 46/46, smoke 1/1, editor 3/3; Sheaf
`nice make -C projects/synth -j2 test` (923 passed / 2 failed, the known
braid-4 96kHz deadline tests — that recipe aborts there, so run anything after
it directly); `nice make -C projects/synth/apps/miniapp -j2 test`, the only
target that builds the JUCE runtime shell; and the browser e2e suite, whose
`pages` project runs against a headerless origin. Never above `-j2`, always
`nice`.

## 0. Hygiene

- [ ] 0.1 Sweep the packaging surface. `app/build-launcher.sh` and the release
      workflows are the tree this change touches; report dead steps, stale
      paths, and anything naming a tree the last change retired.

## 1. The macOS bundle carries a signature that matches itself

- [ ] 1.1 Sign the bundle in `app/build-launcher.sh` AFTER assembling it —
      after the binary is copied in and after `Info.plist` is placed. Signing
      before assembly is what produced a signature covering neither.
- [ ] 1.2 The bundle passes `codesign --verify --deep --strict` and
      `spctl --assess --type execute` returns a verdict rather than an error.
      Both are one command; neither was ever run.
- [ ] 1.3 Gate it. A build whose signature does not match its contents fails
      the build rather than reaching a release. POSITIVE CONTROL required: show
      the gate failing on a deliberately unsigned or mis-assembled bundle, or
      it is a check that has never been observed to catch anything.
- [ ] 1.4 Verify against a real download, not a local build. Apply
      `com.apple.quarantine` to a packaged `.dmg`, open it, and record what
      macOS actually says. A locally built bundle carries no quarantine, which
      is precisely why this defect survived two releases.

## 2. The release says what a downloader must do

- [ ] 2.1 `MANUAL.md` states what a downloader sees and the step that opens it,
      for as long as the build is unnotarized. Plain present tense.
- [ ] 2.2 The release body carries the same, so someone who never opens the
      manual is not left with a dialog that says the file is damaged.
- [ ] 2.3 Record notarization as the thing that removes the step: Developer ID
      signing plus notarization is what makes a download open normally. It
      needs an Apple Developer account and CI secrets that do not exist, so it
      is named here, not attempted.

## 3. Windows builds through CMake

- [ ] 3.1 Trace and report before writing anything: what `juce_build.mk`
      assembles for the standalone (sources, defines, include paths, link
      flags), and what `app/vst/CMakeLists.txt` already does for the same JUCE
      modules. The overlap is the thing being moved.
- [ ] 3.2 Give the standalone a CMake build that produces the same application
      as `build-launcher.sh` does on macOS — Sheaf's runtime shell, not JUCE's
      `Standalone` plugin wrapper. That wrapper substitutes JUCE's own audio
      settings dialog for Sheaf's Audio page, so the two platforms would ship
      different applications and the no-input selector would not exist on
      Windows.
- [ ] 3.3 macOS keeps building byte-identically through whichever path ships,
      with counts reported. A port that quietly changes the macOS build is a
      regression wearing a feature's clothes.
- [ ] 3.4 The Windows job builds and the gate runs there. Report what it takes;
      if something cannot run on a Windows runner, say which step and why
      rather than marking it unverified and moving on.
- [ ] 3.5 Only once Windows genuinely builds: the desktop release ships both
      platforms again, and `MANUAL.md` stops saying Windows is in progress.

## 4. Carried from the archived automation change

- [ ] 4.1 Fix `sim/Fuegoize.hpp`'s divide-by-zero at full fuego: move the cast
      off the divisor so it matches the firmware's form, and add a test that
      drives fuego to maximum. Nothing exercises that path today, so it needs
      its own coverage — and the test must fail before the fix.

## 5. Close

- [ ] 5.1 All gates green, counts reported, including the miniapp target and
      the browser e2e `pages` project.
- [ ] 5.2 Postflight: implementation versus proposal, plus a duplication pass
      over the whole diff for every new named concept, and a check that no
      surviving script, workflow or manifest names a path that no longer
      exists.
- [ ] 5.3 OPERATOR: download the published `.dmg` through a browser — not a
      local build — and confirm it opens.
- [ ] 5.4 OPERATOR: retire the v1 release once that download opens, and not
      before. It is still the only desktop download anyone can open.

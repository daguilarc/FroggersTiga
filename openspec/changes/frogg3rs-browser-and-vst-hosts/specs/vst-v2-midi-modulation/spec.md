# Delta — `vst-v2-midi-modulation`

**Removed 2026-08-18 in `frogg3rs-browser-and-vst-hosts`, in its
entirety.** The capability specifies the deleted `desktop-v2/` VST
wrapper (deleted in commit `0be9ab0`). The dual-identity parameter idea
it contained is re-specified against the current six-bank model in this
change's `froggers-vst-host` delta, and `froggers-v2-app-manifest`
(untouched) independently specifies dual IDs — removal orphans nothing.
The spec directory is deleted at archive-time sync. This delta file was
added at the second 2026-08-18 omni-rule audit (previously prose-only in
the `froggers-vst-host` delta; repo convention is one delta file per
affected capability).

## REMOVED Requirements

### Requirement: vst-v2-full-parameter-surface

### Requirement: vst-v2-dual-parameter-ids

### Requirement: vst-v2-daw-midi-to-any-parameter

### Requirement: vst-v2-editor-parity-with-desktop-v2

### Requirement: vst-v2-stereo-output-default

### Requirement: VST v2 MIDI maps through host parameters

### Requirement: Hosted runtime projection excludes hardware configuration

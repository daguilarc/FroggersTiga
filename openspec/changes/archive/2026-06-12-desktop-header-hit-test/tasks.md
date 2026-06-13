# Desktop header hit-test fix — tasks

## 1. Record cluster click policy

- [x] 1.1 `RecordExportCluster` constructor: `setInterceptsMouseClicks(false, true)`

## 2. Layout bounds (OMNI: compute rects, apply once)

- [x] 2.1 `MainComponent::resized`: remove `m_recordCluster.setBounds(header)`
- [x] 2.2 Set `m_recordCluster.setBounds(recordGlobal.getUnion(formatGlobal))`
- [x] 2.3 `layoutChrome` with locals translated to cluster origin (not header origin)

## 3. Transport z-order

- [x] 3.1 After layout: `toFront` transport + cluster siblings, then `m_cableOverlay.toFront(false)`

## 4. Documentation

- [x] 4.1 Footnote `desktop-chrome-cohesion/design.md` §3 — cluster bounds = union, not full header

## 5. Verification

- [ ] 5.1 Cold launch → click Play → audio runs
- [ ] 5.2 Click Audio → device dialog; MIDI → MIDI dialog
- [ ] 5.3 RECORD + OGG toggle still work
- [ ] 5.4 Play → Record → Play sim → Record → save WAV → file plays back in external player (`desktop-audio-export` §6.1)
- [ ] 5.5 Verify stereo width in exported file when delay width active (`desktop-audio-export` §6.2)
- [ ] 5.6 Record without Play shows feedback, no file (`desktop-audio-export` §6.3)
- [ ] 5.7 FLAC/OGG/MP3 on builds with encoders linked (`desktop-audio-export` §6.4)
- [ ] 5.8 Visual: RECORD label not clipped; MIDI | Audio | RECORD order (`desktop-audio-export` §6.5)
- [x] 5.9 Desktop Release build

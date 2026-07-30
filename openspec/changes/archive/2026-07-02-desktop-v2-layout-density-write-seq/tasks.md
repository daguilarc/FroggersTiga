# Tasks: desktop-v2-layout-density-write-seq

## Section 1: Chrome constants (DesktopV2ChromeLayout.hpp)

- [x] kModuleRowCenterClusterX = gridPx(15), kCenterGlobalClusterW = gridPx(15), kModuleRowModX = gridPx(31)
- [x] kModCellW = gridPx(18), kSequencerH = gridPx(18), kSequencerStepCellSize = gridPx(3), kPerfMarblesColW = gridPx(6)
- [x] Remove moduleRowModX(rowWidth) usage; use kModuleRowModX + kModCellW

## Section 2: CenterGlobalClusterV2

- [x] Create desktop-v2/Source/ui/CenterGlobalClusterV2.hpp/cpp (vertical stack: Rand All, Rand Mods, Rand waveforms, Rand Resample, Crunchy label+ring, Shift)
- [x] Same callbacks as GlobalStripV2; Rand Mods sends MessageIn::RandSequencerMods to control core (add this message type)
- [x] Add to CMakeLists.txt
- [x] PageCarouselComponent: host center cluster at kModuleRowCenterClusterX, full viewport height beside encoder area (split content area: encoder viewport left, center cluster at fixed X)
- [x] MainComponent: remove m_globalStrip; bind host to carousel center cluster; setShiftHeld via carousel; refresh via carousel
- [x] HostedMainComponentV2: same

## Section 3: SubmodulePagePanel layoutRows use kModuleRowModX not moduleRowModX

- [x] PerformanceBandV2 already uses kPerfMarblesColW constant

## Section 5: Sequencer/control core

- [x] SequencerStepSnapshot: add modSource[kNumHostPages][kNumRows] uint8_t and modDepth float arrays
- [x] SequencerState: add m_writeSeqJustStarted bool
- [x] captureSequencerStepSnapshot/applySequencerStepSnapshot: copy/restore mod per page/row via applyHostModRoute
- [x] captureFactoryStepSnapshot: seed mod fields to kNoSelection/0
- [x] FroggersV2HostBridge::onSequencerStepAdvance: skip capture if m_writeSeqJustStarted (clear flag after); factory-seed blank steps; set m_editStep=m_playhead when writeSeq armed
- [x] SequencerPanelComponent: on Start Sequence capture set m_writeSeqJustStarted=true; set m_editStep=m_playhead when armed+playing; capture flash on step cells (m_captureFlashStep + timestamp, paint override)
- [x] onRandSequencerStep Step scope: target m_playhead when playing else m_editStep
- [x] Add onRandSequencerMods(scope): randomize mod into target snapshot(s); if playing and target==playhead apply snapshot to live
- [x] SequencerPanelComponent: add getRandSeqScope() returning kRandSeqScopeStep or kRandSeqScopePattern from m_patternScope

## Section 6: DesktopV2LookAndFeel

- [x] Override drawToggleButton: radioGroupId != 0 => circular radio; else checkbox default

## Tests (ControlCoreBridge_test.cpp)

- [x] Three beats steps 0,1,2 hasData without duplicate step0 on first beat
- [x] Blank step factory seed on advance
- [x] Rand-seq playhead target while playing
- [x] Rand Mods per-step snapshots

## Docs

- [x] QUICK_DICT.md center cluster + Write Seq workflow

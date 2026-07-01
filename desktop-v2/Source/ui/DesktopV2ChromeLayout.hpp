#pragma once

#include <cstdint>

// Desktop v2 chrome — fixed 10px grid unit (u). Spec: desktop-v2-grid-layout.
//
// Control footprints (width × height in u):
//   Encoder ring ........................ 5 × 5
//   Arrow / icon button ................. 2 × 2
//   Text button ......................... (textU + 2) × 3  (textU = ceil(glyphWidth / kGridUnitPx))
//   Toggle / scene (S1–S3, G1–G2) ....... 3 × 3
//   Mod source dropdown cell ............ 7 × 5
//   Row label column .................... 9 × 5
//   Encoder row height .................. 5

namespace DesktopV2ChromeLayout
{
constexpr int kGridUnitPx = 10;

constexpr int gridPx(int units) noexcept
{
    return units * kGridUnitPx;
}

constexpr int kDefaultWidth = gridPx(128);
constexpr int kDefaultHeight = gridPx(92);
constexpr int kHostedEditorMinWidth = gridPx(128);
constexpr int kHostedEditorMinHeight = gridPx(72);

constexpr int kTransportRowH = gridPx(7);  // standalone: transport + VCO EF scope
constexpr int kVstScopeStripH = gridPx(5); // VST: full-width VCO EF scope strip
constexpr int kGlobalStripH = gridPx(4);
constexpr int kPerformanceBandH = gridPx(7);
constexpr int kCarouselHeaderH = gridPx(3);
constexpr int kSequencerH = gridPx(13);
constexpr int kTextButtonH = gridPx(3);
constexpr int kArrowButtonSize = gridPx(2);
constexpr int kRandomizeButtonW = gridPx(11); // "Randomize" textU 9 + 2
constexpr int kRandModButtonW = gridPx(9);    // "Randmod" textU 7 + 2
constexpr int kChromePad = gridPx(1);
constexpr int kSectionGap = gridPx(1);

constexpr int kEncoderRowH = gridPx(5);
constexpr int kEncoderRingSize = gridPx(5);
constexpr int kModCellW = gridPx(7);
constexpr int kRowLabelW = gridPx(9);

constexpr int kVisibleEncoderSlots = 10;

constexpr int kPerfSceneLabelW = gridPx(4);
constexpr int kPerfSceneButtonSize = gridPx(3);
constexpr int kPerfMarblesLabelH = gridPx(1);
constexpr int kGlobalStripRandAllW = gridPx(8);
constexpr int kGlobalStripRandModsW = gridPx(9);
constexpr int kGlobalStripRandWaveformsW = gridPx(13);
constexpr int kGlobalStripRandResampleW = gridPx(12);
constexpr int kGlobalStripCrunchyLabelW = gridPx(6);
constexpr int kGlobalStripShiftW = gridPx(7);
constexpr int kSequencerToolbarH = gridPx(3);
constexpr int kSequencerStepCellSize = gridPx(2);
constexpr int kPerfBlendEndpointLabelW = gridPx(1);
constexpr int kPerfSceneBlendW = gridPx(8);
constexpr int kPerfGestureToggleW = gridPx(4);
constexpr int kPerfGestureWeightW = gridPx(7);
constexpr int kPerfSeqTransportW = 108;
constexpr int kPerfSeqRecordW = gridPx(6);
constexpr int kPerfBpmLabelW = gridPx(3);
constexpr int kPerfBpmSliderW = gridPx(10);
constexpr int kPerfStepsLabelW = gridPx(4);
constexpr int kPerfStepsSliderW = gridPx(8);
constexpr int kPerfMarblesColW = gridPx(3);

constexpr int encoderDocumentHeight(int rowCount) noexcept
{
    return rowCount * kEncoderRowH;
}
} // namespace DesktopV2ChromeLayout

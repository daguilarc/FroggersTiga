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

constexpr int kTransportRowH = gridPx(7);
constexpr int kVstScopeStripH = gridPx(5);
// Packet 17 (D14): two readable command/scope rows at 1280px need a 10px
// inter-row gap (kTextButtonH + kSectionGap + kTextButtonH).
constexpr int kGlobalCommandBandH = gridPx(7);
constexpr int kGlobalStripH = kGlobalCommandBandH;
constexpr int kPerformanceBandH = gridPx(7);
constexpr int kCarouselHeaderH = gridPx(3);
constexpr int kTextButtonH = gridPx(3);
constexpr int kArrowButtonSize = gridPx(2);
constexpr int kRandomizeButtonW = gridPx(11);
constexpr int kRandModButtonW = gridPx(9);
constexpr int kChromePad = gridPx(1);
constexpr int kSectionGap = gridPx(1);

constexpr int kEncoderRowH = gridPx(5);
constexpr int kEncoderRingSize = gridPx(5);
constexpr int kModCellW = gridPx(18);
constexpr int kModCellHeight = 48;
constexpr int kRowLabelW = gridPx(9);

constexpr int kModuleRowLabelOffset = 0;
constexpr int kModuleRowEncoderOffset = kRowLabelW;
constexpr int kModuleRowModGap = kSectionGap;
constexpr int kVisibleEncoderSlots = 10;

constexpr int kTransportScopeMaxWidth = 320;
constexpr int kRecordButtonMinWidth = gridPx(12);
constexpr int kTransportPlayStopW = 64;
constexpr int kTransportSettingsW = 72;
constexpr int kRuntimePageRailW = gridPx(6);
constexpr int kRuntimePageButtonH = gridPx(3);
constexpr int kTransportGapSm = 6;
constexpr int kTransportGapMd = 8;
constexpr int kModLabelStripH = 14;

constexpr int kSceneButtonMinWidth = 44;
constexpr int kBlendLabelMinWidth = 16;

constexpr int kPerfSceneLabelW = gridPx(5);
constexpr int kPerfSceneButtonSize = kSceneButtonMinWidth;
constexpr int kPerfMarblesLabelH = gridPx(2);
constexpr int kGlobalStripRandAllW = gridPx(8);
constexpr int kGlobalStripRandModsW = gridPx(11);
constexpr int kGlobalStripRandWaveformsW = gridPx(13);
constexpr int kGlobalStripRandResampleW = gridPx(12);
constexpr int kGlobalStripCrunchyLabelW = gridPx(6);
// Packet 17: scope radio minima sized for full label text at 1280px
// (toggle chrome ≈ 24px beyond glyph width).
constexpr int kGlobalScopeSceneAllW = gridPx(10);
constexpr int kGlobalScopeSceneCurrentW = gridPx(13);
constexpr int kGlobalScopeStepAllW = gridPx(9);
constexpr int kGlobalScopeStepCurrentW = gridPx(12);
constexpr int kSequencerToolbarH = gridPx(3);
constexpr int kSequencerRandSeqLabelW = gridPx(7);
constexpr int kSequencerScopeAllStepsW = gridPx(14);
constexpr int kSequencerStepCellSize = gridPx(3);
constexpr int kSequencerDirectionSpeedStripH = gridPx(6);
constexpr int kPerfBlendEndpointLabelW = gridPx(3);
constexpr int kPerfSceneBlendW = gridPx(8);
constexpr int kPerfSeqTransportW = 108;
constexpr int kPerfSeqRecordW = gridPx(6);
constexpr int kPerfBpmLabelW = gridPx(3);
constexpr int kPerfBpmSliderW = gridPx(10);
constexpr int kPerfStepsLabelW = gridPx(4);
constexpr int kPerfStepsSliderW = gridPx(8);
// Minimum column for "Random S&H N"; leftover width is distributed in resized.
constexpr int kPerfMarblesColW = gridPx(12);

struct ModuleRowColumnLayout
{
    int labelW;
    int encoderW;
    int gapW;
    int labelEncoderW;
    int modX;
    int modW;
    int contentW;
};

inline ModuleRowColumnLayout moduleRowColumns(int rowWidth) noexcept
{
    const int labelW = kRowLabelW;
    const int gapW = kModuleRowModGap;
    // Packet 15-C2 (D12): the mod-source dropdown column is retired, so its
    // width is folded back into the encoder column instead of going dead —
    // the encoder column now claims the rest of the row past the label.
    const int reclaimedEncoderW = rowWidth - labelW;
    const int encoderW = reclaimedEncoderW > kEncoderRingSize ? reclaimedEncoderW : kEncoderRingSize;
    const int labelEncoderW = labelW + encoderW;
    const int modX = labelEncoderW;
    const int modW = 0;
    return {labelW, encoderW, gapW, labelEncoderW, modX, modW, rowWidth};
}

constexpr int encoderDocumentHeight(int rowCount) noexcept
{
    return rowCount * kEncoderRowH;
}

constexpr int topChromeStackHeight() noexcept
{
    return kTransportRowH + kSectionGap + kGlobalCommandBandH;
}
} // namespace DesktopV2ChromeLayout

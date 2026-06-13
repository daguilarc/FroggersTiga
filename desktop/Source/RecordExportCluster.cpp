#include "RecordExportCluster.h"

#include "DesktopChromeLayout.hpp"

RecordExportCluster::RecordExportCluster()
{
    setInterceptsMouseClicks(false, true);

    m_fmtWav.setRadioGroupId(1);
    m_fmtMp3.setRadioGroupId(1);
    m_fmtFlac.setRadioGroupId(1);
    m_fmtOgg.setRadioGroupId(1);
    m_fmtWav.setClickingTogglesState(true);
    m_fmtMp3.setClickingTogglesState(true);
    m_fmtFlac.setClickingTogglesState(true);
    m_fmtOgg.setClickingTogglesState(true);
    m_fmtWav.setToggleState(true, juce::dontSendNotification);

    m_record.onClick = [this]() {
        if (onRecordClick)
        {
            onRecordClick();
        }
    };

    addAndMakeVisible(m_record);
    for (juce::ToggleButton* btn : {&m_fmtWav, &m_fmtMp3, &m_fmtFlac, &m_fmtOgg})
    {
        addAndMakeVisible(btn);
    }
}

void RecordExportCluster::setRecording(bool recording)
{
    m_record.setRecording(recording);
}

ExportFormat RecordExportCluster::selectedFormat() const
{
    if (m_fmtMp3.getToggleState())
    {
        return ExportFormat::Mp3;
    }
    if (m_fmtFlac.getToggleState())
    {
        return ExportFormat::Flac;
    }
    if (m_fmtOgg.getToggleState())
    {
        return ExportFormat::Ogg;
    }
    return ExportFormat::Wav;
}

void RecordExportCluster::layoutFormatRows(juce::Rectangle<int> area)
{
    constexpr int kRowCount = 4;
    const int base = area.getHeight() / kRowCount;
    const int extra = area.getHeight() % kRowCount;
    juce::ToggleButton* toggles[] = {&m_fmtWav, &m_fmtMp3, &m_fmtFlac, &m_fmtOgg};
    int y = area.getY();
    for (int i = 0; i < kRowCount; ++i)
    {
        const int rowH = base + (i < extra ? 1 : 0);
        toggles[i]->setBounds(area.getX(), y, area.getWidth(), rowH);
        y += rowH;
    }
}

void RecordExportCluster::layoutChrome(juce::Rectangle<int> recordArea,
                                       juce::Rectangle<int> formatArea)
{
    m_record.setBounds(recordArea);
    layoutFormatRows(formatArea);
}

void RecordExportCluster::resized()
{
    auto area = getLocalBounds();
    const int recordH = DesktopChromeLayout::kRecordRowH;
    juce::Rectangle<int> recordArea(area.getX(), area.getY(), area.getWidth(), recordH);
    juce::Rectangle<int> formatArea(
        area.getX(),
        area.getY() + DesktopChromeLayout::kTransportRowH,
        DesktopChromeLayout::kRecordClusterW,
        DesktopChromeLayout::kModRackRowH);
    layoutChrome(recordArea, formatArea);
}

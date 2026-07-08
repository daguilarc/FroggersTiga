#pragma once

#include "ExportFormat.hpp"
#include "RecordButton.h"

#include <juce_gui_basics/juce_gui_basics.h>

class RecordExportCluster : public juce::Component
{
public:
    RecordExportCluster();

    std::function<void()> onRecordClick;
    void setRecording(bool recording);
    ExportFormat selectedFormat() const;
    void layoutChrome(juce::Rectangle<int> recordArea, juce::Rectangle<int> formatArea);

    void resized() override;

private:
    void layoutFormatRows(juce::Rectangle<int> area);

    RecordButton m_record;
    juce::ToggleButton m_fmtWav{"WAV"};
    juce::ToggleButton m_fmtMp3{"MP3"};
    juce::ToggleButton m_fmtFlac{"FLAC"};
    juce::ToggleButton m_fmtOgg{"OGG"};
};

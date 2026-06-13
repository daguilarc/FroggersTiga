#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

class RecordButton : public juce::Component
{
public:
    RecordButton();
    void setRecording(bool recording);
    bool isRecording() const { return m_recording; }
    std::function<void()> onClick;

    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    bool m_recording = false;
};

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HelpDocsDialog : public juce::Component
{
public:
    HelpDocsDialog(juce::String title, juce::String body);

    void resized() override;

    static void show(juce::Component* parent, const juce::String& title, const juce::String& body);

private:
    juce::Label m_title;
    juce::TextEditor m_body;
    juce::TextButton m_close{"Close"};
};

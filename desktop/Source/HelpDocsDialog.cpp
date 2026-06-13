#include "HelpDocsDialog.h"

HelpDocsDialog::HelpDocsDialog(juce::String title, juce::String body)
{
    m_title.setText(title, juce::dontSendNotification);
    m_title.setFont(juce::Font(18.0f, juce::Font::bold));
    m_body.setMultiLine(true);
    m_body.setReadOnly(true);
    m_body.setScrollbarsShown(true);
    m_body.setText(body);
    m_close.onClick = [this]() {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        {
            dw->exitModalState(0);
        }
    };
    addAndMakeVisible(m_title);
    addAndMakeVisible(m_body);
    addAndMakeVisible(m_close);
    setSize(640, 480);
}

void HelpDocsDialog::resized()
{
    auto area = getLocalBounds().reduced(12);
    m_title.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    m_close.setBounds(area.removeFromBottom(32).removeFromRight(88));
    area.removeFromBottom(8);
    m_body.setBounds(area);
}

void HelpDocsDialog::show(juce::Component* parent, const juce::String& title, const juce::String& body)
{
    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle = title;
    opts.dialogBackgroundColour = juce::Colour(0xff2b3038);
    opts.content.setOwned(new HelpDocsDialog(title, body));
    opts.componentToCentreAround = parent;
    opts.useNativeTitleBar = true;
    opts.resizable = true;
    opts.launchAsync();
}

#pragma once

#include "HelpDocsDialog.h"

#include <juce_gui_basics/juce_gui_basics.h>

class AppMenuBar : public juce::MenuBarModel
{
public:
    enum ItemId
    {
        kManual = 1,
        kQuickDict = 2,
        kLicense = 3
    };

    explicit AppMenuBar(juce::Component* parentForDialogs);

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    juce::Component::SafePointer<juce::Component> m_parent;
};

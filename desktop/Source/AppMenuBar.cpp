#include "AppMenuBar.h"

#include "BinaryData.h"

namespace
{
juce::String loadEmbeddedDoc(const char* data, int size)
{
    return juce::String::fromUTF8(data, size);
}
} // namespace

AppMenuBar::AppMenuBar(juce::Component* parentForDialogs)
    : m_parent(parentForDialogs)
{
}

juce::StringArray AppMenuBar::getMenuBarNames()
{
#if JUCE_MAC
    return {"About"};
#else
    return {"Help"};
#endif
}

juce::PopupMenu AppMenuBar::getMenuForIndex(int, const juce::String&)
{
    juce::PopupMenu menu;
    menu.addItem(kManual, "Manual");
    menu.addItem(kQuickDict, "Quick Dict");
    menu.addItem(kLicense, "License");
    return menu;
}

void AppMenuBar::menuItemSelected(int menuItemID, int)
{
    juce::String title;
    juce::String body;
    switch (menuItemID)
    {
        case kManual:
            title = "Manual";
            body = loadEmbeddedDoc(BinaryData::SIM_MANUAL_md, BinaryData::SIM_MANUAL_mdSize);
            break;
        case kQuickDict:
            title = "Quick Dict";
            body = loadEmbeddedDoc(BinaryData::QUICK_DICT_md, BinaryData::QUICK_DICT_mdSize);
            break;
        case kLicense:
            title = "License";
            body = loadEmbeddedDoc(BinaryData::LICENSE, BinaryData::LICENSESize);
            break;
        default:
            return;
    }
    HelpDocsDialog::show(m_parent.getComponent(), title, body);
}

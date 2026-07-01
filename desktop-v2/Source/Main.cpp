#include <juce_gui_extra/juce_gui_extra.h>

#include "BinaryData.h"
#include "MainComponent.h"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/DesktopV2LookAndFeel.hpp"

namespace
{
DesktopV2LookAndFeel g_desktopV2LookAndFeel;
}

class FroggersTigaV2Application : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "FroggersTigaV2"; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&g_desktopV2LookAndFeel);
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                 .findColour(juce::ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setContentOwned(new MainComponent(), true);
            const juce::Image icon = juce::ImageCache::getFromMemory(
                BinaryData::Icon_png,
                BinaryData::Icon_pngSize);
            if (icon.isValid())
            {
                setIcon(icon);
            }
            centreWithSize(DesktopV2ChromeLayout::kDefaultWidth,
                            DesktopV2ChromeLayout::kDefaultHeight);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(FroggersTigaV2Application)

#include <juce_gui_extra/juce_gui_extra.h>

#include "AppMenuBar.h"
#include "BinaryData.h"
#include "DesktopChromeLayout.hpp"
#include "MainComponent.h"

class FroggersTigaApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "FroggersTiga"; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
        menuBar = std::make_unique<AppMenuBar>(mainWindow.get());
#if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu(menuBar.get());
#else
        mainWindow->setMenuBar(menuBar.get());
#endif
    }

    void shutdown() override
    {
#if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu(nullptr);
#endif
        menuBar = nullptr;
        mainWindow = nullptr;
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
            setResizeLimits(1024, 600, 8192, 4320);
            setContentOwned(new MainComponent(), true);
            const juce::Image icon = juce::ImageCache::getFromMemory(
                BinaryData::Icon_png,
                BinaryData::Icon_pngSize);
            if (icon.isValid())
            {
                setIcon(icon);
            }
            centreWithSize(DesktopChromeLayout::kDefaultWidth, DesktopChromeLayout::kDefaultHeight);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<AppMenuBar> menuBar;
};

START_JUCE_APPLICATION(FroggersTigaApplication)

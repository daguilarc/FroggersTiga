// Direct-launch entry point for the Frogg3rs build (proposal
// openspec/changes/frogg3rs-audio-safety-and-ui-rework/tasks.md, §G-PLAN/§G;
// originally PROPOSAL-direct-launch.md, folded in 2026-08-02). Modelled on
// External/Sheaf/projects/synth/apps/sheaf-patch/Main.cpp: same
// window/session-owner plumbing, minus the "Select an app" picker. Registers
// and launches only Frogg3rs -- no Braid4Registration.hpp, no
// MiniAppRegistration.hpp, no Launcher.hpp -- so initialise() resolves the
// data root, creates the window, and launches Frogg3rs immediately.
//
// FroggersRegistration.hpp is included directly (the app dir is already on
// the include path via -I, see app/build-launcher.sh); the
// SHEAF_PATCH_EXTRA_APP_* macros are the picker build's mechanism and are
// not defined here.
//
// Data-path correctness: launches via the same
// synth::SheafPatchDataPathsForApp(dataRoot, "frogg3rs") helper the picker
// would have used (Launcher.hpp's ActivateApp), with the same "frogg3rs"
// appId as FroggersRegistration.hpp's FroggersManifest().appId, so existing
// saved patches under ~/Library/Sheaf/synth/sheaf-patch/patches/frogg3rs/
// are not orphaned.

#include "FroggersRegistration.hpp"
#include "HostDataPaths.hpp"
#include "Shell.hpp"
#include "synth/AppRegistry.hpp"
#include "synth/ThreadId.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

#include <exception>
#include <filesystem>
#include <memory>

namespace synth_frogg3rs_main {

class FroggersMainApplication final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Frogg3rs"; }
    const juce::String getApplicationVersion() override { return "0.1"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override {
        synth::SetCurrentThreadId(synth::ThreadId::Message);

        try {
            dataRoot_ = synth_runtime::SheafUserApplicationDataRoot();

            window_ = std::make_unique<MainWindow>("Frogg3rs");

            LaunchRegisteredApp<synth_froggers::FroggersApp>(
                synth::SheafPatchDataPathsForApp(dataRoot_, "frogg3rs"));
        } catch (const std::exception& e) {
            INFO("FroggersMainApplication::initialise failed: %s", e.what());
            setApplicationReturnValue(1);
            quit();
        }
    }

    void shutdown() override {
        window_.reset();
        activeSession_.reset();
    }

    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override {}

private:
    class MainWindow final : public juce::DocumentWindow {
    public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(std::move(name), juce::Colours::black, DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setVisible(true);
        }

        void ShowContent(juce::Component& component, int width, int height) {
            setContentNonOwned(&component, false);
            setSize(width, height);
            centreWithSize(width, height);
            setVisible(true);
        }

        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

    template <synth::SynthApplication App>
    void LaunchRegisteredApp(synth::RuntimeDataPaths paths) {
        try {
            auto session = synth_runtime::MakeRuntimeSessionOwner<App>(std::move(paths));
            const synth::RuntimeConfig config = App::Config();

            window_->setName(juce::String(config.appName));
            // Size the window from the SHELL COMPONENT, not from
            // `config.uiWidth/uiHeight`. The session sizes its component to
            // `MainPane::IntrinsicBounds()` (`runtime/Shell.hpp:86-87`), which
            // is `config.uiWidth + RuntimePages::Layout::kSidebarWidth` (96)
            // by `config.uiHeight` (`RuntimeMainComponent.hpp:212-218`) --
            // the app surface PLUS the runtime sidebar that carries Audio /
            // Controllers / Sync / File.
            //
            // Sizing from `config` alone opens the window exactly 96px too
            // narrow and clips that sidebar off the right edge, where it stays
            // invisible until the user drags the window wider (operator,
            // 2026-08-05: "i still had to resize the window to see the buttons
            // on the right hand side"). Sheaf ships two window paths and this
            // file was modelled on the wrong one: `apps/sheaf-patch/Main.cpp`
            // :87-99 sizes from `config` and has the same defect, while
            // `runtime/Shell.hpp:193-197` reads the intrinsic bounds and is
            // correct. Read the component's own size and both stay right even
            // if the sidebar width changes upstream.
            juce::Component& content = session->Component();
            window_->ShowContent(content, content.getWidth(), content.getHeight());
            activeSession_ = std::move(session);
        } catch (const std::exception& e) {
            INFO("FroggersMainApplication::LaunchRegisteredApp failed: %s", e.what());
        }
    }

    std::filesystem::path dataRoot_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<synth_runtime::RuntimeSessionOwner> activeSession_;
};

}  // namespace synth_frogg3rs_main

START_JUCE_APPLICATION(synth_frogg3rs_main::FroggersMainApplication)

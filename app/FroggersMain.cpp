// Direct-launch entry point for the Frogg3rs build. Modelled on
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

#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <vector>

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
            // Holds the CONCRETE session (not the
            // type-erased synth_runtime::RuntimeSessionOwner, which exposes
            // only Component()) so RegisterRecordingCallbacks below can
            // reach the running FroggersApp via GetRuntime().GetEngine().
            // Application() (Shell.hpp/Runtime.hpp/Engine.hpp) to register
            // the Record button's host-facing seams.
            auto session = std::make_unique<synth_runtime::RuntimeShellSession<App>>(std::move(paths));
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
            // invisible until the user drags the window wider (a real
            // reported symptom: "i still had to resize the window to see the buttons
            // on the right hand side"). Sheaf ships two window paths and this
            // file was modelled on the wrong one: `apps/sheaf-patch/Main.cpp`
            // :87-99 sizes from `config` and has the same defect, while
            // `runtime/Shell.hpp:193-197` reads the intrinsic bounds and is
            // correct. Read the component's own size and both stay right even
            // if the sidebar width changes upstream.
            juce::Component& content = session->Component();
            window_->ShowContent(content, content.getWidth(), content.getHeight());
            activeSession_ = std::move(session);
            // activeSession_'s declared type is concretely
            // RuntimeShellSession<synth_froggers::FroggersApp> (see this
            // method's own comment above), regardless of this method's own
            // App template parameter -- valid because this file only ever
            // instantiates LaunchRegisteredApp with
            // synth_froggers::FroggersApp (see this file's own header
            // comment: "Registers and launches only Frogg3rs").
            RegisterRecordingCallbacks(activeSession_->GetRuntime().GetEngine().Application());
        } catch (const std::exception& e) {
            INFO("FroggersMainApplication::LaunchRegisteredApp failed: %s", e.what());
        }
    }

    // Registers the two host-facing seams
    // FroggersAppCore exposes (SetOnRecordRefused/SetOnRecordingFinished --
    // see that file's own comment) with their JUCE-side behaviour. This is
    // the ONLY place in the app that JUCE dialog/file-write code for Record
    // lives -- FroggersUiSurface.hpp only calls ArmRecording()/
    // StopRecording() and fires these two callbacks; it never sees
    // AlertWindow/FileChooser/FileOutputStream, and the core
    // (FroggersAppCore.hpp) never sees JUCE at all (check-no-juce).
    void RegisterRecordingCallbacks(synth_froggers::FroggersApp& app) {
        app.SetOnRecordRefused([](const char* reason) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Recording",
                                                    juce::String(reason));
        });

        // Captures `app` BY REFERENCE: this lambda is itself stored as a
        // member of `app` (SetOnRecordingFinished, FroggersAppCore.hpp), so
        // it cannot outlive the object it refers to. Everything the ASYNC
        // continuation below needs (the samples, sample rate, truncation
        // flag) is copied out of `app` here, synchronously, on the message
        // thread, before the file chooser's own async callback -- which
        // fires later (after the user picks a file) and must not read `app`
        // at all, since a second Record press could re-arm (and reallocate
        // FroggersAppCore's capture buffer) before the dialog closes.
        app.SetOnRecordingFinished([&app] {
            const std::vector<float> audioSamples(app.RecordedAudio().begin(), app.RecordedAudio().end());
            const float sampleRate = app.RecordSampleRate();
            const bool truncated = app.RecordingTruncated();

            auto chooser = std::make_shared<juce::FileChooser>(
                "Save Recording",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("frogg3rs-recording.wav"),
                "*.wav");
            const int flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                               juce::FileBrowserComponent::warnAboutOverwriting;
            // `chooser` is captured into its own completion callback (JUCE's
            // documented FileChooser lifetime contract: the object must
            // outlive the dialog), so it self-destructs once this fires.
            chooser->launchAsync(flags, [chooser, audioSamples, sampleRate, truncated](const juce::FileChooser& fc) {
                const juce::File file = fc.getResult();
                if (file == juce::File{}) {
                    return;  // Cancelled.
                }

                // The encoding itself is core-side, pure std:: (no
                // juce::AudioFormatWriter anywhere in this app) -- this host
                // layer is dialog-plus-byte-stream only, deliberately.
                // Streamed to disk via
                // juce::FileOutputStream (not std::ofstream): the target
                // came back as a juce::File from the FileChooser, and this
                // stays in the same JUCE idiom as everything else in this
                // file rather than round-tripping through a raw path.
                const std::vector<std::uint8_t> wavBytes =
                    synth_froggers::EncodeWavPcm16Mono(audioSamples, sampleRate);

                file.deleteFile();
                juce::FileOutputStream stream(file);
                bool ok = stream.openedOk();
                if (ok) {
                    ok = stream.write(wavBytes.data(), wavBytes.size());
                    stream.flush();
                }

                juce::String message =
                    ok ? juce::String("Recording saved.") : juce::String("Failed to write recording.");
                // v1-style, verbatim wording (desktop/Source/
                // MainComponent.cpp:219, reference only) appended to the
                // same completion alert rather than a separate one.
                if (ok && truncated) {
                    message += " (stopped at the 30-minute limit)";
                }
                juce::AlertWindow::showMessageBoxAsync(
                    ok ? juce::AlertWindow::InfoIcon : juce::AlertWindow::WarningIcon, "Recording", message);
            });
        });
    }

    std::filesystem::path dataRoot_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<synth_runtime::RuntimeShellSession<synth_froggers::FroggersApp>> activeSession_;
};

}  // namespace synth_frogg3rs_main

START_JUCE_APPLICATION(synth_frogg3rs_main::FroggersMainApplication)

#include "runtime/DesktopV2RuntimeProjection.hpp"

namespace
{
RuntimeControlOverlay standaloneInteractive()
{
    return {false, false, true};
}

RuntimeControlOverlay standaloneHidden()
{
    return {true, false, false};
}

RuntimeControlOverlay hostedHidden()
{
    return {true, false, false};
}

RuntimeControlOverlay hostedReadOnlyPanel()
{
    return {false, true, true};
}
} // namespace

RuntimeControlOverlay runtimeControlOverlay(RuntimeProjectionContext context, RuntimeControlId control)
{
    if (context == RuntimeProjectionContext::DesktopStandalone)
    {
        switch (control)
        {
            case RuntimeControlId::HostedAudioStatusPanel:
            case RuntimeControlId::HostedMidiStatusPanel:
                return standaloneHidden();
            case RuntimeControlId::RuntimePageRail:
            case RuntimeControlId::FilePatchPage:
            case RuntimeControlId::AudioPage:
            case RuntimeControlId::ControllersPage:
            case RuntimeControlId::HardwareAudioInputSelector:
            case RuntimeControlId::HardwareAudioOutputSelector:
            case RuntimeControlId::HardwareMidiInputSelector:
            case RuntimeControlId::HardwareMidiOutputSelector:
            case RuntimeControlId::QwertyVirtualSourceSelector:
            case RuntimeControlId::RecordExportControl:
            case RuntimeControlId::FilePatchSaveLoad:
                return standaloneInteractive();
            case RuntimeControlId::Count:
                break;
        }
        return standaloneInteractive();
    }

    switch (control)
    {
        case RuntimeControlId::RuntimePageRail:
        case RuntimeControlId::FilePatchPage:
        case RuntimeControlId::AudioPage:
        case RuntimeControlId::ControllersPage:
        case RuntimeControlId::HardwareAudioInputSelector:
        case RuntimeControlId::HardwareAudioOutputSelector:
        case RuntimeControlId::HardwareMidiInputSelector:
        case RuntimeControlId::HardwareMidiOutputSelector:
        case RuntimeControlId::QwertyVirtualSourceSelector:
        case RuntimeControlId::RecordExportControl:
        case RuntimeControlId::FilePatchSaveLoad:
            return hostedHidden();
        case RuntimeControlId::HostedAudioStatusPanel:
        case RuntimeControlId::HostedMidiStatusPanel:
            return hostedReadOnlyPanel();
        case RuntimeControlId::Count:
            break;
    }
    return hostedHidden();
}

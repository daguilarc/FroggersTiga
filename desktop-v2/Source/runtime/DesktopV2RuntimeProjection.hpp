#pragma once

#include <cstdint>

enum class RuntimePageKind : uint8_t
{
    None = 0,
    FilePatch,
    Audio,
    Controllers
};

enum class RuntimeProjectionContext : uint8_t
{
    DesktopStandalone = 0,
    PluginHosted
};

enum class RuntimeControlId : uint8_t
{
    RuntimePageRail = 0,
    FilePatchPage,
    AudioPage,
    ControllersPage,
    HardwareAudioInputSelector,
    HardwareAudioOutputSelector,
    HardwareMidiInputSelector,
    HardwareMidiOutputSelector,
    QwertyVirtualSourceSelector,
    RecordExportControl,
    FilePatchSaveLoad,
    HostedAudioStatusPanel,
    HostedMidiStatusPanel,
    Count
};

struct RuntimeControlOverlay
{
    bool hidden = false;
    bool readOnly = false;
    bool interactive = true;
};

RuntimeControlOverlay runtimeControlOverlay(RuntimeProjectionContext context, RuntimeControlId control);

inline bool runtimeControlVisible(RuntimeProjectionContext context, RuntimeControlId control)
{
    return !runtimeControlOverlay(context, control).hidden;
}

inline bool runtimeControlInteractive(RuntimeProjectionContext context, RuntimeControlId control)
{
    const RuntimeControlOverlay overlay = runtimeControlOverlay(context, control);
    return overlay.interactive && !overlay.hidden && !overlay.readOnly;
}

// RuntimeBootSmoke_test -- Packet 10 shell cutover (additive step). Same
// fork/exec/poll-alive/SIGTERM pattern as BootSmoke_test.cpp, pointed at the
// new FroggersTigaDesktopV2Runtime binary (MainRuntime.cpp's
// SYNTH_RUNTIME_MAIN(FroggersApp)) instead of the legacy
// FroggersTigaDesktopV2 one. Proves FroggersApp actually boots under the
// vendored Sheaf Runtime shell without crashing on first render -- this is
// where the Config() uiWidth/uiHeight fix (FroggersAppCore.cpp) matters:
// without it, RuntimeMainComponent::ValidateApplicationTree throws on the
// very first BuildTree() and the process would exit immediately instead of
// surviving the poll window below.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>

namespace
{
constexpr int kPollIntervalMs = 50;
constexpr int kPollCount = 40;

bool fileExecutable(const char* path)
{
    return path != nullptr && path[0] != '\0' && access(path, X_OK) == 0;
}

const char* resolveBinaryPath()
{
    if (const char* envPath = std::getenv("FROGGERS_TIGA_RUNTIME_BIN"))
    {
        if (fileExecutable(envPath))
        {
            return envPath;
        }
    }

    static const char* kCandidates[] = {
        // Actual single-config Makefiles layout (no Release/Debug
        // subdirectory) when ctest runs with cwd == the build directory.
        "FroggersTigaDesktopV2Runtime_artefacts/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        "desktop-v2/build/FroggersTigaDesktopV2Runtime_artefacts/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        // Multi-config generator fallbacks (Xcode/Ninja Multi-Config).
        "FroggersTigaDesktopV2Runtime_artefacts/Release/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        "desktop-v2/build/FroggersTigaDesktopV2Runtime_artefacts/Release/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        "FroggersTigaDesktopV2Runtime_artefacts/Debug/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        "desktop-v2/build/FroggersTigaDesktopV2Runtime_artefacts/Debug/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        "build/desktop-v2/FroggersTigaDesktopV2Runtime_artefacts/Release/FroggersTigaRuntime.app/Contents/MacOS/FroggersTigaRuntime",
        nullptr,
    };

    for (const char* candidate : kCandidates)
    {
        if (fileExecutable(candidate))
        {
            return candidate;
        }
    }

    return nullptr;
}

bool childTerminated(pid_t child, int& status)
{
    const pid_t result = waitpid(child, &status, WNOHANG);
    if (result == child)
    {
        return WIFEXITED(status) || WIFSIGNALED(status);
    }
    return result < 0;
}

void printEarlyExit(int status)
{
    std::printf("FAIL: FroggersTigaRuntime exited early during boot smoke (status=%d", status);
    if (WIFEXITED(status))
    {
        std::printf(", exit=%d", WEXITSTATUS(status));
    }
    if (WIFSIGNALED(status))
    {
        std::printf(", signal=%d", WTERMSIG(status));
    }
    std::printf(")\n");
}

bool test_runtime_boot_smoke()
{
    const char* binaryPath = resolveBinaryPath();
    if (binaryPath == nullptr)
    {
        std::printf("SKIP: FroggersTigaRuntime binary not found (set FROGGERS_TIGA_RUNTIME_BIN)\n");
        return true;
    }

    const pid_t child = fork();
    if (child < 0)
    {
        std::printf("FAIL: fork failed\n");
        return false;
    }

    if (child == 0)
    {
        execl(binaryPath, binaryPath, static_cast<char*>(nullptr));
        _exit(127);
    }

    for (int poll = 0; poll < kPollCount; ++poll)
    {
        int status = 0;
        if (childTerminated(child, status))
        {
            printEarlyExit(status);
            waitpid(child, nullptr, 0);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }

    int status = 0;
    if (childTerminated(child, status))
    {
        printEarlyExit(status);
        waitpid(child, nullptr, 0);
        return false;
    }

    kill(child, SIGTERM);
    waitpid(child, nullptr, 0);
    return true;
}
} // namespace

int main()
{
    if (!test_runtime_boot_smoke())
    {
        return 1;
    }
    std::printf("PASS: RuntimeBootSmoke_test\n");
    return 0;
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>

namespace
{
bool fileExecutable(const char* path)
{
    return path != nullptr && path[0] != '\0' && access(path, X_OK) == 0;
}

const char* resolveBinaryPath()
{
    if (const char* envPath = std::getenv("FROGGERS_TIGA_V2_BIN"))
    {
        if (fileExecutable(envPath))
        {
            return envPath;
        }
    }

    static const char* kCandidates[] = {
        "desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app/Contents/MacOS/FroggersTigaV2",
        "desktop-v2/build/FroggersTigaDesktopV2_artefacts/Debug/FroggersTigaV2.app/Contents/MacOS/FroggersTigaV2",
        "build/desktop-v2/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app/Contents/MacOS/FroggersTigaV2",
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

bool test_boot_smoke()
{
    const char* binaryPath = resolveBinaryPath();
    if (binaryPath == nullptr)
    {
        std::printf("SKIP: FroggersTigaV2 binary not found (set FROGGERS_TIGA_V2_BIN)\n");
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

    sleep(2);

    if (kill(child, 0) != 0)
    {
        int status = 0;
        waitpid(child, &status, 0);
        std::printf("FAIL: FroggersTigaV2 exited early (status=%d)\n", status);
        return false;
    }

    kill(child, SIGTERM);
    waitpid(child, nullptr, 0);
    return true;
}
} // namespace

int main()
{
    if (!test_boot_smoke())
    {
        return 1;
    }
    std::printf("PASS: BootSmoke_test\n");
    return 0;
}

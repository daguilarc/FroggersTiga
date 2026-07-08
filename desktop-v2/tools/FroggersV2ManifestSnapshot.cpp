#include "manifest/FroggersV2AppManifest.hpp"

#include <cstdio>
#include <filesystem>

int main(int argc, char** argv)
{
    const std::filesystem::path outputDir = argc > 1 ? std::filesystem::path(argv[1])
                                                     : std::filesystem::path("build/manifest");
    if (!froggers_v2::manifest::validateFoundation())
    {
        std::printf("FAIL: Froggers v2 manifest foundation validation failed\n");
        return 1;
    }
    if (!froggers_v2::manifest::writeReviewerArtifacts(outputDir))
    {
        std::printf("FAIL: could not write Froggers v2 manifest artifacts to %s\n",
                    outputDir.string().c_str());
        return 1;
    }
    std::printf("PASS: wrote Froggers v2 manifest artifacts to %s\n", outputDir.string().c_str());
    return 0;
}

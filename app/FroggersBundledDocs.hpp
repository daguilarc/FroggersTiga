#pragma once

// Opens one of the app's bundled operator documents (MANUAL.md or
// QUICK_DICT.md) with the operating system's default handler for that file
// type -- no in-app text viewer, no network.
//
// Works unmodified from both the standalone launcher and the plugin editor:
// juce::File::currentApplicationFile resolves to the CALLING module's own
// bundle root via dladdr (JUCE's juce_Files_mac.mm, the currentApplicationFile
// case: it walks up from the executable file to the enclosing bundle root
// whenever the executable sits under a "Contents/MacOS" directory, which is
// true of an .app, a .vst3, and a .component alike) -- the standalone app
// bundle for the launcher, the .vst3 or .component bundle for the plugin.
// Both hosts therefore read the SAME two files, copied into each bundle's
// Contents/Resources at build time from the repository's single copy (see
// app/build-launcher.sh and app/vst/CMakeLists.txt) -- no second checked-in
// copy anywhere for either document to drift from.

#include <juce_core/juce_core.h>

namespace frogg3rs_docs {

inline void OpenBundledDoc(const char* filename) {
    const juce::File bundleRoot = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    const juce::File doc = bundleRoot.getChildFile("Contents/Resources").getChildFile(filename);
    doc.startAsProcess();
}

}  // namespace frogg3rs_docs

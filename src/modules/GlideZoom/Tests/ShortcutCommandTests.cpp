#include "PortableTestAdapter.h"

#define main Run_shortcut_commands
#include "shortcut_commands_tests.cpp"
#undef main

namespace GlideZoomTests
{
    TEST_CLASS (ShortcutCommandTests)
    {
    public:
        TEST_METHOD (AltGrTextPreservesModifiersWhileTrueControlLeftAltCommandsOwnCompletePairs)
        {
            RunPortableSuite([] { return Run_shortcut_commands(); });
        }
    };
}

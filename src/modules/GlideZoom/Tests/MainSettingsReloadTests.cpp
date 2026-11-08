#include "PortableTestAdapter.h"

#define main Run_settings_reload
#include "settings_reload_tests.cpp"
#undef main

namespace GlideZoomTests
{
    TEST_CLASS (MainSettingsReloadTests)
    {
    public:
        TEST_METHOD (CompatibleSettingsPreserveCameraPolicy)
        {
            RunPortableSuite([] { return Run_settings_reload(); });
        }
    };
}

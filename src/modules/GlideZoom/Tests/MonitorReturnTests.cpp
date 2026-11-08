#include "PortableTestAdapter.h"

#define main Run_monitor_return
#include "monitor_return_tests.cpp"
#undef main

namespace GlideZoomTests
{
    TEST_CLASS (MonitorReturnTests)
    {
    public:
        TEST_METHOD (PhysicalReentryRevealsCameraBeforeMappingCommit)
        {
            RunPortableSuite([] { return Run_monitor_return(); });
        }
    };
}

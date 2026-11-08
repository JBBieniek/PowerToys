#include "PortableTestAdapter.h"

int Run_pointer_native();

namespace GlideZoomTests
{
    TEST_CLASS (PointerNativeTests)
    {
    public:
        TEST_METHOD (ProductionNativeOpsHonorsDesktopClipDpiMonitorAndPhysicalApiBoundaries)
        {
            RunPortableSuite([] { return Run_pointer_native(); });
        }
    };
}

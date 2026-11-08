#include "PortableTestAdapter.h"

#define main Run_pointer_speed_composition
#include "pointer_speed_composition_tests.cpp"
#undef main

namespace GlideZoomTests
{
    TEST_CLASS (PointerSpeedCompositionTests)
    {
    public:
        TEST_METHOD (ProcessedPacketsKeepOneVisibleSpeedFactorAcrossZoomAndDrag)
        {
            RunPortableSuite([] { return Run_pointer_speed_composition(); });
        }
    };
}

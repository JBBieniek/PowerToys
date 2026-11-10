// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE in the project root.

#include "CppUnitTest.h"
#include "../GlideZoom/Resources.h"
#include "../GlideZoomModuleInterface/Generated Files/resource.h"

#include <array>
#include <filesystem>
#include <iterator>
#include <limits>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GlideZoomTests
{
    namespace
    {
        class ResourceModule
        {
        public:
            explicit ResourceModule(const wchar_t* filename)
            {
                wchar_t modulePath[32768]{};
                const auto length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), modulePath, static_cast<DWORD>(std::size(modulePath)));
                Assert::IsTrue(length != 0 && length < std::size(modulePath));
                // These projects publish to <platform>/<config>/tests/GlideZoom.
                const auto path = std::filesystem::path(modulePath).parent_path().parent_path().parent_path() / filename;
                module_ = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
                Assert::IsNotNull(module_, path.c_str());
            }
            ~ResourceModule()
            {
                if (module_)
                    FreeLibrary(module_);
            }
            ResourceModule(const ResourceModule&) = delete;
            ResourceModule& operator=(const ResourceModule&) = delete;
            HINSTANCE Get() const noexcept { return module_; }

        private:
            HMODULE module_ = nullptr;
        };
    }

    TEST_CLASS (NativeResourcesTests)
    {
    public:
        TEST_METHOD (BuiltEngineAndInterfaceContainEnglishUiResources)
        {
            ResourceModule engine{ L"PowerToys.GlideZoom.exe" };
            ResourceModule module{ L"PowerToys.GlideZoomModuleInterface.dll" };
            // Exercise the built PE resources rather than an XML or hard-coded
            // substitute. Loading as data never executes the engine or DllMain.
            for (const UINT id : { IDS_NATIVEUNKNOWNWINDOWSERROR, IDS_NATIVEPOWERTOYSGLIDEZOOM, IDS_NATIVEOPENGLIDEZOOM, IDS_NATIVEMINIMIZETOTRAY, IDS_NATIVEEXITGLIDEZOOM, IDS_NATIVESTATUSFORMAT, IDS_NATIVETRAYSTATUSFORMAT, IDS_NATIVEGRAPHICSRESULTFORMAT, IDS_NATIVETHREEFINGERGESTUREDISABLED, IDS_NATIVEPOINTERADJUSTMENTPAUSED })
            {
                Assert::IsFalse(get_english_fallback_string(id, engine.Get()).empty());
            }
            for (const UINT id : { IDS_MODULENAME, IDS_MODULEDESCRIPTION, IDS_CONFIGWHEELMODIFIER, IDS_CONFIGMAXIMUMZOOM, IDS_CONFIGSENSITIVITY, IDS_CONFIGZOOMMULTIPLIER, IDS_CONFIGEDGEPANMULTIPLIER, IDS_CONFIGZOOMSCOPE, IDS_CONFIGSCOPEINDICATORMODE, IDS_CONFIGSCOPEINDICATORSTYLE, IDS_CONFIGPOINTERSPEED, IDS_CONFIGOVERVIEWOPACITY, IDS_CONFIGSCOPEINDICATORSIZE, IDS_CONFIGSCOPEINDICATOROPACITY, IDS_CONFIGSCOPEINDICATORBORDER, IDS_CONFIGSCOPEINDICATORDURATION, IDS_CONFIGWHEELACCELERATIONLIMIT, IDS_CONFIGSCOPEINDICATORX, IDS_CONFIGSCOPEINDICATORY, IDS_CONFIGSCOPEINDICATORFADE, IDS_CONFIGDOUBLECONTROL, IDS_CONFIGMODIFIEDDOUBLECLICK, IDS_CONFIGREVERSEDIRECTION, IDS_CONFIGCENTERFOLLOW, IDS_CONFIGTHREEFINGERTOUCHPAD, IDS_CONFIGZOOMSCALEDPOINTER, IDS_CONFIGSCOPEINDICATORDETAILS, IDS_CONFIGWHEELACCELERATION })
            {
                Assert::IsFalse(get_english_fallback_string(id, module.Get()).empty());
            }
            const auto name = get_english_fallback_string(IDS_MODULENAME, module.Get());
            Assert::AreEqual(L"GlideZoom", name.c_str());
        }

        TEST_METHOD (EveryWindowFitResultHasAnEmbeddedUiMessage)
        {
            ResourceModule engine{ L"PowerToys.GlideZoom.exe" };
            using Reason = glide::window_fit::Reason;
            for (const auto reason : { Reason::Pending, Reason::Fitted, Reason::Busy, Reason::InvalidViewport, Reason::UnavailableDesktop, Reason::NoWindow, Reason::OwnWindow, Reason::NotResizable, Reason::Fullscreen, Reason::Maximized, Reason::Hung, Reason::UnavailableGeometry, Reason::ContextChanged, Reason::NativeFailure, Reason::AppConstrained, Reason::TimedOut, Reason::Idle })
            {
                Assert::IsFalse(get_english_fallback_string(glide::ui::WindowFitResourceId(reason), engine.Get()).empty());
            }
            Assert::AreNotEqual(glide::ui::WindowFitResourceId(Reason::Fitted), glide::ui::WindowFitResourceId(Reason::Fullscreen));
        }

        TEST_METHOD (ResourceFallbackAndLookupPreserveFailureDiagnostics)
        {
            ResourceModule engine{ L"PowerToys.GlideZoom.exe" };
            const auto fallback = get_resource_string(std::numeric_limits<UINT>::max(), engine.Get(), L"missing resource fallback");
            Assert::AreEqual(L"missing resource fallback", fallback.c_str());
            SetLastError(ERROR_ACCESS_DENIED);
            // The cached helper must preserve the preceding failed native API's
            // error, including on its first resource load in this test module.
            glide::ui::Text(IDS_NATIVEUNKNOWNWINDOWSERROR);
            Assert::AreEqual(static_cast<DWORD>(ERROR_ACCESS_DENIED), GetLastError());
        }

        TEST_METHOD (TranslatedFormatsCanReorderWithoutInterpretingArguments)
        {
            const std::array<std::wstring_view, 3> values{ L"Ready", L"200%", L"{0}%n" };
            const auto formatted = glide::ui::FormatResource(L"{2} / {1} / {0}", L"{0} {1} {2}", values);
            Assert::AreEqual(L"{0}%n / 200% / Ready", formatted.c_str());
            for (const auto invalid : { L"{0} {1}", L"{0} {1} {3}", L"{0} {1} {2", L"{0} {1} {2}}", L"%ls", L"{00} {1} {2}" })
            {
                const auto fallback = glide::ui::FormatResource(invalid, L"{0} {1} {2}", values);
                Assert::AreEqual(L"Ready 200% {0}%n", fallback.c_str());
            }
            std::wstring unchanged = L"previous UI text";
            Assert::IsFalse(glide::ui::TryFormatResource(L"{4}", values, unchanged));
            Assert::AreEqual(L"previous UI text", unchanged.c_str());
        }

        TEST_METHOD (ShippedDynamicFormatsPreserveAllStatusFields)
        {
            ResourceModule engine{ L"PowerToys.GlideZoom.exe" };
            const auto status = get_english_fallback_string(IDS_NATIVESTATUSFORMAT, engine.Get());
            std::wstring formatted;
            Assert::IsTrue(glide::ui::TryFormatResource(status, std::array<std::wstring_view, 4>{ L"Ready", L"2.50", L"Independent monitors", L" | Pointer adjustment paused" }, formatted));
            Assert::AreEqual(L"Ready  |  2.50x  |  Independent monitors | Pointer adjustment paused", formatted.c_str());
            Assert::IsTrue(glide::ui::TryFormatResource(get_english_fallback_string(IDS_NATIVETRAYSTATUSFORMAT, engine.Get()), std::array<std::wstring_view, 1>{ L"Paused" }, formatted));
            Assert::AreEqual(L"GlideZoom - Paused", formatted.c_str());
            Assert::IsTrue(glide::ui::TryFormatResource(get_english_fallback_string(IDS_NATIVEGRAPHICSRESULTFORMAT, engine.Get()), std::array<std::wstring_view, 1>{ L"80070005" }, formatted));
            Assert::AreEqual(L"\nGraphics result: 0x80070005", formatted.c_str());
        }
    };
}

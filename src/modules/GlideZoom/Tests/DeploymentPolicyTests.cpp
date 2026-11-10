// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "CppUnitTest.h"
#include "../GlideZoom/deployment_policy.h"
#include <iterator>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GlideZoomTests
{
    TEST_CLASS (DeploymentPolicyTests)
    {
    public:
        TEST_METHOD (SecureRootsRequireCanonicalDescendantBoundaries)
        {
            using namespace glide::deployment;
            Assert::IsTrue(BelowSecureRoot(L"c:\\PROGRAM FILES\\PowerToys", L"C:\\Program Files\\"));
            Assert::IsTrue(BelowSecureRoot(L"D:\\Windows\\System32\\PowerToys\\", L"d:\\windows\\system32"));
            Assert::IsFalse(BelowSecureRoot(L"C:\\Program Files", L"C:\\Program Files"));
            Assert::IsFalse(BelowSecureRoot(L"C:\\Program FilesBackup\\PowerToys", L"C:\\Program Files"));
            Assert::IsFalse(BelowSecureRoot(L"C:\\Users\\User\\PowerToys", L"C:\\Program Files"));
            Assert::IsFalse(BelowSecureRoot(L"C:\\Windows\\Temp\\PowerToys", L"C:\\Windows\\System32"));
            Assert::IsFalse(BelowSecureRoot(L"C:\\PowerToys", L"C:\\"));
        }

        TEST_METHOD (PathAliasesAndUntrustedNamespaceAreRejected)
        {
            using namespace glide::deployment;
            const std::wstring_view invalid[] = {
                L"", L"PowerToys", L"C:PowerToys", L"\\\\server\\share\\PowerToys", L"\\\\?\\C:\\Program Files\\PowerToys", L"C:/Program Files/PowerToys", L"C:\\Program Files\\..\\Users\\PowerToys", L"C:\\Program Files\\.\\PowerToys", L"C:\\Program Files\\\\PowerToys", L"C:\\Program Files \\PowerToys", L"C:\\Program Files.\\PowerToys", L"C:\\Program Files\\PowerToys:stream", L"C:\\Program Files\\Power*Toys", L"C:\\Program Files\\Power?Toys"
            };
            for (const auto path : invalid)
            {
                Assert::IsFalse(CanonicalPath(path));
                Assert::IsFalse(BelowSecureRoot(path, L"C:\\Program Files"));
            }
            const wchar_t nul[] = L"C:\\Program Files\\Power\0Toys";
            Assert::IsFalse(CanonicalPath({ nul, std::size(nul) - 1 }));
        }

        TEST_METHOD (UpgradeCannotTreatMissingPreviousPathAsFirstInstall)
        {
            using namespace glide::deployment;
            // A first install may have no previous directory and a new target
            // that does not exist yet. An upgrade must verify the old directory
            // before checking processes, even when the new target differs.
            Assert::IsTrue(CanVerifyPreviousInstallation(false, false));
            Assert::IsTrue(CanVerifyPreviousInstallation(false, true));
            Assert::IsFalse(CanVerifyPreviousInstallation(true, false));
            Assert::IsTrue(CanVerifyPreviousInstallation(true, true));
            ShutdownPolicy emptyNewTarget;
            Assert::IsTrue(emptyNewTarget.Observe(100, Scan::Empty) == ShutdownDecision::Wait);
            Assert::IsTrue(emptyNewTarget.Observe(600, Scan::Empty) == ShutdownDecision::Complete);
            Assert::IsFalse(CanVerifyPreviousInstallation(true, false));
        }

        TEST_METHOD (ShutdownWaitsForEngineAndGuardianAndFreshQuietScans)
        {
            using namespace glide::deployment;
            ShutdownPolicy policy;
            Assert::IsTrue(policy.Observe(100, Scan::Running) == ShutdownDecision::Wait);
            Assert::IsTrue(policy.Observe(200, Scan::Empty) == ShutdownDecision::Wait);
            Assert::IsTrue(policy.Observe(699, Scan::Empty) == ShutdownDecision::Wait);
            // A replacement engine or surviving guardian invalidates quiet time.
            Assert::IsTrue(policy.Observe(700, Scan::Running) == ShutdownDecision::Wait);
            Assert::IsTrue(policy.Observe(800, Scan::Empty) == ShutdownDecision::Wait);
            Assert::IsTrue(policy.Observe(1299, Scan::Empty) == ShutdownDecision::Wait);
            Assert::IsTrue(policy.Observe(1300, Scan::Empty) == ShutdownDecision::Complete);
        }

        TEST_METHOD (UnknownTimeoutAndBackwardTimeNeverAuthorizeFileChanges)
        {
            using namespace glide::deployment;
            ShutdownPolicy unknown;
            Assert::IsTrue(unknown.Observe(100, Scan::Unknown) == ShutdownDecision::Refuse);
            Assert::IsTrue(unknown.Observe(1000, Scan::Empty) == ShutdownDecision::Refuse);
            ShutdownPolicy timeout;
            Assert::IsTrue(timeout.Observe(100, Scan::Running) == ShutdownDecision::Wait);
            Assert::IsTrue(timeout.Observe(30099, Scan::Running) == ShutdownDecision::Wait);
            Assert::IsTrue(timeout.Observe(30100, Scan::Empty) == ShutdownDecision::Refuse);
            ShutdownPolicy backward;
            Assert::IsTrue(backward.Observe(100, Scan::Empty) == ShutdownDecision::Wait);
            Assert::IsTrue(backward.Observe(99, Scan::Empty) == ShutdownDecision::Refuse);
            Assert::IsTrue(backward.Observe(600, Scan::Empty) == ShutdownDecision::Refuse);
        }
    };
}

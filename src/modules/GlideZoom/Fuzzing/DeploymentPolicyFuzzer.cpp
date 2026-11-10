// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "../GlideZoom/deployment_policy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

void FuzzDeploymentPolicy(const std::uint8_t* data, std::size_t size)
{
    using namespace glide::deployment;
    for (std::size_t index = 0; index < size && index < 256; ++index)
    {
        const bool upgrading = (data[index] & 1) != 0;
        const bool previousVerified = (data[index] & 2) != 0;
        if (upgrading && !previousVerified && CanVerifyPreviousInstallation(upgrading, previousVerified))
            std::abort();
    }
    const std::wstring root = L"C:\\Program Files";
    std::wstring candidate = L"C:\\Program Files\\";
    const auto count = (std::min)(size, std::size_t{ 256 });
    for (std::size_t index = 0; index < count; ++index)
        candidate.push_back(static_cast<wchar_t>(data[index]));
    if (BelowSecureRoot(candidate, root) &&
        (!CanonicalPath(candidate) || candidate.size() <= root.size() ||
         candidate[root.size()] != L'\\' || !EqualPath(candidate.substr(0, root.size()), root)))
        std::abort();
    // Appending a lookalike root suffix is never secure ancestry.
    if (BelowSecureRoot(root + L"Backup\\" + candidate, root))
        std::abort();

    ShutdownPolicy policy;
    std::uint64_t now = 100, quietAt = 0;
    bool quiet = false, refused = false;
    for (std::size_t index = 0; index + 1 < size && index < 512; index += 2)
    {
        now += data[index];
        const auto observation = static_cast<Scan>(data[index + 1] % 3);
        if (observation == Scan::Empty)
        {
            if (!quiet)
                quietAt = now;
            quiet = true;
        }
        else
            quiet = false;
        const auto decision = policy.Observe(now, observation);
        if (refused && decision != ShutdownDecision::Refuse)
            std::abort();
        if (decision == ShutdownDecision::Refuse)
            refused = true;
        if (decision == ShutdownDecision::Complete)
        {
            if (!quiet || now - quietAt < ShutdownPolicy::QuietMs || observation != Scan::Empty)
                std::abort();
            break;
        }
        if (observation == Scan::Unknown && decision != ShutdownDecision::Refuse)
            std::abort();
    }
}

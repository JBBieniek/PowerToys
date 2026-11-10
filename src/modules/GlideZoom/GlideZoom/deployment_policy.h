// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <string_view>

namespace glide::deployment
{
    inline constexpr wchar_t FoldAscii(wchar_t character) noexcept
    {
        return character >= L'A' && character <= L'Z' ? static_cast<wchar_t>(character + (L'a' - L'A')) : character;
    }

    inline constexpr bool EqualPath(std::wstring_view left, std::wstring_view right) noexcept
    {
        if (left.size() != right.size())
            return false;
        for (std::size_t index = 0; index < left.size(); ++index)
            if (FoldAscii(left[index]) != FoldAscii(right[index]))
                return false;
        return true;
    }

    inline constexpr std::wstring_view TrimDirectory(std::wstring_view path) noexcept
    {
        while (path.size() > 3 && path.back() == L'\\')
            path.remove_suffix(1);
        return path;
    }

    // Accept only canonical local DOS paths. Device/UNC paths and Win32 aliases
    // must not turn an unsupported installation into an available module.
    inline constexpr bool CanonicalPath(std::wstring_view path) noexcept
    {
        if (path.size() < 3 || path.size() > 32767 ||
            FoldAscii(path[0]) < L'a' || FoldAscii(path[0]) > L'z' ||
            path[1] != L':' || path[2] != L'\\')
            return false;
        std::size_t start = 3;
        for (std::size_t index = 3; index <= path.size(); ++index)
        {
            if (index < path.size())
            {
                const wchar_t character = path[index];
                if (character < 32 || character == L'/' || character == L':' ||
                    character == L'<' || character == L'>' || character == L'"' ||
                    character == L'|' || character == L'?' || character == L'*')
                    return false;
                if (character != L'\\')
                    continue;
            }
            if (index == start)
                return index == path.size(); // One final directory separator.
            const auto component = path.substr(start, index - start);
            if (component == L"." || component == L".." || component.back() == L'.' || component.back() == L' ')
                return false;
            start = index + 1;
        }
        return true;
    }

    inline constexpr bool BelowSecureRoot(std::wstring_view directory, std::wstring_view root) noexcept
    {
        if (!CanonicalPath(directory) || !CanonicalPath(root))
            return false;
        directory = TrimDirectory(directory);
        root = TrimDirectory(root);
        return root.size() > 3 && directory.size() > root.size() &&
               directory[root.size()] == L'\\' && EqualPath(directory.substr(0, root.size()), root);
    }

    // Upgrade discovery can fail without producing a previous path. An empty or
    // inaccessible directory never establishes that the old runtime is absent.
    inline constexpr bool CanVerifyPreviousInstallation(bool upgrading, bool verifiedPrevious) noexcept
    {
        return !upgrading || verifiedPrevious;
    }

    enum class Scan
    {
        Unknown,
        Running,
        Empty
    };
    enum class ShutdownDecision
    {
        Wait,
        Complete,
        Refuse
    };

    // All matching engine AND guardian processes must remain absent across fresh
    // scans. A timeout/uncertain scan never authorizes termination or file changes.
    class ShutdownPolicy
    {
    public:
        static constexpr std::uint64_t TimeoutMs = 30000, QuietMs = 500;

        constexpr ShutdownDecision Observe(std::uint64_t now, Scan scan) noexcept
        {
            if (finished_)
                return decision_;
            if (!started_)
            {
                started_ = true;
                startedAt_ = lastAt_ = now;
            }
            if (now < lastAt_ || scan == Scan::Unknown || now - startedAt_ >= TimeoutMs)
                return Finish(ShutdownDecision::Refuse);
            lastAt_ = now;
            if (scan == Scan::Running)
            {
                quiet_ = false;
                return ShutdownDecision::Wait;
            }
            if (!quiet_)
            {
                quiet_ = true;
                quietAt_ = now;
            }
            return now - quietAt_ >= QuietMs ? Finish(ShutdownDecision::Complete) : ShutdownDecision::Wait;
        }

    private:
        constexpr ShutdownDecision Finish(ShutdownDecision decision) noexcept
        {
            finished_ = true;
            decision_ = decision;
            return decision;
        }
        std::uint64_t startedAt_ = 0, lastAt_ = 0, quietAt_ = 0;
        bool started_ = false, quiet_ = false, finished_ = false;
        ShutdownDecision decision_ = ShutdownDecision::Wait;
    };
}

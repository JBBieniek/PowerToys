// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <shlobj.h>
#include <string>
#include <memory>

#include "deployment_policy.h"

namespace glide::deployment
{
    inline bool FinalPath(const std::wstring& path, bool directory, std::wstring& output)
    {
        if (!CanonicalPath(path))
            return false;
        std::wstring final(32768, L'\0');
        const HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, directory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;
        const DWORD length = GetFinalPathNameByHandleW(file, final.data(), static_cast<DWORD>(final.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        CloseHandle(file);
        if (!length || length >= final.size())
            return false;
        final.resize(length);
        if (final.starts_with(L"\\\\?\\"))
            final.erase(0, 4);
        if (!CanonicalPath(final))
            return false;
        output = std::move(final);
        return true;
    }

    inline std::wstring ParentDirectory(std::wstring_view file)
    {
        const auto separator = file.rfind(L'\\');
        return separator == std::wstring_view::npos ? std::wstring{} : std::wstring(file.substr(0, separator));
    }

    // First release supports standard machine installation locations only.
    // Windows still verifies the signature and grants the actual UIAccess token;
    // this capability check never grants a privilege or relaxes a Windows policy.
    inline bool SupportedInstallation(const std::wstring& executable)
    {
        if (!CanonicalPath(executable))
            return false;
        std::wstring final;
        if (!FinalPath(executable, false, final))
            return false;
        const auto lexicalDirectory = ParentDirectory(executable);
        const auto finalDirectory = ParentDirectory(final);
        for (const KNOWNFOLDERID* id : { &FOLDERID_ProgramFiles, &FOLDERID_ProgramFilesX86, &FOLDERID_System })
        {
            PWSTR value = nullptr;
            const HRESULT result = SHGetKnownFolderPath(*id, KF_FLAG_DEFAULT, nullptr, &value);
            const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> release(value, CoTaskMemFree);
            if (SUCCEEDED(result) && value)
            {
                const std::wstring root(value);
                std::wstring finalRoot;
                if (FinalPath(root, true, finalRoot) && BelowSecureRoot(lexicalDirectory, root) && BelowSecureRoot(finalDirectory, finalRoot))
                    return true;
            }
        }
        return false;
    }
}

// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE in the project root.

#pragma once

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace glide::ui
{
    // Resource strings use numbered, typeless placeholders. Translations may
    // reorder them; percent signs and substituted text are always literal.
    template<std::size_t Count>
    bool TryFormatResource(std::wstring_view format, const std::array<std::wstring_view, Count>& values, std::wstring& output)
    {
        static_assert(Count <= 10);
        std::array<bool, Count> used{};
        std::wstring text;
        for (std::size_t index = 0; index < format.size(); ++index)
        {
            const wchar_t character = format[index];
            if (character == L'{')
            {
                if (index + 2 >= format.size() || format[index + 1] < L'0' || format[index + 1] > L'9' || format[index + 2] != L'}')
                    return false;
                const auto argument = static_cast<std::size_t>(format[index + 1] - L'0');
                if (argument >= Count)
                    return false;
                text.append(values[argument]);
                used[argument] = true;
                index += 2;
            }
            else if (character == L'}')
                return false;
            else
                text.push_back(character);
        }
        for (const bool present : used)
            if (!present)
                return false;
        output = std::move(text);
        return true;
    }

    template<std::size_t Count>
    std::wstring FormatResource(std::wstring_view localized, std::wstring_view english, const std::array<std::wstring_view, Count>& values)
    {
        std::wstring result;
        if (!TryFormatResource(localized, values, result))
            TryFormatResource(english, values, result);
        return result;
    }
}

#pragma once

#include <string_view>

namespace fizzy
{
bool utf8_validate(const uint8_t* start, const uint8_t* end) noexcept;

inline bool utf8_validate(std::string_view input) noexcept
{
    return utf8_validate(reinterpret_cast<const uint8_t*>(input.begin()),
        reinterpret_cast<const uint8_t*>(input.end()));
}
}  // namespace fizzy

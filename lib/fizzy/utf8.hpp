#pragma once

#include <string_view>

namespace fizzy
{
bool utf8_validate(const uint8_t* start, const uint8_t* end) noexcept;
}  // namespace fizzy

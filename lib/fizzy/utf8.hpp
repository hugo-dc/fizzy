#pragma once

#include <string_view>

namespace fizzy
{
bool utf8_validate(std::string_view input) noexcept;
}  // namespace fizzy

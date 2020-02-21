#pragma once

#include "exceptions.hpp"
#include <cstdint>
#include <limits>

namespace fizzy
{
template <typename T>
std::pair<T, const uint8_t*> leb128u_decode(const uint8_t* input, const uint8_t* end)
{
    static_assert(!std::numeric_limits<T>::is_signed);

    T result = 0;
    int result_shift = 0;

    for (; result_shift < std::numeric_limits<T>::digits; ++input, result_shift += 7)
    {
        if (input == end)
            throw parser_error("Unexpected EOF");

        result |= static_cast<T>((static_cast<T>(*input) & 0x7F) << result_shift);
        if ((*input & 0x80) == 0)
        {
            if (*input != (result >> result_shift))
                throw parser_error("Invalid LEB128 encoding: unused bits set.");

            return {result, input + 1};
        }
    }

    throw parser_error("Invalid LEB128 encoding: too many bytes.");
}

template <typename T>
std::pair<T, const uint8_t*> leb128s_decode(const uint8_t* input, const uint8_t* end)
{
    static_assert(std::numeric_limits<T>::is_signed);

    using T_unsigned = typename std::make_unsigned<T>::type;
    T_unsigned result = 0;
    size_t result_shift = 0;

    for (; result_shift < std::numeric_limits<T_unsigned>::digits; ++input, result_shift += 7)
    {
        if (input == end)
            throw parser_error("Unexpected EOF");

        result |= static_cast<T_unsigned>((static_cast<T_unsigned>(*input) & 0x7F) << result_shift);
        if ((*input & 0x80) == 0)
        {
            constexpr auto all_ones = std::numeric_limits<T_unsigned>::max();

            if (result_shift + 7 < sizeof(T_unsigned) * 8)
            {
                // non-terminal byte of the encoding, extend the sign bit, if needed
                if ((*input & 0x40) != 0)
                {
                    const auto mask = static_cast<T_unsigned>(all_ones << (result_shift + 7));
                    result |= mask;
                }
            }
            else
            {
                // terminal byte of the encoding, need to check unused bits
                const auto unused_bits_mask = ~static_cast<uint8_t>(all_ones >> result_shift);
                // unused bits must be set if and only if sign bit is set
                const auto unused_bits_expected =
                    static_cast<T>(result) < 0 ? (unused_bits_mask & 0x7F) : 0;
                if ((*input & unused_bits_mask) != unused_bits_expected)
                {
                    throw parser_error(
                        "Invalid LEB128 encoding: unused bits not equal to sign bit.");
                }
            }

            return {static_cast<T>(result), input + 1};
        }
    }

    throw parser_error("Invalid LEB128 encoding: too many bytes.");
}

}  // namespace fizzy
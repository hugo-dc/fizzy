#include "utf8.hpp"
#include <cassert>

/*
 * The Unicode Standard, Version 6.0
 * (https://www.unicode.org/versions/Unicode6.0.0/ch03.pdf)
 *
 * Page 94, Table 3-7. Well-Formed UTF-8 Byte Sequences
 *
 * +--------------------+------------+-------------+------------+-------------+
 * | Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0000..U+007F     | 00..7F     |             |            |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0080..U+07FF     | C2..DF     | 80..BF      |            |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0800..U+0FFF     | E0         | A0..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+1000..U+CFFF     | E1..EC     | 80..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+D000..U+D7FF     | ED         | 80..9F      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+E000..U+FFFF     | EE..EF     | 80..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+10000..U+3FFFF   | F0         | 90..BF      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+40000..U+FFFFF   | F1..F3     | 80..BF      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+100000..U+10FFFF | F4         | 80..8F      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 */

// Ranges for 2nd byte
enum class Rule : unsigned
{
    Range80BF,  // 80..BF
    RangeA0BF,  // A0..BF
    Range809F,  // 80..9F
    Range90BF,  // 90..BF
    Range808F,  // 80..8F
};

namespace fizzy
{
bool utf8_validate(std::string_view input) noexcept
{
    const auto len = input.size();
    unsigned pos = 0;

    while (pos < len)
    {
    unsigned required_bytes = 1;
    auto rule = Rule::Range80BF;

    const uint8_t byte1 = static_cast<uint8_t>(input[pos++]);
    if (byte1 <= 0x7F)
        // Shortcut for valid ASCII (also valid UTF-8)
        return true;
    else if (byte1 < 0xC2)
        return false;
    else if (byte1 <= 0xDF)
    {
        required_bytes = 2;
        rule = Rule::Range80BF;
    }
    else if (byte1 == 0xE0)
    {
        required_bytes = 3;
        rule = Rule::RangeA0BF;
    }
    else if (byte1 <= 0xEC)
    {
        required_bytes = 3;
        rule = Rule::Range80BF;
    }
    else if (byte1 == 0xED)
    {
        required_bytes = 3;
        rule = Rule::Range809F;
    }
    else if (byte1 <= 0xEF)
    {
        required_bytes = 3;
        rule = Rule::Range80BF;
    }
    else if (byte1 == 0xF0)
    {
        required_bytes = 4;
        rule = Rule::Range90BF;
    }
    else if (byte1 <= 0xF3)
    {
        required_bytes = 4;
        rule = Rule::Range80BF;
    }
    else if (byte1 == 0xF4)
    {
        required_bytes = 4;
        rule = Rule::Range808F;
    }
    else
        return false;

    // At this point need to read at least one more byte
    if ((pos + len) < required_bytes)
        return false;

    // Byte2 may have exceptional encodings
    const uint8_t byte2 = static_cast<uint8_t>(input[pos++]);
    switch (rule)
    {
    case Rule::Range80BF:
        if (byte2 < 0x80 || byte2 > 0xBF)
            return false;
        break;
    case Rule::RangeA0BF:
        if (byte2 < 0xA0 || byte2 > 0xBF)
            return false;
        break;
    case Rule::Range809F:
        if (byte2 < 0x80 || byte2 > 0x9F)
            return false;
        break;
    case Rule::Range90BF:
        if (byte2 < 0x90 || byte2 > 0xBF)
            return false;
        break;
    case Rule::Range808F:
        if (byte2 < 0x80 || byte2 > 0x8F)
            return false;
        break;
    default:
        assert(false);
    }

    // Byte3 always has regular encoding
    if (required_bytes > 2)
    {
        const uint8_t byte3 = static_cast<uint8_t>(input[pos++]);
        if (byte3 < 0x80 || byte3 > 0xBF)
            return false;
    }

    // Byte4 always has regular encoding
    if (required_bytes > 3)
    {
        const uint8_t byte4 = static_cast<uint8_t>(input[pos++]);
        if (byte4 < 0x80 || byte4 > 0xBF)
            return false;
    }
    }

    return true;
}

}  // namespace fizzy

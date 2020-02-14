#include "utf8.hpp"
#include <gtest/gtest.h>
#include <test/utils/asserts.hpp>
#include <test/utils/hex.hpp>

using namespace fizzy;

TEST(utf8, validate)
{
    std::vector<std::pair<bytes, bool>> testcases = {
        {"00"_bytes, true},
        {"7f"_bytes, true},
        {"80"_bytes, false},
        {"c2"_bytes, false},
        {"c280"_bytes, true},
        {"c2bf"_bytes, true},
        {"c2e0"_bytes, false},
    };
    for (auto const& testcase : testcases)
    {
        const auto input = testcase.first;
        EXPECT_EQ(utf8_validate(std::string(input.begin(), input.end())), testcase.second);
    }
}
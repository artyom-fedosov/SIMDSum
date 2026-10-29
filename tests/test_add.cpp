// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

import simdsum;

namespace {

struct Vec2 {
    int x{};
    int y{};
    friend Vec2 operator+(const Vec2& l, const Vec2& r) { return {l.x + r.x, l.y + r.y}; }
    friend bool operator==(const Vec2&, const Vec2&) = default;
};

// Summable but not default-constructible: the returning overload must not need T{}.
struct Tagged {
    explicit Tagged(int v) : value{v} {}
    int value;
    friend Tagged operator+(const Tagged& l, const Tagged& r) { return Tagged{l.value + r.value}; }
};

} // namespace

TEST(Add, EmptyArrays)
{
    const std::vector<int> a;
    const std::vector<int> b;
    std::vector<int> out;
    simdsum::add(a, b, out);
    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(simdsum::add(a, b).empty());
}

TEST(Add, SingleElement)
{
    const std::array a{40};
    const std::array b{2};
    std::array<int, 1> out{};
    simdsum::add(a, b, out);
    EXPECT_EQ(out[0], 42);
}

TEST(Add, SmallKnownValues)
{
    const std::vector a{1, 2, 3, 4, 5};
    const std::vector b{10, 20, 30, 40, 50};
    EXPECT_EQ(simdsum::add(a, b), (std::vector{11, 22, 33, 44, 55}));
}

TEST(Add, AcceptsSpansAndCArrays)
{
    const double a[] = {0.5, 1.5, 2.5};
    const std::vector b{0.5, 0.5, 0.5};
    double out[3]{};
    simdsum::add(std::span{a}, std::span{b}, std::span{out});
    EXPECT_EQ(out[0], 1.0);
    EXPECT_EQ(out[1], 2.0);
    EXPECT_EQ(out[2], 3.0);
}

TEST(Add, MismatchedInputSizesThrow)
{
    const std::vector a{1, 2, 3};
    const std::vector b{1, 2};
    std::vector<int> out(3);
    EXPECT_THROW(simdsum::add(a, b, out), std::invalid_argument);
    EXPECT_THROW((void)simdsum::add(a, b), std::invalid_argument);
}

TEST(Add, MismatchedOutputSizeThrows)
{
    const std::vector a{1, 2, 3};
    const std::vector b{1, 2, 3};
    std::vector<int> too_small(2);
    std::vector<int> too_big(4);
    EXPECT_THROW(simdsum::add(a, b, too_small), std::invalid_argument);
    EXPECT_THROW(simdsum::add(a, b, too_big), std::invalid_argument);
}

TEST(Add, InPlace)
{
    std::vector a{1, 2, 3};
    const std::vector b{4, 5, 6};
    simdsum::add(a, b, a);
    EXPECT_EQ(a, (std::vector{5, 7, 9}));
}

TEST(GenericAdd, StringsConcatenate)
{
    const std::vector<std::string> a{"foo", "", "SIMD"};
    const std::vector<std::string> b{"bar", "x", "Sum"};
    EXPECT_EQ(simdsum::add(a, b), (std::vector<std::string>{"foobar", "x", "SIMDSum"}));
}

TEST(GenericAdd, UserDefinedStruct)
{
    const std::vector<Vec2> a{{1, 2}, {3, 4}};
    const std::vector<Vec2> b{{10, 20}, {30, 40}};
    std::vector<Vec2> out(2);
    simdsum::add(a, b, out);
    EXPECT_EQ(out, (std::vector<Vec2>{{11, 22}, {33, 44}}));
}

TEST(GenericAdd, ChronoDurations)
{
    using namespace std::chrono_literals;
    const std::vector a{1ms, 2ms};
    const std::vector b{999ms, 3ms};
    EXPECT_EQ(simdsum::add(a, b), (std::vector{1000ms, 5ms}));
}

TEST(GenericAdd, NonDefaultConstructibleType)
{
    const std::vector a{Tagged{1}, Tagged{2}};
    const std::vector b{Tagged{3}, Tagged{4}};
    const auto out = simdsum::add(a, b);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].value, 4);
    EXPECT_EQ(out[1].value, 6);
}

TEST(GenericAdd, BoolIsNotRoutedThroughByteKernels)
{
    // true + true == 2 converts back to true; a byte-wise SIMD add would produce an invalid bool.
    const std::array<bool, 3> a{true, true, false};
    const std::array<bool, 3> b{true, false, false};
    std::array<bool, 3> out{};
    simdsum::add(a, b, out);
    EXPECT_EQ(out, (std::array{true, true, false}));
}

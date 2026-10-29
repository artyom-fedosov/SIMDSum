// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <complex>
#include <cstdint>
#include <list>
#include <mutex>
#include <span>
#include <string>
#include <vector>

import simdsum;

namespace {

struct Vec2 {
    double x{};
    double y{};
    friend Vec2 operator+(const Vec2& l, const Vec2& r) { return {l.x + r.x, l.y + r.y}; }
};

struct NotAddable {
    int v{};
};

} // namespace

TEST(Summable, AcceptsArithmeticTypes)
{
    static_assert(simdsum::Summable<int>);
    static_assert(simdsum::Summable<std::int8_t>);
    static_assert(simdsum::Summable<std::uint64_t>);
    static_assert(simdsum::Summable<float>);
    static_assert(simdsum::Summable<double>);
    static_assert(simdsum::Summable<long double>);
}

TEST(Summable, AcceptsClassTypesWithPlus)
{
    static_assert(simdsum::Summable<std::string>);
    static_assert(simdsum::Summable<std::complex<double>>);
    static_assert(simdsum::Summable<std::chrono::milliseconds>);
    static_assert(simdsum::Summable<Vec2>);
}

TEST(Summable, RejectsTypesWithoutPlus)
{
    static_assert(!simdsum::Summable<NotAddable>);
    static_assert(!simdsum::Summable<std::mutex>);
    static_assert(!simdsum::Summable<void*>);
    static_assert(!simdsum::Summable<std::vector<int>>);
}

TEST(SummableRange, RequiresContiguousStorage)
{
    static_assert(simdsum::SummableRange<std::vector<int>>);
    static_assert(simdsum::SummableRange<std::array<float, 4>>);
    static_assert(simdsum::SummableRange<std::span<const double>>);
    static_assert(simdsum::SummableRange<int[8]>);
    static_assert(!simdsum::SummableRange<std::list<int>>);
    static_assert(!simdsum::SummableRange<std::vector<NotAddable>>);
}

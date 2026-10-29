// SPDX-License-Identifier: GPL-3.0-or-later
// Exercises every SIMD kernel (one parameter per instruction set) against a naive reference loop.
#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

import simdsum;

namespace {

template <class T>
struct is_complex : std::false_type {};
template <class F>
struct is_complex<std::complex<F>> : std::true_type {};

template <class T>
T random_value(std::mt19937_64& rng)
{
    if constexpr (is_complex<T>::value) {
        using F = typename T::value_type;
        return {random_value<F>(rng), random_value<F>(rng)};
    } else if constexpr (std::is_floating_point_v<T>) {
        return std::uniform_real_distribution<T>{T(-1e6), T(1e6)}(rng);
    } else {
        // Full bit range, so additions overflow regularly.
        return static_cast<T>(rng());
    }
}

// Reference result with the library's documented semantics: integers wrap around.
template <class T>
T reference_add(T x, T y)
{
    if constexpr (std::is_integral_v<T>) {
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(static_cast<U>(static_cast<U>(x) + static_cast<U>(y)));
    } else {
        return x + y;
    }
}

template <class T>
std::vector<T> random_vector(std::size_t n, std::mt19937_64& rng)
{
    std::vector<T> v(n);
    for (auto& x : v) {
        x = random_value<T>(rng);
    }
    return v;
}

template <class T>
::testing::AssertionResult equal_to_reference(std::span<const T> a, std::span<const T> b, std::span<const T> out)
{
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (out[i] != reference_add(a[i], b[i])) {
            return ::testing::AssertionFailure() << "mismatch at index " << i << " of " << out.size();
        }
    }
    return ::testing::AssertionSuccess();
}

template <class T>
void check_all_lengths(std::size_t max_len, std::mt19937_64& rng)
{
    SCOPED_TRACE(::testing::Message() << "sizeof(T)=" << sizeof(T));
    for (std::size_t n = 0; n <= max_len; ++n) {
        const auto a = random_vector<T>(n, rng);
        const auto b = random_vector<T>(n, rng);
        std::vector<T> out(n);
        simdsum::add(a, b, out);
        ASSERT_TRUE(equal_to_reference<T>(a, b, out)) << "n=" << n;
    }
}

template <class T>
void check_large(std::mt19937_64& rng)
{
    constexpr std::size_t n = (std::size_t{1} << 20) + 13;
    const auto a = random_vector<T>(n, rng);
    const auto b = random_vector<T>(n, rng);
    const auto out = simdsum::add(a, b);
    ASSERT_TRUE(equal_to_reference<T>(a, b, out));
}

// Offsets every pointer by one element so no access is vector-aligned.
template <class T>
void check_misaligned(std::mt19937_64& rng)
{
    for (std::size_t n : {1u, 7u, 63u, 64u, 65u, 257u, 1000u}) {
        const auto a = random_vector<T>(n + 1, rng);
        const auto b = random_vector<T>(n + 2, rng);
        std::vector<T> out(n + 3);
        const std::span<const T> sa{a.data() + 1, n};
        const std::span<const T> sb{b.data() + 2, n};
        const std::span<T> so{out.data() + 3, n};
        simdsum::add(sa, sb, so);
        ASSERT_TRUE(equal_to_reference<T>(sa, sb, so)) << "n=" << n;
    }
}

template <class T>
void check_in_place(std::mt19937_64& rng)
{
    for (std::size_t n : {0u, 1u, 31u, 129u, 4099u}) {
        const auto a = random_vector<T>(n, rng);
        const auto b = random_vector<T>(n, rng);

        auto into_a = a;
        simdsum::add(into_a, b, into_a);
        ASSERT_TRUE(equal_to_reference<T>(a, b, into_a)) << "out==a, n=" << n;

        auto into_b = b;
        simdsum::add(a, into_b, into_b);
        ASSERT_TRUE(equal_to_reference<T>(a, b, into_b)) << "out==b, n=" << n;

        auto doubled = a;
        simdsum::add(doubled, doubled, doubled);
        ASSERT_TRUE(equal_to_reference<T>(a, a, doubled)) << "a==b==out, n=" << n;
    }
}

// Above the library's streaming-store threshold (4 MiB of output), with an `out` that is not
// vector-aligned (scalar prologue), a length that leaves a tail, and in-place variants.
template <class T>
void check_streaming(std::mt19937_64& rng)
{
    constexpr std::size_t n = (std::size_t{9} << 20) / sizeof(T) + 37;
    const auto a = random_vector<T>(n, rng);
    const auto b = random_vector<T>(n, rng);
    for (std::size_t offset : {0u, 1u, 3u}) {
        std::vector<T> storage(n + offset);
        const std::span<T> out{storage.data() + offset, n};
        simdsum::add(a, b, out);
        ASSERT_TRUE(equal_to_reference<T>(a, b, out)) << "offset=" << offset;
    }
    auto in_place = a;
    simdsum::add(in_place, b, in_place);
    ASSERT_TRUE(equal_to_reference<T>(a, b, in_place)) << "in place";
}

template <class... Ts>
struct TypeList {};

using SimdTypes = TypeList<std::int8_t, std::uint8_t, char, std::int16_t, std::uint16_t, std::int32_t,
                           std::uint32_t, std::int64_t, std::uint64_t, long long, unsigned long long, float,
                           double, std::complex<float>, std::complex<double>>;

template <class... Ts, class F>
void for_each_type(TypeList<Ts...>, F&& f)
{
    (f.template operator()<Ts>(), ...);
}

class Kernels : public ::testing::TestWithParam<simdsum::Isa> {
protected:
    void SetUp() override
    {
        if (!simdsum::isa_supported(GetParam())) {
            GTEST_SKIP() << "instruction set not supported by this CPU";
        }
        ASSERT_TRUE(simdsum::force_isa(GetParam()));
        ASSERT_EQ(simdsum::active_isa(), GetParam());
    }

    void TearDown() override { simdsum::force_isa(best_); }

    std::mt19937_64 rng_{0x51D5u};

private:
    simdsum::Isa best_ = simdsum::active_isa();
};

std::string isa_name(const ::testing::TestParamInfo<simdsum::Isa>& info)
{
    return std::string{simdsum::to_string(info.param)};
}

template <class T>
std::vector<T> repeat(std::initializer_list<T> pattern, std::size_t n)
{
    std::vector<T> v;
    v.reserve(n);
    while (v.size() < n) {
        for (const T& x : pattern) {
            if (v.size() < n) {
                v.push_back(x);
            }
        }
    }
    return v;
}

template <class T>
void check_float_specials()
{
    constexpr T inf = std::numeric_limits<T>::infinity();
    constexpr T nan = std::numeric_limits<T>::quiet_NaN();
    constexpr T den = std::numeric_limits<T>::denorm_min();
    constexpr T max = std::numeric_limits<T>::max();
    constexpr std::size_t n = 203; // several full vectors plus a tail on every ISA
    const auto a = repeat<T>({inf, inf, -inf, nan, T(-0.0), T(-0.0), den, max, T(1)}, n);
    const auto b = repeat<T>({T(1), -inf, -inf, T(1), T(-0.0), T(0.0), den, max, nan}, n);
    std::vector<T> out(n);
    simdsum::add(a, b, out);
    for (std::size_t i = 0; i < n; ++i) {
        const T expected = a[i] + b[i];
        if (std::isnan(expected)) {
            ASSERT_TRUE(std::isnan(out[i])) << "i=" << i;
        } else {
            // Bitwise: distinguishes -0.0 from +0.0.
            using Bits = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
            ASSERT_EQ(std::bit_cast<Bits>(out[i]), std::bit_cast<Bits>(expected))
                << "i=" << i;
        }
    }
}

} // namespace

TEST_P(Kernels, EveryLengthUpTo300)
{
    for_each_type(SimdTypes{}, [this]<class T>() { check_all_lengths<T>(300, rng_); });
}

TEST_P(Kernels, LargeArray)
{
    for_each_type(SimdTypes{}, [this]<class T>() { check_large<T>(rng_); });
}

TEST_P(Kernels, StreamingStoresForLargeOutputs)
{
    for_each_type(SimdTypes{}, [this]<class T>() { check_streaming<T>(rng_); });
}

TEST_P(Kernels, MisalignedPointers)
{
    for_each_type(SimdTypes{}, [this]<class T>() { check_misaligned<T>(rng_); });
}

TEST_P(Kernels, InPlace)
{
    for_each_type(SimdTypes{}, [this]<class T>() { check_in_place<T>(rng_); });
}

TEST_P(Kernels, IntegerWrapAround)
{
    using L8 = std::numeric_limits<std::int8_t>;
    using L64 = std::numeric_limits<std::int64_t>;
    using U64 = std::numeric_limits<std::uint64_t>;
    const auto i8 = simdsum::add(std::vector<std::int8_t>(100, L8::max()), std::vector<std::int8_t>(100, 1));
    EXPECT_EQ(i8, std::vector<std::int8_t>(100, L8::min()));
    const auto i64 = simdsum::add(std::vector<std::int64_t>(37, L64::min()), std::vector<std::int64_t>(37, -1));
    EXPECT_EQ(i64, std::vector<std::int64_t>(37, L64::max()));
    const auto u64 = simdsum::add(std::vector<std::uint64_t>(37, U64::max()), std::vector<std::uint64_t>(37, 1));
    EXPECT_EQ(u64, std::vector<std::uint64_t>(37, 0));
    const auto u16 = simdsum::add(std::vector<std::uint16_t>(70, 65535), std::vector<std::uint16_t>(70, 2));
    EXPECT_EQ(u16, std::vector<std::uint16_t>(70, 1));
}

TEST_P(Kernels, FloatSpecialValues)
{
    check_float_specials<float>();
    check_float_specials<double>();
}

INSTANTIATE_TEST_SUITE_P(AllIsas, Kernels,
                         ::testing::Values(simdsum::Isa::Scalar, simdsum::Isa::SSE2, simdsum::Isa::AVX2,
                                           simdsum::Isa::AVX512),
                         isa_name);

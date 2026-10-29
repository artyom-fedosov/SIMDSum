// SPDX-License-Identifier: GPL-3.0-or-later
module;

#include <algorithm>
#include <complex>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#include "simdsum_export.h"

export module simdsum;

export namespace simdsum {

/// Library version, "MAJOR.MINOR.PATCH".
SIMDSUM_API std::string_view version() noexcept;

/// Instruction sets the library has kernels for, from slowest to fastest.
enum class Isa : std::uint8_t { Scalar, SSE2, AVX2, AVX512 };

constexpr std::string_view to_string(Isa isa) noexcept
{
    switch (isa) {
    case Isa::Scalar:
        return "Scalar";
    case Isa::SSE2:
        return "SSE2";
    case Isa::AVX2:
        return "AVX2";
    case Isa::AVX512:
        return "AVX512";
    }
    return "Unknown";
}

/// Whether this CPU (and OS) can run the kernels for `isa`.
SIMDSUM_API bool isa_supported(Isa isa) noexcept;

/// The instruction set currently used by add(). Defaults to the best supported one.
SIMDSUM_API Isa active_isa() noexcept;

/// Switches all subsequent add() calls to `isa` (intended for tests and benchmarks).
/// Returns false, leaving the active set unchanged, if the CPU does not support it.
SIMDSUM_API bool force_isa(Isa isa) noexcept;

/// A type whose values can be added and the result stored back into a T.
template <class T>
concept Summable = std::assignable_from<T&, T> && requires(const T& x, const T& y) {
    { x + y } -> std::convertible_to<T>;
};

/// A contiguous, sized range of Summable elements (std::vector, std::array, std::span, C array, ...).
template <class R>
concept SummableRange = std::ranges::contiguous_range<R> && std::ranges::sized_range<R>
                        && Summable<std::remove_cv_t<std::ranges::range_value_t<R>>>;

} // namespace simdsum

// C ABI of the shared library: each call dispatches to the active instruction set.
// `out` may equal `a` and/or `b`; partial overlap is not supported.
extern "C" {
SIMDSUM_API void simdsum_add_u8(const void* a, const void* b, void* out, std::size_t n) noexcept;
SIMDSUM_API void simdsum_add_u16(const void* a, const void* b, void* out, std::size_t n) noexcept;
SIMDSUM_API void simdsum_add_u32(const void* a, const void* b, void* out, std::size_t n) noexcept;
SIMDSUM_API void simdsum_add_u64(const void* a, const void* b, void* out, std::size_t n) noexcept;
SIMDSUM_API void simdsum_add_f32(const void* a, const void* b, void* out, std::size_t n) noexcept;
SIMDSUM_API void simdsum_add_f64(const void* a, const void* b, void* out, std::size_t n) noexcept;
}

namespace simdsum::detail {

template <class T>
inline constexpr bool is_complex_v = false;
template <class F>
inline constexpr bool is_complex_v<std::complex<F>> = true;

// Which library kernel handles T, if any.
// Signed and unsigned two's-complement addition are bit-identical, so integers go by size only.
// bool is excluded: true + true must stay true, not become the byte 0x02.
enum class Kernel : std::uint8_t { None, U8, U16, U32, U64, F32, F64 };

template <class T>
consteval Kernel kernel_for() noexcept
{
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
        switch (sizeof(T)) {
        case 1:
            return Kernel::U8;
        case 2:
            return Kernel::U16;
        case 4:
            return Kernel::U32;
        case 8:
            return Kernel::U64;
        default:
            return Kernel::None;
        }
    } else if constexpr (std::is_same_v<T, float>) {
        return Kernel::F32;
    } else if constexpr (std::is_same_v<T, double>) {
        return Kernel::F64;
    } else if constexpr (is_complex_v<T>) {
        // std::complex<F> is guaranteed to be laid out as F[2].
        return kernel_for<typename T::value_type>();
    } else {
        return Kernel::None;
    }
}

template <Kernel K>
void call_kernel(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    if constexpr (K == Kernel::U8) {
        simdsum_add_u8(a, b, out, n);
    } else if constexpr (K == Kernel::U16) {
        simdsum_add_u16(a, b, out, n);
    } else if constexpr (K == Kernel::U32) {
        simdsum_add_u32(a, b, out, n);
    } else if constexpr (K == Kernel::U64) {
        simdsum_add_u64(a, b, out, n);
    } else if constexpr (K == Kernel::F32) {
        simdsum_add_f32(a, b, out, n);
    } else {
        static_assert(K == Kernel::F64);
        simdsum_add_f64(a, b, out, n);
    }
}

// Number of kernel elements per T (2 for std::complex).
template <class T>
inline constexpr std::size_t lanes_per_element = is_complex_v<T> ? 2 : 1;

template <class R>
using element_t = std::remove_cv_t<std::ranges::range_value_t<R>>;

template <class T>
void add_generic(std::span<const T> a, std::span<const T> b, std::span<T> out)
{
    std::ranges::transform(a, b, out.begin(), [](const T& x, const T& y) -> T { return x + y; });
}

template <class T>
void add_span(std::span<const T> a, std::span<const T> b, std::span<T> out)
{
    if (a.size() != b.size() || a.size() != out.size()) {
        throw std::invalid_argument("simdsum::add: input and output sizes must match");
    }
    if constexpr (constexpr Kernel kernel = kernel_for<T>(); kernel != Kernel::None) {
        call_kernel<kernel>(a.data(), b.data(), out.data(), a.size() * lanes_per_element<T>);
    } else {
        add_generic(a, b, out);
    }
}

} // namespace simdsum::detail

export namespace simdsum {

/// out[i] = a[i] + b[i] for every i.
///
/// All three ranges must have the same size, otherwise std::invalid_argument is thrown.
/// `out` may be the very same storage as `a` or `b` (in-place add); partial overlap is not supported.
/// Integer overflow wraps around (two's complement) for every integral type.
template <SummableRange A, SummableRange B, SummableRange Out>
    requires std::same_as<detail::element_t<A>, detail::element_t<B>>
             && std::same_as<detail::element_t<A>, std::ranges::range_value_t<Out>>
             && (!std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<Out>>>)
void add(const A& a, const B& b, Out&& out)
{
    using T = detail::element_t<A>;
    detail::add_span<T>(std::span<const T>{std::ranges::data(a), std::ranges::size(a)},
                        std::span<const T>{std::ranges::data(b), std::ranges::size(b)},
                        std::span<T>{std::ranges::data(out), std::ranges::size(out)});
}

/// Returns a new vector with a[i] + b[i]. Throws std::invalid_argument if the sizes differ.
template <SummableRange A, SummableRange B>
    requires std::same_as<detail::element_t<A>, detail::element_t<B>>
[[nodiscard]] std::vector<detail::element_t<A>> add(const A& a, const B& b)
{
    using T = detail::element_t<A>;
    const auto n = std::ranges::size(a);
    if (n != std::ranges::size(b)) {
        throw std::invalid_argument("simdsum::add: input sizes must match");
    }
    std::vector<T> out;
    if constexpr (std::is_default_constructible_v<T>) {
        out.resize(n);
        add(a, b, out);
    } else {
        out.reserve(n);
        const T* pa = std::ranges::data(a);
        const T* pb = std::ranges::data(b);
        for (std::size_t i = 0; i < n; ++i) {
            out.push_back(pa[i] + pb[i]);
        }
    }
    return out;
}

} // namespace simdsum

// SPDX-License-Identifier: GPL-3.0-or-later
module;

#include <algorithm>
#include <concepts>
#include <cstddef>
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

namespace simdsum::detail {

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
    add_generic(a, b, out);
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

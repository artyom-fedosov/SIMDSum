// SPDX-License-Identifier: GPL-3.0-or-later
//
// Private loop skeleton textually included by the kernel translation units (never installed).
//
// The kernel TUs are compiled with ISA-specific flags (-mavx2, -mavx512f, ...). They are deliberately not
// module units: a BMI has to be imported with the target flags it was built with. To keep AVX encodings from
// leaking into code that runs on older CPUs, everything here lives in an anonymous namespace (internal
// linkage, so nothing is COMDAT-merged with default-flag code) and only freestanding headers are used.
#include <cstddef>
#include <cstdint>

namespace {

// Element-wise add over raw bytes. T is an unsigned integer or floating-point type, so overflow wraps and
// nothing is UB; memcpy keeps it independent of the caller's element type (strict aliasing).
template <class T>
inline void scalar_loop(const unsigned char* a, const unsigned char* b, unsigned char* out, std::size_t n) noexcept
{
    for (std::size_t i = 0; i < n; ++i) {
        T x;
        T y;
        __builtin_memcpy(&x, a + i * sizeof(T), sizeof(T));
        __builtin_memcpy(&y, b + i * sizeof(T), sizeof(T));
        const T r = static_cast<T>(x + y);
        __builtin_memcpy(out + i * sizeof(T), &r, sizeof(T));
    }
}

// Ops provides: elem, lanes, reg, load(p), store(p, v), add(x, y), tail(a, b, out, n) for n < lanes.
//
// Every store only overwrites bytes that the same step has already loaded, so out == a and/or out == b
// (in-place add) is safe. Partial overlap is not supported.
template <class Ops>
inline void simd_loop(const void* va, const void* vb, void* vout, std::size_t n) noexcept
{
    constexpr std::size_t lanes = Ops::lanes;
    constexpr std::size_t step = sizeof(typename Ops::elem) * lanes; // bytes per vector
    const auto* a = static_cast<const unsigned char*>(va);
    const auto* b = static_cast<const unsigned char*>(vb);
    auto* out = static_cast<unsigned char*>(vout);

    // Four independent load/add/store chains per iteration keep both load ports busy.
    std::size_t i = 0;
    for (; i + 4 * lanes <= n; i += 4 * lanes) {
        const auto a0 = Ops::load(a + 0 * step);
        const auto a1 = Ops::load(a + 1 * step);
        const auto a2 = Ops::load(a + 2 * step);
        const auto a3 = Ops::load(a + 3 * step);
        const auto b0 = Ops::load(b + 0 * step);
        const auto b1 = Ops::load(b + 1 * step);
        const auto b2 = Ops::load(b + 2 * step);
        const auto b3 = Ops::load(b + 3 * step);
        Ops::store(out + 0 * step, Ops::add(a0, b0));
        Ops::store(out + 1 * step, Ops::add(a1, b1));
        Ops::store(out + 2 * step, Ops::add(a2, b2));
        Ops::store(out + 3 * step, Ops::add(a3, b3));
        a += 4 * step;
        b += 4 * step;
        out += 4 * step;
    }
    for (; i + lanes <= n; i += lanes) {
        Ops::store(out, Ops::add(Ops::load(a), Ops::load(b)));
        a += step;
        b += step;
        out += step;
    }
    if (i < n) {
        Ops::tail(a, b, out, n - i);
    }
}

// Ops for one vector width share everything except elem/lanes/add; this fills in the scalar tail.
template <class T, std::size_t VectorBytes>
struct ScalarTail {
    using elem = T;
    static constexpr std::size_t lanes = VectorBytes / sizeof(T);

    static void tail(const unsigned char* a, const unsigned char* b, unsigned char* out, std::size_t n) noexcept
    {
        scalar_loop<T>(a, b, out, n);
    }
};

} // namespace

// Defines the six C-linkage entry points of one instruction set, e.g. simdsum_avx2_add_u32.
#define SIMDSUM_DEFINE_KERNELS(isa, IntOps, F32Ops, F64Ops)                                                     \
    extern "C" {                                                                                               \
    void simdsum_##isa##_add_u8(const void* a, const void* b, void* out, std::size_t n) noexcept               \
    {                                                                                                          \
        simd_loop<IntOps<std::uint8_t>>(a, b, out, n);                                                         \
    }                                                                                                          \
    void simdsum_##isa##_add_u16(const void* a, const void* b, void* out, std::size_t n) noexcept              \
    {                                                                                                          \
        simd_loop<IntOps<std::uint16_t>>(a, b, out, n);                                                        \
    }                                                                                                          \
    void simdsum_##isa##_add_u32(const void* a, const void* b, void* out, std::size_t n) noexcept              \
    {                                                                                                          \
        simd_loop<IntOps<std::uint32_t>>(a, b, out, n);                                                        \
    }                                                                                                          \
    void simdsum_##isa##_add_u64(const void* a, const void* b, void* out, std::size_t n) noexcept              \
    {                                                                                                          \
        simd_loop<IntOps<std::uint64_t>>(a, b, out, n);                                                        \
    }                                                                                                          \
    void simdsum_##isa##_add_f32(const void* a, const void* b, void* out, std::size_t n) noexcept              \
    {                                                                                                          \
        simd_loop<F32Ops>(a, b, out, n);                                                                       \
    }                                                                                                          \
    void simdsum_##isa##_add_f64(const void* a, const void* b, void* out, std::size_t n) noexcept              \
    {                                                                                                          \
        simd_loop<F64Ops>(a, b, out, n);                                                                       \
    }                                                                                                          \
    }

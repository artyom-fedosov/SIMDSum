// SPDX-License-Identifier: GPL-3.0-or-later
// 128-bit kernels, compiled with -msse2.
#include <immintrin.h>

#include "kernel_loop.inc"

namespace {

struct Sse2Io {
    static __m128i load(const unsigned char* p) noexcept { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p)); }
    static void store(unsigned char* p, __m128i v) noexcept { _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v); }
};

template <class T>
struct Sse2Int : Sse2Io, ScalarTail<T, 16> {
    static __m128i add(__m128i x, __m128i y) noexcept
    {
        if constexpr (sizeof(T) == 1) {
            return _mm_add_epi8(x, y);
        } else if constexpr (sizeof(T) == 2) {
            return _mm_add_epi16(x, y);
        } else if constexpr (sizeof(T) == 4) {
            return _mm_add_epi32(x, y);
        } else {
            return _mm_add_epi64(x, y);
        }
    }
};

struct Sse2F32 : ScalarTail<float, 16> {
    static __m128 load(const unsigned char* p) noexcept { return _mm_loadu_ps(reinterpret_cast<const float*>(p)); }
    static void store(unsigned char* p, __m128 v) noexcept { _mm_storeu_ps(reinterpret_cast<float*>(p), v); }
    static __m128 add(__m128 x, __m128 y) noexcept { return _mm_add_ps(x, y); }
};

struct Sse2F64 : ScalarTail<double, 16> {
    static __m128d load(const unsigned char* p) noexcept { return _mm_loadu_pd(reinterpret_cast<const double*>(p)); }
    static void store(unsigned char* p, __m128d v) noexcept { _mm_storeu_pd(reinterpret_cast<double*>(p), v); }
    static __m128d add(__m128d x, __m128d y) noexcept { return _mm_add_pd(x, y); }
};

} // namespace

SIMDSUM_DEFINE_KERNELS(sse2, Sse2Int, Sse2F32, Sse2F64)

// SPDX-License-Identifier: GPL-3.0-or-later
// 256-bit kernels, compiled with -mavx2.
#include <immintrin.h>

#include "kernel_loop.inc"

namespace {

struct Avx2Io {
    static __m256i load(const unsigned char* p) noexcept
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
    }
    static void store(unsigned char* p, __m256i v) noexcept { _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v); }
};

template <class T>
struct Avx2Int : Avx2Io, ScalarTail<T, 32> {
    static __m256i add(__m256i x, __m256i y) noexcept
    {
        if constexpr (sizeof(T) == 1) {
            return _mm256_add_epi8(x, y);
        } else if constexpr (sizeof(T) == 2) {
            return _mm256_add_epi16(x, y);
        } else if constexpr (sizeof(T) == 4) {
            return _mm256_add_epi32(x, y);
        } else {
            return _mm256_add_epi64(x, y);
        }
    }
};

struct Avx2F32 : ScalarTail<float, 32> {
    static __m256 load(const unsigned char* p) noexcept { return _mm256_loadu_ps(reinterpret_cast<const float*>(p)); }
    static void store(unsigned char* p, __m256 v) noexcept { _mm256_storeu_ps(reinterpret_cast<float*>(p), v); }
    static __m256 add(__m256 x, __m256 y) noexcept { return _mm256_add_ps(x, y); }
};

struct Avx2F64 : ScalarTail<double, 32> {
    static __m256d load(const unsigned char* p) noexcept { return _mm256_loadu_pd(reinterpret_cast<const double*>(p)); }
    static void store(unsigned char* p, __m256d v) noexcept { _mm256_storeu_pd(reinterpret_cast<double*>(p), v); }
    static __m256d add(__m256d x, __m256d y) noexcept { return _mm256_add_pd(x, y); }
};

} // namespace

SIMDSUM_DEFINE_KERNELS(avx2, Avx2Int, Avx2F32, Avx2F64)

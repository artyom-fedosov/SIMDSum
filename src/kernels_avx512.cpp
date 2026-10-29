// SPDX-License-Identifier: GPL-3.0-or-later
// 512-bit kernels, compiled with -mavx512f -mavx512bw. The tail is a single masked load/add/store.
#include <immintrin.h>

#include "kernel_loop.inc"

namespace {

// Mask with the low n bits set, n < number of lanes.
template <class Mask>
constexpr Mask low_bits(std::size_t n) noexcept
{
    return static_cast<Mask>((std::uint64_t{1} << n) - 1);
}

struct Avx512Io {
    static __m512i load(const unsigned char* p) noexcept { return _mm512_loadu_si512(p); }
    static void store(unsigned char* p, __m512i v) noexcept { _mm512_storeu_si512(p, v); }
    static void stream(unsigned char* p, __m512i v) noexcept { _mm512_stream_si512(reinterpret_cast<__m512i*>(p), v); }
    static void fence() noexcept { _mm_sfence(); }
};

template <class T>
struct Avx512Int : Avx512Io {
    using elem = T;
    static constexpr std::size_t lanes = 64 / sizeof(T);

    static __m512i add(__m512i x, __m512i y) noexcept
    {
        if constexpr (sizeof(T) == 1) {
            return _mm512_add_epi8(x, y);
        } else if constexpr (sizeof(T) == 2) {
            return _mm512_add_epi16(x, y);
        } else if constexpr (sizeof(T) == 4) {
            return _mm512_add_epi32(x, y);
        } else {
            return _mm512_add_epi64(x, y);
        }
    }

    static void tail(const unsigned char* a, const unsigned char* b, unsigned char* out, std::size_t n) noexcept
    {
        if constexpr (sizeof(T) == 1) {
            const auto m = low_bits<__mmask64>(n);
            _mm512_mask_storeu_epi8(out, m, _mm512_add_epi8(_mm512_maskz_loadu_epi8(m, a), _mm512_maskz_loadu_epi8(m, b)));
        } else if constexpr (sizeof(T) == 2) {
            const auto m = low_bits<__mmask32>(n);
            _mm512_mask_storeu_epi16(out, m,
                                     _mm512_add_epi16(_mm512_maskz_loadu_epi16(m, a), _mm512_maskz_loadu_epi16(m, b)));
        } else if constexpr (sizeof(T) == 4) {
            const auto m = low_bits<__mmask16>(n);
            _mm512_mask_storeu_epi32(out, m,
                                     _mm512_add_epi32(_mm512_maskz_loadu_epi32(m, a), _mm512_maskz_loadu_epi32(m, b)));
        } else {
            const auto m = low_bits<__mmask8>(n);
            _mm512_mask_storeu_epi64(out, m,
                                     _mm512_add_epi64(_mm512_maskz_loadu_epi64(m, a), _mm512_maskz_loadu_epi64(m, b)));
        }
    }
};

struct Avx512F32 {
    using elem = float;
    static constexpr std::size_t lanes = 16;
    static __m512 load(const unsigned char* p) noexcept { return _mm512_loadu_ps(p); }
    static void store(unsigned char* p, __m512 v) noexcept { _mm512_storeu_ps(p, v); }
    static __m512 add(__m512 x, __m512 y) noexcept { return _mm512_add_ps(x, y); }
    static void stream(unsigned char* p, __m512 v) noexcept { _mm512_stream_ps(reinterpret_cast<float*>(p), v); }
    static void fence() noexcept { _mm_sfence(); }

    static void tail(const unsigned char* a, const unsigned char* b, unsigned char* out, std::size_t n) noexcept
    {
        const auto m = low_bits<__mmask16>(n);
        _mm512_mask_storeu_ps(out, m, _mm512_add_ps(_mm512_maskz_loadu_ps(m, a), _mm512_maskz_loadu_ps(m, b)));
    }
};

struct Avx512F64 {
    using elem = double;
    static constexpr std::size_t lanes = 8;
    static __m512d load(const unsigned char* p) noexcept { return _mm512_loadu_pd(p); }
    static void store(unsigned char* p, __m512d v) noexcept { _mm512_storeu_pd(p, v); }
    static __m512d add(__m512d x, __m512d y) noexcept { return _mm512_add_pd(x, y); }
    static void stream(unsigned char* p, __m512d v) noexcept { _mm512_stream_pd(reinterpret_cast<double*>(p), v); }
    static void fence() noexcept { _mm_sfence(); }

    static void tail(const unsigned char* a, const unsigned char* b, unsigned char* out, std::size_t n) noexcept
    {
        const auto m = low_bits<__mmask8>(n);
        _mm512_mask_storeu_pd(out, m, _mm512_add_pd(_mm512_maskz_loadu_pd(m, a), _mm512_maskz_loadu_pd(m, b)));
    }
};

} // namespace

SIMDSUM_DEFINE_KERNELS(avx512, Avx512Int, Avx512F32, Avx512F64)

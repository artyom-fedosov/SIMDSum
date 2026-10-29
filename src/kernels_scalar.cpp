// SPDX-License-Identifier: GPL-3.0-or-later
// Portable fallback, compiled with the default target flags (and the only kernel set on non-x86 targets).
#include "kernel_loop.inc"

#define SIMDSUM_SCALAR_KERNEL(suffix, T)                                                                       \
    void simdsum_scalar_add_##suffix(const void* a, const void* b, void* out, std::size_t n) noexcept         \
    {                                                                                                          \
        scalar_loop<T>(static_cast<const unsigned char*>(a), static_cast<const unsigned char*>(b),             \
                       static_cast<unsigned char*>(out), n);                                                   \
    }

extern "C" {
SIMDSUM_SCALAR_KERNEL(u8, std::uint8_t)
SIMDSUM_SCALAR_KERNEL(u16, std::uint16_t)
SIMDSUM_SCALAR_KERNEL(u32, std::uint32_t)
SIMDSUM_SCALAR_KERNEL(u64, std::uint64_t)
SIMDSUM_SCALAR_KERNEL(f32, float)
SIMDSUM_SCALAR_KERNEL(f64, double)
}

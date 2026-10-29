// SPDX-License-Identifier: GPL-3.0-or-later
// Runtime instruction-set dispatch: picks the best kernel set once at load time (CPUID).
module;

#include <atomic>
#include <cstddef>
#include <string_view>

module simdsum;

using kernel_fn = void (*)(const void*, const void*, void*, std::size_t) noexcept;

#define SIMDSUM_DECLARE_KERNELS(isa)                                                                           \
    void simdsum_##isa##_add_u8(const void*, const void*, void*, std::size_t) noexcept;                        \
    void simdsum_##isa##_add_u16(const void*, const void*, void*, std::size_t) noexcept;                       \
    void simdsum_##isa##_add_u32(const void*, const void*, void*, std::size_t) noexcept;                       \
    void simdsum_##isa##_add_u64(const void*, const void*, void*, std::size_t) noexcept;                       \
    void simdsum_##isa##_add_f32(const void*, const void*, void*, std::size_t) noexcept;                       \
    void simdsum_##isa##_add_f64(const void*, const void*, void*, std::size_t) noexcept;

#define SIMDSUM_KERNEL_TABLE(isa_enum, isa)                                                                    \
    KernelTable{isa_enum,                 &simdsum_##isa##_add_u8,  &simdsum_##isa##_add_u16,                  \
                &simdsum_##isa##_add_u32, &simdsum_##isa##_add_u64, &simdsum_##isa##_add_f32,                  \
                &simdsum_##isa##_add_f64}

#if defined(__x86_64__) || defined(__i386__)
#define SIMDSUM_X86 1
#else
#define SIMDSUM_X86 0
#endif

extern "C" {
SIMDSUM_DECLARE_KERNELS(scalar)
#if SIMDSUM_X86
SIMDSUM_DECLARE_KERNELS(sse2)
SIMDSUM_DECLARE_KERNELS(avx2)
SIMDSUM_DECLARE_KERNELS(avx512)
#endif
}

namespace simdsum {
namespace {

struct KernelTable {
    Isa isa;
    kernel_fn u8;
    kernel_fn u16;
    kernel_fn u32;
    kernel_fn u64;
    kernel_fn f32;
    kernel_fn f64;
};

constexpr KernelTable scalar_table = SIMDSUM_KERNEL_TABLE(Isa::Scalar, scalar);
#if SIMDSUM_X86
constexpr KernelTable sse2_table = SIMDSUM_KERNEL_TABLE(Isa::SSE2, sse2);
constexpr KernelTable avx2_table = SIMDSUM_KERNEL_TABLE(Isa::AVX2, avx2);
constexpr KernelTable avx512_table = SIMDSUM_KERNEL_TABLE(Isa::AVX512, avx512);
#endif

const KernelTable* table_for(Isa isa) noexcept
{
    switch (isa) {
    case Isa::Scalar:
        return &scalar_table;
#if SIMDSUM_X86
    case Isa::SSE2:
        return &sse2_table;
    case Isa::AVX2:
        return &avx2_table;
    case Isa::AVX512:
        return &avx512_table;
#endif
    default:
        return nullptr;
    }
}

const KernelTable* best_table() noexcept
{
    for (auto isa : {Isa::AVX512, Isa::AVX2, Isa::SSE2}) {
        if (isa_supported(isa)) {
            return table_for(isa);
        }
    }
    return &scalar_table;
}

// Tables are constant-initialised, so relaxed ordering is enough to publish a pointer to one.
std::atomic<const KernelTable*> active_table{best_table()};

const KernelTable& active() noexcept
{
    return *active_table.load(std::memory_order_relaxed);
}

} // namespace

std::string_view version() noexcept
{
    return SIMDSUM_VERSION_STRING;
}

bool isa_supported(Isa isa) noexcept
{
#if SIMDSUM_X86
    // Safe to call before libgcc's own constructor has run (we are called during static initialisation).
    __builtin_cpu_init();
    switch (isa) {
    case Isa::Scalar:
        return true;
    case Isa::SSE2:
        return __builtin_cpu_supports("sse2");
    case Isa::AVX2:
        return __builtin_cpu_supports("avx2");
    case Isa::AVX512:
        return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw");
    }
    return false;
#else
    return isa == Isa::Scalar;
#endif
}

Isa active_isa() noexcept
{
    return active().isa;
}

bool force_isa(Isa isa) noexcept
{
    if (!isa_supported(isa)) {
        return false;
    }
    active_table.store(table_for(isa), std::memory_order_relaxed);
    return true;
}

} // namespace simdsum

extern "C" {
void simdsum_add_u8(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    simdsum::active().u8(a, b, out, n);
}
void simdsum_add_u16(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    simdsum::active().u16(a, b, out, n);
}
void simdsum_add_u32(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    simdsum::active().u32(a, b, out, n);
}
void simdsum_add_u64(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    simdsum::active().u64(a, b, out, n);
}
void simdsum_add_f32(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    simdsum::active().f32(a, b, out, n);
}
void simdsum_add_f64(const void* a, const void* b, void* out, std::size_t n) noexcept
{
    simdsum::active().f64(a, b, out, n);
}
}

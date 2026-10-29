// SPDX-License-Identifier: GPL-3.0-or-later
// Compares a plain loop (auto-vectorised for the default target) against every supported kernel set.
#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

import simdsum;

namespace {

template <class T>
struct Data {
    explicit Data(std::size_t n) : a(n, T(1)), b(n, T(2)), out(n) {}
    std::vector<T> a;
    std::vector<T> b;
    std::vector<T> out;
};

template <class T>
void set_counters(benchmark::State& state, std::size_t n)
{
    // Two reads and one write per element.
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * n * 3 * sizeof(T)));
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * n));
}

template <class T>
void BM_PlainLoop(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    Data<T> d(n);
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            d.out[i] = static_cast<T>(d.a[i] + d.b[i]);
        }
        benchmark::DoNotOptimize(d.out.data());
        benchmark::ClobberMemory();
    }
    set_counters<T>(state, n);
}

template <class T>
void BM_Simdsum(benchmark::State& state, simdsum::Isa isa)
{
    if (!simdsum::force_isa(isa)) {
        state.SkipWithMessage("instruction set not supported by this CPU");
        return;
    }
    const auto n = static_cast<std::size_t>(state.range(0));
    Data<T> d(n);
    for (auto _ : state) {
        simdsum::add(d.a, d.b, d.out);
        benchmark::DoNotOptimize(d.out.data());
        benchmark::ClobberMemory();
    }
    set_counters<T>(state, n);
}

template <class T>
void register_type(const std::string& type)
{
    auto sizes = [](benchmark::internal::Benchmark* b) {
        b->Arg(1 << 10)->Arg(1 << 16)->Arg(1 << 20)->Arg(1 << 22)->Arg(1 << 24);
    };
    sizes(benchmark::RegisterBenchmark("PlainLoop<" + type + ">", BM_PlainLoop<T>));
    for (auto isa : {simdsum::Isa::Scalar, simdsum::Isa::SSE2, simdsum::Isa::AVX2, simdsum::Isa::AVX512}) {
        if (simdsum::isa_supported(isa)) {
            sizes(benchmark::RegisterBenchmark("Simdsum<" + type + ">/" + std::string{simdsum::to_string(isa)},
                                               BM_Simdsum<T>, isa));
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    register_type<std::uint8_t>("u8");
    register_type<std::int32_t>("i32");
    register_type<float>("f32");
    register_type<double>("f64");
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

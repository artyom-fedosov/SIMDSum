# SIMDSum

Fast element-wise addition of two arrays — `out[i] = a[i] + b[i]` — as a C++20 **shared library**
with hand-written SIMD kernels (compiler intrinsics) and runtime CPU dispatch, consumed as a
**C++20 module** (`import simdsum;`).

- Works for **any summable `T`** (`a + b` convertible to `T`): integers, floats, `std::complex`,
  `std::string`, `std::chrono` durations, your own types…
- Integral types, `float`, `double` and `std::complex<float|double>` run on SIMD kernels in `libsimdsum.so`;
  everything else uses a generic loop instantiated in your code.
- Scalar / SSE2 / AVX2 / AVX-512 kernels in one portable binary; the best one is picked once at load time via CPUID.
- Streaming (non-temporal) stores for outputs ≥ 4 MiB, so huge arrays don't pay a read-for-ownership on every output line.

## Usage

```cpp
#include <vector>
import simdsum;

std::vector<float> a(1'000'000, 1.0f), b(1'000'000, 2.0f), out(1'000'000);

simdsum::add(a, b, out);            // into an existing buffer (fastest)
auto sum = simdsum::add(a, b);      // returns std::vector<float>
simdsum::add(a, b, a);              // in place is fine
```

Arguments can be any contiguous sized range: `std::vector`, `std::array`, `std::span`, C arrays.

Semantics:

- `a`, `b` and `out` must have the same length, or `std::invalid_argument` is thrown.
- `out` may be exactly `a` and/or `b`; partial overlap is not supported.
- Integer overflow wraps around (two's complement) for signed and unsigned types alike.
- Floating-point results are bit-identical to `a[i] + b[i]` (IEEE-754, including NaN/±inf/−0.0/denormals).
- `bool` is deliberately not routed through the byte kernels (`true + true` stays `true`).

Instruction-set control (mostly for tests and benchmarks):

```cpp
simdsum::active_isa();                    // Isa::AVX2, ...
simdsum::isa_supported(simdsum::Isa::AVX512);
simdsum::force_isa(simdsum::Isa::SSE2);   // false if the CPU can't run it
```

### From CMake

```cmake
find_package(simdsum REQUIRED)
target_link_libraries(app PRIVATE simdsum::simdsum)
```

The installed package ships the module interface; CMake builds its BMI for your compiler.
Module support requires CMake ≥ 3.28 and the Ninja generator.

## Building

Requirements: CMake ≥ 3.28, Ninja, and a compiler with C++20 module support (GCC ≥ 14 or Clang ≥ 18 with
`clang-scan-deps`). GoogleTest and Google Benchmark are fetched automatically.

```bash
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /usr/local
```

| Option | Default | |
|---|---|---|
| `SIMDSUM_BUILD_TESTS` | `ON` (top level) | GTest suite |
| `SIMDSUM_BUILD_BENCHMARKS` | `OFF` | Google Benchmark suite (`bench/simdsum_bench`) |
| `SIMDSUM_SANITIZE` | `OFF` | AddressSanitizer + UndefinedBehaviorSanitizer |

The tests run every kernel the CPU supports (unsupported instruction sets, e.g. AVX-512 on most laptops, are
reported as skipped) against a reference loop for every length 0–300, large and misaligned arrays,
in-place use, integer wrap-around and floating-point special values.

## Benchmarks

```bash
cmake -G Ninja -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DSIMDSUM_BUILD_BENCHMARKS=ON
cmake --build build-bench
taskset -c 0 ./build-bench/bench/simdsum_bench
```

`PlainLoop` is the obvious `out[i] = a[i] + b[i]` loop at `-O3` (auto-vectorised for the default target).
Medians on an Intel i7-1260P (AVX2), bytes moved = 3 × n × sizeof(T):

| Case | PlainLoop | simdsum (AVX2) |
|---|---|---|
| `uint8_t`, 1 Ki elements (L1) | 2.8 GiB/s | 106 GiB/s |
| `float`, 1 Ki elements (L1) | 74 GiB/s | 132 GiB/s |
| `float`, 16 Mi elements (DRAM) | 12.5 GiB/s | ~17 GiB/s |

The `uint8_t` plain loop barely vectorises because `uint8_t*` stores may alias the vector's own members.
For DRAM-sized arrays the kernels are memory-bound; the gain there comes from streaming stores.

## Design

```
src/simdsum.cppm      module interface: concepts, add() templates, type → kernel routing, C ABI declarations
src/dispatch.cpp      module implementation: CPUID detection, kernel tables, exported C entry points
src/kernel_loop.inc   shared loop skeleton (×4 unrolled, streaming-store path, tail handling)
src/kernels_*.cpp     one TU per instruction set, compiled with its own -m flags
```

- Signed and unsigned addition are bit-identical, so four integer kernels (8/16/32/64-bit) cover every integral type;
  `std::complex<F>` is added as `2n` values of `F`.
- The `.so` exports only a small C ABI (`simdsum_add_u8 … simdsum_add_f64`) plus the ISA control functions;
  all per-ISA kernels have hidden visibility.
- The kernel TUs are deliberately not module units: a BMI must be imported with the target flags it was built
  with, and keeping AVX code in internal-linkage functions in separate TUs guarantees no AVX instruction can
  leak into code that runs on older CPUs.
- AVX-512 handles the tail with one masked load/add/store; SSE2/AVX2 use a short scalar tail.

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).

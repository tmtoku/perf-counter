#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include "perf_counter.h"

template <std::int32_t NUM_KERNEL_LOOPS, typename KernelFunc>
std::uint64_t run_benchmark(const perf_counter* const cycle_counter, KernelFunc kernel_func)
{
    constexpr auto NUM_TRIALS = std::int32_t{10};
    constexpr auto NUM_WARMUPS = std::int32_t{3};
    auto min_cycles = std::numeric_limits<std::uint64_t>::max();

    for (std::int32_t i = 0; i < NUM_WARMUPS + NUM_TRIALS; ++i)
    {
        // Clear all YMM registers.
        __asm__ volatile("vzeroall\n\t" ::: "memory");

        const auto start = perf_counter_read(cycle_counter);

        for (std::int32_t j = 0; j < NUM_KERNEL_LOOPS; ++j)
        {
            kernel_func();
        }

        const auto end = perf_counter_read(cycle_counter);
        const auto cycles = (start < end) ? end - start : 0;

        if (i >= NUM_WARMUPS)
        {
            min_cycles = std::min(min_cycles, cycles);
        }
    }
    return min_cycles;
}

void print_benchmark_result(const std::string& benchmark_label, const std::uint64_t best_cycles,
                            const std::uint64_t total_instructions)
{
    const auto instructions_per_cycle = static_cast<double>(total_instructions) / static_cast<double>(best_cycles);
    const auto cycles_per_instruction = 1.0 / instructions_per_cycle;

    std::cout << benchmark_label << "\n";
    std::cout << "  Instructions : " << total_instructions << "\n";
    std::cout << "  Best Cycles  : " << best_cycles << "\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  CPI          : " << cycles_per_instruction << "\n";
    std::cout << "  IPC          : " << instructions_per_cycle << "\n\n";
}

// ymm0 = ymm0 + ymm1 * ymm1
// ymm0 = ymm0 + ymm1 * ymm1
// ymm0 = ymm0 + ymm1 * ymm1
// ...
#define LATENCY_KERNEL()                   \
    __asm__ volatile(                      \
        ".intel_syntax noprefix\n\t"       \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        "vfmadd231ps ymm0, ymm1, ymm1\n\t" \
        ".att_syntax prefix\n\t" ::        \
            : "ymm0")

// ymm1 = ymm1 + ymm0 * ymm0
// ymm2 = ymm2 + ymm0 * ymm0
// ymm3 = ymm3 + ymm0 * ymm0
// ...
#define THROUGHPUT_KERNEL()                 \
    __asm__ volatile(                       \
        ".intel_syntax noprefix\n\t"        \
        "vfmadd231ps ymm1, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm2, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm3, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm4, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm5, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm6, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm7, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm8, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm9, ymm0, ymm0\n\t"  \
        "vfmadd231ps ymm10, ymm0, ymm0\n\t" \
        "vfmadd231ps ymm11, ymm0, ymm0\n\t" \
        "vfmadd231ps ymm12, ymm0, ymm0\n\t" \
        ".att_syntax prefix\n\t" ::         \
            : "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12")

int main()
{
    constexpr auto NUM_KERNEL_LOOPS = std::int32_t{100'000};
    constexpr auto LATENCY_INSTS_PER_LOOP = std::uint64_t{12};
    constexpr auto THROUGHPUT_INSTS_PER_LOOP = std::uint64_t{12};
    constexpr auto LATENCY_TOTAL_INSTS = LATENCY_INSTS_PER_LOOP * NUM_KERNEL_LOOPS;
    constexpr auto THROUGHPUT_TOTAL_INSTS = THROUGHPUT_INSTS_PER_LOOP * NUM_KERNEL_LOOPS;

    auto cycle_counter = perf_counter_open_by_id(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1);
    if (!perf_counter_is_valid(&cycle_counter))
    {
        std::cerr << "Failed to open a cycle counter.\n";
        return EXIT_FAILURE;
    }
    perf_counter_enable(&cycle_counter);

    // Latency
    const auto latency_cycles = run_benchmark<NUM_KERNEL_LOOPS>(&cycle_counter, []() { LATENCY_KERNEL(); });
    print_benchmark_result("AVX2 vfmadd231ps - Latency", latency_cycles, LATENCY_TOTAL_INSTS);

    // Throughput
    const auto throughput_cycles = run_benchmark<NUM_KERNEL_LOOPS>(&cycle_counter, []() { THROUGHPUT_KERNEL(); });
    print_benchmark_result("AVX2 vfmadd231ps - Throughput", throughput_cycles, THROUGHPUT_TOTAL_INSTS);

    perf_counter_disable(&cycle_counter);
    perf_counter_close(&cycle_counter);

    return EXIT_SUCCESS;
}

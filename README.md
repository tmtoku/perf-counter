# perf-counter

A C11 library for low-overhead hardware performance monitoring on Linux.

## Overview

`perf-counter` offers a programmable alternative to the Linux `perf stat` command. Unlike `perf stat`, which measures an entire process, this library lets you define custom measurement intervals directly in your code. You can precisely profile specific loops, functions, or critical sections.

Internally, reading a counter value takes a single hardware instruction (`rdpmc`), avoiding the `read()` system calls. This enables high-precision measurements with minimal overhead.

## Requirements

- x86-64 CPU
- Linux kernel with `perf_event_open` support
- C11-capable compiler (GCC or Clang)
- CMake >= 3.10
- (Optional) [`libpfm4`](https://sourceforge.net/p/perfmon2/libpfm4/ci/master/tree/) for named event support

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To also build the example programs:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DPERF_COUNTER_BUILD_EXAMPLES=ON
cmake --build build
```

If `libpfm4` is installed, it will be detected automatically and named event support will be enabled.

## Usage

The typical call sequence is:

1. Open a counter for the desired event
2. Enable the counter
3. Read the counter value before and after the target code section
4. Disable and close the counter

```c
// 1. Open a counter for the desired event
struct perf_counter pc = perf_counter_open_by_id(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1);
if (!perf_counter_is_valid(&pc)) { /* handle error */ }

// 2. Enable the counter
perf_counter_enable(&pc);

// 3. Read the counter value before and after the target code section
uint64_t start = perf_counter_read(&pc);
// ... code to measure ...
uint64_t end = perf_counter_read(&pc);

// 4. Disable and close the counter
perf_counter_disable(&pc);
perf_counter_close(&pc);
```

`perf_counter_open_by_id` accepts any `type` / `config` combination documented in [`perf_event_open(2)`](https://man7.org/linux/man-pages/man2/perf_event_open.2.html). Set `group_fd` to `-1` to measure a single event. If measuring multiple events simultaneously, pass the file descriptor of an existing counter.

When built with `libpfm4`, you can open a counter by human-readable event name instead:

```c
struct perf_counter pc = perf_counter_open_by_name("perf::INSTRUCTIONS", -1);
```

For full control, pass a `perf_event_attr` struct to `perf_counter_open` directly:

```c
struct perf_counter pc = perf_counter_open(&attr, group_fd);
```

## Listing available events

When built with `libpfm4`, the `list_all_events` example prints all PMU events supported on the current CPU.

```sh
./build/examples/list_all_events             # print to stdout
./build/examples/list_all_events events.txt  # write to a file
```

Each line of the output contains an event name and a short description:

```
perf::L1-DCACHE-LOADS        # L1 cache load accesses
perf::L1-DCACHE-LOAD-MISSES  # L1 cache load misses
```

The event name (e.g., `perf::INSTRUCTIONS`) can be passed directly to `perf_counter_open_by_name`.

## License

See [LICENSE](LICENSE).

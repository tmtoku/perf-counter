# perf_counter

A C11 library for low-overhead hardware performance monitoring on Linux.

## Overview

`perf_counter` offers a programmable alternative to the Linux `perf stat` command. Unlike the `perf stat` command that measures entire processes, this library allows you to define **custom measurement intervals** directly in your code. You can precisely profile specific loops, functions, or critical sections.

Technically, `perf_counter` enables user-space applications to fetch Performance Monitoring Counter (PMC) values with a single hardware instruction. This avoids the latency of `read()` system calls, enabling high-precision measurements with minimal overhead.

## Key Features

- **Direct PMC Access**: Retrieves PMC values via the `rdpmc` instruction, bypassing system call overheads.
- **C and C++ Compatibility**: Implemented in standard C11 and fully compatible with C++11 and later.
- **Named Event Support**: Optionally integrates with [`libpfm4`](https://sourceforge.net/p/perfmon2/libpfm4/ci/master/tree/) to configure events using human-readable names (e.g., `RETIRED_INSTRUCTIONS`) instead of raw hexadecimal codes (e.g., `0xc0`).

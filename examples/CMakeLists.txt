add_executable(list_all_events list_all_events.c)
target_link_libraries(list_all_events PRIVATE perf_counter::perf_counter)

add_executable(perf_avx2_fma perf_avx2_fma.cpp)
target_link_libraries(perf_avx2_fma PRIVATE perf_counter::perf_counter)
target_compile_options(perf_avx2_fma PRIVATE -Wall -Wextra -Wpedantic -mavx2 -mfma -O3)

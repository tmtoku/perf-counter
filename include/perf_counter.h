#ifndef PERF_COUNTER_H
#define PERF_COUNTER_H

#include <linux/perf_event.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct perf_counter
    {
        int32_t fd;
        struct perf_event_mmap_page* metadata_page;
    };

    struct perf_counter perf_counter_open(const struct perf_event_attr* attr, int32_t group_fd);
    struct perf_counter perf_counter_open_by_id(uint32_t event_type, uint64_t event_config, int32_t group_fd);
#ifdef PERF_COUNTER_WITH_LIBPFM
    struct perf_counter perf_counter_open_by_name(const char* event_name, int32_t group_fd);
    void perf_counter_print_available_events(FILE* fp);
#endif
    void perf_counter_close(struct perf_counter* pc);
    int32_t perf_counter_enable(const struct perf_counter* pc);
    int32_t perf_counter_disable(const struct perf_counter* pc);

#define PERF_COUNTER_COMPILER_BARRIER() __asm__ volatile("" ::: "memory")
#define PERF_COUNTER_EXECUTION_FENCE() __asm__ volatile("lfence" ::: "memory")

    static inline uint64_t perf_counter_read(const struct perf_counter* const pc)
    {
        uint32_t low = 0;   // NOLINT(misc-const-correctness)
        uint32_t high = 0;  // NOLINT(misc-const-correctness)
        uint32_t seq;
        int64_t offset;

        const struct perf_event_mmap_page* const perf_metadata_page = pc->metadata_page;
        if (perf_metadata_page == NULL)
        {
            return 0;
        }

        PERF_COUNTER_EXECUTION_FENCE();

        do
        {
            seq = perf_metadata_page->lock;
            PERF_COUNTER_COMPILER_BARRIER();

            const uint32_t counter_id = perf_metadata_page->index;
            offset = perf_metadata_page->offset;

            if (counter_id == 0)
            {
                return 0;
            }

            __asm__ volatile("rdpmc" : "=a"(low), "=d"(high) : "c"(counter_id - 1));

            PERF_COUNTER_COMPILER_BARRIER();
        } while (perf_metadata_page->lock != seq);

        PERF_COUNTER_EXECUTION_FENCE();

        const uint64_t count = ((uint64_t)high << 32) | low;
        return count + (uint64_t)offset;
    }

    static inline bool perf_counter_is_valid(const struct perf_counter* const pc)
    {
        return pc->fd != -1 && pc->metadata_page != NULL;
    }

#undef PERF_COUNTER_COMPILER_BARRIER
#undef PERF_COUNTER_EXECUTION_FENCE

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PERF_COUNTER_H

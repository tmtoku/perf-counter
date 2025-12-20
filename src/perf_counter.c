#include "perf_counter.h"

#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef PERF_COUNTER_WITH_LIBPFM
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <stdatomic.h>
#endif

static int32_t sys_perf_event_open(const struct perf_event_attr* const attr, const int32_t group_fd)
{
    // pid = 0, cpu = -1: Measure the calling process/thread on any CPU.
    return (int32_t)syscall(SYS_perf_event_open, attr, 0, -1, group_fd, 0);
}

static int64_t get_page_size(void)
{
    const int64_t page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 1)
    {
        perror("sysconf(_SC_PAGESIZE)");
        return -1;
    }
    return page_size;
}

static struct perf_event_mmap_page* mmap_perf_metadata_page(const int32_t fd)
{
    const int64_t page_size = get_page_size();
    if (page_size < 0)
    {
        return NULL;
    }
    void* const mapped_area = mmap(NULL, (size_t)page_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped_area == MAP_FAILED)
    {
        perror("mmap");
        return NULL;
    }
    return (struct perf_event_mmap_page*)mapped_area;
}

struct perf_counter perf_counter_open(const struct perf_event_attr* const attr, int32_t group_fd)
{
    const int32_t fd = sys_perf_event_open(attr, group_fd);

    if (fd < 0)
    {
        return (struct perf_counter){.fd = -1, .metadata_page = NULL};
    }

    struct perf_event_mmap_page* const metadata_page = mmap_perf_metadata_page(fd);

    if (metadata_page == NULL)
    {
        close(fd);
        return (struct perf_counter){.fd = -1, .metadata_page = NULL};
    }

    return (struct perf_counter){.fd = fd, .metadata_page = metadata_page};
}

struct perf_counter perf_counter_open_by_id(const uint32_t event_type, const uint64_t event_config,
                                            const int32_t group_fd)
{
    struct perf_event_attr attr = {0};

    attr.size = sizeof(struct perf_event_attr);
    attr.type = event_type;
    attr.config = event_config;

    if (group_fd == -1)
    {
        attr.pinned = 1;  // Always schedule on CPU
    }

    attr.disabled = 1;        // Must be enabled manually.
    attr.exclude_kernel = 1;  // Don't count kernel
    attr.exclude_hv = 1;      // Don't count hypervisor

    return perf_counter_open(&attr, group_fd);
}

#ifdef PERF_COUNTER_WITH_LIBPFM
static bool ensure_libpfm_initialized(void)
{
    enum
    {
        STATE_UNINITIALIZED,
        STATE_IN_PROGRESS,
        STATE_SUCCESS,
        STATE_FAILED,
    };

    static atomic_int current_state = STATE_UNINITIALIZED;

    // Return immediately if initialization is already complete.
    const int32_t current_value = atomic_load(&current_state);
    if (current_value == STATE_SUCCESS)
    {
        return true;
    }
    if (current_value == STATE_FAILED)
    {
        return false;
    }

    // Try to claim initialization responsibility.
    int expected_state = STATE_UNINITIALIZED;
    if (atomic_compare_exchange_strong(&current_state, &expected_state, STATE_IN_PROGRESS))
    {
        const int32_t result_state = (pfm_initialize() == PFM_SUCCESS) ? STATE_SUCCESS : STATE_FAILED;
        atomic_store(&current_state, result_state);
        return result_state == STATE_SUCCESS;
    }

    // Wait for the other thread to complete initialization.
    while (atomic_load(&current_state) == STATE_IN_PROGRESS)
    {
    }

    return atomic_load(&current_state) == STATE_SUCCESS;
}

struct perf_counter perf_counter_open_by_name(const char* const event_name, const int32_t group_fd)
{
    if (!ensure_libpfm_initialized())
    {
        return (struct perf_counter){.fd = -1, .metadata_page = NULL};
    }

    struct perf_event_attr attr = {0};
    attr.size = sizeof(struct perf_event_attr);

    pfm_perf_encode_arg_t arg = {0};
    arg.attr = &attr;
    arg.size = sizeof(pfm_perf_encode_arg_t);

    // Translate the event name into raw hardware attributes.
    // PFM_PLM3: Monitor events in user space only.
    const int32_t ret = pfm_get_os_event_encoding(event_name, PFM_PLM3, PFM_OS_PERF_EVENT_EXT, &arg);
    if (ret != PFM_SUCCESS)
    {
        return (struct perf_counter){.fd = -1, .metadata_page = NULL};
    }

    if (group_fd == -1)
    {
        attr.pinned = 1;  // Always schedule on CPU
    }
    attr.disabled = 1;    // Must be enabled manually.

    return perf_counter_open(&attr, group_fd);
}
#endif

void perf_counter_close(struct perf_counter* const pc)
{
    if (pc->metadata_page != NULL)
    {
        const int64_t page_size = get_page_size();
        if (page_size > 0)
        {
            munmap(pc->metadata_page, (size_t)page_size);
        }
        pc->metadata_page = NULL;
    }
    if (pc->fd != -1)
    {
        close(pc->fd);
        pc->fd = -1;
    }
}

int32_t perf_counter_enable(const struct perf_counter* const pc)
{
    return ioctl(pc->fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
}

int32_t perf_counter_disable(const struct perf_counter* const pc)
{
    return ioctl(pc->fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
}

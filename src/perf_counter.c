#include "perf_counter.h"

#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

static void initialize_perf_event_attr(struct perf_event_attr* const attr, const uint32_t event_type,
                                       const uint64_t config, const int32_t group_fd)
{
    *attr = (struct perf_event_attr){0};

    attr->size = sizeof(struct perf_event_attr);
    attr->type = event_type;
    attr->config = config;

    if (group_fd == -1)
    {
        attr->pinned = 1;  // Always schedule on CPU
    }

    attr->disabled = 1;        // Must be enabled manually.
    attr->exclude_kernel = 1;  // Don't count kernel
    attr->exclude_hv = 1;      // Don't count hypervisor
}

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

struct perf_counter perf_counter_open_by_id(const uint32_t event_type, const uint64_t config, const int32_t group_fd)
{
    struct perf_event_attr attr;
    initialize_perf_event_attr(&attr, event_type, config, group_fd);

    const int32_t fd = sys_perf_event_open(&attr, group_fd);

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

/* SPDX-License-Identifier: MIT */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "morse_hid_uapi.h"

#define DEFAULT_DEVICE "/dev/morse_hid"
#define TEST_TIMEOUT_MS 250

static int failures;

static void pass(const char *message)
{
    printf("[PASS] %s\n", message);
}

static void fail(const char *message)
{
    fprintf(stderr, "[FAIL] %s: %s\n", message,
            errno ? strerror(errno) : "unexpected result");
    failures++;
}

static int validate_event(const struct morse_hid_event *event)
{
    if (event->type < MORSE_HID_EVENT_DOT ||
        event->type > MORSE_HID_EVENT_DEVICE_DISCONNECTED)
        return -1;

    if (event->sequence_len > MORSE_HID_MAX_SEQUENCE_LEN)
        return -1;

    if (event->reserved != 0)
        return -1;

    if (event->sequence[event->sequence_len] != '\0')
        return -1;

    return 0;
}

static void print_event(const struct morse_hid_event *event)
{
    printf("  id=%llu timestamp=%llu type=%u flags=0x%x "
           "value=%d ascii=%u sequence=\"%s\"\n",
           (unsigned long long)event->event_id,
           (unsigned long long)event->timestamp_ns,
           event->type,
           event->flags,
           event->value,
           event->ascii,
           (const char *)event->sequence);
}

static int drain_events(int fd)
{
    struct morse_hid_event events[16];
    int total = 0;

    for (;;) {
        ssize_t bytes = read(fd, events, sizeof(events));

        if (bytes < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return total;

            perror("read");
            return -1;
        }

        if (bytes == 0)
            return total;

        if ((size_t)bytes % sizeof(events[0]) != 0) {
            fprintf(stderr, "[FAIL] Partial event record received\n");
            return -1;
        }

        int count = (int)((size_t)bytes / sizeof(events[0]));

        for (int i = 0; i < count; i++) {
            if (validate_event(&events[i]) != 0) {
                fprintf(stderr, "[FAIL] Invalid event record\n");
                return -1;
            }

            print_event(&events[i]);
        }

        total += count;
    }
}

static int run_self_test(const char *path)
{
    struct morse_hid_info info = {0};
    struct morse_hid_stats stats = {0};
    struct morse_hid_event event;
    struct pollfd pfd;
    unsigned char small_buffer[sizeof(event)];
    int fd;

    printf("\nMorseMouse character-device self-test\n");
    printf("Device: %s\n\n", path);

    fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Test 1: GET_INFO */
    if (ioctl(fd, MORSE_HID_IOC_GET_INFO, &info) < 0) {
        fail("GET_INFO ioctl");
        goto out;
    }

    if (info.abi_version != MORSE_HID_ABI_VERSION ||
        info.struct_size != sizeof(info) ||
        info.event_size != sizeof(event) ||
        info.diagnostic_queue_capacity != MORSE_HID_DIAG_QUEUE_CAPACITY ||
        info.max_sequence_len != MORSE_HID_MAX_SEQUENCE_LEN) {
        fail("GET_INFO returned incorrect ABI information");
        goto out;
    }

    printf("ABI version: %u\n", info.abi_version);
    printf("Event size: %u bytes\n", info.event_size);
    printf("Diagnostic capacity: %u\n", info.diagnostic_queue_capacity);
    printf("Feature flags: 0x%x\n", info.feature_flags);
    pass("GET_INFO and ABI validation");

    /* Test 2: GET_STATS */
    if (ioctl(fd, MORSE_HID_IOC_GET_STATS, &stats) < 0) {
        fail("GET_STATS ioctl");
        goto out;
    }

    if (stats.abi_version != MORSE_HID_ABI_VERSION ||
        stats.struct_size != sizeof(stats)) {
        fail("GET_STATS returned incorrect structure information");
        goto out;
    }

    printf("Received: %llu, processed: %llu, dropped: %llu\n",
           (unsigned long long)stats.input_events_received,
           (unsigned long long)stats.input_events_processed,
           (unsigned long long)stats.input_events_dropped);

    printf("Input queue: %u, diagnostic queue: %u\n",
           stats.input_queue_depth,
           stats.diagnostic_queue_depth);

    pass("GET_STATS and statistics structure");

    /* Test 3: Exclusive reader */
    int second_fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);

    if (second_fd >= 0) {
        close(second_fd);
        fail("Exclusive open was not enforced");
        goto out;
    }

    if (errno != EBUSY) {
        fail("Second open returned an unexpected error");
        goto out;
    }

    pass("Exclusive reader enforcement");

    /* Test 4: Unsupported ioctl */
    errno = 0;

    if (ioctl(fd, _IO(MORSE_HID_IOC_TYPE, 0x7f), 0) != -1 ||
        errno != ENOTTY) {
        fail("Unsupported ioctl should return ENOTTY");
        goto out;
    }

    pass("Unsupported ioctl returns ENOTTY");

    /* Test 5: Invalid ioctl pointer */
    errno = 0;

    if (ioctl(fd, MORSE_HID_IOC_GET_INFO,
              (void *)(uintptr_t)1) != -1 || errno != EFAULT) {
        fail("Invalid ioctl pointer should return EFAULT");
        goto out;
    }

    pass("Invalid ioctl pointer returns EFAULT");

    /* Test 6: Invalid read size */
    errno = 0;

    if (read(fd, small_buffer, sizeof(event) - 1) != -1 ||
        errno != EINVAL) {
        fail("Invalid read size should return EINVAL");
        goto out;
    }

    pass("Invalid read size returns EINVAL");

    /* Test 7: Invalid read pointer, when a record is available */
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ready = poll(&pfd, 1, 0);

    if (ready < 0) {
        fail("Initial poll");
        goto out;
    }

    if (ready > 0 && (pfd.revents & POLLIN)) {
        long page_size = sysconf(_SC_PAGESIZE);
        void *bad_page;
        ssize_t invalid_read;
        int invalid_errno;

        if (page_size <= 0) {
            fail("Unable to determine system page size");
            goto out;
        }

        bad_page = mmap(NULL, (size_t)page_size, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (bad_page == MAP_FAILED) {
            fail("Unable to allocate inaccessible test page");
            goto out;
        }

        errno = 0;
        invalid_read = read(fd, bad_page, sizeof(event));
        invalid_errno = errno;

        munmap(bad_page, (size_t)page_size);

        if (invalid_read != -1 || invalid_errno != EFAULT) {
            errno = invalid_errno;
            fail("Invalid read pointer should return EFAULT");
            goto out;
        }

        ssize_t bytes = read(fd, &event, sizeof(event));

        if (bytes != (ssize_t)sizeof(event) ||
            validate_event(&event) != 0) {
            fail("Record was not retained after EFAULT");
            goto out;
        }

        pass("Invalid read pointer returns EFAULT without consuming event");
        print_event(&event);
    } else {
        printf("[SKIP] Invalid read-pointer test: no event available\n");
    }

    /* Test 8: Drain existing diagnostic records */
    int drained = drain_events(fd);

    if (drained < 0) {
        failures++;
        goto out;
    }

    printf("Drained %d diagnostic records\n", drained);
    pass("Diagnostic event records validated");

    /* Test 9: Empty non-blocking read */
    errno = 0;

    if (read(fd, &event, sizeof(event)) != -1 ||
        errno != EAGAIN) {
        fail("Empty non-blocking read should return EAGAIN");
        goto out;
    }

    pass("Empty non-blocking read returns EAGAIN");

    /* Test 10: Poll timeout or readiness */
    pfd.revents = 0;
    ready = poll(&pfd, 1, TEST_TIMEOUT_MS);

    if (ready < 0) {
        fail("poll");
        goto out;
    }

    if (ready == 0) {
        pass("poll timeout on empty diagnostic queue");
    } else if (pfd.revents & POLLIN) {
        pass("poll reports diagnostic data as readable");
        drained = drain_events(fd);

        if (drained < 0) {
            failures++;
            goto out;
        }
    } else {
        fail("poll returned unexpected event flags");
        goto out;
    }

    /* Test 11: Final statistics */
    memset(&stats, 0, sizeof(stats));

    if (ioctl(fd, MORSE_HID_IOC_GET_STATS, &stats) < 0) {
        fail("Final GET_STATS ioctl");
        goto out;
    }

    pass("Final GET_STATS ioctl");

out:
    close(fd);

    printf("\n====================================\n");

    if (failures) {
        printf("RESULT: FAILED (%d failures)\n", failures);
        return 1;
    }

    printf("RESULT: ALL EXECUTED TESTS PASSED\n");
    return 0;
}

static int wait_for_event(const char *path, int watch)
{
    struct morse_hid_event events[16];
    int fd = open(path, O_RDONLY | O_CLOEXEC);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Waiting for MorseMouse diagnostic events.\n");
    printf("Generate mouse input to produce events.\n");
    printf("Press Ctrl+C to stop watch mode.\n\n");

    do {
        size_t count = watch ? 16 : 1;
        ssize_t bytes = read(fd, events, count * sizeof(events[0]));

        if (bytes < 0) {
            if (errno == EINTR && watch)
                continue;

            perror("read");
            close(fd);
            return 1;
        }

        if (bytes == 0)
            break;

        if ((size_t)bytes % sizeof(events[0]) != 0) {
            fprintf(stderr, "Invalid event-record length\n");
            close(fd);
            return 1;
        }

        int records = (int)((size_t)bytes / sizeof(events[0]));

        for (int i = 0; i < records; i++) {
            if (validate_event(&events[i]) != 0) {
                fprintf(stderr, "Invalid event record\n");
                close(fd);
                return 1;
            }

            print_event(&events[i]);
        }
    } while (watch);

    close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "--self-test";
    const char *path = argc > 2 ? argv[2] : DEFAULT_DEVICE;

    if (strcmp(mode, "--self-test") == 0)
        return run_self_test(path);

    if (strcmp(mode, "--wait") == 0)
        return wait_for_event(path, 0);

    if (strcmp(mode, "--watch") == 0)
        return wait_for_event(path, 1);

    fprintf(stderr,
            "Usage: %s [--self-test|--wait|--watch] [device-path]\n",
            argv[0]);

    return 2;
}

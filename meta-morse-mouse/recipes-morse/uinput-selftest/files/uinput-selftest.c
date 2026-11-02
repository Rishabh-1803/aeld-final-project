#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_NAME "MorseMouse-uinput-selftest"
#define EVENT_SCAN_ATTEMPTS 50
#define EVENT_SCAN_DELAY_MS 100
#define EVENT_READ_TIMEOUT_MS 3000

static int write_input_event(int fd, unsigned short type,
                             unsigned short code, int value)
{
    struct input_event event;
    ssize_t written;

    memset(&event, 0, sizeof(event));
    if (gettimeofday(&event.time, NULL) < 0) {
        perror("gettimeofday");
        return -1;
    }

    event.type = type;
    event.code = code;
    event.value = value;

    do {
        written = write(fd, &event, sizeof(event));
    } while (written < 0 && errno == EINTR);

    if (written < 0) {
        perror("write /dev/uinput");
        return -1;
    }
    if ((size_t)written != sizeof(event)) {
        fprintf(stderr, "Short write to /dev/uinput: %zd of %zu bytes\n",
                written, sizeof(event));
        return -1;
    }

    return 0;
}

static int find_event_device(char *path, size_t path_size)
{
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = EVENT_SCAN_DELAY_MS * 1000000L
    };

    for (int attempt = 0; attempt < EVENT_SCAN_ATTEMPTS; ++attempt) {
        DIR *directory = opendir("/sys/class/input");
        struct dirent *entry;

        if (directory != NULL) {
            while ((entry = readdir(directory)) != NULL) {
                char name_path[256];
                char device_name[128];
                FILE *name_file;
                size_t length;

                if (strncmp(entry->d_name, "event", 5) != 0 ||
                    !isdigit((unsigned char)entry->d_name[5])) {
                    continue;
                }

                if (snprintf(name_path, sizeof(name_path),
                             "/sys/class/input/%s/device/name", entry->d_name)
                    >= (int)sizeof(name_path)) {
                    continue;
                }

                name_file = fopen(name_path, "r");
                if (name_file == NULL) {
                    continue;
                }

                if (fgets(device_name, sizeof(device_name), name_file) == NULL) {
                    fclose(name_file);
                    continue;
                }
                fclose(name_file);

                length = strcspn(device_name, "\r\n");
                device_name[length] = '\0';
                if (strcmp(device_name, DEVICE_NAME) != 0) {
                    continue;
                }

                if (snprintf(path, path_size, "/dev/input/%s",
                             entry->d_name) >= (int)path_size) {
                    closedir(directory);
                    errno = ENAMETOOLONG;
                    return -1;
                }

                if (access(path, R_OK) == 0) {
                    closedir(directory);
                    return 0;
                }
            }
            closedir(directory);
        }

        if (attempt + 1 < EVENT_SCAN_ATTEMPTS) {
            while (nanosleep(&delay, &delay) < 0 && errno == EINTR) {
                /* Resume the remaining delay after a signal. */
            }
            delay.tv_sec = 0;
            delay.tv_nsec = EVENT_SCAN_DELAY_MS * 1000000L;
        }
    }

    errno = ETIMEDOUT;
    return -1;
}

static int verify_key_events(int event_fd)
{
    bool saw_press = false;
    bool saw_release = false;
    struct pollfd descriptor = {.fd = event_fd, .events = POLLIN};

    while (!saw_press || !saw_release) {
        struct input_event events[16];
        int poll_result;
        ssize_t bytes;
        size_t count;

        do {
            poll_result = poll(&descriptor, 1, EVENT_READ_TIMEOUT_MS);
        } while (poll_result < 0 && errno == EINTR);

        if (poll_result < 0) {
            perror("poll evdev device");
            return -1;
        }
        if (poll_result == 0) {
            fprintf(stderr,
                    "Timed out waiting for KEY_A press/release events\n");
            return -1;
        }

        do {
            bytes = read(event_fd, events, sizeof(events));
        } while (bytes < 0 && errno == EINTR);

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            perror("read evdev device");
            return -1;
        }
        if (bytes == 0 || (size_t)bytes % sizeof(events[0]) != 0) {
            fprintf(stderr, "Unexpected evdev read size: %zd bytes\n", bytes);
            return -1;
        }

        count = (size_t)bytes / sizeof(events[0]);
        for (size_t index = 0; index < count; ++index) {
            const struct input_event *event = &events[index];

            if (event->type == EV_SYN && event->code == SYN_REPORT) {
                puts("  EV_SYN / SYN_REPORT");
                continue;
            }

            if (event->type != EV_KEY || event->code != KEY_A) {
                continue;
            }

            printf("  EV_KEY / KEY_A (%u), value %d\n",
                   event->code, event->value);
            if (event->value == 1) {
                saw_press = true;
            } else if (event->value == 0) {
                saw_release = true;
            } else {
                fprintf(stderr, "Unexpected KEY_A event value: %d\n",
                        event->value);
                return -1;
            }
        }
    }

    return 0;
}

int main(void)
{
    int uinput_fd = -1;
    int event_fd = -1;
    bool device_created = false;
    char event_path[128];
    struct uinput_setup setup;
    int result = EXIT_FAILURE;

    puts("MorseMouse uinput self-test");
    printf("Creating virtual keyboard: %s\n", DEVICE_NAME);

    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (uinput_fd < 0) {
        perror("open /dev/uinput (run as root or grant uinput access)");
        goto cleanup;
    }

    if (ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY) < 0) {
        perror("ioctl UI_SET_EVBIT EV_KEY");
        goto cleanup;
    }
    if (ioctl(uinput_fd, UI_SET_KEYBIT, KEY_A) < 0) {
        perror("ioctl UI_SET_KEYBIT KEY_A");
        goto cleanup;
    }

    memset(&setup, 0, sizeof(setup));
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x4d4d;
    setup.id.product = 0x0001;
    setup.id.version = 1;
    (void)snprintf(setup.name, sizeof(setup.name), "%s", DEVICE_NAME);

    if (ioctl(uinput_fd, UI_DEV_SETUP, &setup) < 0) {
        perror("ioctl UI_DEV_SETUP");
        goto cleanup;
    }
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        perror("ioctl UI_DEV_CREATE");
        goto cleanup;
    }
    device_created = true;

    if (find_event_device(event_path, sizeof(event_path)) < 0) {
        perror("locate virtual keyboard evdev node");
        goto cleanup;
    }

    printf("Virtual keyboard registered as %s\n", event_path);
    event_fd = open(event_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (event_fd < 0) {
        perror("open virtual keyboard evdev node");
        goto cleanup;
    }

    puts("Injecting KEY_A press, sync, release, sync...");
    if (write_input_event(uinput_fd, EV_KEY, KEY_A, 1) < 0 ||
        write_input_event(uinput_fd, EV_SYN, SYN_REPORT, 0) < 0 ||
        write_input_event(uinput_fd, EV_KEY, KEY_A, 0) < 0 ||
        write_input_event(uinput_fd, EV_SYN, SYN_REPORT, 0) < 0) {
        goto cleanup;
    }

    puts("Verifying events through evdev:");
    if (verify_key_events(event_fd) < 0) {
        goto cleanup;
    }

    puts("PASS: virtual keyboard creation and KEY_A press/release readback verified.");
    result = EXIT_SUCCESS;

cleanup:
    if (event_fd >= 0) {
        close(event_fd);
    }
    if (uinput_fd >= 0) {
        if (device_created && ioctl(uinput_fd, UI_DEV_DESTROY) < 0) {
            perror("ioctl UI_DEV_DESTROY");
            result = EXIT_FAILURE;
        }
        close(uinput_fd);
    }
    return result;
}

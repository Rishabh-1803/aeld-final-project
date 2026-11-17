/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "morse_hid_uapi.h"

static volatile sig_atomic_t stopping;
static void stop(int sig) { (void)sig; stopping = 1; }
static unsigned long long milliseconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (unsigned long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
int main(int argc, char **argv) {
    char device[256] = "/dev/morse_hid", line[512];
    unsigned interval = 1000;
    const char *config = argc > 1 ? argv[1] : "/etc/morsemouse/morsed.conf";
    FILE *c = fopen(config, "r");
    if (!c) { perror(config); return 2; }
    while (fgets(line, sizeof(line), c)) {
        char key[64], value[256], extra;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (!strchr(line, '\n') && !feof(c)) {
            fprintf(stderr, "configuration line too long\n"); fclose(c); return 2;
        }
        if (sscanf(line, " %63[^=]=%255s %c", key, value, &extra) != 2) {
            fprintf(stderr, "invalid configuration line\n"); fclose(c); return 2;
        }
        if (!strcmp(key, "device")) {
            if (value[0] != '/') { fclose(c); return 2; }
            strcpy(device, value);
        } else if (!strcmp(key, "interval_ms")) {
            char *end; unsigned long n = strtoul(value, &end, 10);
            if (strspn(value, "0123456789") != strlen(value) || *end || n < 100 || n > 10000) { fclose(c); return 2; }
            interval = (unsigned)n;
        } else if (!strcmp(key, "power_mode")) {
            if (strcmp(value, "disabled") && strcmp(value, "dry-run")) {
                fprintf(stderr, "power execution is unavailable\n"); fclose(c); return 2;
            }
        } else { fprintf(stderr, "unknown configuration key: %s\n", key); fclose(c); return 2; }
    }
    if (ferror(c)) { fclose(c); return 2; }
    fclose(c);
    if (argc > 2 && !strcmp(argv[2], "--check-config")) return 0;
    if (argc > 2) return 2;
    if (mkdir("/run/morsemouse", 0755) && errno != EEXIST) { perror("mkdir"); return 1; }
    int lock = open("/run/morsemouse/daemon.lock", O_CREAT|O_RDWR|O_CLOEXEC, 0600);
    struct flock fl = {.l_type=F_WRLCK, .l_whence=SEEK_SET};
    if (lock < 0 || fcntl(lock, F_SETLK, &fl)) { perror("daemon lock"); return 1; }
    signal(SIGTERM, stop); signal(SIGINT, stop);
    int fd = -1;
    unsigned long long updated = 0, last_id = 0, losses = 0;
    struct morse_hid_stats stats = {0};
    struct morse_hid_event last = {0};
    const char *status = "unavailable";
    do {
        if (fd < 0 && !stopping) {
            fd = open(device, O_RDONLY|O_NONBLOCK|O_CLOEXEC);
            if (fd >= 0) {
                struct morse_hid_info info = {0};
                if (ioctl(fd, MORSE_HID_IOC_GET_INFO, &info) ||
                    info.abi_version != MORSE_HID_ABI_VERSION || info.event_size != sizeof(last) ||
                    info.struct_size != sizeof(info)) {
                    fprintf(stderr, "unsupported driver ABI\n"); close(fd); fd = -1;
                    status = "abi-error";
                } else { status = "ready"; last_id = 0; memset(&last,0,sizeof(last)); }
            } else status = errno == EBUSY ? "busy" : "unavailable";
        }
        if (fd >= 0 && !stopping) {
            struct pollfd p = {.fd=fd, .events=POLLIN};
            int rc = poll(&p, 1, (int)interval);
            if (rc > 0 && (p.revents & POLLIN)) {
                struct morse_hid_event ev[16];
                ssize_t n = read(fd, ev, sizeof(ev));
                if (n > 0 && (size_t)n % sizeof(ev[0]) == 0) {
                    for (size_t i=0; i<(size_t)n/sizeof(ev[0]); i++) {
                        if (ev[i].sequence_len > MORSE_HID_MAX_SEQUENCE_LEN ||
                            ev[i].sequence[ev[i].sequence_len] || ev[i].reserved ||
                            ev[i].type < 1 || ev[i].type > MORSE_HID_EVENT_DEVICE_DISCONNECTED) {
                            status="invalid-record"; close(fd); fd=-1; break;
                        }
                        if ((last_id && ev[i].event_id != last_id+1) ||
                            (ev[i].flags & MORSE_HID_EVENT_F_STREAM_GAP)) losses++;
                        last=ev[i]; last_id=last.event_id;
                    }
                } else if (n >= 0 || (errno != EAGAIN && errno != EINTR)) {
                    close(fd); fd=-1; status="unavailable";
                }
            }
            if (rc > 0 && (p.revents & (POLLERR|POLLHUP|POLLNVAL)) && fd >= 0) {
                close(fd); fd=-1; status="unavailable";
            }
            if (fd >= 0 && ioctl(fd, MORSE_HID_IOC_GET_STATS, &stats)) {
                close(fd); fd=-1; status="unavailable";
            }
        } else if (!stopping) {
            struct timespec wait = {.tv_sec=interval/1000, .tv_nsec=(interval%1000)*1000000L};
            nanosleep(&wait, NULL);
        }
        unsigned long long now = milliseconds();
        if (now-updated >= interval || stopping) {
            FILE *out=fopen("/run/morsemouse/status.tmp", "w");
            if (!out) { perror("snapshot"); break; }
            fprintf(out,"version=1\nmonotonic_ms=%llu\ninterval_ms=%u\ndaemon=%s\ndriver=%s\npower=disabled\n",
                    now, interval, stopping ? "stopped":"running",status);
            fprintf(out,"connected=%u\nreceived=%llu\nprocessed=%llu\ndropped=%llu\ninput_overflows=%llu\ndiagnostic_dropped=%llu\ninput_queue=%u\ndiagnostic_queue=%u\n",
                    !!(stats.flags & MORSE_HID_STATS_F_INPUT_CONNECTED),
                    (unsigned long long)stats.input_events_received,(unsigned long long)stats.input_events_processed,
                    (unsigned long long)stats.input_events_dropped,(unsigned long long)stats.input_queue_overflows,
                    (unsigned long long)stats.diagnostic_events_dropped,stats.input_queue_depth,stats.diagnostic_queue_depth);
            fprintf(out,"event_id=%llu\nevent_type=%u\nascii=%u\nsequence=%.*s\nloss_notifications=%llu\n",
                    last_id,last.type,last.ascii,(int)last.sequence_len,last.sequence,losses);
            int failed=ferror(out);
            if (fclose(out)) failed=1;
            if (failed || rename("/run/morsemouse/status.tmp","/run/morsemouse/status")) {
                perror("publish snapshot"); if(fd>=0) close(fd); close(lock); return 1;
            }
            updated=now;
        }
    } while (!stopping);
    if(fd>=0) close(fd);
    close(lock);
    return stopping ? 0 : 1;
}

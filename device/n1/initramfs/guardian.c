// SPDX-License-Identifier: MIT
// RAM-root handoff guard: no writable partitions and no dependencies on old /.
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/watchdog.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int blocks_fd, devices_fd;

static bool virtual_device(const char *name) {
    return (!strncmp(name, "ram", 3) && name[3] >= '0' && name[3] <= '9') ||
           (!strncmp(name, "zram", 4) && name[4] >= '0' && name[4] <= '9') ||
           (!strncmp(name, "loop", 4) && name[4] >= '0' && name[4] <= '9');
}

static bool protect_storage(bool check_only) {
    int copy = dup(blocks_fd);
    if (copy < 0)
        return false;
    DIR *directory = fdopendir(copy);
    if (!directory) {
        close(copy);
        return false;
    }
    rewinddir(directory);
    bool found_emmc = false, ok = true;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(directory))) {
        const char *name = entry->d_name;
        if (name[0] == '.' || virtual_device(name))
            continue;
        int fd = openat(devices_fd, name, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        struct stat info;
        int readonly = 0;
        if (fd < 0 || fstat(fd, &info) || !S_ISBLK(info.st_mode) ||
            ioctl(fd, BLKROGET, &readonly)) {
            if (fd >= 0)
                close(fd);
            ok = false;
            break;
        }
        if (!readonly && !check_only) {
            int one = 1;
            if (ioctl(fd, BLKROSET, &one) || ioctl(fd, BLKROGET, &readonly))
                readonly = 0;
        }
        close(fd);
        if (!readonly) {
            ok = false;
            break;
        }
        if (!strcmp(name, "mmcblk0"))
            found_emmc = true;
        errno = 0;
    }
    if (errno)
        ok = false;
    closedir(directory);
    return ok && found_emmc;
}

int main(int argc, char **argv) {
    const bool check_only = argc == 2 && !strcmp(argv[1], "--check-only");
    if (argc != 1 && !check_only)
        return 2;
    blocks_fd = open("/sys/class/block", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    devices_fd = open("/dev", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (blocks_fd < 0 || devices_fd < 0 || !protect_storage(check_only))
        return 1;
    if (check_only) {
        puts("storage-readonly=PASS");
        return 0;
    }
    int run_fd = open("/run", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (run_fd < 0)
        return 1;
    // Saved mount descriptors stay usable after init moves /sys and /dev.
    // Never close the watchdog with its magic character; failure means reset.
    int watchdog_fd = openat(devices_fd, "watchdog0", O_WRONLY | O_CLOEXEC);
    int timeout = 30;
    if (watchdog_fd < 0 || ioctl(watchdog_fd, WDIOC_SETTIMEOUT, &timeout) ||
        timeout < 10 || timeout > 60 || write(watchdog_fd, "\0", 1) != 1)
        return 1;
    signal(SIGTERM, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    int ready = open("/run/n1-guardian-ready", O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (ready < 0)
        return 1;
    if (dprintf(ready, "%ld\n", (long)getpid()) < 0 || close(ready))
        return 1;
    unsigned tick = 0;
    time_t handoff_started = 0;
    for (;;) {
        if (!protect_storage(false)) {
            // Retain the open descriptor and stop feeding after any failure.
            for (;;)
                pause();
        }
        if (!faccessat(run_fd, "n1-handoff-start", F_OK, 0)) {
            struct timespec now;
            if (clock_gettime(CLOCK_MONOTONIC, &now))
                for (;;)
                    pause();
            if (!handoff_started)
                handoff_started = now.tv_sec;
            if (now.tv_sec - handoff_started >= 120 &&
                faccessat(run_fd, "n1-systemd-confirmed", F_OK, 0))
                for (;;)
                    pause();
        }
        if (++tick == 25) {
            if (write(watchdog_fd, "\0", 1) != 1)
                for (;;)
                    pause();
            tick = 0;
        }
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 200000000};
        while (nanosleep(&delay, &delay) && errno == EINTR) {}
    }
}

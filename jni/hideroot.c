#include <stdio.h>
#include <stdlib.h>
#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
#include <android/log.h>
#include <sys/system_properties.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <sched.h>

#include "log.h"
#include "utils/vector.h"

int switch_mnt_ns(int pid) {
    char mnt[32];
    snprintf(mnt, sizeof(mnt), "/proc/%d/ns/mnt", pid);
    if(access(mnt, R_OK) == -1) {
        DEBUG_PRINT("[switch_mnt_ns] %s can't access!", mnt);
        return 1;
    }

    int fd, ret;
    fd = open(mnt, O_RDONLY);
    if (fd < 0) {
        DEBUG_PRINT("[switch_mnt_ns] open %s failed!", mnt);
        return 1;
    }
    ret = setns(fd, 0);
    close(fd);
    return ret;
}

ssize_t readline(char **lineptr, size_t *n, FILE *stream)
{
    int ch;
    char *line = *lineptr;
    size_t alloced = *n;
    size_t len = 0;

    do {
        ch = fgetc(stream);
        if (ch == EOF)
            break;
        if (len + 1 >= alloced) {
            alloced += alloced/4 + 64;
            line = realloc(line, alloced);
        }
        line[len++] = ch;
    } while (ch != '\n');

    if (len == 0)
        return -1;

    line[len] = '\0';
    *lineptr = line;
    *n = alloced;
    return len;
}

int file_to_vector(const char* filename, struct vector *v) {
    if (access(filename, R_OK) != 0)
        return 1;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        return 1;
    while ((read = readline(&line, &len, fp)) != -1) {
        // Remove end newline
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';
        vec_push_back(v, line);
        line = NULL;
        len = 0;
    }
    fclose(fp);
    return 0;
}

static void lazy_unmount(const char* mountpoint) {
    if (umount2(mountpoint, MNT_DETACH) != -1) {
        LOGD("[lazy_unmount]: Unmounted (%s)\n", mountpoint);
    }
    else {
        LOGD("[lazy_unmount]: Unmount %s failed\n", mountpoint);
    }
}

int main(int argc, char** argv) {
    DEBUG_PRINT("hideroot start\n");
    if (argc < 2) {
        DEBUG_PRINT("usage: ./hideroot {pid}\n");
        return 1;
    }

    pid_t target_pid = atoi(argv[1]);
    DEBUG_PRINT("target pid is %d\n", target_pid);

    kill(target_pid, SIGSTOP);

    DEBUG_PRINT("hideroot switch_mnt_ns\n");
    if (switch_mnt_ns(target_pid)) {
        DEBUG_PRINT("");
        return 1;
    }

    DEBUG_PRINT("hideroot read mounts\n");
    char buffer[256];
    char* line = NULL;
    struct vector mount_list;
    snprintf(buffer, sizeof(buffer), "/proc/%d/mounts", target_pid);
    DEBUG_PRINT("hideroot init mount_list\n");
    vec_init(&mount_list);
    DEBUG_PRINT("hideroot read file\n");
    file_to_vector(buffer, &mount_list);

    DEBUG_PRINT("hideroot unmount dummy skeletons and /sbin links\n");
    // Unmount dummy skeletons and /sbin links
    vec_for_each(&mount_list, line) {
        if (strstr(line, "tmpfs /system/") || strstr(line, "tmpfs /vendor/") || strstr(line, "tmpfs /sbin")) {
            sscanf(line, "%*s %4096s", buffer);
            lazy_unmount(buffer);
        }
        free(line);
    }
    vec_destroy(&mount_list);

    // Re-read mount infos
    snprintf(buffer, sizeof(buffer), "/proc/%d/mounts", target_pid);
    vec_init(&mount_list);
    file_to_vector(buffer, &mount_list);

    DEBUG_PRINT("hideroot unmount everything under /system, /vendor\n");
    // Unmount everything under /system, /vendor, and loop mounts
    vec_for_each(&mount_list, line) {
        if (strstr(line, "/dev/block/loop") || strstr(line, " /system/") || strstr(line, " /vendor/")) {
            sscanf(line, "%*s %4096s", buffer);
            lazy_unmount(buffer);
        }
        free(line);
    }
    vec_destroy(&mount_list);
exit:
    // Send resume signal
    kill(target_pid, SIGCONT);
    sleep(100);
    return 0;
}
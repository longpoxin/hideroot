Magisk源码[地址](https://github.com/topjohnwu/Magisk)
隐藏root功能的代码位于工程的 Magisk/native/jni/magiskhide目录下
这里直接把功能实现核心代码抽离出来。完整代码见附件，以下是主要部分：

```c
int main(int argc, char** argv) {
    DEBUG_PRINT("hideroot start\n");
    if (argc < 2) {
        DEBUG_PRINT("usage: ./hideroot {pid}\n");
        return 1;
    }

    pid_t target_pid = atoi(argv[1]);
    DEBUG_PRINT("target pid is %d\n", target_pid);

    // 1. 暂停目标进程
    kill(target_pid, SIGSTOP);

    // 2. 关联到目标进程的mnt-namespace
    DEBUG_PRINT("hideroot switch_mnt_ns\n");
    if (switch_mnt_ns(target_pid)) {
        DEBUG_PRINT("");
        return 1;
    }

    // 3. 卸载相关文件系统
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
    // 4. 目标进程继续运行
    kill(target_pid, SIGCONT);

    sleep(100);

    return 0;
}
```

switch_mnt_ns的实现如下:

```c
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

    // setns的详细解释见 http://man7.org/linux/man-pages/man2/setns.2.html
    ret = setns(fd, 0);
    close(fd);
    return ret;
}

```


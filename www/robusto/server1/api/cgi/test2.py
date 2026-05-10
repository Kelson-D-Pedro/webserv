#!/usr/bin/env python3
import os
import stat

print("Content-Type: text/plain\n", flush=True)

fd_dir = "/proc/self/fd"

print("Inherited file descriptors:\n")

for fd_name in sorted(os.listdir(fd_dir), key=lambda x: int(x)):
    fd_path = os.path.join(fd_dir, fd_name)

    try:
        target = os.readlink(fd_path)
        st = os.stat(fd_path)
    except Exception as e:
        print(f"fd {fd_name}: <error reading> {e}")
        continue

    fd_type = "unknown"

    if stat.S_ISSOCK(st.st_mode):
        fd_type = "socket"
    elif stat.S_ISFIFO(st.st_mode):
        fd_type = "pipe / fifo"
    elif stat.S_ISREG(st.st_mode):
        fd_type = "regular file"
    elif stat.S_ISCHR(st.st_mode):
        fd_type = "char device"
    elif stat.S_ISDIR(st.st_mode):
        fd_type = "directory"

    # Special kernel anon inodes
    if "eventpoll" in target:
        fd_type = "epoll instance"
    elif "signalfd" in target:
        fd_type = "signalfd"
    elif "timerfd" in target:
        fd_type = "timerfd"
    elif "inotify" in target:
        fd_type = "inotify"

    print(f"fd {fd_name}: {fd_type}")
    print(f"    -> {target}")

# se quiseres testar filesystem relativo, faz aqui:
open("./file", "a").close()

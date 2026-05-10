#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <iostream>
#include <string.h>

int main() {
    DIR* dir = opendir("/proc/self/fd");
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent* ent;
    char path[PATH_MAX];
    char target[PATH_MAX];

    std::cout << "Inherited FDs:\n";

    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;

        snprintf(path, sizeof(path), "/proc/self/fd/%s", ent->d_name);
        ssize_t len = readlink(path, target, sizeof(target) - 1);
        if (len == -1) continue;

        target[len] = '\0';

        std::cout << "fd " << ent->d_name << " -> " << target;

        if (strstr(target, "eventpoll"))
            std::cout << "  [EPOLL]";

        std::cout << "\n";
    }

    closedir(dir);
    return 0;
}

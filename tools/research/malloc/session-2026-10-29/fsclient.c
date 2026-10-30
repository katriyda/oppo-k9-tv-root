#include <sys/socket.h>
#include <stddef.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    const char *name = "FACTORY_SERVER";
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 2; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;
    memcpy(addr.sun_path + 1, name, strlen(name));
    socklen_t len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + strlen(name));

    if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
        fprintf(stderr, "connect errno=%d\n", errno);
        perror("connect");
        return 3;
    }
    printf("connected\n");
    fflush(stdout);

    if (argc > 1) {
        size_t blen = strlen(argv[1]);
        if (write(fd, argv[1], blen) != (ssize_t)blen) perror("write");
        if (write(fd, "\n", 1) != 1) perror("write");
        printf("sent: %s\n", argv[1]);
        fflush(stdout);
    }

    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        fprintf(stderr, "read errno=%d\n", errno);
        perror("read");
    } else if (n == 0) {
        printf("eof\n");
    } else {
        buf[n] = 0;
        printf("resp(%zd): %s\n", n, buf);
    }
    close(fd);
    return 0;
}

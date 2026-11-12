#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int write_all(int fd, const char *buffer, size_t length)
{
    size_t total_written = 0;

    while (total_written < length) {
        ssize_t written = write(fd, buffer + total_written,
                                length - total_written);

        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (written == 0) {
            errno = EIO;
            return -1;
        }

        total_written += (size_t)written;
    }

    return 0;
}

int main(void)
{
    char buffer[16];
    int fd = open("input.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    int failed = 0;
    for (;;) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));

        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            failed = 1;
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        if (write_all(STDOUT_FILENO, buffer, (size_t)bytes_read) == -1) {
            perror("write");
            failed = 1;
            break;
        }
    }

    if (close(fd) == -1) {
        perror("close");
        failed = 1;
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

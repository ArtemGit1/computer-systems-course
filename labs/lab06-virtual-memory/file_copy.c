#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

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

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int source = open(argv[1], O_RDONLY);
    if (source == -1) {
        perror("open source");
        return EXIT_FAILURE;
    }

    int destination = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination == -1) {
        perror("open destination");
        if (close(source) == -1) {
            perror("close source");
        }
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    int failed = 0;

    for (;;) {
        ssize_t bytes_read = read(source, buffer, sizeof(buffer));

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

        if (write_all(destination, buffer, (size_t)bytes_read) == -1) {
            perror("write");
            failed = 1;
            break;
        }
    }

    if (close(source) == -1) {
        perror("close source");
        failed = 1;
    }
    if (close(destination) == -1) {
        perror("close destination");
        failed = 1;
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

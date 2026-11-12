#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    const char message[] = "Hello from write()\n";
    const size_t requested = strlen(message);
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    ssize_t written = write(fd, message, requested);
    if (written == -1) {
        perror("write");
        if (close(fd) == -1) {
            perror("close");
        }
        return EXIT_FAILURE;
    }

    printf("Requested: %zu bytes\n", requested);
    printf("Written:   %zd bytes\n", written);

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    if ((size_t)written != requested) {
        fprintf(stderr, "write completed only partially\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

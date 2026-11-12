#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int print_next_five(int fd, const char *label)
{
    char buffer[6];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes_read == -1) {
        perror("read");
        return -1;
    }

    buffer[(size_t)bytes_read] = '\0';
    printf("%s [%s]\n", label, buffer);
    return 0;
}

int main(void)
{
    int fd = open("input.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    int failed = print_next_five(fd, "First");
    if (failed == 0) {
        failed = print_next_five(fd, "Second");
    }
    if (failed == 0 && lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        failed = -1;
    }
    if (failed == 0) {
        failed = print_next_five(fd, "After lseek");
    }

    if (close(fd) == -1) {
        perror("close");
        failed = -1;
    }

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

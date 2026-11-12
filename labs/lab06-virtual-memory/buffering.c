#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int flush_stdout = argc == 2 && strcmp(argv[1], "--flush") == 0;

    if (argc > 2 || (argc == 2 && !flush_stdout)) {
        fprintf(stderr, "Usage: %s [--flush]\n", argv[0]);
        return 2;
    }

    printf("A");
    printf("B");
    printf("C");

    if (flush_stdout && fflush(stdout) == EOF) {
        perror("fflush");
        return 1;
    }

    if (write(STDOUT_FILENO, "D", 1) == -1) {
        perror("write");
        return 1;
    }

    return 0;
}

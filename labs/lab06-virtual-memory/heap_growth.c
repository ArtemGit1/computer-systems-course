#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    const size_t size = 100 * 1024 * 1024;
    long page_size = sysconf(_SC_PAGESIZE);

    if (page_size == -1) {
        perror("sysconf");
        return EXIT_FAILURE;
    }

    printf("PID: %ld\n", (long)getpid());

    char *memory = malloc(size);
    if (memory == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    printf("Allocated %zu bytes\n", size);
    printf("Address: %p\n", (void *)memory);
    printf("Press Enter to touch memory...\n");
    fflush(stdout);

    (void)getchar();

    for (size_t i = 0; i < size; i += (size_t)page_size) {
        memory[i] = 1;
    }

    printf("Memory touched. Press Enter to exit...\n");
    fflush(stdout);
    (void)getchar();

    free(memory);
    return EXIT_SUCCESS;
}

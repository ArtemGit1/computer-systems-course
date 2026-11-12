#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    long page_size = sysconf(_SC_PAGESIZE);

    if (page_size == -1) {
        perror("sysconf");
        return EXIT_FAILURE;
    }

    printf("Page size: %ld bytes\n", page_size);
    return EXIT_SUCCESS;
}

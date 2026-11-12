#include <stdio.h>
#include <stdlib.h>

#define N 1000000L

int main(void)
{
    long total = 0;

    for (long i = 1; i <= N; i++) {
        total += i;
    }

    printf("Sequential result: %ld\n", total);
    printf("Expected result: %ld\n", N * (N + 1) / 2);

    return EXIT_SUCCESS;
}

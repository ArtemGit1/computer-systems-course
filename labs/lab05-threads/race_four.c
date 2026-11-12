#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 500000

long counter = 0;

void *increment(void *arg)
{
    (void)arg;

    for (long i = 0; i < ITERATIONS; i++) {
        counter++;
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[4];

    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, increment, NULL);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected: %ld\n", 4L * ITERATIONS);
    printf("Actual:   %ld\n", counter);

    return EXIT_SUCCESS;
}

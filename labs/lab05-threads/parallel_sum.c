#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREAD_COUNT 4
#define N 1000000L

typedef struct {
    long start;
    long end;
    long result;
} Task;

void *calculate_sum(void *arg)
{
    Task *task = arg;
    long sum = 0;

    for (long i = task->start; i <= task->end; i++) {
        sum += i;
    }

    task->result = sum;

    return NULL;
}

int main(void)
{
    Task tasks[THREAD_COUNT];
    pthread_t threads[THREAD_COUNT];

    long chunk = N / THREAD_COUNT;
    long remainder = N % THREAD_COUNT;
    long start = 1;

    for (int i = 0; i < THREAD_COUNT; i++) {
        tasks[i].start = start;
        tasks[i].end = start + chunk - 1;

        if (i == THREAD_COUNT - 1) {
            tasks[i].end += remainder;
        }

        tasks[i].result = 0;
        start = tasks[i].end + 1;
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, calculate_sum, &tasks[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    long total = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        total += tasks[i].result;
        printf("Thread %d: %ld\n", i + 1, tasks[i].result);
    }

    printf("Parallel result: %ld\n", total);
    printf("Expected result: %ld\n", N * (N + 1) / 2);

    return EXIT_SUCCESS;
}

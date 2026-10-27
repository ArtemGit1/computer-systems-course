#include <stdio.h>
#include <stdlib.h>

int global_initialized = 10;
int global_uninitialized;

static int static_initialized = 20;
static int static_uninitialized;

static int marker(void)
{
    return 0;
}

int main(void)
{
    int local = 30;
    int *dynamic = malloc(sizeof(*dynamic));

    if (dynamic == NULL) {
        return 1;
    }

    *dynamic = 40;

    printf("Code/function:           %p\n", (void *)marker);
    printf("Global initialized:      %p\n", (void *)&global_initialized);
    printf("Global uninitialized:    %p\n", (void *)&global_uninitialized);
    printf("Static initialized:      %p\n", (void *)&static_initialized);
    printf("Static uninitialized:    %p\n", (void *)&static_uninitialized);
    printf("String literal:           %p\n", (void *)"Computer Systems");
    printf("Heap allocation:          %p\n", (void *)dynamic);
    printf("Stack/local:              %p\n", (void *)&local);

    free(dynamic);
    return 0;
}
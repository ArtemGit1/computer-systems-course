#include <stdio.h>

int initialized = 123;
int uninitialized;

static int static_initialized = 456;
static int static_uninitialized;

#ifdef LARGE_ARRAY
int large_array[100000];
#endif

const char message[] = "Computer Systems";

int main(void)
{
    printf("%d %d %d %d %s\n",
           initialized,
           uninitialized,
           static_initialized,
           static_uninitialized,
           message);

    return 0;
}
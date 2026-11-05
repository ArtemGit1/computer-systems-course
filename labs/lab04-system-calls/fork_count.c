#include <stdio.h>
#include <unistd.h>

int main(void)
{
    fork();
    fork();

    printf("PID=%d\n", (int)getpid());

    return 0;
}

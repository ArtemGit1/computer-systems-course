#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("PID:  %d\n", (int)getpid());
    printf("PPID: %d\n", (int)getppid());

    sleep(30);

    return 0;
}

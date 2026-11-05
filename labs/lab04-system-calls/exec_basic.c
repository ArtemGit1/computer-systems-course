#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("Before exec: PID=%d\n", (int)getpid());
    fflush(stdout);

    execlp("echo",
           "echo",
           "Hello from exec!",
           (char *)NULL);

    perror("execlp");

    return EXIT_FAILURE;
}

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [arguments...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Parent PID: %d\n", (int)getpid());
    printf("Creating child...\n\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("Child PID: %d\n", (int)getpid());
        printf("Child PPID: %d\n", (int)getppid());
        printf("Executing: %s\n\n", argv[1]);
        fflush(stdout);

        execvp(argv[1], &argv[1]);

        perror("execvp");
        _exit(127);
    }

    printf("Child PID: %d\n", (int)pid);

    int status;
    pid_t result;
    do {
        result = waitpid(pid, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    printf("\nChild %d completed\n", (int)pid);

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        printf("Exit status: %d\n", exit_status);
        printf("Parent continues\n");
        return exit_status;
    }

    if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        printf("Child terminated by signal: %d\n", signal_number);
        printf("Parent continues\n");
        return 128 + signal_number;
    }

    fprintf(stderr, "Child ended in an unexpected state\n");
    return EXIT_FAILURE;
}

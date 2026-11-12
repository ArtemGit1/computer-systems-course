#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    const char out[] = "Normal output\n";
    const char err[] = "Error output\n";

    if (write(STDOUT_FILENO, out, sizeof(out) - 1) == -1) {
        perror("write stdout");
        return EXIT_FAILURE;
    }

    if (write(STDERR_FILENO, err, sizeof(err) - 1) == -1) {
        perror("write stderr");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

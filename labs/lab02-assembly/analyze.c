#include <stdio.h>

int process_data(int a, int b)
{
    int sum = 0;
    if (a > b) {
        for (int i = 0; i < b; i++) {
            sum += a;
        }
    } else {
        sum = b - a;
    }
    return sum;
}

int main(void)
{
    int result = process_data(5, 3);
    printf("Result: %d\n", result);
    return 0;
}
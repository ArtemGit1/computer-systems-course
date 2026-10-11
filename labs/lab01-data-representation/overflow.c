#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint8_t value = 254;
    
    printf("Initial: %u\n", value);
    
    value++;
    printf("value + 1: %u\n", value);
    
    value++;
    printf("value + 2: %u\n", value);
    
    value++;
    printf("value + 3: %u\n", value);

    return 0;
}
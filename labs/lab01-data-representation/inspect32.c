#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint32_t value = 0x12345678;
    
    unsigned char *p = (unsigned char *)&value;

    printf("Value: 0x%08X\n\n", value);
    printf("Size: %zu bytes\n\n", sizeof(value));

    printf("Memory:\n");
    for (size_t i = 0; i < sizeof(value); i++) {
        printf("byte %zu: 0x%02X\n", i, p[i]);
    }
    printf("\n");

    if (p[0] == (value & 0xFF)) {
        printf("Byte order: Little-endian\n");
    } else {
        printf("Byte order: Big-endian\n");
    }

    return 0;
}
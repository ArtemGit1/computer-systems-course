#include <stdio.h>

void print_binary(unsigned char value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

int main(void)
{
    unsigned char values[] = {0, 1, 10, 65, 127, 128, 255};
    int num_values = sizeof(values) / sizeof(values[0]);

    for (int j = 0; j < num_values; j++) {
        unsigned char value = values[j];
        
        printf("Decimal: %3u | ", value);
        printf("Hex: 0x%02X | ", value);
        printf("Binary: ");
        print_binary(value);
        printf("\n");
    }

    return 0;
}
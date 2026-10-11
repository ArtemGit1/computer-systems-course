#include <stdio.h>

void print_binary(unsigned char value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

int main(void)
{
    int input;
    
    printf("Enter value (0-255): ");
    if (scanf("%d", &input) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    unsigned char value = (unsigned char)input;

    printf("\nDecimal:      %u\n", value);
    printf("Hexadecimal:  0x%02X\n", value);
    printf("Binary:       ");
    print_binary(value);
    printf("\n");

    if (value >= 32 && value <= 126) {
        printf("Character:    %c\n", value);
    }

    return 0;
}

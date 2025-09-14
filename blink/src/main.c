#include <stdint.h>

#define GPIOB_BASE 0x58020400
#define RCC_AHB4ENR (0x58024400 + 0x0E0)

// Correct LED pinout for STM32H753ZI Nucleo:
// LD1 (Green): PB0  
// LD2 (Red):   PE1
// LD3 (Blue):  PB14

void simple_delay(volatile uint32_t count) {
    while(count--) {
        __asm__("nop");
    }
}

int main() {

    *(volatile uint32_t*)(RCC_AHB4ENR) |= (1 << 1);
    
    *(volatile uint32_t*)(GPIOB_BASE) |= (1 << 0);
    *(volatile uint32_t*)(GPIOB_BASE) &= ~(1 << 1);

    *(volatile uint32_t*)(GPIOB_BASE + 0x04) &= ~(1 << 0);


    while (1) {

        *(volatile uint32_t*)(GPIOB_BASE + 0x18) = (1 << 0);
        simple_delay(2000000);
        *(volatile uint32_t*)(GPIOB_BASE + 0x18) = (1 << 16);
        simple_delay(2000000);

    }

    return 0;
}

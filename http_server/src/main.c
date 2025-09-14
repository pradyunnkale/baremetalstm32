#include <stdint.h>
#include "../inc/uart.h"

#define GPIOB_BASE 0x58020400
#define RCC_AHB4ENR (0x58024400 + 0x0E0)

void simple_delay(volatile uint32_t count) {
    while (count--) {
        asm("nop");
    }
}


int main() {

    uart_init();

    *(volatile uint32_t*)(RCC_AHB4ENR) |= (1 << 1);
    
    *(volatile uint32_t*)(GPIOB_BASE) |= (1 << 0);
    *(volatile uint32_t*)(GPIOB_BASE) &= ~(1 << 1);

    *(volatile uint32_t*)(GPIOB_BASE + 0x04) &= ~(1 << 0);

    while (1) {
        
            uart_putstring("Hello World!\r\n");

            simple_delay(1000000);

    }

    return 0;
}

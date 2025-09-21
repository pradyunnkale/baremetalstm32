#include "../inc/uart.h"

void uart_init(void) { 
    // Enable clocks
    *(volatile uint32_t*)(RCC_BASE + 0xE8) |= (1 << 18); // USART3 Clock (APB1LENR bit 18)
    *(volatile uint32_t*)(RCC_BASE + 0xE0) |= (1 << 3); // GPIO_D Clock (AHB4ENR bit 3)
    
    // Configure PD8 and PD9 as alternate function
    *(volatile uint32_t*)(GPIOD_BASE + 0x00) &= ~((3 << 16) | (3 << 18)); // Clear bits 16-17 and 18-19
    *(volatile uint32_t*)(GPIOD_BASE + 0x00) |= (2 << 16) | (2 << 18); // Set to alternate function
    
    // Set alternate function to AF7 (USART3)
    *(volatile uint32_t*)(GPIOD_BASE + 0x24) &= ~((0xF << 0) | (0xF << 4));
    *(volatile uint32_t*)(GPIOD_BASE + 0x24) |= (7 << 0) | (7 << 4);

    // Configure USART3
    *(volatile uint32_t*)(USART3_BASE + 0x04) |= (1 << 20);
    *(volatile uint32_t*)(USART3_BASE + 0x00) |= (1 << 3) | (1 << 2) | (1 << 0); //TE | RE | UE
}

void uart_putchar(char c) {
    volatile int timeout = 0;
    while (!(*(volatile uint32_t*)(USART3_BASE + 0x1C) & (1 << 7)));
    *(volatile uint32_t*)(USART3_BASE + 0x28) = c;
    while (!(*(volatile uint32_t*)(USART3_BASE + 0x1C) & (1 << 6)));
}

void uart_putstring(const char* str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

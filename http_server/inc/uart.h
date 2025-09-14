#include <stdint.h>

#define RCC_BASE 0x58024400
#define USART3_BASE 0x40004800
#define GPIOD_BASE 0x58020C00

void uart_init(void);
void uart_putchar(char c);
void uart_putstring(const char* str);

uint32_t uart_receive(void);
void uart_print(char* str);


#include "uart.h"

// UART base address for QEMU virt machine (NS16550A)
#define UART_BASE 0x10000000

// UART registers
#define UART_THR (volatile uint8_t*)(UART_BASE + 0) // Transmitter Holding Register
#define UART_RBR (volatile uint8_t*)(UART_BASE + 0) // Receiver Buffer Register
#define UART_LSR (volatile uint8_t*)(UART_BASE + 5) // Line Status Register

#define LSR_THRE 0x20 // Transmit Holding Register Empty
#define LSR_DR   0x01 // Data Ready

void uart_init(void) {
    // UART is already initialized by QEMU
}

void uart_putc(char c) {
    // Wait until transmit holding register is empty
    while ((*UART_LSR & LSR_THRE) == 0);
    *UART_THR = c;
}

char uart_getc(void) {
    // Wait until data is ready
    while ((*UART_LSR & LSR_DR) == 0);
    return *UART_RBR;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void uart_put_hex(unsigned long value) {
    char hex[] = "0123456789ABCDEF";
    uart_putc('0');
    uart_putc('x');
    
    int started = 0;
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (value >> i) & 0xF;
        if (digit != 0 || started || i == 0) {
            uart_putc(hex[digit]);
            started = 1;
        }
    }
}

void uart_put_dec(unsigned long value) {
    if (value == 0) {
        uart_putc('0');
        return;
    }
    
    char buffer[20];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (i > 0) {
        uart_putc(buffer[--i]);
    }
}
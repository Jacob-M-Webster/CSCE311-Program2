#ifndef UART_H
#define UART_H

#include "kernel.h"

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);
void uart_put_hex(unsigned long value);
void uart_put_dec(unsigned long value);

#endif
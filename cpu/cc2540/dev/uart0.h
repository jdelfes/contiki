#ifndef UART_0_H
#define UART_0_H

#include "contiki-conf.h"
#include "cc2540.h"
#include "uart.h"

#ifdef UART0_CONF_ENABLE
void uart0_init();
void uart0_writeb(uint8_t byte);
uint8_t uart0_readb(void);
#else
#define uart0_init(...)
#define uart0_writeb(...)
#define uart0_readb(...) 0
#endif /* UART0_ENABLE */
#endif /* UART_0_H */

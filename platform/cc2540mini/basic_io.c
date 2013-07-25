#include "dev/uart0.h"
#include <stdio.h>

void
putchar(char c)
{
  uart0_writeb(c);
}

char
getchar(void)
{
  return uart0_readb();
}
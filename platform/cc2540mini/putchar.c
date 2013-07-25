#include "dev/uart0.h"

void
putchar(char c)
{
  uart0_writeb(c);
}

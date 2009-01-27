#include <simics.h>
#include <stdarg.h>
#include <stdio/stdio.h>

void SIM_printf(char* fmt, ...)
{
  char str[256];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf (str, 254, fmt, ap);
  va_end(ap);

  SIM_puts(str);

}




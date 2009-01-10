#include <simics.h>
#include <stdarg.h>
#include <stdio.h>

void SIM_printf(char* fmt, ...)
{
  char str[256];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf (str, 255, fmt, ap);
  va_end(ap);

  SIM_puts(str);
}




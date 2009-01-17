#include <simics.h>
#include <stdarg.h>
#include <stdio/stdio.h>
#include <hypervisor.h>

void SIM_printf(char* fmt, ...)
{
  char str[256];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf (str, 254, fmt, ap);
  va_end(ap);
  strcat(str, "\n");

  //SIM_puts(str);

  HYPERVISOR_console_io(CONSOLEIO_write,
			strlen(str),
			str);
}




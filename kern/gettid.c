
#include <syscall_int.h>
#include <syscall_wrapper.h>
#include <simics.h>

int 
gettid(void)
{
  return syscall_wrapper_0(GETTID_INT, 0);
}

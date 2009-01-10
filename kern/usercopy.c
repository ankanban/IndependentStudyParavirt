
#define DEBUG_LEVEL KDBG_LOG
#include <vmm_page.h>
#include <task.h>
#include <sched.h>
#include <string.h>
#include <kdebug.h>


char
copy_user_byte(uint32_t user_str)
{
  char * pch = (char *)(user_str);
  kdverbose("ch: \'%c\' @ (usr):0x%x|(phys):0x%x", *pch, user_str, pch); 
  return (*pch);
}

uint32_t
copy_user_dword(uint32_t user_dword)
{
  uint32_t uword;
  char * uword_ptr = (char *)&uword;
  int i = 0;
  for (; i < sizeof(uint32_t); i++) {
    *(uword_ptr + i) = copy_user_byte((uint32_t)
				      ((char *)user_dword + i));
  }
  kdtrace("Read dword: 0x%x", uword);
  return uword;
}

/*
 * Returns bytes copied, including 
 * null terminator.
 */
int 
strncpy_user(char * usr_str, 
	     char * buf,
	     int size)
{
  int bytes = 0;
  char ch = 0;
  
  if (size == 0) {
    return -1;
  }
  
  kdtrace("strncpy_user(%p,%p,%d)",
	  usr_str, buf, size);
  
  while ((ch = copy_user_byte((uint32_t)(usr_str + bytes))) != '\0') {
    if (bytes >= (size - 1)) {
      kdtrace("Partial read, error");
      return -1;
    }
    
    kdverbose("ch:%c", ch);

    *(buf + bytes) = ch;
    bytes++;
  }

  kdtrace("buf:%p, bytes:%d",
	  buf, bytes);

  *(buf + bytes) = '\0';
  bytes++;
  
  kdtrace("string: \"%s\"", buf);
  return bytes;
}

int
copy_user_string(void * userarg, 
		 int arg_num,
		 char * buf,
		 int size)	    
{
  char * usr_str = (char *)copy_user_dword((uint32_t)((uint32_t *)userarg + 
						      arg_num));

  kdtrace("Copying string from usr addr: %p", 
	  usr_str);

  return strncpy_user(usr_str, 
		      buf, 
		      size);
  
}

void
get_string_array_dim(char ** argvec,
		     int * argc,
		     int * argvec_array_size,
		     int * argvec_str_size)
{
  *argc = 0;
  *argvec_str_size = 0;
  *argvec_array_size = 0;
  
  for (; argvec[(*argc)] != NULL; (*argc)++) {
    *argvec_str_size += strlen(argvec[(*argc)]) + 1;
  }

  kdinfo("argc: %d", *argc);
  
  *argvec_array_size = (sizeof(char *) * ((*argc) + 1));

}

int
copy_string_array(char * dest_buf,
		  char ** argvec,
		  int argc,
		  int argvec_array_size)
{
  int i = 0;
  
  char ** stack_argvec = (char **)dest_buf;

  char * stack_argvec_str = (char *)((uint32_t)stack_argvec + 
				     argvec_array_size);
  
  for (; i < argc; i++) {
    stack_argvec[i] = stack_argvec_str;
    strcpy(stack_argvec_str,
	   argvec[i]);
    kdinfo("copied str: %p:\"%s\"", 
	   stack_argvec_str,
	   stack_argvec_str);
    stack_argvec_str += strlen(stack_argvec[i]) + 1;
  }
  
  stack_argvec[i] = NULL;

  return 0;
}

int
copy_user_int(void * userarg,
	      int arg_num)
{
  return (int)(*((uint32_t *)userarg + arg_num));
}

void
copy_to_user_byte(uint32_t kern_byte_ptr,
		  uint32_t usr_byte_ptr)
{
  char * pch = (char *)(usr_byte_ptr);
  *pch = *((char *)kern_byte_ptr);
  kdverbose("wr ch: \'%c\' @ (usr):0x%x|(phys):0x%x", *pch, usr_byte_ptr, pch);
}


void
copy_to_user_dword(uint32_t kern_dword,
		   uint32_t usr_dword_ptr)
{
  char * kern_char_ptr = (char *)&kern_dword;
  int i = 0;
  for (; i < sizeof(uint32_t); i++) {
    copy_to_user_byte((uint32_t)(kern_char_ptr + i),
		      (uint32_t)(usr_dword_ptr + i));
  }
  kdtrace("Wrote dword: 0x%x", kern_dword);
}

void
copy_to_user_buf(char * kern_buf,
		 int size,
		 char * usr_buf)
{
  int i = 0;
  for (; i < size; i++) {
    copy_to_user_byte((uint32_t)(kern_buf + i),
		      (uint32_t)(usr_buf + i));
  }
}


void
copy_char_to_user_argpkt_buf(char kern_ch,
			     int offset,
			     int size,
			     void * argpkt,
			     int arg_num)
{

  if (offset >= size) {
    return;
  }

  char * usr_buf = (char *)copy_user_dword((uint32_t)((uint32_t *)argpkt + 
						      arg_num));

  
  
  kdtrace("Copying char to usr addr: %p", 
	  usr_buf);
  
  copy_to_user_byte((uint32_t)(&kern_ch),
		    (uint32_t)(usr_buf + offset));
}

void
copy_buf_to_user_argpkt_buf(char * kern_buf,
			    int offset,
			    int size,
			    void * argpkt,
			    int arg_num)
{
  char * usr_buf = (char *)copy_user_dword((uint32_t)((uint32_t *)argpkt + 
						      arg_num));

  kdtrace("Copying buffer to usr addr: %p", 
	  usr_buf + offset);
  
  copy_to_user_buf(kern_buf,
		   size,
		   usr_buf + offset);
}

void
copy_to_user_argpkt_buf(char * kern_buf,
			int size,
			void * argpkt,
			int arg_num)
{
  char * usr_buf = (char *)copy_user_dword((uint32_t)((uint32_t *)argpkt + 
						      arg_num));

  kdtrace("Copying buffer to usr addr: %p", 
	  usr_buf);
  
  copy_to_user_buf(kern_buf,
		   size,
		   usr_buf);
}

void
copy_to_user_argpkt_int(int kern_int,
			void * argpkt,
			int arg_num)
{
  void * usr_dword_ptr = (char *)copy_user_dword((uint32_t)((uint32_t *)argpkt + 
							    arg_num));
  copy_to_user_dword((uint32_t)kern_int,
		     (uint32_t)usr_dword_ptr);
  
}


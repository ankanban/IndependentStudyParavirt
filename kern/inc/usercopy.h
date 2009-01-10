#ifndef __USERCOPY_H__
#define __USERCOPY_H__

char
copy_user_byte(char * user_str);

uint32_t
copy_user_dword(char * user_str);

int 
strncpy_user(char * usr_str, 
	     char * buf,
	     int size);

int
copy_user_string(void * userarg, 
		 int arg_num,
		 char * buf,
		 int size);	    

void
get_string_array_dim(char ** argvec,
		     int * argc,
		     int * argvec_array_size,
		     int * argvec_str_size);

int
copy_string_array(char * dest_buf,
		  char ** argvec,
		  int argc,
		  int argvec_array_size);

int
copy_user_int(void * userarg,
	      int arg_num);


void
copy_to_user_byte(uint32_t kern_byte_ptr,
		  uint32_t usr_byte_ptr);

void
copy_to_user_dword(uint32_t kern_dword,
		   uint32_t user_dword_ptr);

void
copy_to_user_buf(char * kern_buf,
		 int size,
		 char * usr_buf);

void
copy_char_to_user_argpkt_buf(char kern_ch,
			     int offset,
			     int size,
			     void * argpkt,
			     int arg_num);

void
copy_buf_to_user_argpkt_buf(char * kern_buf,
			       int offset,
			       int size,
			       void * argpkt,
			    int arg_num);

void
copy_to_user_argpkt_buf(char * kern_buf,
			int size,
			void * argpkt,
			int arg_num);

void
copy_to_user_argpkt_int(int kern_int,
			void * argpkt,
			int arg_num);


#endif /* __USERCOPY_H__ */

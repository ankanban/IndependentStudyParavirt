/* The initial program
 *
 *     init.c
 *
 */
#include <syscall.h>
#include <stdio.h>

int main()
{
  int pid, exitstatus;
  char shell[] = "shell";
  char * args[] = {shell, 0};

  while(1) {
    pid = fork();
    if (!pid)
      exec(shell, args);
    
    while (pid != wait(&exitstatus));
  
    printf("Shell exited with status %d; starting it back up...", exitstatus);
  }
}

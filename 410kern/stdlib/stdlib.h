#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <malloc.h>  /* "Listen to what the man says" */

long atol(const char *__str);
#define atoi(str) ((int)atol(str))

long strtol(const char *__p, char **__out_p, int __base);
unsigned long strtoul(const char *__p, char **__out_p, int __base);

int rand(void);
void srand(unsigned new_seed);

void panic(const char *, ...);

#endif

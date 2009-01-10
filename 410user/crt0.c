/* @file crt0.c
 * @brief The 15-410 userland C runtime setup function
 *
 * @note The build infrastructure "knows" that _main is the entrypoint
 *       function and will do the right thing as a result.
 */

#include <stdlib.h>

extern int main(int argc, char *argv[]);

void _main(int argc, char *argv[])
{
  exit(main(argc, argv));
}

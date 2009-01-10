/* Test helper for 15-410
 * Michael Berman(mberman)
 *
 * This file is intended to be included by 
 * the npgs_* tests.  Before the #include line,
 * test_name must be declared.
 *
 * exhaustion() tries to use up all of the
 * available memory, and reports the location
 * and size of the first block (which should be
 * large) in firstblock_base and firstblock_size.
 */



#define ADDR 0x40000000
#define GIGABYTE (1024*1024*1024)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

void * firstblock_base = 0x0;
int firstblock_size = 0;

int
exhaustion(void)
{
	int size, attempt;
	void *base;

	attempt = 0;
	base = (void *)ADDR;
	size = GIGABYTE;

	while (size > PAGE_SIZE) {
		char msg[80];

		if (new_pages(base, size) == 0) {
		  firstblock_base = base;
		  firstblock_size = size;
			base += size;
		} else {
			size /= 2;
		}
		snprintf(msg, sizeof (msg), "Attempt %d: allocated %d pages, size now %d", ++attempt, ((int)base - (int)ADDR) / PAGE_SIZE, size);
		REPORT_MISC(msg);
	}
	return (((int)base - (int)ADDR) / PAGE_SIZE);
}

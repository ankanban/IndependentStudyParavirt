/* Taken from 410kern/lib/x86/seg.c.  Originally by smuckle, modified by zra. */

/*
 * returns a pointer to the beginning of the installed IDT.
 */
void *idt_base(void)
{
    unsigned int ptr[2];
    asm ("sidt (%0)": :"p" (((char *) ptr)+2));
    return (void *) ptr[1];
}


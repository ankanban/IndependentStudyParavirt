
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include <hypervisor.h>
#include <xen/io/console.h>
#include <kdebug.h>

/* Copies all print output to the Xen emergency console apart
   of standard dom0 handled console */
#define USE_XEN_CONSOLE

/* Low level functions defined in xencons_ring.c */
extern int xencons_ring_init(void);
extern int xencons_ring_send(const char *data, unsigned len);
extern int xencons_ring_send_no_notify(const char *data, unsigned len);


/* If console not initialised the printk will be sent to xen serial line 
   NOTE: you need to enable verbose in xen/Rules.mk for it to work. */
static int xen_console_initialised = 0;


void xencons_rx(char *buf, unsigned len, struct pt_regs *regs)
{
    if(len > 0)
    {
        /* Just repeat what's written */
        buf[len] = '\0';
        printk("%s", buf);
        
        if(buf[len-1] == '\r')
            printk("\nNo console input handler.\n");
    }
}

void xencons_tx(void)
{
    /* Do nothing, handled by _rx */
}


void xen_console_print(const char *rodata, int length)
{
  char* data = (char *)rodata;
  char *curr_char, saved_char;
    int part_len;
    int (*ring_send_fn)(const char *data, unsigned length);

    if(!xen_console_initialised)
        ring_send_fn = xencons_ring_send_no_notify;
    else
        ring_send_fn = xencons_ring_send;
        
    for(curr_char = data; curr_char < data+length-1; curr_char++)
    {
        if(*curr_char == '\n')
        {
            saved_char = *(curr_char+1);
            *(curr_char+1) = '\r';
            part_len = curr_char - data + 2;
            ring_send_fn(data, part_len);
            *(curr_char+1) = saved_char;
            data = curr_char+1;
            length -= part_len - 1;
        }
    }
    
    ring_send_fn(data, length);
    
    if(data[length-1] == '\n')
        ring_send_fn("\r", 1);
}

void xen_print(int direct, const char *fmt, va_list args)
{
    static char   buf[1024];
    
    (void)vsnprintf(buf, sizeof(buf), fmt, args);
    strcat(buf, "\n");

    if(direct)
    {
        (void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
        return;
    } else {
#ifndef USE_XEN_CONSOLE
    if(!xen_console_initialised)
#endif    
            (void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
        
        xen_console_print(buf, strlen(buf));
    }
}

void printk(const char *fmt, ...)
{
    va_list       args;
    va_start(args, fmt);
    xen_print(0, fmt, args);
    va_end(args);        
}

void xprintk(const char *fmt, ...)
{
    va_list       args;
    va_start(args, fmt);
    xen_print(1, fmt, args);
    va_end(args);        
}

void xen_init_console(void)
{   
    printk("Initialising console ... ");
    xencons_ring_init();    
    xen_console_initialised = 1;
    /* This is also required to notify the daemon */
    printk("done.\n");
}

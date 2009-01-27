/**
 * @file boot/head.S
 * @author matthewj
 * @author de0u
 */

/* N.B. This is NOT to be construed as style guidance!! */
/* N.B. This is NOT to be construed as style guidance!! */
/* N.B. This is NOT to be construed as style guidance!! */

#include <x86/video_defines.h>
#include <x86/seg.h>
#include <pebbles_elfnote.h>
#include <xen/elfnote.h>

#define PAGE_SIZE_asm 4096
#define __PAGE_OFFSET 0x00000000
	
/** @brief Entry point from the bootloader */
.global _start
/** @brief Start of the kernel image */
.global __kimg_start
/** @brief Start of the init GDT */
.global init_gdt
/** @brief Start of the init IDT */
.global init_idt
/** @brief Start of the init TSS */
.global init_tss
.global hypercall_page
.global shared_info
	
.text

__kimg_start:

mb_header:
    /* Not a complete Multiboot header, but GRUB can live with it. */
    .long 0x1BADB002      /**< Multiboot magic word */
    .long 0x00000000      /**< Flags field */
    .long (0 - 0x1BADB002 - 0x00000000)   /**< Checksum */
    .space 244

init_idt: /* 0x100100 */
    .space 64
    .long 0x00100998
    .long 0x00108f00
    .space 1976
init_gdt: /* 0x100900 */
    .long 0x00000000
    .long 0x00000000
    .long 0x09300067
    .long 0x00008910
    .long 0x0000ffff
    .long 0x00cf9b00
    .long 0x0000ffff
    .long 0x00cf9300
    .long 0x0000ffff
    .long 0x00cffb00
    .long 0x0000ffff
    .long 0x00cff200
init_tss: /* 0x100930 */
    .space 8
    .word 0x0018
    .space 92 /* fills with 0's */
    .word 0x0068
df_handler: /* 0x100998 */
    cli
    /* Get something "out there" first thing */
    leal fmsg, %eax
    movl $0x1BADD00D, %ebx
    xchg %ebx, %ebx
    /* We got here because we were in trouble, so be conservative */
    movl $SEGSEL_KERNEL_DS, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss
    leal istack, %esp
    subl $2048, %esp      /* try to avoid wiping state */
    /* Now we probably have a runtime environment */
    leal fbytes, %eax
    pushl %eax
    call blat
df_handler_loop:
    jmp df_handler_loop

#define SAVE_ALL \
	cld; \
	pushl %es; \
	pushl %ds; \
	pushl %eax; \
	pushl %ebp; \
	pushl %edi; \
	pushl %esi; \
	pushl %edx; \
	pushl %ecx; \
	pushl %ebx; \
	movl $(SEGSEL_KERNEL_DS),%edx; \
	movl %edx,%ds; \
	movl %edx,%es;

#define RESTORE_ALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %esi;	\
	popl %edi;	\
	popl %ebp;	\
	popl %eax;	\
	popl %ds;	\
	popl %es;	\
	addl $4,%esp;	\
	iret;		

ES		= 0x20
ORIG_EAX	= 0x24
EIP		= 0x28
CS		= 0x2C

	
/* Hypervisor callback handlers */
# A note on the "critical region" in our callback handler.
# We want to avoid stacking callback handlers due to events occurring
# during handling of the last event. To do this, we keep events disabled
# until weve done all processing. HOWEVER, we must enable events before
# popping the stack frame (cant be done atomically) and so it would still
# be possible to get enough handler activations to overflow the stack.
# Although unlikely, bugs of that kind are hard to track down, so wed
# like to avoid the possibility.
# So, on entry to the handler we detect whether we interrupted an
# existing activation in its critical region -- if so, we pop the current
# activation and restart the handler using the previous one.
.global hypervisor_callback
hypervisor_callback:	
        pushl %eax
        SAVE_ALL
        movl EIP(%esp),%eax
        cmpl $scrit,%eax
        jb   11f
        cmpl $ecrit,%eax
        jb   critical_region_fixup
11:     push %esp
        call do_hypervisor_callback
        add  $4,%esp
        movl xen_shared_info,%esi
        xorl %eax,%eax
        movb CS(%esp),%cl
    	test $2,%cl          # slow return to ring 2 or 3
        jne  safesti
safesti:movb $0,1(%esi)     # reenable event callbacks
scrit:  /**** START OF CRITICAL REGION ****/
        testb $0xFF,(%esi)
        jnz  14f              # process more events if necessary...
        RESTORE_ALL
14:     movb $1,1(%esi)
        jmp  11b
ecrit:  /**** END OF CRITICAL REGION ****/
# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so how many saved
# registers are in each frame. We do this quickly using the lookup table
# 'critical_fixup_table'. For each byte offset in the critical region, it
# provides the number of bytes which have already been popped from the
# interrupted stack frame. 
critical_region_fixup:
        addl $critical_fixup_table-scrit,%eax
        movzbl (%eax),%eax    # %eax contains num bytes popped
        mov  %esp,%esi
        add  %eax,%esi        # %esi points at end of src region
        mov  %esp,%edi
        add  $0x34,%edi       # %edi points at end of dst region
        mov  %eax,%ecx
        shr  $2,%ecx          # convert words to bytes
        je   16f              # skip loop if nothing to copy
15:     subl $4,%esi          # pre-decrementing copy loop
        subl $4,%edi
        movl (%esi),%eax
        movl %eax,(%edi)
        loop 15b
16:     movl %edi,%esp        # final %edi is top of merged stack
        jmp  11b
         
critical_fixup_table:        
        .byte 0x00,0x00,0x00                  # testb $0xff,(%esi)
        .byte 0x00,0x00                       # jne  14f
        .byte 0x00                            # pop  %ebx
        .byte 0x04                            # pop  %ecx
        .byte 0x08                            # pop  %edx
        .byte 0x0c                            # pop  %esi
        .byte 0x10                            # pop  %edi
        .byte 0x14                            # pop  %ebp
        .byte 0x18                            # pop  %eax
        .byte 0x1c                            # pop  %ds
        .byte 0x20                            # pop  %es
        .byte 0x24,0x24,0x24                  # add  $4,%esp
        .byte 0x28                            # iret
        .byte 0x00,0x00,0x00,0x00             # movb $1,1(%esi)
        .byte 0x00,0x00                       # jmp  11b
       
# Hypervisor uses this for application faults while it executes.
.global failsafe_callback
failsafe_callback:	
      pop  %ds
      pop  %es
      pop  %fs
      pop  %gs
      iret
	

/* The kernel entry point */
.global loop_forever
.global resume_kernel

_start:

loop_forever:
    movl $0, %ecx
    cmpl $0, %ecx
    jz loop_forever

resume_kernel:

    cld /* Clear direction flag */
    movl %esp, %ebx /* bootstrap stack address */
    leal istack, %esp     /* Load initial stack pointer */
/* Load TSS,GDT,IDT, ltr */
	
/* Call the C entry point */
    pushl %ebx            /* Pass in top of bootstrap stack, given to us
	                     by Xen */
    pushl %esp            /* Pass in the top of the initial stack */
    pushl %esi            /* Pass in the start_info_t structure provided by Xen */
    call mb_entry         /* Call the C entry point */

    pushl $pbytes
    call blat
stuck:
    jmp stuck

# void blat(uint8_t *bytes)
blat:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    movl 8(%ebp), %eax                 # bytes
    pushl %eax
    call strlen
    movl %eax, %edx                    # EDX: strlen(bytes)
    add $4, %esp

    movl $CONSOLE_MEM_BASE, %edi       # EDI: dest

    movl $CONSOLE_WIDTH, %ebx
    imull $CONSOLE_HEIGHT, %ebx
    sall $1, %ebx                      # 2*width*height
    addl %edi, %ebx                    # EBX: limit
    subl %edx, %ebx                    # EBX: limit (round down)

    movl %es, %eax
    pushl %eax                         # save ES
    movl %ds, %eax
    movl %eax, %es                     # force ES to match DS

movs:
    movl %edx, %ecx                    # ECX: count
    movl 8(%ebp), %esi                 # ESI: source
    rep movsb                          # ECX bytes from DS:ESI to ES:EDI
    cmpl %edi, %ebx
    jg movs

    popl %eax
    movl %eax, %es                     # restore ES

    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret

.data

fmsg:
    .asciz "\n***** Double fault!\n"
fbytes:
    .asciz "D\ro\ru\rb\rl\re\r \rf\ra\ru\rl\rt\r!\r \r"
    
pmsg:
    .asciz "Kernel cannot return()!"
pbytes:
    .asciz "K\re\rr\rn\re\rl\r \rc\ra\rn\rn\ro\rt\r \rr\re\rt\ru\rr\rn\r(\r)\r!\r \r"

.pushsection .bss.page_aligned
	.align PAGE_SIZE_asm
hypercall_page:	
	.skip 0x1000
shared_info:
	.skip 0x1000
.popsection

	ELFNOTE(Xen, XEN_ELFNOTE_GUEST_OS,       .asciz "pebbles_xen")
	ELFNOTE(Xen, XEN_ELFNOTE_GUEST_VERSION,  .asciz "1.0")
	ELFNOTE(Xen, XEN_ELFNOTE_XEN_VERSION,    .asciz "xen-3.0")
	ELFNOTE(Xen, XEN_ELFNOTE_VIRT_BASE,      .long  __PAGE_OFFSET)
	ELFNOTE(Xen, XEN_ELFNOTE_ENTRY,          .long  _start)
	ELFNOTE(Xen, XEN_ELFNOTE_HYPERCALL_PAGE, .long  hypercall_page)
	ELFNOTE(Xen, XEN_ELFNOTE_FEATURES,       .asciz "!writable_page_tables")
	ELFNOTE(Xen, XEN_ELFNOTE_PAE_MODE,       .asciz "no")
	ELFNOTE(Xen, XEN_ELFNOTE_LOADER,         .asciz "generic")
	
/* Initial stack */
.space 4096
istack:

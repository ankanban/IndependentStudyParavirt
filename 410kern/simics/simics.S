/* simics.S - Simics interface (kernel side) */

.global SIM_break
.global SIM_halt
.global SIM_notify_bootup
.global SIM_puts
.global SIM_register_user_from_parent
.global SIM_register_user_proc
.global SIM_switch
.global SIM_unregister_user_proc

#define SIM_NOARG(opcode) \
    pushl %ebx; \
    movl $opcode, %ebx; \
    xchg %ebx, %ebx; \
    popl %ebx; \
    ret

#define SIM_ARG(opcode) \
    movl 4(%esp), %eax; \
    SIM_NOARG(opcode)

#define SIM_2ARG(opcode) \
    movl 8(%esp), %ecx; \
    SIM_ARG(opcode)

SIM_puts:                      SIM_ARG(  0x1BADD00D)
SIM_break:                     SIM_NOARG(0x2BADD00D)

SIM_register_user_proc:        SIM_2ARG( 0x5BADD00D)
SIM_unregister_user_proc:      SIM_ARG(  0x6BADD00D)
SIM_switch:                    SIM_ARG(  0x7BADD00D)
SIM_register_user_from_parent: SIM_2ARG(0x8BADD00D)

SIM_notify_bootup:             SIM_ARG(  0xB00710AD)

SIM_halt:                      SIM_NOARG(0xDEADD00D)

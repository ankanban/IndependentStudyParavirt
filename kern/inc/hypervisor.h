/******************************************************************************
 * hypervisor.h
 * 
 * Linux-specific hypervisor handling.
 * 
 * Copyright (c) 2002-2004, K A Fraser
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

//#include <linux/types.h>
//#include <linux/kernel.h>
//#include <linux/version.h>
//#include <linux/errno.h>

#include <types.h>
#include <stddef.h>
#include <stdint.h>

#include <xen/xen.h>
#include <xen/platform.h>
#include <xen/event_channel.h>
#include <xen/physdev.h>
#include <xen/sched.h>
#include <xen/nmi.h>
//#include <asm/ptrace.h>
//#include <asm/page.h>


extern shared_info_t *HYPERVISOR_shared_info;

#define vcpu_info(cpu) (HYPERVISOR_shared_info->vcpu_info + (cpu))

#define current_vcpu_info() vcpu_info(0)


extern unsigned long hypervisor_virt_start;


/* arch/xen/i386/kernel/setup.c */
extern start_info_t *xen_start_info;

#define is_initial_xendomain() 0


/* arch/xen/kernel/evtchn.c */
/* Force a proper event-channel callback from Xen. */
//void force_evtchn_callback(void);

/* arch/xen/kernel/process.c */
//void xen_cpu_idle (void);

/* arch/xen/i386/kernel/hypervisor.c */
void do_hypervisor_callback(struct pt_regs *regs);


#include <hypercall.h>

#define MULTI_UVMFLAGS_INDEX 3
#define MULTI_UVMDOMID_INDEX 4

#define is_running_on_xen() 1

static inline int
HYPERVISOR_yield(
	void)
{
	int rc = HYPERVISOR_sched_op(SCHEDOP_yield, NULL);


	return rc;
}

static inline int
HYPERVISOR_block(
	void)
{
	int rc = HYPERVISOR_sched_op(SCHEDOP_block, NULL);


	return rc;
}

static inline int
HYPERVISOR_shutdown(
	unsigned int reason)
{
	struct sched_shutdown sched_shutdown = {
		.reason = reason
	};

	int rc = HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);

w
	return rc;
}


static inline void
MULTI_update_va_mapping(
    multicall_entry_t *mcl, unsigned long va,
    pte_t new_val, unsigned long flags)
{
    mcl->op = __HYPERVISOR_update_va_mapping;
    mcl->args[0] = va;

    mcl->args[1] = new_val.pte_low;
    mcl->args[2] = 0;

    mcl->args[MULTI_UVMFLAGS_INDEX] = flags;
}


#endif /* __HYPERVISOR_H__ */

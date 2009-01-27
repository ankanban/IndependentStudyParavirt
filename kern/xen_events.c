#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include <hypervisor.h>
#include <xen_events.h>
#include <kdebug.h>

#define active_evtchns(cpu,sh,idx)              \
    ((sh)->evtchn_pending[idx] &                \
     ~(sh)->evtchn_mask[idx])


#define NR_EVS 1024

/* this represents a event handler. Chaining or sharing is not allowed */
typedef struct _ev_action_t {
	evtchn_handler_t handler;
	void *data;
    uint32_t count;
} ev_action_t;

static ev_action_t ev_actions[NR_EVS];
void default_handler(evtchn_port_t port, struct pt_regs *regs, void *data);

static unsigned long bound_ports[NR_EVS/(8*sizeof(unsigned long))];

extern shared_info_t * xen_shared_info;

inline void mask_evtchn(uint32_t port)
{
    shared_info_t *s = xen_shared_info;
    synch_set_bit(port, &s->evtchn_mask[0]);
}

inline void unmask_evtchn(uint32_t port)
{
    shared_info_t *s = xen_shared_info;
    vcpu_info_t *vcpu_info = &s->vcpu_info[smp_processor_id()];

    synch_clear_bit(port, &s->evtchn_mask[0]);

    /*
     * The following is basically the equivalent of 'hw_resend_irq'. Just like
     * a real IO-APIC we 'lose the interrupt edge' if the channel is masked.
     */
    if (  synch_test_bit        (port,    &s->evtchn_pending[0]) && 
         !synch_test_and_set_bit(port>>5, &vcpu_info->evtchn_pending_sel) )
    {
        vcpu_info->evtchn_upcall_pending = 1;
        if ( !vcpu_info->evtchn_upcall_mask )
            force_evtchn_callback();
    }
}

inline void clear_evtchn(uint32_t port)
{
    shared_info_t *s = xen_shared_info;
    synch_clear_bit(port, &s->evtchn_pending[0]);
}


void unbind_all_ports(void)
{
    int i;

    for (i = 0; i < NR_EVS; i++)
    {
        if (test_and_clear_bit(i, bound_ports))
        {
            struct evtchn_close close;
            mask_evtchn(i);
            close.port = i;
            HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
        }
    }
}
  
/*
 * Demux events to different handlers.
 */
int do_event(evtchn_port_t port, struct pt_regs *regs)
{
    ev_action_t  *action;
    if (port >= NR_EVS) {
        printk("Port number too large: %d\n", port);
		goto out;
    }

    action = &ev_actions[port];
    action->count++;

    /* call the handler */
    action->handler(port, regs, action->data);

 out:
    clear_evtchn(port);

    return 1;

}


void do_hypervisor_callback(struct pt_regs *regs)
{
    unsigned long  l1, l2, l1i, l2i;
    unsigned int   port;
    int            cpu = 0;
    shared_info_t *s = xen_shared_info;
    vcpu_info_t   *vcpu_info = &s->vcpu_info[cpu];

   
    vcpu_info->evtchn_upcall_pending = 0;
    /* NB. No need for a barrier here -- XCHG is a barrier on x86. */
    l1 = xchg(&vcpu_info->evtchn_pending_sel, 0);
    while ( l1 != 0 )
    {
        l1i = __ffs(l1);
        l1 &= ~(1 << l1i);
        
        while ( (l2 = active_evtchns(cpu, s, l1i)) != 0 )
        {
            l2i = __ffs(l2);
            l2 &= ~(1 << l2i);

            port = (l1i << 5) + l2i;
			do_event(port, regs);
        }
    }
}



evtchn_port_t bind_evtchn(evtchn_port_t port, evtchn_handler_t handler,
						  void *data)
{
 	if(ev_actions[port].handler != default_handler)
        printk("WARN: Handler for port %d already registered, replacing\n",
				port);

	ev_actions[port].data = data;
	wmb();
	ev_actions[port].handler = handler;

	/* Finally unmask the port */
	unmask_evtchn(port);

	return port;
}

void unbind_evtchn(evtchn_port_t port )
{
	if (ev_actions[port].handler == default_handler)
		printk("WARN: No handler for port %d when unbinding\n", port);
	ev_actions[port].handler = default_handler;
	wmb();
	ev_actions[port].data = NULL;
}

int bind_virq(uint32_t virq, evtchn_handler_t handler, void *data)
{
	evtchn_bind_virq_t op;
	int rc = 0;
	/* Try to bind the virq to a port */
	op.virq = virq;
	op.vcpu = smp_processor_id();

	if ( (rc = HYPERVISOR_event_channel_op(EVTCHNOP_bind_virq, 
					       &op)) != 0 )
	  {
	    printk("Failed to bind virtual IRQ %d\n", virq);
	    return 1;
	  }
	set_bit(op.port,bound_ports);
	bind_evtchn(op.port, handler, data);
	return 0;
}


/*
 * Initially all events are without a handler and disabled
 */
void xen_init_events(void)
{
    int i;

    /* inintialise event handler */
    for ( i = 0; i < NR_EVS; i++ )
	{
        ev_actions[i].handler = default_handler;
        mask_evtchn(i);
    }
}

void default_handler(evtchn_port_t port, struct pt_regs *regs, void *ignore)
{
    printk("[Port %d] - event received\n", port);
}

/* Create a port available to the pal for exchanging notifications.
   Returns the result of the hypervisor call. */

/* Unfortunate confusion of terminology: the port is unbound as far
   as Xen is concerned, but we automatically bind a handler to it
   from inside mini-os. */

int evtchn_alloc_unbound(domid_t pal, evtchn_handler_t handler,
						 void *data, evtchn_port_t *port)
{
    evtchn_alloc_unbound_t op;
    op.dom = DOMID_SELF;
    op.remote_dom = pal;
    int err = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &op);
    if (err)
		return err;
    *port = bind_evtchn(op.port, handler, data);
    return err;
}

/* Connect to a port so as to allow the exchange of notifications with
   the pal. Returns the result of the hypervisor call. */

int evtchn_bind_interdomain(domid_t pal, evtchn_port_t remote_port,
			    evtchn_handler_t handler, void *data,
			    evtchn_port_t *local_port)
{
    evtchn_bind_interdomain_t op;
    op.remote_dom = pal;
    op.remote_port = remote_port;
    int err = HYPERVISOR_event_channel_op(EVTCHNOP_bind_interdomain, &op);
    if (err)
		return err;
    set_bit(op.local_port,bound_ports);
	evtchn_port_t port = op.local_port;
    clear_evtchn(port);	      /* Without, handler gets invoked now! */
    *local_port = bind_evtchn(port, handler, data);
    return err;
}

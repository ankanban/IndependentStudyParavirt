
#ifndef _XEN_EVENTS_H_
#define _XEN_EVENTS_H_

#include <stdint.h>
#include <vmm_page.h>
#include <xen_bitops.h>
#include <hypervisor.h>
#include<xen/event_channel.h>

typedef void (*evtchn_handler_t)(evtchn_port_t, struct pt_regs *, void *);

/* prototypes */
int do_event(evtchn_port_t port, struct pt_regs *regs);
int bind_virq(uint32_t virq, evtchn_handler_t handler, void *data);
evtchn_port_t bind_evtchn(evtchn_port_t port, evtchn_handler_t handler,
						  void *data);
void unbind_evtchn(evtchn_port_t port);
void xen_init_events(void);
int evtchn_alloc_unbound(domid_t pal, evtchn_handler_t handler,
						 void *data, evtchn_port_t *port);
int evtchn_bind_interdomain(domid_t pal, evtchn_port_t remote_port,
							evtchn_handler_t handler, void *data,
							evtchn_port_t *local_port);
void unbind_all_ports(void);

void mask_evtchn(uint32_t port);
void unmask_evtchn(uint32_t port);
void clear_evtchn(uint32_t port);

static inline int notify_remote_via_evtchn(evtchn_port_t port)
{
    evtchn_send_t op;
    op.port = port;
    int rc = HYPERVISOR_event_channel_op(EVTCHNOP_send, &op);
    return rc;
}


#endif /* _XEN_EVENTS_H_ */

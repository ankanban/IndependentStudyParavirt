#ifndef __KERNEL_EXCEPTIONS_H__
#define __KERNEL_EXCEPTIONS_H__

#include <stdint.h>
#include <exception_wrappers.h>

#define PGF_ERR_ACCESS  0x1 /* 0 means the page is not present */
#define PGF_ERR_WR      0x2
#define PGF_ERR_USR     0x4
#define PGF_ERR_RSVD    0x8

#define KEX_DIV 0
#define KEX_DBG 1
#define KEX_NMI 2
#define KEX_BRK 3
#define KEX_OVF 4
#define KEX_BRE 5
#define KEX_INV 6
#define KEX_DEV 7
#define KEX_DBL 8
#define KEX_SEG 11
#define KEX_STK 12
#define KEX_GPF 13
#define KEX_PGF 14


DECL_EXCEPTION_WRAPPER(div);

DECL_EXCEPTION_WRAPPER(dbg);

DECL_EXCEPTION_WRAPPER(ovf);

DECL_EXCEPTION_WRAPPER(inv);

DECL_EXCEPTION_WRAPPER(dbl);

DECL_EXCEPTION_WRAPPER(seg);

DECL_EXCEPTION_WRAPPER(stk);

DECL_EXCEPTION_WRAPPER(gpf);

DECL_EXCEPTION_WRAPPER(pgf);

DECL_EXCEPTION_WRAPPER(generic);

void
exceptions_init(void);

#endif /* __KERNEL_EXCEPTIONS_H__ */

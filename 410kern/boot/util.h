#ifndef _410KERN_MULTIBOOT_UTILS_H_
#define _410KERN_MULTIBOOT_UTILS_H_

#include <boot/multiboot.h>
#include <lmm/lmm.h>

#include <stdint.h>
#include <xen/xen.h>

void mb_util_cmdline(start_info_t *, int *, char ***, char ***);
void mb_util_lmm(start_info_t *, void *, lmm_t *);

#endif

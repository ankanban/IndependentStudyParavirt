###################### LOCAL BUILD TARGETS ############################
410KERNEL_OBJS = entry.o

410KOBJS = $(410KERNEL_OBJS:%=$(410KDIR)/%)
ALL_410KOBJS += $(410KOBJS)

410KLIBS = $(410KERNEL_LIBS:%=$(410KDIR)/%)
410KCLEANS += $(410KDIR)/partial_kernel.o

$(410KDIR)/partial_kernel.o : $(410KOBJS)
	$(LD) -r $(LDFLAGS) -o $@ $^

########################### LIBRARY INCLUSIONS ################################
include $(patsubst lib%.a,$(410KDIR)/%/kernel.mk,$(410KERNEL_LIBS))


########################## STUDENT KERNEL BUILD ###############################

STUKOBJS = $(KERNEL_OBJS:%=$(STUKDIR)/%)
ALL_STUKOBJS += $(STUKOBJS)

STUKCLEANS += $(STUKDIR)/partial_kernel.o

ifeq ($(STUKOBJS),)
$(STUKDIR)/partial_kernel.o :
	touch $@
else
$(STUKDIR)/partial_kernel.o : $(STUKOBJS)
	$(LD) -r $(LDFLAGS) -o $@ $^
endif

################# TARGET-SPECIFIC RULES #################
$(410UDIR)/exec2obj.dep: INCLUDES=$(KINCLUDES) -I-
$(410UDIR)/exec2obj.dep: CFLAGS=-m32 -Wall -Werror

$(410UDIR)/exec2obj: $(410UDIR)/exec2obj.c
	$(CC) -m32 $(KINCLUDES) -I- -Wall -Werror -o $@ $^

ifeq (1,$(DO_INCLUDE_DEPS))
-include $(410UDIR)/exec2obj.dep
endif

410UCLEANS+=$(410UDIR)/exec2obj $(410UDIR)/exec2obj.dep

$(PROGS:%=$(BUILDDIR)/%) :
$(BUILDDIR)/%.strip : $(BUILDDIR)/%
	strip -o $@ $<

.INTERMEDIATE: $(PROGS:%=$(BUILDDIR)/%.strip)

$(BUILDDIR)/user_apps.o: $(410UDIR)/exec2obj $(PROGS:%=$(BUILDDIR)/%.strip)
	( cd $(BUILDDIR); $(PROJROOT)/$(410UDIR)/exec2obj $(PROGS) ) | \
  $(AS) --32 -o $@

include $(410UDIR)/$(UPROGDIR)/progs.mk

include $(patsubst %.a,$(410UDIR)/%/user.mk,$(410USER_LIBS_EARLY) $(410USER_LIBS_LATE))

################# STUDENT LIBRARY RULES #################

STUU_THREAD_OBJS := $(THREAD_OBJS:%=$(STUUDIR)/libthread/%)
ALL_STUUOBJS += $(STUU_THREAD_OBJS)
STUUCLEANS += $(STUUDIR)/libthread.a
$(STUUDIR)/libthread.a: $(STUU_THREAD_OBJS)

STUU_SYSCALL_OBJS := $(SYSCALL_OBJS:%=$(STUUDIR)/libsyscall/%)
ALL_STUUOBJS += $(STUU_SYSCALL_OBJS)
STUUCLEANS += $(STUUDIR)/libsyscall.a
$(STUUDIR)/libsyscall.a: $(STUU_SYSCALL_OBJS)

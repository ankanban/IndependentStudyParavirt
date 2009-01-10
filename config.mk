###########################################################################
#
#    #####          #######         #######         ######            ###
#   #     #            #            #     #         #     #           ###
#   #                  #            #     #         #     #           ###
#    #####             #            #     #         ######             #
#         #            #            #     #         #
#   #     #            #            #     #         #                 ###
#    #####             #            #######         #                 ###
#
#
# Please read the directions in README and in this config.mk carefully.
# Do -N-O-T- just dump things randomly in here until your kernel builds.
# If you do that, you run an excellent chance of turning in something
# which can't be graded.  If you think the build infrastructure is
# somehow restricting you from doing something you need to do, contact
# the course staff--don't just hit it with a hammer and move on.
#
# [Once you've read this message, please edit it out of your config.mk]
###########################################################################



###########################################################################
# This is the include file for the make file.
###########################################################################
# You should have to edit only this file to get things to build.
#

###########################################################################
# The method for acquiring project updates.
###########################################################################
# This should be "afs" for any Andrew machine, "web" for non-andrew machines
# and "offline" for machines with no network access.
#
# "offline" is strongly not recommended as you may miss important project
# updates.
#
UPDATE_METHOD = web

###########################################################################
# WARNING: Do not put extraneous test programs into the REQPROGS variables.
#          Doing so will put your grader in a bad mood which is likely to
#          make him or her less forgiving for other issues.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN
#
# The idle process is a really good thing to keep here.
#
410REQPROGS = idle init shell ck1 fork_wait fork_test1 fork_bomb \
	exec_basic exec_basic_helper halt_test new_pages getpid_test1 \
	print_basic remove_pages_test1 exec_nonexist fork_exit_bomb \
	deschedule_hang cho cho_variant knife loader_test1 \
	loader_test2 make_crash make_crash_helper mem_eat_test \
	mem_permissions merchant peon \
	remove_pages_test2 slaughter \
	sleep_test1 stack_test1 wait_getpid wild_test1 \
	yield_desc_mkrun 

###########################################################################
# Mandatory programs whose source is provided by you
###########################################################################
# A list of the programs in user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN
#
# Leave this blank unless you are writing custom init/idle/shell programs
# (not generally recommended) or temporarily using tweaked versions for
# debugging purposes (in which case please blank this before turning in).
#
STUDENTREQPROGS =

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# ignored.  Your kernel MUST RUN WITHOUT THEM.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory
#
410TESTS =

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory
#
STUDENTTESTS = 

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P3" we give you can't build libthrgrp.a.  Once you install your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
#410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS =  fork.o exec.o set_status.o vanish.o 			\
		wait.o task_vanish.o 					\
		gettid.o yield.o cas2i_runflag.o get_ticks.o sleep.o 	\
		new_pages.o remove_pages.o 				\
		getchar.o readline.o print.o 				\
		set_term_color.o get_cursor_pos.o			\
		set_cursor_pos.o  ls.o halt.o misbehave.o

###########################################################################
# Parts of your kernel
###########################################################################
#
# Kernel object files you want included from 410kern/
#
410KERNEL_OBJS = load_helper.o

#
# Kernel object files you provide in from kern/
#
KERNEL_OBJS = handler_install.o console.o keyboard.o timer.o 		\
		keyboard_wrapper.o timer_wrapper.o kernel_timer.o 	\
		kernel_exceptions.o exception_wrappers.o        	\
		vmm_zone.o vmm_page.o vm_area.o vmm.o file.o 		\
		task.o thread.o context_switch.o kernel_asm.o 		\
		usercopy.o kernel_syscall.o syscall_wrappers.o 		\
		mm.o consoleio.o sysmisc.o 				\
		loader.o malloc_wrappers.o 				\
		sched.o process.o lock.o kernel.o 

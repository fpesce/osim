CLEANFILES=checks_log.xml *.gcda *.gcov *.gcno check_log.xml checks-debug.log mod_wsso-check_result.xml *~
MAINTAINERCLEANFILES=Makefile.in

TESTS=check_osim
check_PROGRAMS=check_osim
check_osim_SOURCES=check_osim.c check_os_conf.c check_os_fleet.c check_os_parse.c\
		   ../src/os_conf.c ../include/os_conf.h \
		   ../src/os_fleet.c ../include/os_fleet.h \
		   ../src/napr_galife.c ../include/napr_galife.h \
		   ../src/napr_threadpool.c ../include/napr_threadpool.h \
		   ../src/napr_heap.c ../include/napr_heap.h \
		   ../src/os_parse.c ../include/os_parse.h

# -fno-inline to ease the debuging
check_osim_LDADD= @CHECK_LIBS@ -lefence
INCLUDES=-I$(top_srcdir)/include/
check_osim_CFLAGS= -Wall -g @CHECK_CFLAGS@ @APR_CFLAGS@
# -mtune=pentium-m -march=pentium-m -O3 -g -fstrength-reduce -frerun-loop-opt -fexpensive-optimizations -fschedule-insns2 -funroll-loops -fomit-frame-pointer -malign-double -fno-strict-aliasing -pipe -falign-loops=2 -falign-functions=2 -DCPU=686 -ffast-math
#
check_osim_CPPFLAGS= @APR_CPPFLAGS@ -DUNITTEST -DCHECKS_DIR=\"$(top_srcdir)/checks\" -DCONF_DIR=\"$(top_srcdir)/etc/\" 
check_osim_LDFLAGS= @APR_LIBS@

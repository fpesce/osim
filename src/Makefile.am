CLEANFILES= *~
MAINTAINERCLEANFILES=Makefile.in

bin_PROGRAMS=osim

noinst_HEADERS = ../include/debug.h \
		 ../include/os_conf.h \
		 ../include/os_fleet.h \
		 ../include/napr_galife.h \
		 ../include/napr_heap.h \
		 ../include/os_parse.h \
		 ../include/napr_threadpool.h

osim_SOURCES = osim.c \
	       os_conf.c \
	       os_fleet.c \
	       os_parse.c \
	       napr_galife.c \
	       napr_heap.c \
	       napr_threadpool.c

osim_LDFLAGS = @APR_LTLIBS@
osim_CPPFLAGS = @APR_CPPFLAGS@

osim_CFLAGS = @APR_CFLAGS@ -I./ -I$(top_srcdir)/include -Wall -g -O3 -funroll-loops -fomit-frame-pointer -pipe -ffast-math

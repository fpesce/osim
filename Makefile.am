AUTOMAKE_OPTIONS = foreign
EXTRA_DIST= guess.sh simulate.sh script.sh

DISTCHECK_CONFIGURE_FLAGS = "--with-apr-config=@apr_config@"

## Process this file with automake to produce Makefile.in
SUBDIRS = src $(CHECKS_DIR) 
DIST_SUBDIRS = src checks etc

extraclean: clean
	@rm -fr libtool cscope.out configure config.status config.log autom4te.cache aclocal.m4 Makefile.in Makefile Doxyfile missing install-sh depcomp compile tags config.sub config.guess ltmain.sh
## For doxygen building
dox: Doxyfile
if HAVE_DOXYGEN
	@echo "Running doxygen..."
	@doxygen
else
	@echo "doxygen was not found during configure.  Aborting."
endif


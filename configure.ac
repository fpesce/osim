AC_PREREQ(2.52)

# Required initializer
AC_INIT([osim], m4_normalize(m4_include([VERSION])), [francois.pesce@gmail.com])

# Automake initialization
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

dnl can autoconf find the source ?
AC_CONFIG_SRCDIR([src/osim.c])
AC_CONFIG_HEADER([include/config.h])

# Add a test for a compiler.
AC_HEADER_STDC

AC_PROG_CC

AM_PROG_LIBTOOL

AM_PROG_CC_C_O

# Check lib math
AC_CHECK_LIB(m, ceilf, LIBS="$LIBS -lm")

# Now call the APR_CONFIG_CHECK macro that was just specified
APR_CONFIG_CHECK

# Allow running test suites if check was found on system
UNITTEST_CHECK

# Checking doxygen argument and presence
DOXYGEN_CHECK

# Allow running profiling if gprof was found on system
GPROF_CHECK

# perl compatible regex
PCRE_CHECK
USER_CFLAGS=$CFLAGS
CFLAGS=""
AC_SUBST(USER_CFLAGS)

PWD=`pwd`
ABS_TOP_SRCDIR=$PWD$top_srcdir
AC_SUBST(ABS_TOP_SRCDIR)

case $host in
x86_64-*-*) CFLAGS="-march=nocona -mfpmath=sse,387 -DCPU=64" ;;
i686-*-*)   CFLAGS="-march=i686 -malign-double -DCPU=686" ;;
*)          CFLAGS= ;;
esac

# Write config.status and the Makefile
AC_CONFIG_FILES([Doxyfile Makefile src/Makefile checks/Makefile etc/Makefile])
AC_OUTPUT

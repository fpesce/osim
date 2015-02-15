dnl
#
# PCRE_CHECK
#
AC_DEFUN([PCRE_CHECK],[
	AC_ARG_WITH( pcre, AC_HELP_STRING([--with-pcre=PATH], [prefix where libpcre is installed default=auto]), [pcre_dir=$withval],[pcre_dir=])
	if test "x$pcre_dir" != "x"
	    then
	    # If we passed a pcre dir 
	    CFLAGS="$CFLAGS -I$pcre_dir/include"
	    CPPFLAGS="$CPPFLAGS -I$pcre_dir/include"
	    LDFLAGS="$LDFLAGS -L$pcre_dir/lib"
	    LIBS="$LIBS -lpcre"
	else
	    # else check pcre install with pcre-config
	    AC_PATH_PROG(PCRE_CONFIG, pcre-config)
	    if test "x$PCRE_CONFIG" != "x"
		then
		pcre_cflags=`$PCRE_CONFIG --cflags`
		pcre_libs=`$PCRE_CONFIG --libs`
		CFLAGS="$CFLAGS $pcre_cflags"
		CPPFLAGS="$CPPFLAGS $pcre_cflags"
		LIBS="$LIBS $pcre_libs"
	    else
		AC_MSG_ERROR([pcre-config program not found, please make sure you installed devel files for libpcre])
	    fi
	fi

	#
	# Make sure we have "pcre.h".  If we don't, it means we probably
	# don't have libpcre, so don't use it.
	#
	AC_CHECK_HEADER(pcre.h,, [
	    if test "x$pcre_dir" != "x"
	    then
		AC_MSG_ERROR([$pcre_dir not found. Check the value you specified with --with-pcre])
	    else
		AC_MSG_ERROR([lib pcre not found on the system, you need to specify the pcre directory using --with-pcre])
	    fi
	    ])

	# Trivial compilation test
	AC_CHECK_LIB(pcre, pcre_compile,
	    [],
	    [
	    AC_MSG_ERROR([failed to compile a pcre test program, lib pcre not found.])
	    ])
    ])

# Define a macro that is used to parse a --with-apr parameter
# The macro is named "APR_CONFIG_CHECK"
AC_DEFUN([APR_CONFIG_CHECK],[
	AC_ARG_WITH( apr-config, AC_HELP_STRING([--with-apr-config=PATH], [prefix where libapr is installed default=auto]), [apr_config=$withval],[apr_config=])
	if test "x$apr_config" != "x"
	    then
	    # If we passed a apr dir 
	    if test -f $apr_config
	        then
	        APR_CFLAGS="`$apr_config --cflags` -DHAVE_APR" 
	        APR_CPPFLAGS="`$apr_config --cppflags --includes`"
	        APR_LTLIBS="`$apr_config --libs --link-libtool`"
	        APR_LIBS="`$apr_config --libs --link-ld`"
	    else
		AC_MSG_ERROR([apr-config program not found (1), please make sure you installed devel files for libapr])
	    fi
	else
	    # else check apr install with apr-config
	    AC_PATH_PROG(apr_config, apr-config)
	    if test "x$apr_config" != "x"
		then
		APR_CFLAGS="`$apr_config --cflags` -DHAVE_APR" 
		APR_CPPFLAGS="`$apr_config --cppflags --includes`"
		APR_LTLIBS="`$apr_config --libs --link-libtool`"
		APR_LIBS="`$apr_config --libs --link-ld`"
	    else
		AC_MSG_ERROR([apr-config program not found (2), please make sure you installed devel files for libapr])
	    fi
	fi
	AC_SUBST([apr_config])
	AC_SUBST([APR_CFLAGS])
	AC_SUBST([APR_CPPFLAGS])
	AC_SUBST([APR_LTLIBS])
	AC_SUBST([APR_LIBS])

    ])

AC_DEFUN([DOXYGEN_CHECK],[
	AC_ARG_ENABLE(doxygen, AC_HELP_STRING([--disable-doxygen], [disable documentation with doxygen (default=yes)])
			      ,,[enable_doxygen="yes"])
	AC_ARG_ENABLE(dot, AC_HELP_STRING([--disable-dot], [disable graphs in doxygen via dot (default=yes)])
			      ,,[enable_dot="yes"])
        
        if test "x$enable_doxygen" = xyes; then
           AC_CHECK_PROG(DOXYGEN, doxygen, yes, no)
	   if test x$DOXYGEN = xno; then
		AC_MSG_WARN([*** doxygen not found, docs will not be available])
	        enable_doxygen=no
           else
           	AC_DEFINE_UNQUOTED(HAVE_DOXYGEN, 1, [whether or not we have doxygen])

           	if test "x$enable_dot" = xyes; then
	        	AC_CHECK_PROG(DOT, dot, yes, no)

                	if test $DOT = no; then
			   enable_dot=no;
			   AC_MSG_WARN([*** dot not found, graphs will not be available. Please install graphviz wich includes dot])
			else
			   AC_DEFINE_UNQUOTED(HAVE_DOT, 1, [whether or not we have dot])
			fi
                else
			AC_MSG_WARN([*** dot not found, graphs will not be available])
                fi
           fi
        else
           enable_dot="no"
        fi

        if test "x$enable_doxygen" = xyes; then
          AM_CONDITIONAL(HAVE_DOXYGEN, true)
        else
          AM_CONDITIONAL(HAVE_DOXYGEN, false)
        fi

        AC_SUBST(enable_doxygen)
        AC_SUBST(enable_dot)
])

# Compile and run tests suites if check found
AC_DEFUN([UNITTEST_CHECK],[
	m4_ifdef([AM_PATH_CHECK],[ AM_PATH_CHECK(0.9.2, [check_found=true], [check_found=false])])
	if test x$check_found = xtrue; then
		CHECKS_DIR="checks"
		AC_SUBST(CHECKS_DIR)
	else 
		if test x$check_found = xfalse; then
			AC_MSG_WARN([*** Invalid check version, you can download the latest one at http://check.sf.net])
		else
			AC_MSG_WARN([*** Check not found, you can download the latest version at http://check.sf.net])
		fi
	fi	
])

AC_DEFUN([EXPAT_CHECK], [
	AC_ARG_WITH(expat,
		AC_HELP_STRING([--with-expat=PATH],[prefix where libexpat is installed default=auto]),, with_expat=yes)

	if test with_expat = no; then
		AC_MSG_ERROR([library Expat is mandatory for XML parsing functions])	
	fi

	EXPAT_CFLAGS=
	EXPAT_LDFLAGS=
	EXPAT_LIBS=
	if [ test $with_expat != yes && test x$with_expat != x ]; then
		EXPAT_CFLAGS="-I$with_expat/include"
		EXPAT_LDFLAGS="-L$with_expat/lib"
	fi
	AC_CHECK_LIB(expat, XML_ParserCreate,
		     [ EXPAT_LIBS="-lexpat"
		       expat_found=yes ],
		     [ expat_found=no ],
		     [ $EXPAT_LDFLAGS $EXPAT_LIBS ])
	if test $expat_found = no; then
		AC_MSG_ERROR([Could not find the Expat library])
	fi

	CFLAGS="$CFLAGS $EXPAT_CFLAGS"
	CXXFLAGS="$CXXFLAGS $EXPAT_CFLAGS";

	AC_CHECK_HEADERS(expat.h, , expat_found=no)
	if test $expat_found = no; then
		AC_MSG_ERROR([Could not find expat.h])
	fi

       	LIBS="$LIBS $EXPAT_LIBS";
       	LDFLAGS="$LDFLAGS $EXPAT_LDFLAGS";
])

# Compile and allow linking to gprof if it is found
AC_DEFUN([GPROF_CHECK],[
	AC_ARG_ENABLE(gprof, AC_HELP_STRING([--enable-gprof], [enable linking to gprof library in order to profile (default=no)])
	    ,[enable_gprof="yes"],)

	if test "x$enable_gprof" = xyes; then
	LIBS="$LIBS  -g -pg";
	CFLAGS="$CFLAGS -DHAVE_GPROF -g -pg";
	AC_DEFINE(HAVE_GPROF, [], [enabled if compiled with -pg])
	fi

	AC_SUBST(enable_gprof)
	])




dnl Copyright (c) 1998 N. D. Bellamy

AC_DEFUN(AC_CHECK_SOCKS5,
[
  AC_MSG_CHECKING(whether to enable SOCKS5 support)

  WITH_SOCKS5="no"
  SOCKS_LIBS=""
  SOCKS_LIBDIR=""
  SOCKS_INCDIR=""
	
  AC_ARG_ENABLE(
    socks5,
    [  --enable-socks5         enable SOCKS5 firewall support],
    WITH_SOCKS5=yes)

  AC_ARG_WITH(
    socks5-inc,
    [  --with-socks5-inc=PATH  include path for SOCKS5 headers],
    socks_incdir="$withval", socks_incdir="no")
  
  AC_ARG_WITH(
    socks5-lib,
    [  --with-socks5-lib=PATH  library path for SOCKS5 libraries],
    socks_libdir="$withval", socks_libdir="no")

  if test "$WITH_SOCKS5" = "no"; then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)

    if test "$socks_libdir" = "no"; then
	AC_CHECK_LIB(socks5, SOCKSconnect, SOCKS_LIBS="-lsocks5")
    else
    	AC_MSG_CHECKING(where to look for the SOCKS5 library)
	SOCKS_LIBS="-lsocks5"
	SOCKS_LIBDIR="-L$socks_libdir"
	AC_MSG_RESULT($socks_libdir)
    fi

    if test "$socks_incdir" = "no"; then
        AC_CHECK_HEADER(socks.h)
    else
    	AC_MSG_CHECKING(where to look for the SOCKS5 headers)
	SOCKS_INCDIR="-I$socks_incdir"
	AC_MSG_RESULT($socks_incdir)
    fi
    AC_DEFINE(USE_SOCKS5)
  fi

  dnl Substitute these even if they're null, so as not to mess up makefiles
  
  AC_SUBST(SOCKS_LIBS)
  AC_SUBST(SOCKS_LIBDIR)
  AC_SUBST(SOCKS_INCDIR)
])

dnl Check if it is possible to turn off run time type information (RTTI)
AC_DEFUN(AC_PROG_CXX_FNO_RTTI,
[AC_CACHE_CHECK(whether ${CXX-g++} accepts -fno-rtti, ac_cv_prog_cxx_fno_rtti,
[echo 'void f(){}' > conftest.cc
if test -z "`${CXX-g++} -fno-rtti -c conftest.cc 2>&1`"; then
  ac_cv_prog_cxx_fno_rtti=yes
  CXXFLAGS="${CXXFLAGS} -fno-rtti"
else
  ac_cv_prog_cxx_fno_rtti=no
fi
rm -f conftest*
])])

dnl Check if the type socklen_t is defined anywhere
AC_DEFUN(AC_C_SOCKLEN_T,
[AC_CACHE_CHECK(for socklen_t, ac_cv_c_socklen_t,
[ AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
socklen_t foo;
],[
  ac_cv_c_socklen_t=yes
],[
  ac_cv_c_socklen_t=no
  AC_DEFINE(socklen_t, int)
])])])

dnl Check for sys_errlist[] and sys_nerr, check for declaration
dnl Check nicked from aclocal.m4 used by GNU bash 2.01
AC_DEFUN(AC_SYS_ERRLIST,
[AC_MSG_CHECKING([for sys_errlist and sys_nerr])
AC_CACHE_VAL(ac_cv_sys_errlist,
[AC_TRY_LINK([#include <errno.h>],
[extern char *sys_errlist[];
 extern int sys_nerr;
 char *msg = sys_errlist[sys_nerr - 1];],
    ac_cv_sys_errlist=yes, ac_cv_sys_errlist=no)])dnl
AC_MSG_RESULT($ac_cv_sys_errlist)
if test $ac_cv_sys_errlist = yes; then
AC_DEFINE(HAVE_SYS_ERRLIST)
fi
])

AC_PREREQ(2.57)

AC_INIT([compiz-bcop], [0.8.14], [onestone@compiz.org])

AM_INIT_AUTOMAKE([1.9 foreign dist-xz no-dist-gzip check-news])
AC_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC

AC_C_BIGENDIAN

PKG_CHECK_MODULES(LIBXSLT, libxslt)

xsltdir=$datadir/bcop
AC_SUBST(xsltdir)

AC_OUTPUT([
bcop.pc
Makefile
src/Makefile
src/bcop
])

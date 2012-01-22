#                                               -*- Autoconf -*-C_PROG_CC
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([surveillance], [0.3.1], [azman0101@hotmail.com], [surveillance],[https://github.com/azman0101/xproj103])
AC_CONFIG_SRCDIR([src/server.c],[src/client.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AM_PROG_CC_C_O
# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS(unistd.h proc/devname.h proc/wchan.h proc/procps.h proc/readproc.h proc/escape.h proc/sig.h proc/sysinfo.h proc/version.h proc/whattime.h,[],AC_MSG_ERROR([required header file missing]))
# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT


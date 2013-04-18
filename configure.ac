#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
# Version of the code
m4_define([fox_cairo_major], [1])
m4_define([fox_cairo_minor], [0])
m4_define([fox_cairo_level], [0])
m4_define([fox_cairo_version],[fox_cairo_major.fox_cairo_minor.fox_cairo_level])
AC_INIT([fox_cairo], [fox_cairo_version], [hardy.woodland.cypress@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

FOX_CAIRO_MAJOR_VERSION=fox_cairo_major
FOX_CAIRO_MINOR_VERSION=fox_cairo_minor
FOX_CAIRO_PATCH_LEVEL=fox_cairo_level
FOX_CAIRO_INTERFACE_VERSION=0
FOX_CAIRO_BINARY_VERSION=0

# Report what was found
AC_MSG_CHECKING(major version)
AC_MSG_RESULT($FOX_CAIRO_MAJOR_VERSION)
AC_MSG_CHECKING(minor version)
AC_MSG_RESULT($FOX_CAIRO_MINOR_VERSION)
AC_MSG_CHECKING(patch level)
AC_MSG_RESULT($FOX_CAIRO_PATCH_LEVEL)

# Substitute variables in output files
AC_SUBST(FOX_CAIRO_MAJOR_VERSION)
AC_SUBST(FOX_CAIRO_MINOR_VERSION)
AC_SUBST(FOX_CAIRO_PATCH_LEVEL)

LT_INIT

# CURRENT:  Most recent interface implemented by library
# REVISION: The implementation number of the CURRENT interface
# AGE:      The difference between the newest and oldest interfaces that this
#           library implements.  The library implements all the interface numbers
#           in the range from number CURRENT - AGE to CURRENT.
LT_RELEASE=$FOX_CAIRO_MAJOR_VERSION.$FOX_CAIRO_MINOR_VERSION
LT_CURRENT=$FOX_CAIRO_INTERFACE_VERSION
LT_REVISION=`expr $FOX_CAIRO_PATCH_LEVEL - $FOX_CAIRO_INTERFACE_VERSION`
LT_AGE=`expr $FOX_CAIRO_INTERFACE_VERSION - $FOX_CAIRO_BINARY_VERSION`

# Substitute variables in output files
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Check using C++
AC_LANG(C++)

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([rint])

FOX_CFLAGS=`fox-config --cflags`
FOX_LIBS=`fox-config --libs`

LDFLAGS="-no-undefined ${LDFLAGS}"

# Debugging turned on
AC_MSG_CHECKING(for debugging)
AC_ARG_ENABLE(debug,[  --enable-debug          compile for debugging])
AC_MSG_RESULT([$enable_debug])
if test "x$enable_debug" = "xyes" ; then
  CXXFLAGS="-g -DDEBUG"
fi

# Building for release
AC_MSG_CHECKING(for release build)
AC_ARG_ENABLE(release,[  --enable-release        compile for release])
AC_MSG_RESULT([$enable_release])
if test "x$enable_release" = "xyes" ; then
  CXXFLAGS="${CXXFLAGS} -O2 -DNDEBUG"
  if test "${GXX}" = "yes" ; then
    CXXFLAGS="${CXXFLAGS} -Wuninitialized -ffast-math -fstrict-aliasing -finline-functions -fomit-frame-pointer -fexpensive-optimizations"
    LDFLAGS="-s ${LDFLAGS}"
  fi
fi

# Enable profiling
AC_MSG_CHECKING(for profiling)
AC_ARG_WITH(profiling,[  --with-profiling        choices are yes, no, or gprof])
AC_MSG_RESULT([$with_profiling])
if test "x$with_profiling" = "xyes" ; then
CXXFLAGS="${CXXFLAGS} -p"
elif test "x$with_profiling" = "xgprof" ; then
CXXFLAGS="${CXXFLAGS} -pg"
fi

AC_MSG_CHECKING(for libcairo2)
AC_CHECK_HEADER(cairo/cairo.h,AC_DEFINE([HAVE_CAIRO_H], 1, [Define if cairo header exists.]))
if test "x$ac_cv_header_cairo_cairo_h" = "xyes"; then
  USE_CAIRO=1
  CAIRO_CFLAGS=`pkg-config --cflags cairo`
  CAIRO_LDFLAGS=`pkg-config --libs cairo`
  PANGO_CFLAGS=`pkg-config --cflags pangocairo`
  PANGO_LDFLAGS=`pkg-config --libs pangocairo`
fi


AC_SUBST(FOX_CFLAGS)
AC_SUBST(FOX_LIBS)
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LDFLAGS)
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LDFLAGS)

AC_CONFIG_FILES([Makefile
                 fox-cairo-config
                ])
AC_OUTPUT

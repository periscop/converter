dnl
dnl   /*------------------------------------------------------------------+
dnl    |                             Converter                            |
dnl    |------------------------------------------------------------------|
dnl    |                          autoconf.ac                             |
dnl    |------------------------------------------------------------------|
dnl    |                   First version: 18/10/2012                      |
dnl    +------------------------------------------------------------------+
dnl
dnl +------------------------------------------------------------------------+
dnl |                                                                        |
dnl |                                 Converter [osl <-> scoplib]            |
dnl |                                                                        |
dnl +------------------------------------------------------------------------+
dnl | Copyright (C) 2012 University of Paris-Sud                             |
dnl |                                                                        |
dnl | This library is free software; you can redistribute it and/or modify   |
dnl | it under the terms of the GNU Lesser General Public License as         |
dnl | published by the Free Software Foundation; either version 2.1 of the   |
dnl | License, or (at your option) any later version.                        |
dnl |                                                                        |
dnl | This library is distributed in the hope that it will be useful but     |
dnl | WITHOUT ANY WARRANTY; without even the implied warranty of             |
dnl | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       |
dnl | Lesser General Public License for more details.                        |
dnl |                                                                        |
dnl | You should have received a copy of the GNU Lesser General Public       |
dnl | License along with this software; if not, write to the Free Software   |
dnl | Foundation, Inc., 51 Franklin Street, Fifth Floor,                     |
dnl | Boston, MA  02110-1301  USA                                            |
dnl |                                                                        |
dnl | Clay, the Chunky Loop Alteration wizardrY                              |
dnl | Written by Cedric Bastoul, Cedric.Bastoul@u-psud.fr                    |
dnl +------------------------------------------------------------------------*/
dnl
dnl Input file for autoconf to build a configuration shellscript.

m4_define([version_major], [0])
m4_define([version_minor], [0])
m4_define([version_revision], [1])

AC_PREREQ(2.53)
dnl Fill here the @bug email adress.
AC_INIT([converter], [version_major.version_minor.version_revision],
        [taj.khan@inria.fr])
dnl A common file, which serve as a test.
AC_CONFIG_SRCDIR([include/converter/converter.h])
dnl Put as most as possible configuration files to an auxialiry
dnl directory.
AC_CONFIG_AUX_DIR(autoconf)
AC_CONFIG_MACRO_DIR([m4])

dnl Initialize automake.
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])


dnl /**************************************************************************
dnl  *                              Checking                                  *
dnl  **************************************************************************/


dnl Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CHECK_PROG(CD, cd)
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LEX
AC_PROG_YACC
AC_CHECK_PROGS(DOXYGEN,doxygen,doxygen)
  
AX_CC_MAXOPT
AC_SUBST(CFLAGS_WARN)
AX_CFLAGS_WARN_ALL(CFLAGS_WARN)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h stddef.h stdlib.h string.h strings.h unistd.h])

dnl Checks for library functions.
AC_CHECK_FUNCS(strtol)


dnl /**************************************************************************
dnl  *                             Option setting                             *
dnl  **************************************************************************/

dnl /**************************************************************************
dnl  *                            Where is GMP?                               *
dnl  **************************************************************************/

gmp_flag="OSL_GMP_IS_HERE"
AX_SUBMODULE(gmp,no|system|build,system)

case "$with_gmp" in
build)
    CPPFLAGS="-D$gmp_flag -I$with_gmp_builddir $CPPFLAGS"
    LDFLAGS="-L$with_gmp_builddir/$lt_cv_objdir $LDFLAGS"
    ;;
system)
    CPPFLAGS="-D$gmp_flag $CPPFLAGS"
    if test "x$with_gmp_prefix" != "x"; then
        CPPFLAGS="-I$with_gmp_prefix/include $CPPFLAGS"
    fi

    if test "$with_gmp_exec_prefix" != "yes" ; then
        LDFLAGS="-L$with_gmp_exec_prefix/lib $LDFLAGS"
    fi
    ;;
esac
case "$with_gmp" in
build|system)
    AC_CHECK_HEADER(gmp.h,
                    [],
                    [AC_MSG_ERROR("Can't find gmp headers.")])
    AC_CHECK_LIB(gmp,
                 __gmpz_init,
                 [LIBS="$LIBS -lgmp"],
                 [AC_MSG_ERROR("Can't find gmp library.")])
    ;;
esac


dnl /**************************************************************************
dnl  *                   Where is the OpenScop Library?                       *
dnl  **************************************************************************/

AX_SUBMODULE(osl,system|build|bundled,system)

AC_SUBST(OSL_CPPFLAGS)
AC_SUBST(OSL_LDFLAGS)
AC_SUBST(OSL_LIBS)
case "$with_osl" in
bundled)
	OSL_CPPFLAGS="-I$srcdir/osl/include -Iosl/include"
	OSL_LIBS="$srcdir/osl/source/libosl.la"
	;;
build)
	OSL_CPPFLAGS="-I$osl_srcdir/include -I$with_osl_builddir/include"
	OSL_LIBS="$with_osl_builddir/libosl.la"
	;;
system)
	if test "x$with_osl_prefix" != "x"; then
		OSL_CPPFLAGS="-I$with_osl_prefix/include"
	fi
	if test "x$with_osl_exec_prefix" != "x"; then
		OSL_LDFLAGS="-L$with_osl_exec_prefix/lib"
	fi
	OSL_LIBS="$with_osl_exec_prefix/lib/libosl.la -losl"
esac
AM_CONDITIONAL(BUNDLED_OSL, test $with_osl = bundled)


dnl /**************************************************************************
dnl  *                   Where is the Scoplib Library?                       *
dnl  **************************************************************************/

AX_SUBMODULE(scoplib,system|build|bundled,system)

AC_SUBST(SCOPLIB_CPPFLAGS)
AC_SUBST(SCOPLIB_LDFLAGS)
AC_SUBST(SCOPLIB_LIBS)
case "$with_scoplib" in
bundled)
	SCOPLIB_CPPFLAGS="-I$srcdir/scoplib/include -Iscoplib/include"
	SCOPLIB_LIBS="$srcdir/scoplib/source/libscoplib.la"
	;;
build)
	SCOPLIB_CPPFLAGS="-I$scoplib_srcdir/include -I$with_scoplib_builddir/include"
	SCOPLIB_LIBS="$with_scoplib_builddir/libscoplib.la"
	;;
system)
	if test "x$with_scoplib_prefix" != "x"; then
		SCOPLIB_CPPFLAGS="-I$with_scoplib_prefix/include"
	fi
	if test "x$with_scoplib_exec_prefix" != "x"; then
		SCOPLIB_LDFLAGS="-L$with_scoplib_exec_prefix/lib"
	fi
	SCOPLIB_LIBS="$with_scoplib_exec_prefix/lib/libscoplib.la -lscoplib"
esac
AM_CONDITIONAL(BUNDLED_SCOPLIB, test $with_scoplib = bundled)

dnl /**************************************************************************
dnl  *                            Substitutions                               *
dnl  **************************************************************************/


dnl Substitutions to do.
AC_SUBST(ac_aux_dir)
AC_SUBST(abs_top_srcdir)

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

dnl Configure Makefiles.
AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	doc/Doxyfile
	include/converter/macros.h
	],
	[test -z "$CONFIG_HEADERS" || echo timestamp > source/stamp-h.in])

if test $with_osl = bundled; then
	AC_CONFIG_SUBDIRS(osl)
fi

AC_OUTPUT

echo "             /*-----------------------------------------------+"
echo "              | Converter [osl<->scoplib]configuration is OK  |"
echo "              +-----------------------------------------------*/"
echo "It appears that your system is OK to start Conveter compilation. You need"
echo "now to type \"make\". Lastly type \"make install\" to install Clay on"
echo "your system (log as root if necessary)."

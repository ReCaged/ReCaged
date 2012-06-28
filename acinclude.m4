#
# ReCaged - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2009, 2010, 2011, 2012 Mats Wahlberg
#
# This file is part of ReCaged.
#
# ReCaged is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ReCaged is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
#

#
#Custom macros for checking some common (but not OS level) libraries.
#Some inspiration taken from the "PKG_*" macros
#

#
# RC_LIBS_CONFIG()
# Basic tests before starting
#
AC_DEFUN([RC_LIBS_INIT],
[

#using... "w32"?
AC_MSG_CHECKING(if using that crappy OS)
AC_COMPILE_IFELSE( [AC_LANG_SOURCE([
	#ifdef _WIN32
		you fail
	#endif
	])], [ ON_W32=no ], [ ON_W32=yes ] )
AC_MSG_RESULT($ON_W32)

#if so, probably wanting static linking?
AC_ARG_ENABLE(
	[w32static],
	[AS_HELP_STRING([--enable-w32static],
			[Link libraries statically on W32 @<:@default=no@:>@])],
	[STATIC="$enableval"],
	[STATIC="no"] )

#and console output?
AC_ARG_ENABLE(
	[w32console],
	[AS_HELP_STRING([--enable-w32console],
			[Enable console output on W32 @<:@default=no@:>@])],
	[CONSOLE="$enableval"],
	[CONSOLE="no"] )

#pkg-config might exist?
AC_PATH_TOOL([PKG_CONFIG], [pkg-config])

])


#
# RC_LIBS_CHECK(PKG_NAME, CUSTOM_CONFIG, LIB_HEADER, LIB_NAMES_LIST)
# Check for specified library using pkg_config and other if possible
#

AC_DEFUN([RC_LIBS_CHECK],
[

FAILED="yes"

if test "$PKG_CONFIG"; then
	AC_MSG_CHECKING([for $1 usig pkg-config])

	if test "$STATIC" = "no"; then
		TMP_FLAGS=$($PKG_CONFIG --cflags $1 2>/dev/null)
		TMP_LIBS=$($PKG_CONFIG --libs $1 2>/dev/null)
	else
		TMP_FLAGS=$($PKG_CONFIG --cflags --static $1 2>/dev/null)
		TMP_LIBS=$($PKG_CONFIG --libs --static $1 2>/dev/null)
	fi
		

	if test "$TMP_FLAGS" || test "$TMP_LIBS"; then
		AC_MSG_RESULT([yes]);
		RC_FLAGS="$RC_FLAGS $TMP_FLAGS"
		RC_LIBS="$RC_LIBS $TMP_LIBS"
		FAILED="no"
	else
		AC_MSG_RESULT([no])
		FAILED="yes"
	fi
fi

#failed, and got fallback (if exists)
if test "$FAILED" = "yes" && test "$2"; then
	AC_PATH_TOOL($1_CONFIG, [$2])
	TMP_CONFIG="$$1_CONFIG" #note: "$1" gets expanded first, before the whole string

	if test TMP_CONFIG; then
		AC_MSG_CHECKING([for $1 using fallback to $2])

		TMP_FLAGS=$($TMP_CONFIG --cflags 2>/dev/null)

		#little exception: if sdl-config&static, use "--static-libs" instead
		if test "$STATIC" != "no" && test "$1" = "sdl"; then
			TMP_LIBS=$($TMP_CONFIG --static-libs 2>/dev/null)
		else
			TMP_LIBS=$($TMP_CONFIG --libs 2>/dev/null)
		fi


		if test "$TMP_FLAGS" || test "$TMP_LIBS"; then
			AC_MSG_RESULT([yes]);
			RC_FLAGS="$RC_FLAGS $TMP_FLAGS"
			RC_LIBS="$RC_LIBS $TMP_LIBS"
			FAILED="no"
		else
			AC_MSG_RESULT([no])
			FAILED="yes"
		fi
	fi
fi

#final fallback
if test "$FAILED" = "yes"; then
	AC_MSG_WARN([Attempting to guess files for $1 using ac_check_* macros])
	if test "$1" = "ode"; then
		AC_MSG_WARN([Quite Critical (ODE): Don't know if using double or single precision!... Assuming single...])
		CPPFLAGS="$CPPFLAGS -DdSINGLE" #hackish: append to user variable, but should be fine...?
	fi

	AC_CHECK_HEADER([$3],, [ AC_MSG_ERROR([Headers for $1 appears to be missing, install lib$1-dev or similar]) ])

	#why not "ac_search_libs"? because can only test for "main" in gl on windows
	#(for some reason). And -search- quits upon finding "main" that already exists
	#(LIBNAME expands to actual name, store in variable TRYLIB)
	m4_foreach_w([LIBNAME], [$4], [
		if test "$FAILED" = "yes"; then
			TRYLIB="LIBNAME"
			AC_CHECK_LIB([$TRYLIB], [main], [
				FAILED="no"
				RC_LIBS="$RC_LIBS -l$TRYLIB" ])
		fi ])

	#still nothing?
	if test "$FAILED" = "yes"; then
		AC_MSG_ERROR([Library $1 appears to be missing, install lib$1 or similar])
	fi

fi

])


#
# RC_LIBS()
# Check for needed libs
#

AC_DEFUN([RC_LIBS_CONFIG],
[
AC_REQUIRE([RC_LIBS_INIT])

#make sure only enabling w32 options on w32
if test "$ON_W32" = "no"; then
	STATIC="no"
	CONSOLE="no"
fi

#console flag
AC_MSG_CHECKING([if building with w32 console enabled])
if test "$CONSOLE" != "no"; then
	AC_MSG_RESULT([yes])

	RC_LIBS="$RC_LIBS -mconsole"
else
	AC_MSG_RESULT([no])
fi

#static flag
AC_MSG_CHECKING([if building static w32 binary])
if test "$STATIC" != "no"; then
	AC_MSG_RESULT([yes])

	RC_FLAGS="$RC_FLAGS -DGLEW_STATIC"
	#TODO: static-lib* to LDFLAGS?
	RC_LIBS="$RC_LIBS -Wl,-Bstatic -static-libgcc -static-libstdc++"
else
	AC_MSG_RESULT([no])
fi

RC_LIBS_CHECK([ode], [ode-config], [ode/ode.h], [ode])
RC_LIBS_CHECK([sdl], [sdl-config], [SDL/SDL.h], [SDL])
RC_LIBS_CHECK([glew],, [GL/glew.h], [GLEW glew32])
#lua5.2, 5.1, 5.0, ... best way to check?

#static stop
if test "$STATIC" != "no"; then
	RC_LIBS="$RC_LIBS -Wl,-Bdynamic"
fi

#gl never static
RC_LIBS_CHECK([gl],, [GL/gl.h], [GL opengl32])

#make available
AC_SUBST(RC_FLAGS)
AC_SUBST(RC_LIBS)

])

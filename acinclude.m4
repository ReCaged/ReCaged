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
AC_DEFUN([RC_LIBS_CONFIG],
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
			[Link SDL, ODE, GLEW statically on W32 (default=no)])],
	[W32_STATIC="$enableval"],
	[W32_STATIC="no"] )

#pkg-config might exist?
AC_PATH_TOOL([PKG_CONFIG], [pkg-config])

])


#
# RC_LIBS_CHECK(PKG_NAME, CUSTOM_CONFIG, LIB_HEADER, LIB_NAME, LIB_FUNCTION)
# Check for specified library using pkg_config and other if possible
#

AC_DEFUN([RC_LIBS_CHECK],
[

FAILED="yes"

if test "$PKG_CONFIG"; then
	AC_MSG_CHECKING([for $1 usig pkg-config])
	TMP_CXXFLAGS=$($PKG_CONFIG --cflags $1 2>/dev/null)
	TMP_LDFLAGS=$($PKG_CONFIG --libs $1 2>/dev/null)

	if test "$TMP_CFLAGS" || test "$TMP_LDFLAGS"; then
		AC_MSG_RESULT([yes]);
		CXXFLAGS="$CXXFLAGS $TMP_CXXFLAGS"
		LDFLAGS="$LDFLAGS $TMP_LDFLAGS"
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

		TMP_CXXFLAGS=$($TMP_CONFIG --cflags $1 2>/dev/null)
		TMP_LDFLAGS=$($TMP_CONFIG --libs $1 2>/dev/null)

		if test "$TMP_CFLAGS" || test "$TMP_LDFLAGS"; then
			AC_MSG_RESULT([yes]);
			CXXFLAGS="$CXXFLAGS $TMP_CXXFLAGS"
			LDFLAGS="$LDFLAGS $TMP_LDFLAGS"
			FAILED="no"
		else
			AC_MSG_RESULT([no])
			FAILED="yes"
		fi
	fi
fi

#final fallback
#(quite critical if ode, since wont know if double precision... so chance of program compiling but not starting)
if test "$FAILED" = "yes"; then
	AC_MSG_WARN([Attempting to guess configuration for $1 using ac_check_* macros])

	AC_CHECK_HEADER($3,, [ AC_MSG_ERROR([Headers for $1 appears to be missing, install lib$1 or similar]) ])
	AC_SEARCH_LIBS($4, $5,, [ AC_MSG_ERROR([Development library $1 appears to be missing, install lib$1-dev or similar]) ])
fi

])


#
# RC_LIBS()
# Check for needed libs
#

AC_DEFUN([RC_LIBS],
[
AC_REQUIRE([RC_LIBS_CONFIG])

#test if ON_W32 + W32_STATIC
if test "ON_W32" != "no" && test "$W32_STATIC" != "no"; then
	AC_MSG_ERROR([TODO!])
fi

RC_LIBS_CHECK([ode], [ode-config], [ode/ode.h], [dInitODE2], [ode])
RC_LIBS_CHECK([sdl], [sdl-config], [SDL/SDL.h], [SDL_Init], [SDL])
RC_LIBS_CHECK([gl],, [GL/gl.h], [glEnable], [GL opengl32])
RC_LIBS_CHECK([glew],, [GL/glew.h], [glewInit], [GLEW glew32])
#lua5.2, 5.1, 5.0, ... best way to check?


AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)

])

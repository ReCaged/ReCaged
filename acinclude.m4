#
# ReCaged - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2012 Mats Wahlberg
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
# RC_LIBS_INIT()
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

#make this result available for automake
AM_CONDITIONAL([ON_W32], [test "$ON_W32" != "no"])

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

#check for lua interpreter, with possible cross-compiling prefix like with pkg-config
AC_PATH_TOOL([LUA], [lua])

#need windres if on w32
if test "$ON_W32" != "no"; then
	AC_PATH_TOOL([WINDRES], [windres])
	if test -z "$WINDRES"; then
		AC_MSG_ERROR([Program windres appears to be missing])
	fi
fi

])


#
# RC_LIBS_CHECK(PKG_NAME, CUSTOM_CONFIG, HEADER, LIB_NAMES_LIST)
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
# RC_LUA_CHECK()
# Check for lua library and check like RC_LIBS_CHECK
#

AC_DEFUN([RC_LUA_CHECK],
[

FAILED="yes"

#note: Some distros append the version number of the library to its name, which
#is smart, since new lua versions often breaks backwards compatibility. But some
#doesn't. Lets try to find the library with highest version if several are around.

if test "$PKG_CONFIG"; then
	AC_MSG_CHECKING([for highest likely version of lua known by pkg-config])

	#using BRE, so should be fairly portable grep syntax, no need for egrep
	#note: extra square brackets to counter expansion as m4
	LUA_LIST=$($PKG_CONFIG --list-all|grep '^lua[[0-9.-]]*[[[:space:]]]'|cut -d' ' -f1)

	#a fallback/dummy name of "lua" will be tested if nothing found
	LUA_PKG="lua"
	for LUA_CANDIDATE in $LUA_LIST; do
		AS_VERSION_COMPARE([LUA_CANDIDATE], [LUA_PKG], [LUA_PKG=$LUA_CANDIDATE])
	done

	AC_MSG_RESULT([$LUA_PKG])

	AC_MSG_CHECKING([for $LUA_PKG usig pkg-config])

	if test "$STATIC" = "no"; then
		LUA_FLAGS=$($PKG_CONFIG --cflags $LUA_PKG 2>/dev/null)
		LUA_LIBS=$($PKG_CONFIG --libs $LUA_PKG 2>/dev/null)
	else
		LUA_FLAGS=$($PKG_CONFIG --cflags --static $LUA_PKG 2>/dev/null)
		LUA_LIBS=$($PKG_CONFIG --libs --static $LUA_PKG 2>/dev/null)
	fi

	if test "$LUA_FLAGS" || test "$LUA_LIBS"; then
		AC_MSG_RESULT([yes]);
		RC_FLAGS="$RC_FLAGS $LUA_FLAGS"
		RC_LIBS="$RC_LIBS $LUA_LIBS"
		FAILED="no"
	else
		AC_MSG_RESULT([no])
		FAILED="yes"
	fi

fi


#success! Now the pkg-config flags should make it possible to include "lua.h"
if test "$FAILED" != "yes"; then
	AC_DEFINE_UNQUOTED([HEADER_LUA_H], [<lua.h>])
	AC_DEFINE_UNQUOTED([HEADER_LUALIB_H], [<lualib.h>])
	AC_DEFINE_UNQUOTED([HEADER_LAUXLIB_H], [<lauxlib.h>])
#failure, did not have pkg-config or missing lua? fallback...
else
	AC_MSG_WARN([Attempting to guess files for lua using ac_check_* macros])

	#As noted above, the lua files might not be in obvious places, but assume
	#that if a distro doesn't bother with pkg-config, then it probably wont
	#bother renaming lua directories and objects... "lua -v" would be useful
	#if ever wanted to try checking for renamed versions, like "lua5.2".

	#naming candidates:
	LUA_DIRS=". lua"

	FOUND_HEADER="no"
	FOUND_LIB="no"
	for TRY_DIR in $LUA_DIRS; do
		if test "$FOUND_HEADER" != "yes"; then
			AC_CHECK_HEADER(["$TRY_DIR/lua.h"], [
				FOUND_HEADER="yes"
				AC_DEFINE_UNQUOTED([HEADER_LUA_H], [<$TRY_DIR/lua.h>])
				AC_DEFINE_UNQUOTED([HEADER_LUALIB_H], [<$TRY_DIR/lualib.h>])
				AC_DEFINE_UNQUOTED([HEADER_LAUXLIB_H], [<$TRY_DIR/lauxlib.h>]) ])
		fi
	done

	if test "$FOUND_LIB" != "yes"; then
		AC_CHECK_LIB([lua], [main], [
			FOUND_LIB="yes"
			RC_LIBS="$RC_LIBS -llua" ])
	fi

	if test "$FOUND_HEADER" = "no"; then
		AC_MSG_ERROR([Headers for lua appears to be missing, install liblua-dev or similar])
	fi

	if test "$FOUND_LIB" = "no"; then
		AC_MSG_ERROR([Library lua appears to be missing, install liblua or similar])
	fi

fi

])


#
# RC_LIBS_CONFIG()
# Check for needed libs
#

AC_DEFUN([RC_LIBS_CONFIG],
[
AC_REQUIRE([RC_LIBS_INIT])

#make sure only enabling w32 options on w32
if test "$ON_W32" != "no"; then

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
fi

#actual library checks:
RC_LIBS_CHECK([ode], [ode-config], [ode/ode.h], [ode])
RC_LIBS_CHECK([sdl], [sdl-config], [SDL/SDL.h], [SDL])
RC_LIBS_CHECK([glew],, [GL/glew.h], [GLEW glew32])
RC_LUA_CHECK()

#static stop (if enabled and on w32)
if test "$ON_W32" != "no" && test "$STATIC" != "no"; then
	RC_LIBS="$RC_LIBS -Wl,-Bdynamic"
fi

#gl never static anyway
RC_LIBS_CHECK([gl],, [GL/gl.h], [GL opengl32])

#make available
AC_SUBST(RC_FLAGS)
AC_SUBST(RC_LIBS)

])

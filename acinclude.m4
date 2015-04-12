#
# RCX - a Free Software, Futuristic, Racing Game
#
# Copyright (C) 2012, 2014, 2015 Mats Wahlberg
#
# This file is part of RCX.
#
# RCX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RCX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RCX.  If not, see <http://www.gnu.org/licenses/>.
#


#
#Custom macros for checking some libraries, config flags, and set up.
#



#
# RCX_LIBS_INIT()
#
# Basic tests before starting
#

AC_DEFUN([RCX_LIBS_INIT],
[

AC_MSG_CHECKING(if building for a terrible OS)
case "$target_os" in

mingw*|cygwin*)
	AC_MSG_RESULT(yes)
	RCX_TARGET="w32"
	;;

*apple*|*darwin*)
	AC_MSG_RESULT(yes)
	RCX_TARGET="osx"

	;;

*)
	AC_MSG_RESULT(probably not)
	RCX_TARGET="good"
	;;

esac

#make this result available for automake (certain code alterations)
AM_CONDITIONAL([ON_W32], [test "$RCX_TARGET" = "w32"])

#force a certain lua package name (disables autodetection)?
AC_ARG_WITH(
	[lua],
	[AS_HELP_STRING([--with-lua=LUAPACKAGE],
		[Override detection of which lua package to use])], [
		if test "x$with_lua" = "xno"; then
			AC_MSG_ERROR([You can not disable lua])
		elif test "x$with_lua" != "xyes"; then
			LUAPKG="$with_lua"
		fi
            ])

#(note: the two "if/else" checks below prevents STATIC and CONSOLE from staying
#empty, to avoid using (IMHO) ugly looking "x$STATIC" = "xyes" tests later on)

#if w32, probably wanting static linking?
AC_ARG_ENABLE(
	[w32static],
	[AS_HELP_STRING([--enable-w32static],
			[Link libraries statically on W32 @<:@default=no@:>@])], [
		if test "x$enableval" != "xno"; then
			STATIC="yes"
		else
			STATIC="no"
		fi
		], [STATIC="no"] )

#and console output?
AC_ARG_ENABLE(
	[w32console],
	[AS_HELP_STRING([--enable-w32console],
			[Enable console output on W32 @<:@default=no@:>@])], [
		if test "x$enableval" != "xno"; then
			CONSOLE="yes"
		else
			CONSOLE="no"
		fi
		], [CONSOLE="no"] )

#need windres if on w32
if test "$RCX_TARGET" = "w32"; then
	AC_PATH_TOOL([WINDRES], [windres])
	if test -z "$WINDRES"; then
		AC_MSG_ERROR([Program windres appears to be missing])
	fi
fi

])



#
# RCX_CHECK_PROG(PROGRAM, FLAG_ARGS, LIB_ARGS, [ACTION-ON-FAILURE]
#
# Find and configure library by running PROGRAM with arguments FLAG_ARGS and
# LIB_ARGS, perform other action on failuer.
#

AC_DEFUN([RCX_CHECK_PROG],
[

FAILED="yes"

if test -n "$1"; then

	#try running program for flags
	AC_MSG_CHECKING([for flags using $1 $2])

	#(these if statements check the return status, not returned string)
	if tmp_flags=$($1 $2 2>/dev/null); then
		#ok
		AC_MSG_RESULT([$tmp_flags])

		#now try running it for libs
		AC_MSG_CHECKING([for libraries using $1 $3])

		if tmp_libs=$($1 $3 2>/dev/null); then
			#both were ok, make sure not performing fallback
			FAILED="no"
			AC_MSG_RESULT([$tmp_libs])

			#safely provide these strings for building
			RCX_FLAGS="$RCX_FLAGS $tmp_flags"
			RCX_LIBS="$RCX_LIBS $tmp_libs"
		else
			AC_MSG_RESULT([failed])
		fi
	else
		AC_MSG_RESULT([failed])
	fi
fi

if test "$FAILED" = "yes"; then
	$4
fi

])



#
# RCX_LIBS_CONFIG()
#
# Check and configure needed libraries
#

AC_DEFUN([RCX_LIBS_CONFIG],
[

AC_REQUIRE([RCX_LIBS_INIT])

#make sure only enabling w32 options on w32
if test "$RCX_TARGET" = "w32"; then

	#console flag
	AC_MSG_CHECKING([if building with w32 console enabled])
	if test "$CONSOLE" = "yes"; then
		AC_MSG_RESULT([yes])

		RCX_LIBS="$RCX_LIBS -mconsole"
	else
		AC_MSG_RESULT([no])
	fi

	#static flag
	AC_MSG_CHECKING([if building static w32 binary])
	if test "$STATIC" = "yes"; then
		AC_MSG_RESULT([yes])

		RCX_FLAGS="$RCX_FLAGS -DGLEW_STATIC"
		#TODO: move static-lib* to LDFLAGS?
		RCX_LIBS="$RCX_LIBS -Wl,-Bstatic -static-libgcc -static-libstdc++"
		pkg_static="--static"
	else
		AC_MSG_RESULT([no])
	fi
fi


#
#actual library checks:
#

AC_PATH_TOOL([PKG_CONFIG], [pkg-config])

#ODE:
RCX_CHECK_PROG([$PKG_CONFIG], [--cflags ode], [$pkg_static --libs ode],
[
	AC_PATH_TOOL([ODE_CONFIG], [ode-config])
	RCX_CHECK_PROG([$ODE_CONFIG], [--cflags], [--libs],
	[
		AC_MSG_WARN([Attempting to guess configuration for ODE using ac_check_* macros])
		AC_MSG_WARN([Don't know if using double or single precision!... Assuming double...])
		CPPFLAGS="$CPPFLAGS -DdDOUBLE" #hack: appends to user/global variable, but should be fine...?

		AC_CHECK_HEADER([ode/ode.h],, [ AC_MSG_ERROR([Header for ODE appears to be missing, install libode-dev or similar]) ])
		AC_CHECK_LIB([ode], [dInitODE2],
			[RCX_LIBS="$RCX_LIBS -lode"],
			[AC_MSG_ERROR([ODE library appears to be missing, install libode1 or similar])])
	])
])

#SDL:
RCX_CHECK_PROG([$PKG_CONFIG], [--cflags sdl], [$pkg_static --libs sdl],
[
	AC_PATH_TOOL([SDL_CONFIG], [sdl-config])

	#normally "--libs", but might change in certain situation (w32+static)
	if test "$RCX_TARGET" = "w32" && test "$STATIC" = "yes"; then
		sdl_libs="--static-libs"
	else
		sdl_libs="--libs"
	fi

	RCX_CHECK_PROG([$SDL_CONFIG], [--cflags], [$sdl_libs],
	[
		AC_MSG_WARN([Attempting to guess configuration for SDL using ac_check_* macros])
		AC_CHECK_HEADER([SDL/SDL.h],, [ AC_MSG_ERROR([Header for SDL appears to be missing, install libsdl-dev or similar]) ])
		AC_CHECK_LIB([SDL], [SDL_Init],
			[RCX_LIBS="$RCX_LIBS -lSDL"],
			[AC_MSG_ERROR([SDL library appears to be missing, install libsdl-1.2 or similar])])
	])
])

#GLEW:
RCX_CHECK_PROG([$PKG_CONFIG], [--cflags glew], [$pkg_static --libs glew],
[
	AC_MSG_WARN([Attempting to guess configuration for GLEW using ac_check_* macros])
	AC_CHECK_HEADER([GL/glew.h],, [ AC_MSG_ERROR([Header for GLEW appears to be missing, install libglew-dev or similar]) ])

	#w32 uses (of course...) a different name
	if test "$RCX_TARGET" = "w32"; then
		AC_CHECK_LIB([glew32], [main],
			[RCX_LIBS="$RCX_LIBS -lglew32"],
			[AC_MSG_ERROR([GLEW library appears to be missing, install libglew or similar])])
	else
		AC_CHECK_LIB([GLEW], [main],
			[RCX_LIBS="$RCX_LIBS -lGLEW"],
			[AC_MSG_ERROR([GLEW library appears to be missing, install libglew or similar])])
	fi

])

#LUA:

#Note: some distros append the version number of the library to its name, which
#is smart since new lua versions often breaks backwards compatibility. But some
#distros doesn't (or use a different suffix). Unless the user specifies which
#version to use, try to automatically find the highest version around...

#first, did user not specify a version?
if test -z "$LUAPKG"; then

	#begin with a fallback/dummy name of "lua" (will be tested if nothing else is found)
	LUAPKG="lua"

	#if got pkg-config, find the most likely highest lua version
	if test -n "$PKG_CONFIG"; then
		AC_PROG_AWK
		AC_MSG_CHECKING([for likely highest version of lua known by pkg-config])

		#find package names beginning with lua and (at most) any version suffix
		#note: extra square brackets to counter expansions
		LUA_LIST=$($PKG_CONFIG --list-all|$AWK '/^lua[[0-9.-]]* / { print $[]1 }')

		#loop through all candidates and find the highest-versioned one
		#(LUAVER will be empty at first check, but that's ok)
		for LUA_CANDIDATE in $LUA_LIST; do
			LUA_CANDIDATE_VER=$($PKG_CONFIG --modversion $LUA_CANDIDATE)
			AS_VERSION_COMPARE([$LUAVER], [$LUA_CANDIDATE_VER], [
				LUAVER=$LUA_CANDIDATE_VER
				LUAPKG=$LUA_CANDIDATE
				])
		done

		AC_MSG_RESULT([$LUAPKG])
	fi

fi

		
#whatever we got above (should be at least "lua"), try to use it
RCX_CHECK_PROG([$PKG_CONFIG], [--cflags $LUAPKG], [$pkg_static --libs $LUAPKG],
[
	AC_MSG_WARN([Attempting to guess configuration for LUA using ac_check_* macros])

	#check all three headers
	FAILED="no"
	AC_CHECK_HEADER([$LUAPKG/lua.h],, [FAILED="yes"])
	AC_CHECK_HEADER([$LUAPKG/lualib.h],, [FAILED="yes"])
	AC_CHECK_HEADER([$LUAPKG/lauxlib.h],, [FAILED="yes"])

	#if any one failed
	if test "$FAILED" = "no"; then
		#export the special paths as defines
		AC_DEFINE([OVERRIDE_LUA_HEADERS])
		AC_DEFINE_UNQUOTED([OVERRIDE_LUA_H], [<$LUAPKG/lua.h>])
		AC_DEFINE_UNQUOTED([OVERRIDE_LUALIB_H], [<$LUAPKG/lualib.h>])
		AC_DEFINE_UNQUOTED([OVERRIDE_LAUXLIB_H], [<$LUAPKG/lauxlib.h>])
	#if failed, there might still be another possibility...
	#"lua"=default string (user/pkg-config detection has not specified anything different)
	elif test "$LUAPKG" = "lua"; then
		#okay, check again, but this time directly in include path
		FAILED="no"
		AC_CHECK_HEADER([lua.h],, [FAILED="yes"])
		AC_CHECK_HEADER([lualib.h],, [FAILED="yes"])
		AC_CHECK_HEADER([lauxlib.h],, [FAILED="yes"])
	fi

	#if nothing succeeded...
	if test "$FAILED" = "yes"; then
		AC_MSG_ERROR([Headers for LUA appear to be missing, install liblua-dev or similar])
	fi

	#check library as usual - I assume lua_close exists in all versions
	AC_CHECK_LIB([$LUAPKG], [lua_close],
		[RCX_LIBS="$RCX_LIBS -l$LUAPKG"],
		[AC_MSG_ERROR([LUA library appears to be missing, install liblua or similar])])
])



#static stop (if enabled and on w32)
if test "$RCX_TARGET" = "w32" && test "$STATIC" = "yes"; then
	RCX_LIBS="$RCX_LIBS -Wl,-Bdynamic"
fi

#GL (never static):
RCX_CHECK_PROG([$PKG_CONFIG], [--cflags gl], [--libs gl],
[
	AC_MSG_WARN([Attempting to guess configuration for GL using ac_check_* macros])

	#here's a fun part: both w32 and osx likes to brake naming conventions for opengl...
	if test "$RCX_TARGET" = "osx"; then
		AC_CHECK_HEADER([OpenGL/gl.h],, [ AC_MSG_ERROR([Header for GL appears to be missing, you go and figure this one out]) ])
	else
		AC_CHECK_HEADER([GL/gl.h],, [ AC_MSG_ERROR([Header for GL appears to be missing, install libgl1-mesa-dev or similar]) ])
	fi

	#also GL libraries got unreliable symbols (so just check for dummy main)
	case "$RCX_TARGET" in

	w32)
		AC_CHECK_LIB([opengl32], [main],
			[RCX_LIBS="$RCX_LIBS -lopengl32"],
			[AC_MSG_ERROR([GL library appears to be missing, install libgl1-mesa or similar])])
		;;

	osx)
		AC_MSG_WARN([Just guessing "-framework OpenGL" can be used for linking gl library, no guarantees!])
		RCX_LIBS="$RCX_LIBS -framework OpenGL"
		;;
	
	*)
		AC_CHECK_LIB([GL], [main],
			[RCX_LIBS="$RCX_LIBS -lGL"],
			[AC_MSG_ERROR([GL library appears to be missing, install libgl1-mesa or similar])])
		;;
	esac

])

#make available
AC_SUBST(RCX_FLAGS)
AC_SUBST(RCX_LIBS)

])

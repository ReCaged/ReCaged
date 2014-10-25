#
# ReCaged - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2012, 2014 Mats Wahlberg
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
#



#
# RC_LIBS_INIT()
#
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

#need windres if on w32
if test "$ON_W32" != "no"; then
	AC_PATH_TOOL([WINDRES], [windres])
	if test -z "$WINDRES"; then
		AC_MSG_ERROR([Program windres appears to be missing])
	fi
fi

])



#
# RC_CHECK_PROG(PROGRAM, FLAG_ARGS, LIB_ARGS, [ACTION-ON-FAILURE]
#
# Find and configure library by running PROGRAM with arguments FLAG_ARGS and
# LIB_ARGS, perform other action on failuer.
#

AC_DEFUN([RC_CHECK_PROG],
[

FAILED="yes"

if test "$1"; then

	AC_MSG_CHECKING([for flags using $1 $2])
	tmp_flags=$($1 $2 2>/dev/null)

	if test "$tmp_flags"; then
		FAILED="no"
		AC_MSG_RESULT([$tmp_flags])
	else
		AC_MSG_RESULT([no])
	fi

	AC_MSG_CHECKING([for libraries using $1 $3])
	tmp_libs=$($1 $3 2>/dev/null)

	if test "$tmp_libs"; then
		FAILED="no"
		AC_MSG_RESULT([$tmp_libs])
	else
		AC_MSG_RESULT([no])
	fi
fi

if test "$FAILED" = "no"; then
	RC_FLAGS="$RC_FLAGS $tmp_flags"
	RC_LIBS="$RC_LIBS $tmp_libs"
else
	$4
fi

])



#
# RC_LIBS_CONFIG()
#
# Check and configure needed libraries
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
		#TODO: move static-lib* to LDFLAGS?
		RC_LIBS="$RC_LIBS -Wl,-Bstatic -static-libgcc -static-libstdc++"
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
RC_CHECK_PROG([$PKG_CONFIG], [--cflags ode], [$pkg_static --libs ode],
[
	AC_PATH_TOOL([ODE_CONFIG], [ode-config])
	RC_CHECK_PROG([$ODE_CONFIG], [--cflags], [--libs],
	[
		AC_MSG_WARN([Attempting to guess configuration for ODE using ac_check_* macros])
		AC_MSG_WARN([Don't know if using double or single precision!... Assuming double...])
		CPPFLAGS="$CPPFLAGS -DdDOUBLE" #hack: appends to user/global variable, but should be fine...?

		AC_CHECK_HEADER([ode/ode.h],, [ AC_MSG_ERROR([Headers for ODE appears to be missing, install libode-dev or similar]) ])
		AC_CHECK_LIB([ode], [dInitODE2],
			[RC_LIBS="$RC_LIBS -lode"],
			[AC_MSG_ERROR([ODE library appears to be missing, install libode1 or similar])])
	])
])

#SDL:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags sdl], [$pkg_static --libs sdl],
[
	AC_PATH_TOOL([SDL_CONFIG], [sdl-config])

	#normally "--libs", but might change in certain situation (w32+static)
	if test "$ON_W32" != "no" && test "$STATIC" != "no"; then
		sdl_libs="--static-libs"
	else
		sdl_libs="--libs"
	fi

	RC_CHECK_PROG([$SDL_CONFIG], [--cflags], [$sdl_libs],
	[
		AC_MSG_WARN([Attempting to guess configuration for SDL using ac_check_* macros])
		AC_CHECK_HEADER([SDL/SDL.h],, [ AC_MSG_ERROR([Headers for SDL appears to be missing, install libsdl-dev or similar]) ])
		AC_CHECK_LIB([SDL], [SDL_Init],
			[RC_LIBS="$RC_LIBS -lSDL"],
			[AC_MSG_ERROR([SDL library appears to be missing, install libsdl-1.2 or similar])])
	])
])

#LIBPNG:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags libpng], [$pkg_static --libs libpng],
[
	AC_PATH_TOOL([PNG_CONFIG], [libpng-config])

	#add pkg_static for the potential "--static" flag
	RC_CHECK_PROG([$PNG_CONFIG], [--cflags], [$pkg_static --ldflags],
	[
		AC_MSG_WARN([Attempting to guess configuration for LIBPNG using ac_check_* macros])
		AC_CHECK_HEADER([png.h],, [ AC_MSG_ERROR([Headers for LIBPNG appears to be missing, install libpng-dev or similar]) ])
		AC_CHECK_LIB([png], [png_destroy_write_struct],
			[RC_LIBS="$RC_LIBS -lpng"],
			[AC_MSG_ERROR([LIBPNG library appears to be missing, install libpng or similar])])

		#static linking on windows requires zlib
		if test "$ON_W32" != "no" && test "$STATIC" != "no"; then
			AC_CHECK_LIB([z], [gzread],
				[RC_LIBS="$RC_LIBS -lz"],
				[AC_MSG_ERROR([ZLIB library appears to be missing, install zlib or similar])])
		fi
	])
])

#LIBJPEG:
#note: Have never seen a libjpeg.pc in the wild, but this can't harm... right?
RC_CHECK_PROG([$PKG_CONFIG], [--cflags libjpeg], [$pkg_static --libs libjpeg],
[
	#okay, most likely outcome: check for existence directly
	AC_MSG_WARN([Attempting to guess configuration for LIBJPEG using ac_check_* macros])
	AC_CHECK_HEADER([jpeglib.h],, [ AC_MSG_ERROR([Headers for LIBJPEG appears to be missing, install libjpeg-dev or similar]) ])
	AC_CHECK_LIB([jpeg], [jpeg_destroy_decompress],
		[RC_LIBS="$RC_LIBS -ljpeg"],
		[AC_MSG_ERROR([LIBJPEG library appears to be missing, install libjpeg or similar])])
])

#GLEW:
RC_CHECK_PROG([$PKG_CONFIG], [--cflags glew], [$pkg_static --libs glew],
[
	AC_MSG_WARN([Attempting to guess configuration for GLEW using ac_check_* macros])
	AC_CHECK_HEADER([GL/glew.h],, [ AC_MSG_ERROR([Headers for GLEW appears to be missing, install libglew-dev or similar]) ])

	if test "$ON_W32" = "no"; then
		AC_CHECK_LIB([GLEW], [main],
			[RC_LIBS="$RC_LIBS -lGLEW"],
			[AC_MSG_ERROR([GLEW library appears to be missing, install libglew or similar])])
	else
		AC_CHECK_LIB([glew32], [main],
			[RC_LIBS="$RC_LIBS -lglew32"],
			[AC_MSG_ERROR([GLEW library appears to be missing, install libglew or similar])])
	fi

])

#static stop (if enabled and on w32)
if test "$ON_W32" != "no" && test "$STATIC" != "no"; then
	RC_LIBS="$RC_LIBS -Wl,-Bdynamic"
fi

#GL (never static):
RC_CHECK_PROG([$PKG_CONFIG], [--cflags gl], [--libs gl],
[
	AC_MSG_WARN([Attempting to guess configuration for GL using ac_check_* macros])
	AC_CHECK_HEADER([GL/gl.h],, [ AC_MSG_ERROR([Headers for GL appears to be missing, install libgl1-mesa-dev or similar]) ])

	#note: w32 likes to brake naming conventions (opengl32).
	#also GL libraries got unreliable symbols (so just check for dummy main)
	if test "$ON_W32" = "no"; then
		AC_CHECK_LIB([GL], [main],
			[RC_LIBS="$RC_LIBS -lGL"],
			[AC_MSG_ERROR([GL library appears to be missing, install libgl1-mesa or similar])])
	else
		AC_CHECK_LIB([opengl32], [main],
			[RC_LIBS="$RC_LIBS -lopengl32"],
			[AC_MSG_ERROR([GL library appears to be missing, install libgl1-mesa or similar])])
	fi

])

#make available
AC_SUBST(RC_FLAGS)
AC_SUBST(RC_LIBS)

])

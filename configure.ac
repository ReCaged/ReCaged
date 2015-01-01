# Process this file with autoconf to produce a configure script.

#
# RCX - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2012, 2013, 2014 Mats Wahlberg
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

AC_PREREQ([2.60]) #should work (and be old enough)
AC_INIT([RCX], [0.8.0wip], [rcxslinger@gmail.com], [rcx], [http://recaged.net])

# Define codename and year (for title and version printout)
AC_DEFINE([PACKAGE_CODENAME], ["The Devil's Mind is an Idle Workshop"])
AC_DEFINE([PACKAGE_YEAR], ["2015"])

# Only a safety check
AC_CONFIG_SRCDIR([src/rcx.cpp])

# No need for "AC_CONFIG_HEADERS" right now?

# Arguments for automake
AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for header files.
AC_CHECK_HEADERS([limits.h stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor memset pow sqrt strcasecmp strrchr strtol])

# Check for additional libraries (in acinclude.m4)
RCX_LIBS_CONFIG

# Finally: what to generate
AC_CONFIG_FILES([Makefile config/Makefile data/Makefile icons/Makefile man/Makefile src/Makefile w32/Makefile])

AC_OUTPUT

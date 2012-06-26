# Process this file with autoconf to produce a configure script.

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

AC_PREREQ([2.68]) #defaulted
AC_INIT([ReCaged], [0.7.1], [rcxslinger@gmail.com]) #TODO: more here!
#codename?
#AC_CONFIG_SRCDIR([src/main.cpp]) #only a safety check
#AC_CONFIG_HEADERS([src/config.h]) #TODO: move to include dir...?

#for automake
AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
#AC_PROG_CC
AC_PROG_CXX

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
RC_LIBS

#TODO: add files from "data", generate ChangeLog from git log at "make dist*"
AC_OUTPUT([Makefile src/Makefile])

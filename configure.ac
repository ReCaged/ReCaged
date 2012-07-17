# Process this file with autoconf to produce a configure script.

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

AC_PREREQ([2.60]) #should work (and be old enough)
AC_INIT([ReCaged], [0.7.1], [rcxslinger@gmail.com], [recaged], [http://www.nongnu.org/recaged])

# Define codename and year (for title and version printout)
AC_DEFINE([PACKAGE_CODENAME], ["The Sky's the Limit"])
AC_DEFINE([PACKAGE_YEAR], ["2012"])

# Only a safety check
AC_CONFIG_SRCDIR([src/recaged.cpp])

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
RC_LIBS_CONFIG

# Finally: what to generate
AC_CONFIG_FILES([Makefile conf/Makefile data/Makefile icons/Makefile man/Makefile src/Makefile])

AC_OUTPUT

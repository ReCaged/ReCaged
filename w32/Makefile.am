#
# RCX - a Free Software, Futuristic, Racing Game
#
# Copyright (C) 2012 Mats Wahlberg
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

EXTRA_DIST = README header.bmp installer.nsi side.bmp
CLEANFILES = version.nsh

#automatically generate version definition for nsis installer on w32:
if ON_W32

#generate version file when making
all-local: version.nsh

#only updates on changes to w32/Makefile (like version change)
version.nsh: Makefile
	( echo "!define VERSION \"${PACKAGE_VERSION}\""; \
	echo -n "!define W32VERSION "; \
	echo -n "$(PACKAGE_VERSION)" | sed 's/\([0-9.]*\).*/\1/'; \
	echo ".0"; ) > $@

endif


#
# RCX - a Free Software, Futuristic, Racing Simulator
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


AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

#TODO!
SUBDIRS = config data icons man src w32

#TODO!
EXTRA_DIST		= AUTHORS COPYING INSTALL NEWS README ChangeLog \
			  build.sh \
			  licenses/CC0 \
			  licenses/CC-BY-SA-4.0 \
			  licenses/GPL-3

#create ChangeLog from git log
dist-hook:
	( echo "Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015 Mats Wahlberg"; \
	echo "Copying and distribution of this file, with or without modification,"; \
	echo "are permitted in any medium without royalty provided the copyright"; \
	echo "notice and this notice are preserved. This file is offered as-is,"; \
	echo "without any warranty."; \
	echo ""; \
	cd "$(top_srcdir)" && ./missing --run git log \
	--stat --name-only --date=short --abbrev-commit ) \
	> ${distdir}/ChangeLog

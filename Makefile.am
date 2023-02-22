#
# ReCaged - a Free Software, Futuristic, Racing Game
#
# Copyright (C) 2012, 2015 Mats Wahlberg
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


AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

SUBDIRS                 = config data man src w32

#documentation installed
dist_doc_DATA		= AUTHORS COPYING INSTALL NEWS README ChangeLog

#TODO: when icon...
#pixmapsdir = $(datadir)/pixmaps
#dist_pixmaps_DATA = #data/icon...

#helper script for obscure builds, and license directory
EXTRA_DIST		= build.sh licenses


#create ChangeLog from git log
dist-hook:
	( echo "Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2023 Mats Wahlberg"; \
	echo "Copying and distribution of this file, with or without modification,"; \
	echo "are permitted in any medium without royalty provided the copyright"; \
	echo "notice and this notice are preserved. This file is offered as-is,"; \
	echo "without any warranty."; \
	echo ""; \
	cd "$(top_srcdir)" && ./missing --run git log \
	--stat --name-only --date=short --abbrev-commit ) \
	> ${distdir}/ChangeLog

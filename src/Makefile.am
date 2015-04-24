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


AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

#compiler flags
AM_CPPFLAGS =	$(RCX_FLAGS) -Wall \
		-DDATADIR='"$(datadir)/games/rcx/"' \
		-DCONFDIR='"$(sysconfdir)/xdg/rcx/"'

#AM_LDFLAGS not used right now
LDADD = $(RCX_LIBS)

#place in subdir "games" instead of "bin"
bindir=$(exec_prefix)/games

bin_PROGRAMS = rcx

rcx_SOURCES =	./rcx.cpp \
		assets/assets.cpp \
		assets/assets.hpp \
		assets/car.cpp \
		assets/car.hpp \
		assets/conf.cpp \
		assets/conf.hpp \
		assets/image.cpp \
		assets/image.hpp \
		assets/model.cpp \
		assets/model_draw.cpp \
		assets/model.hpp \
		assets/model_mesh.cpp \
		assets/obj.cpp \
		assets/object.cpp \
		assets/object.hpp \
		assets/profile.cpp \
		assets/profile.hpp \
		assets/road.cpp \
		assets/script.hpp \
		assets/text_file.cpp \
		assets/text_file.hpp \
		assets/track.cpp \
		assets/track.hpp \
		common/directories.cpp \
		common/directories.hpp \
		common/internal.cpp \
		common/internal.hpp \
		common/log.cpp \
		common/log.hpp \
		common/threads.cpp \
		common/threads.hpp \
		interface/geom_render.cpp \
		interface/geom_render.hpp \
		interface/interface.cpp \
		interface/profile.cpp \
		interface/render_list.cpp \
		interface/render_list.hpp \
		simulation/body.cpp \
		simulation/body.hpp \
		simulation/camera.cpp \
		simulation/camera.hpp \
		simulation/car.cpp \
		simulation/collision_feedback.cpp \
		simulation/collision_feedback.hpp \
		simulation/component.cpp \
		simulation/component.hpp \
		simulation/event_buffers.cpp \
		simulation/event_buffers.hpp \
		simulation/geom.cpp \
		simulation/geom.hpp \
		simulation/joint.cpp \
		simulation/joint.hpp \
		simulation/simulation.cpp \
		simulation/space.cpp \
		simulation/space.hpp \
		simulation/track.cpp \
		simulation/timers.cpp \
		simulation/timers.hpp \
		simulation/wheel.cpp \
		simulation/wheel.hpp

#a trick to make all objects recompile if Makefile gets updated (reconfigure)
$(rcx_OBJECTS): Makefile

#remove w32 files on all platforms (just in case)
CLEANFILES = resources.rc resources.o

if ON_W32

#TODO: add icon (in 0.7.3)
LDADD += resources.o

#info file, only updates on changes to src/Makefile (like version change)
#note: sed usage might be a ugly/unreliable, but only replaces dots anyway
resources.rc: Makefile
	( echo "1 VERSIONINFO"; \
	echo -n "FILEVERSION "; \
	echo -n "$(PACKAGE_VERSION)" | sed 's/\([0-9.]*\).*/\1/; s/\./,/g'; \
	echo ",0"; \
	echo -n "PRODUCTVERSION "; \
	echo -n "$(PACKAGE_VERSION)" | sed 's/\([0-9.]*\).*/\1/; s/\./,/g'; \
	echo ",0"; \
	echo "BEGIN"; \
	echo "BLOCK \"StringFileInfo\""; \
	echo "BEGIN"; \
	echo "BLOCK \"00000000\""; \
	echo "BEGIN"; \
	echo "VALUE \"ProductName\", \"RCX\""; \
	echo "VALUE \"FileVersion\", \"$(PACKAGE_VERSION)\""; \
	echo "VALUE \"ProductVersion\", \"$(PACKAGE_VERSION)\""; \
	echo "VALUE \"Comments\", \"A Free Software, Futuristic, Racing Game\""; \
	echo "VALUE \"FileDescription\", \"A Free Software, Futuristic, Racing Game\""; \
	echo "VALUE \"LegalCopyright\", \"Copyright (C) Mats Wahlberg\""; \
	echo "END"; \
	echo "END"; \
	echo "BLOCK \"VarFileInfo\""; \
	echo "BEGIN"; \
	echo "VALUE \"Translation\", 0x0000, 0x0000"; \
	echo "END";\
	echo "END" ) > $@


#windres creates resource obj from rc
resources.o: resources.rc
	$(WINDRES) -i $? -o $@

endif


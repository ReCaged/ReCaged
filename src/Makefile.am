#
# ReCaged - a Free Software, Futuristic, Racing Game
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


AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

#compiler flags
AM_CPPFLAGS =	$(RC_FLAGS) -Wall \
		-DDATADIR='"$(datadir)/recaged/"' \
		-DCONFDIR='"$(sysconfdir)/xdg/recaged/"'
#AM_LDFLAGS not used right now
LDADD = $(RC_LIBS)

bin_PROGRAMS = recaged

recaged_SOURCES =	./recaged.cpp \
			assets/car.cpp \
			assets/conf.cpp \
			assets/conf.hpp \
			assets/image.cpp \
			assets/image.hpp \
			assets/obj.cpp \
			assets/object.cpp \
			assets/profile.cpp \
			assets/road.cpp \
			assets/text_file.cpp \
			assets/text_file.hpp \
			assets/track.cpp \
			assets/trimesh.cpp \
			interface/geom_render.cpp \
			interface/geom_render.hpp \
			interface/interface.cpp \
			interface/profile.cpp \
			interface/render_list.cpp \
			interface/render_list.hpp \
			shared/body.cpp \
			shared/body.hpp \
			shared/camera.cpp \
			shared/camera.hpp \
			shared/car.cpp \
			shared/car.hpp \
			shared/component.cpp \
			shared/component.hpp \
			shared/directories.cpp \
			shared/directories.hpp \
			shared/geom.cpp \
			shared/geom.hpp \
			shared/internal.cpp \
			shared/internal.hpp \
			shared/joint.cpp \
			shared/joint.hpp \
			shared/log.cpp \
			shared/log.hpp \
			shared/object.cpp \
			shared/object.hpp \
			shared/profile.cpp \
			shared/profile.hpp \
			shared/racetime_data.cpp \
			shared/racetime_data.hpp \
			shared/runlevel.cpp \
			shared/runlevel.hpp \
			shared/script.hpp \
			shared/space.cpp \
			shared/space.hpp \
			shared/surface.cpp \
			shared/surface.hpp \
			shared/threads.cpp \
			shared/threads.hpp \
			shared/track.cpp \
			shared/track.hpp \
			shared/trimesh_3d.cpp \
			shared/trimesh.cpp \
			shared/trimesh_geom.cpp \
			shared/trimesh.hpp \
			simulation/body.cpp \
			simulation/camera.cpp \
			simulation/car.cpp \
			simulation/collision_feedback.cpp \
			simulation/collision_feedback.hpp \
			simulation/event_buffers.cpp \
			simulation/event_buffers.hpp \
			simulation/geom.cpp \
			simulation/joint.cpp \
			simulation/simulation.cpp \
			simulation/timers.cpp \
			simulation/timers.hpp \
			simulation/wheel.cpp \
			simulation/wheel.hpp

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
	echo "VALUE \"ProductName\", \"ReCaged\""; \
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
	echo "END" ) > resources.rc


#windres creates resource obj from rc
resources.o: resources.rc
	$(WINDRES) -i $? -o $@

endif


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
		-DDATADIR='"$(datadir)/rcx/"' \
		-DCONFDIR='"$(sysconfdir)/xdg/rcx/"'
#AM_LDFLAGS not used right now
LDADD = $(RCX_LIBS)

bin_PROGRAMS = rcx

rcx_SOURCES =	./rcx.cpp \
		assets/assets.cpp \
		assets/car.cpp \
		assets/conf.cpp \
		assets/obj.cpp \
		assets/object.cpp \
		assets/profile.cpp \
		assets/road.cpp \
		assets/text_file.cpp \
		assets/track.cpp \
		assets/trimesh_3d.cpp \
		assets/trimesh.cpp \
		assets/trimesh_geom.cpp \
		common/directories.cpp \
		common/internal.cpp \
		common/log.cpp \
		common/runlevel.cpp \
		common/threads.cpp \
		interface/geom_render.cpp \
		interface/interface.cpp \
		interface/profile.cpp \
		interface/render_list.cpp \
		simulation/body.cpp \
		simulation/camera.cpp \
		simulation/car.cpp \
		simulation/collision_feedback.cpp \
		simulation/component.cpp \
		simulation/event_buffers.cpp \
		simulation/geom.cpp \
		simulation/joint.cpp \
		simulation/simulation.cpp \
		simulation/space.cpp \
		simulation/timers.cpp \
		simulation/wheel.cpp

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


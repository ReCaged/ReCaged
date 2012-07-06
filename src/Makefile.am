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


AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

#ok?
AM_CPPFLAGS = $(RC_FLAGS)
#AM_LDFLAGS not used right now
LDADD = $(RC_LIBS)

bin_PROGRAMS = recaged

recaged_SOURCES =	./recaged.cpp \
			interface/camera.cpp \
			interface/geom_render.cpp \
			interface/geom_render.hpp \
			interface/interface.cpp \
			interface/profile.cpp \
			interface/render_list.cpp \
			interface/render_list.hpp \
			loaders/car.cpp \
			loaders/conf.cpp \
			loaders/conf.hpp \
			loaders/obj.cpp \
			loaders/object.cpp \
			loaders/profile.cpp \
			loaders/road.cpp \
			loaders/text_file.cpp \
			loaders/text_file.hpp \
			loaders/track.cpp \
			loaders/trimesh.cpp \
			shared/body.cpp \
			shared/body.hpp \
			shared/camera.cpp \
			shared/camera.hpp \
			shared/car.cpp \
			shared/car.hpp \
			shared/component.cpp \
			shared/component.hpp \
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

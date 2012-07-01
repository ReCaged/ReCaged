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

recaged_SOURCES = \
		recaged.cpp \
		interface/camera.cpp \
		interface/geom_render.cpp \
		interface/interface.cpp \
		interface/profile.cpp \
		interface/render_list.cpp \
		loaders/car.cpp \
		loaders/conf.cpp \
		loaders/obj.cpp \
		loaders/object.cpp \
		loaders/profile.cpp \
		loaders/road.cpp \
		loaders/text_file.cpp \
		loaders/track.cpp \
		loaders/trimesh.cpp \
		shared/body.cpp \
		shared/camera.cpp \
		shared/car.cpp \
		shared/component.cpp \
		shared/geom.cpp \
		shared/internal.cpp \
		shared/joint.cpp \
		shared/log.cpp \
		shared/object.cpp \
		shared/profile.cpp \
		shared/racetime_data.cpp \
		shared/runlevel.cpp \
		shared/space.cpp \
		shared/threads.cpp \
		shared/track.cpp \
		shared/trimesh_3d.cpp \
		shared/trimesh.cpp \
		shared/trimesh_geom.cpp \
		simulation/body.cpp \
		simulation/camera.cpp \
		simulation/car.cpp \
		simulation/collision_feedback.cpp \
		simulation/event_buffers.cpp \
		simulation/geom.cpp \
		simulation/joint.cpp \
		simulation/simulation.cpp \
		simulation/timers.cpp \
		simulation/wheel.cpp


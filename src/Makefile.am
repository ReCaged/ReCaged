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

#TODO! add sdl/ode flags (+gl, +glew)
AM_CPPFLAGS = $(DEPS_CFLAGS)
recaged_LDADD = $(DEPS_LIBS)

bin_PROGRAMS = recaged

recaged_SOURCES = \
		./simulation/timers.cpp \
		./simulation/wheel.cpp \
		./simulation/event_buffers.cpp \
		./simulation/camera.cpp \
		./simulation/car.cpp \
		./simulation/geom.cpp \
		./simulation/body.cpp \
		./simulation/joint.cpp \
		./simulation/simulation.cpp \
		./simulation/collision_feedback.cpp \
		./main.cpp \
		./interface/render_list.cpp \
		./interface/camera.cpp \
		./interface/geom_render.cpp \
		./interface/interface.cpp \
		./interface/profile.cpp \
		./shared/trimesh_3d.cpp \
		./shared/component.cpp \
		./shared/printlog.cpp \
		./shared/threads.cpp \
		./shared/trimesh.cpp \
		./shared/runlevel.cpp \
		./shared/camera.cpp \
		./shared/space.cpp \
		./shared/car.cpp \
		./shared/track.cpp \
		./shared/racetime_data.cpp \
		./shared/geom.cpp \
		./shared/trimesh_geom.cpp \
		./shared/body.cpp \
		./shared/object.cpp \
		./shared/joint.cpp \
		./shared/internal.cpp \
		./shared/profile.cpp \
		./loaders/obj.cpp \
		./loaders/trimesh.cpp \
		./loaders/conf.cpp \
		./loaders/text_file.cpp \
		./loaders/car.cpp \
		./loaders/track.cpp \
		./loaders/object.cpp \
		./loaders/profile.cpp \
		./loaders/road.cpp


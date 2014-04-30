/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include "space.hpp"
#include "geom.hpp"
#include "log.hpp"
#include "track.hpp"
#include <ode/ode.h>

Space::Space(Object *obj): Component(obj)
{
	space_id = dSimpleSpaceCreate(space);

	Log_Add(2, "(autoselecting default space for object)");
	obj->selected_space=space_id;
}

Space::~Space()
{
	Geom *g;

	while (dSpaceGetNumGeoms(space_id)) //while contains geoms
	{
		//remove first geom - next time first will be the next geom
		g = (Geom*)dGeomGetData(dSpaceGetGeom(space_id, 0));
		delete g;
	}

	dSpaceDestroy(space_id);
}

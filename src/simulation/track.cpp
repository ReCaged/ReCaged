/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2015 Mats Wahlberg
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

#include <ode/ode.h>

#include "assets/track.hpp"
#include "assets/car.hpp"
#include "geom.hpp"
#include "body.hpp"

//check for bodies below "restart height"
//TODO: can use arbitrary geoms and collisions instead, but better when lua
void Track_Physics_Step()
{
	Body *body, *bnext = Body::head;

	Geom *geom, *gnext;

	while ((body = bnext))
	{
		//store pointer to next (if removing below)
		bnext = body->next;

		const dReal *pos = dBodyGetPosition(body->body_id); //get position
		if (pos[2] < track.restart) //under restart height
		{
			Car *car = dynamic_cast<Car*>(body->object_parent);

			//this is part of a car, it can be recreated
			if (car)
				car->Recreate(track.start[0], track.start[1], track.start[2]);
			//else, this is part of an object, destroy it (and any attached geom)
			else
			{
				//find all geoms that are attached to this body (proper cleanup)
				//ode lacks a "dBodyGetGeom" routine (why?! it's easy to implement!)...
				gnext = Geom::head;
				while ((geom = gnext))
				{
					gnext=geom->next; //needs this after possibly destroying the geom (avoid segfault)
					if (dGeomGetBody(geom->geom_id) == body->body_id)
						delete geom;
				}

				//and remove the body
				delete body;
			}
		}
	}
}

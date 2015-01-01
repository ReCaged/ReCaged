/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
 *
 * This file is part of RCX.
 *
 * RCX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RCX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RCX.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef _RCX_BODY_H
#define _RCX_BODY_H
#include <ode/ode.h>
#include <SDL/SDL.h>
#include "script.hpp"
#include "object.hpp"
#include "component.hpp"
#include "script.hpp"

//body_data: data for body (describes mass and mass positioning), used for:
//currently only for triggering event script (force threshold and event variables)
//as well as simple air/liquid drag simulations
//
//>Dynamically allocated
class Body: public Component
{
	public:
		//methods
		Body (dBodyID body, Object *obj);
		~Body();

		void Set_Event(dReal thresh, dReal buff, Script *scr);
		void Update_Mass(); //must be called if change of mass

		//set linear/angular air/liquid drag coefficients
		void Set_Linear_Drag(dReal drag);
		void Set_Angular_Drag(dReal drag);

		//like above, but different coefficients for each (local) axis
		void Set_Axis_Linear_Drag(dReal x, dReal y, dReal z);
		void Set_Axis_Angular_Drag(dReal x, dReal y, dReal z); //TODO: perhaps change to matrix (like inertia tensor)?

		static void Physics_Step(dReal step);

		//body data bellongs to
		dBodyID body_id;

		//if rendering body, point at model
		Trimesh_3D *model;

		//buffer events (sent from geoms)
		void Set_Buffer_Event(dReal thresh, dReal buff, Script *scr);
		void Increase_Buffer(dReal add);
		void Damage_Buffer(dReal force, dReal step);
		bool Buffer_Event_Configured(); //check if configured (by geom)

	private:
		//used to find next/prev link in dynamically allocated chain
		//set next to null in last link in chain (prev = NULL in first)
		Body *prev, *next;
		static Body *head;
		friend void Render_List_Update(); //to allow loop through bodies

		//data for drag (air+water friction)
		//instead of the simple spherical drag model, use a
		//"squeezed/stretched" sphere?
		bool use_axis_linear_drag;
		bool use_axis_angular_drag;

		//drag values (must be adjusted to the body mass)
		dReal mass; //used for drag
		dReal linear_drag[3];
		dReal angular_drag[3];

		//event processing
		bool buffer_event; //buffer has just been depleted
		dReal threshold; //if allocated forces exceeds, eat buffer
		dReal buffer; //if buffer reaches zero, trigger event
		Script *buffer_script; //execute on event

		//private methods for drag
		void Linear_Drag(dReal step);
		void Angular_Drag(dReal step);
		void Axis_Linear_Drag(dReal step);
		void Axis_Angular_Drag(dReal step);
};

#endif

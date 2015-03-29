/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014 Mats Wahlberg
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

#ifndef _RCX_JOINT_H
#define _RCX_JOINT_H
#include <ode/ode.h>
#include <SDL/SDL.h>

#include "joint.hpp"
#include "component.hpp"

#include "assets/object.hpp"
#include "assets/script.hpp"

//Joint: (meta)data for joint (connects bodies), is used for:
//currently only for triggering event script (force threshold)
//
//>Dynamically allocated
class Joint: public Component
{
	public:
		Joint (dJointID joint, Object *obj);
		~Joint();

		static void Physics_Step(dReal step);

		//geom data bellongs to
		dJointID joint_id;

		//buffer event
		void Set_Buffer_Event(dReal thresh, dReal buff, Script *scr);
		void Increase_Buffer(dReal add);

	private:
		//used to find next/prev link in dynamically allocated chain
		//set next to null in last link in chain (prev = NULL in first)
		static Joint *head;
		Joint *prev, *next;

		//if car wheel attached
		bool *carwheel;

		//events
		bool buffer_event;
		//for buffer event processing
		dJointFeedback *feedback; //used if checking forces
		dReal threshold; //if force on body exceeds threshold, eat buffer
		dReal buffer; //if buffer reaches zero, trigger event
		Script *buffer_script; //the script to run

		friend class Car_Module;
		friend class Car;
		friend void Event_Buffers_Process(dReal); //to allow looping
};

#endif

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

#include "joint.hpp"
#include "component.hpp"
#include "simulation/event_buffers.hpp"

Joint *Joint::head = NULL;

Joint::Joint (dJointID joint, Object *obj): Component(obj)
{
	//add it to the list
	next = head;
	head = this;
	prev = NULL;

	if (next) next->prev = this;

	//add it to the joint
	dJointSetData (joint, (void*)(this));
	joint_id = joint;

	//default values (currently only event triggering)
	buffer_event = false; //disables event testing
	//TODO: send_to_body option?
	feedback = NULL;

	//not car suspension by default
	carwheel=NULL;
}

//destroys a joint, and removes it from the list
Joint::~Joint ()
{
	//remove all events
	Event_Buffer_Remove_All(this);

	//1: remove it from the list
	if (!prev) //head in list, change head pointer
		head = next;
	else //not head in list, got a previous link to update
		prev->next = next;

	if (next) //not last link in list
		next->prev = prev;

	//2: remove it from memory
	if (feedback) delete feedback;

	dJointDestroy(joint_id);
}


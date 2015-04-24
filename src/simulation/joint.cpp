/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015 Mats Wahlberg
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
#include "event_buffers.hpp"
#include "component.hpp"
#include "common/internal.hpp"

//creation/destruction:
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

//set event
void Joint::Set_Buffer_Event(dReal thres, dReal buff, Script *scr)
{
	if (thres > 0 && buff > 0 && scr)
	{
		feedback=new dJointFeedback;
		dJointSetFeedback (joint_id, feedback);

		//set feedback values to zero. note: normally not necessary
		//(since ode overwrites them at the next step), but there's a
		//slight chance it might get read before the next step, causing
		//unexpected weakening/breakage of joints (random values
		//instead of zero)
		memset(feedback, 0, sizeof(dJointFeedback));

		threshold=thres;
		buffer=buff;
		buffer_script=scr;

		//make sure no old event is left
		Event_Buffer_Remove_All(this);

		buffer_event=true;
	}
	else
	{
		buffer_event=false;
		//remove feedback data
		if (feedback)
		{
			delete feedback;
			feedback=NULL;
		}
		Event_Buffer_Remove_All(this);
		//disable
		dJointSetFeedback(joint_id, 0);
	}
}

//check for joint triggering
void Joint::Physics_Step (dReal step)
{
	Joint *d = Joint::head;
	dReal delt1, delt2, delt;

	while (d)
	{
		if (d->buffer_event)
		{
			//TODO: check torque also?
			delt1 = dLENGTH(d->feedback->f1);
			delt2 = dLENGTH(d->feedback->f2);

			if (delt1>delt2)
				delt = delt1;
			else
				delt = delt2;

			if (delt > d->threshold)
			{
				if (d->buffer < 0) //already depleted, just damage more
					d->buffer -= delt*step;
				else
				{
					d->buffer -= delt*step;
					if (d->buffer < 0)
						Event_Buffer_Add_Depleted(d);
				}
			}
		}

		d = d->next;
	}
}

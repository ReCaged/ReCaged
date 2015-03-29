/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014, 2015 Mats Wahlberg
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

#include <SDL/SDL_timer.h>
#include <SDL/SDL_mutex.h>
#include <ode/ode.h>

#include "common/threads.hpp"
#include "common/internal.hpp"
#include "common/runlevel.hpp"
#include "common/log.hpp"
#include "assets/track.hpp"
#include "assets/car.hpp"
#include "body.hpp"
#include "geom.hpp"
#include "camera.hpp"
#include "joint.hpp"

#include "collision_feedback.hpp"
#include "event_buffers.hpp"
#include "timers.hpp"

#include "interface/render_list.hpp"


unsigned int simulation_count = 0;
unsigned int simulation_lag_count = 0;
Uint32 simulation_lag_time = 0;
Uint32 simulation_time = 0;

bool Simulation_Init(void)
{
	Log_Add(0, "Initiating simulation");
	if (!dInitODE2(0))
	{
		Log_Add(-1, "Could not initiate ODE!");
		return false;
	}
	if (!dAllocateODEDataForThread(dAllocateFlagBasicData | dAllocateFlagCollisionData))
	{
		Log_Add(-1, "Could not allocate thread data for ODE!");
		return false;
	}

	world = dWorldCreate();

	//set global ode parameters (except those specific to track)

	space = dHashSpaceCreate(0);
	dHashSpaceSetLevels(space, internal.hash_levels[0], internal.hash_levels[1]);

	contactgroup = dJointGroupCreate(0);

	dWorldSetQuickStepNumIterations (world, internal.iterations);

	//autodisable
	dWorldSetAutoDisableFlag (world, 1);
	dWorldSetAutoDisableLinearThreshold (world, internal.dis_linear);
	dWorldSetAutoDisableAngularThreshold (world, internal.dis_angular);
	dWorldSetAutoDisableSteps (world, internal.dis_steps);
	dWorldSetAutoDisableTime (world, internal.dis_time);

	//joint softness
	dWorldSetERP (world, internal.erp);
	dWorldSetCFM (world, internal.cfm);

	//surface layer depth
	dWorldSetContactSurfaceLayer(world, internal.surface_layer);

	return true;
}


int Simulation_Loop (void *d)
{
	Log_Add(1, "Starting simulation loop");

	simulation_time = SDL_GetTicks(); //set simulated time to realtime
	simulation_count=0;
	simulation_lag_count=0;
	simulation_lag_time=0;

	Uint32 time; //real time
	Uint32 stepsize_ms = (Uint32) (internal.stepsize*1000.0+0.0001);
	dReal divided_stepsize = internal.stepsize/internal.multiplier;

	//keep running until done
	while (runlevel != done)
	{
		//only if in active mode do we simulate
		if (runlevel == running)
		{
			//technically, collision detection doesn't need locking, but this is easier
			SDL_mutexP(ode_mutex);

			for (int i=0; i<internal.multiplier; ++i)
			{
				//perform collision detection
				Geom::Clear_Collisions(); //clear all collision flags
				dSpaceCollide (space, (void*)(&divided_stepsize), &Geom::Collision_Callback);

				//special
				Wheel::Physics_Step(); //create contacts and rolling resistance
				Car::Physics_Step(divided_stepsize); //control, antigrav...
				Geom::Physics_Step(); //sensor/radar handling

				//simulate
				dWorldQuickStep (world, divided_stepsize);
				dJointGroupEmpty (contactgroup); //clean up collision joints

				//more
				Collision_Feedback::Physics_Step(divided_stepsize); //forces from collisions
				Body::Physics_Step(divided_stepsize); //drag (air/liquid "friction") and respawning
				Joint::Physics_Step(divided_stepsize); //joint forces
				camera.Physics_Step(divided_stepsize); //calculate velocity and move
			}

			//previous simulations might have caused events (to be processed by scripts)...
			Event_Buffers_Process(internal.stepsize);

			//process timers:
			Animation_Timer::Events_Step(internal.stepsize);

			//done with ode
			SDL_mutexV(ode_mutex);

			//opdate for interface:
			Render_List_Update(); //make copy of position/rotation for rendering
		}
		else
			Render_List_Update(); //still copy (in case camera updates or something)

		//broadcast to wake up sleeping threads
		if (internal.sync_interface)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondBroadcast (sync_cond);
			SDL_mutexV(sync_mutex);
		}

		simulation_time += stepsize_ms;

		//sync simulation with realtime
		if (internal.sync_simulation)
		{
			//got some time to while away?
			time = SDL_GetTicks();
			if (simulation_time > time) //ahead of reality, wait
			{
				//busy-waiting:
				if (internal.spinning)
					while (simulation_time > SDL_GetTicks());
				//sleep:
				else
					SDL_Delay (simulation_time-time);
			}
			else //oh no, we're lagging behind!
			{
				++simulation_lag_count; //increase lag step counter
				simulation_lag_time+=time-simulation_time; //add lag time
				simulation_time=time; //and pretend like nothing just hapened...
			}
		}

		//count how many steps
		++simulation_count;
	}

	//remove buffers for building rendering list
	Render_List_Clear_Simulation();

	return 0;
}

void Simulation_Quit (void)
{
	Log_Add(1, "Quit simulation");
	dJointGroupDestroy (contactgroup);
	dSpaceDestroy (space);
	dWorldDestroy (world);
	dCloseODE();
}


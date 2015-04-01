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
#include "common/log.hpp"
#include "common/lua.hpp"
#include "common/threads.hpp"
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

	//create lua state for this thread
	simulation_thread.lua_state = luaL_newstate();
	luaL_openlibs(simulation_thread.lua_state);

	//more ode stuff
	simulation_thread.world = dWorldCreate();

	//set global ode parameters (except those specific to track)

	simulation_thread.space = dHashSpaceCreate(0);
	dHashSpaceSetLevels(simulation_thread.space, internal.hash_levels[0], internal.hash_levels[1]);

	simulation_thread.contactgroup = dJointGroupCreate(0);

	dWorldSetQuickStepNumIterations (simulation_thread.world, internal.iterations);

	//autodisable
	dWorldSetAutoDisableFlag (simulation_thread.world, 1);
	dWorldSetAutoDisableLinearThreshold (simulation_thread.world, internal.dis_linear);
	dWorldSetAutoDisableAngularThreshold (simulation_thread.world, internal.dis_angular);
	dWorldSetAutoDisableSteps (simulation_thread.world, internal.dis_steps);
	dWorldSetAutoDisableTime (simulation_thread.world, internal.dis_time);

	//joint softness
	dWorldSetERP (simulation_thread.world, internal.erp);
	dWorldSetCFM (simulation_thread.world, internal.cfm);

	//surface layer depth
	dWorldSetContactSurfaceLayer(simulation_thread.world, internal.surface_layer);

	//okay, ready:
	simulation_thread.runlevel = running;

	return true;
}


int Simulation_Loop (void *d)
{
	Log_Add(1, "Starting simulation loop");

	Uint32 simulation_time = SDL_GetTicks(); //set simulated time to realtime
	simulation_thread.count=0;
	simulation_thread.lag_count=0;
	simulation_thread.lag_time=0;

	Uint32 time; //real time
	Uint32 stepsize_ms = (Uint32) (internal.stepsize*1000.0+0.0001);
	dReal divided_stepsize = internal.stepsize/internal.multiplier;

	//keep running until done
	while (simulation_thread.runlevel != done)
	{
		//only if in active mode do we simulate
		if (simulation_thread.runlevel == running)
		{
			//technically, collision detection doesn't need locking, but this is easier
			SDL_mutexP(simulation_thread.ode_mutex);

			for (int i=0; i<internal.multiplier; ++i)
			{
				//perform collision detection
				Geom::Clear_Collisions(); //clear all collision flags
				dSpaceCollide (simulation_thread.space, (void*)(&divided_stepsize), &Geom::Collision_Callback);

				//special
				Wheel::Physics_Step(); //create contacts and rolling resistance
				Car::Physics_Step(divided_stepsize); //control, antigrav...
				Geom::Physics_Step(); //sensor/radar handling

				//simulate
				dWorldQuickStep (simulation_thread.world, divided_stepsize);
				dJointGroupEmpty (simulation_thread.contactgroup); //clean up collision joints

				//more
				Collision_Feedback::Physics_Step(divided_stepsize); //forces from collisions
				Body::Physics_Step(divided_stepsize); //drag (air/liquid "friction") and respawning
				Joint::Physics_Step(divided_stepsize); //joint forces
				default_camera.Physics_Step(divided_stepsize); //calculate velocity and move
			}

			//previous simulations might have caused events (to be processed by scripts)...
			Event_Buffers_Process(internal.stepsize);

			//process timers:
			Animation_Timer::Events_Step(internal.stepsize);

			//done with ode
			SDL_mutexV(simulation_thread.ode_mutex);

			//opdate for interface:
			Render_List_Update(); //make copy of position/rotation for rendering
		}
		else
			Render_List_Update(); //still copy (in case camera updates or something)

		//broadcast to wake up sleeping threads
		if (internal.sync_interface)
		{
			SDL_mutexP(simulation_thread.sync_mutex);
			SDL_CondBroadcast (simulation_thread.sync_cond);
			SDL_mutexV(simulation_thread.sync_mutex);
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
				++simulation_thread.lag_count; //increase lag step counter
				simulation_thread.lag_time+=time-simulation_time; //add lag time
				simulation_time=time; //and pretend like nothing just hapened...
			}
		}

		//count how many steps
		++simulation_thread.count;
	}

	//remove buffers for building rendering list
	Render_List_Clear_Simulation();

	return 0;
}

void Simulation_Quit (void)
{
	Log_Add(1, "Quit simulation");
	lua_close(simulation_thread.lua_state);
	dJointGroupDestroy (simulation_thread.contactgroup);
	dSpaceDestroy (simulation_thread.space);
	dWorldDestroy (simulation_thread.world);
	dCloseODE();
}


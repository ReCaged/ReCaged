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

//This is a kind of "thread local storage," without using any non-portable
//platform/compiler features. Each thread got its own "Thread" struct, which it
//should pass on through function calls where needed. In the future multiple
//simulation/worker threads will be able to run in parallell, but for now this
//is just preparatory/transitional.

//TODO: this should probably become a class, and sim/int threads inherited

#ifndef _RCX_THREADS_H
#define _RCX_THREADS_H

#include <SDL/SDL_mutex.h>
#include <ode/ode.h>

#include "lua.hpp"

//use a "runlevel" (enum) variable to make all threads/loops aware of status
typedef enum {running, paused, done} runlevel_type;

struct Thread {
	//TODO: type{sim, int, ...}, or dynamic_cast<*>(...) safety checks!

	//for controlling thread (request termination/etc)
	runlevel_type runlevel;

	//lua state
	lua_State *lua_state;

	//for interface thread
	bool render_models;
	bool render_geoms;

	SDL_mutex *render_list_mutex; //prevents switching buffers at same time
	//(probability is low, but it could occur)

	//for simulation threads
	SDL_mutex *ode_mutex; //prevent simultaneous access
	SDL_mutex *sync_mutex; //for signaling a new frame ready to draw
	SDL_cond  *sync_cond; //-''-

	dWorldID world;
	dSpaceID space;
	dJointGroupID contactgroup;

	//statistics
	unsigned int count; //keep track of number of render/simulation steps
	unsigned int lag_count; //mainly for simulation
	unsigned int lag_time; //-''-
};

const Thread thread_defaults = {
	done, //runlevel
	NULL,
	true,
	true,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	0,
	0 };


//used for passing some finishing info
extern Uint32 starttime;
extern Uint32 racetime;

//just these, explicitly, two threads for now:
extern Thread interface_thread;
extern Thread simulation_thread;

//functions for handling the two threads
void Threads_Launch(void);

bool Interface_Init(bool window, bool fullscreen, int xres, int yres);
void Interface_Quit(void);
bool Simulation_Init(void);
void Simulation_Quit (void);

int Interface_Loop (void);
int Simulation_Loop (void *d);


//TMP: used for keeping track for objects spawning
#include "assets/object.hpp"
extern Module *box;
extern Module *sphere;
extern Module *funbox;
extern Module *molecule;

#endif

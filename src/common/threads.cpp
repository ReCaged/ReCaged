/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2015 Mats Wahlberg
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

#include <SDL/SDL.h>
#include "threads.hpp"

//global Thread variables, will be more dynamic in future
Thread interface_thread = thread_defaults;
Thread simulation_thread = thread_defaults;

Uint32 starttime = 0;
Uint32 racetime = 0;
void Threads_Launch(void)
{
	//start
	Log_Add (0, "Launching Threads (and race)");

	//create mutex for ode locking
	simulation_thread.ode_mutex = SDL_CreateMutex();

	//and for signaling new render
	simulation_thread.sync_mutex = SDL_CreateMutex();
	simulation_thread.sync_cond = SDL_CreateCond();

	//prevent (unlikely) update/render collision
	simulation_thread.render_list_mutex = SDL_CreateMutex();

	starttime = SDL_GetTicks(); //how long it took for race to start

	//launch threads
	SDL_Thread *simulation = SDL_CreateThread (Simulation_Loop, NULL);
	Interface_Loop(); //we already got opengl context in main thread

	//just in case, to prevent accidental locks:
	simulation_thread.runlevel=done;

	//wait for threads
	SDL_WaitThread (simulation, NULL);
	racetime = SDL_GetTicks() - starttime;

	//cleanup
	SDL_DestroyMutex(simulation_thread.ode_mutex);
	SDL_DestroyMutex(simulation_thread.sync_mutex);
	SDL_DestroyCond(simulation_thread.sync_cond);
	SDL_DestroyMutex(simulation_thread.render_list_mutex);

	//done!
	Log_Add(0, "Threads (and race) Finished");
}


/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
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

#include "threads.hpp"


//mutexes/conditions:

//prevent threads from switching buffers at the same
//(probability is low, but it could occur)
SDL_mutex *render_list_mutex = NULL;

//make sure only working with one line at a time
SDL_mutex *log_mutex = NULL;

//only one thread for ode
SDL_mutex *ode_mutex = NULL;

//only one thread for sdl
SDL_mutex *sdl_mutex = NULL;

//for using sync_cond
SDL_mutex *sync_mutex = NULL;

//threads can sleep until synced
SDL_cond  *sync_cond  = NULL;


//threads

//simulation
SDL_Thread *simulation_thread = NULL;

//lua states:

//simulation through lua
lua_State *lua_sim = NULL;

//interface through lua
lua_State *lua_int = NULL;


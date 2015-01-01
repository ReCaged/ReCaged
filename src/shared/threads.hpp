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

#ifndef _RCX_THREADS_H
#define _RCX_THREADS_H

#include <SDL/SDL_mutex.h>

//mutexes
extern SDL_mutex *ode_mutex;
extern SDL_mutex *sdl_mutex;
extern SDL_mutex *sync_mutex;
extern SDL_mutex *render_list_mutex;
extern SDL_cond  *sync_cond;
//


//prototypes for communication/statistic variables
//simulation:
extern unsigned int simulation_count;
extern unsigned int simulation_lag_count;
extern unsigned int simulation_lag_time;

//interface:
extern unsigned int interface_count;
extern bool render_models;
extern bool render_geoms;

//functions
bool Interface_Init(bool window, bool fullscreen, int xres, int yres);
void Interface_Quit(void);
bool Simulation_Init(void);
void Simulation_Quit (void);

int Interface_Loop (void);
int Simulation_Loop (void *d);



//TMP: used for keeping track for objects spawning
#include "object.hpp"
extern Module *box;
extern Module *sphere;
extern Module *funbox;
extern Module *molecule;

#endif

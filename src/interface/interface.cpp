/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Mats Wahlberg
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

#include <SDL/SDL.h>
#include <GL/glew.h>

extern "C" {
#include HEADER_LUA_H
#include HEADER_LUALIB_H
#include HEADER_LAUXLIB_H
}

#include "../shared/internal.hpp"
#include "../shared/track.hpp"
#include "../shared/threads.hpp"
#include "../shared/log.hpp"
#include "../shared/profile.hpp"
#include "../shared/directories.hpp"

#include "../shared/camera.hpp"
#include "render_list.hpp"
#include "geom_render.hpp"

SDL_Surface *screen;
const Uint32 flags = SDL_OPENGL | SDL_RESIZABLE;
int joysticks=0;
SDL_Joystick **joystick;

//list of lua functions:
int int_TMP (lua_State *lua);
int int_TMP_run (lua_State *lua);
const luaL_Reg lua_interface[] =
{
	{"tmp_frame", int_TMP},
	{"tmp_running", int_TMP_run},
	{NULL, NULL}
};
//
bool tmp_running=true;
int int_TMP_run (lua_State *lua)
{
	lua_pushboolean(lua, tmp_running);
	return 1;
}


//TMP: keep track of demo spawn stuff
Object_Template *box = NULL;
Object_Template *sphere = NULL;
Object_Template *funbox = NULL;
Object_Template *molecule = NULL;

//count frames
unsigned int interface_count = 0;

//needed by graphics_buffers:
float view_angle_rate_x=0.0;
float view_angle_rate_y=0.0;
//

//remember if rendering background or not
bool background = true;
//

void Resize (int new_w, int new_h)
{
	screen = SDL_SetVideoMode (new_w, new_h, 0, flags);
	int w=screen->w;
	int h=screen->h;

	glViewport (0,0,w,h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	//lets calculate viewing angle based on the players _real_ viewing angle...
	//the player should specify an eye_distance in internal.conf
	//
	//(divide your screens resolution height or width with real height or
	//width, and multiply that with your eyes distance from the screen)
	//
	float angle;

	if (!internal.angle) //good
	{
		//angle between w/2 (distance from center of screen to right edge) and players eye distance
		angle = atan( (((GLfloat) w)/2.0)/internal.dist );
		Log_Add(1, "(perspective: %f degrees, based on (your) eye distance: %f pixels", angle*180/M_PI, internal.dist);
	}
	else //bad...
	{
		Log_Add(1, "Angle forced to: %f degrees. And you are an evil person...", internal.angle);
		angle = ( (internal.angle/2.0) * M_PI/180);;
	}

	//useful for more things:
	//x = rate*depth
	view_angle_rate_x = tan(angle);
	//y = rate*depth (calculated from ratio of window resolution aspect)
	view_angle_rate_y = view_angle_rate_x * ((GLfloat) h/w);
	//

	//x position at close clipping
	GLfloat x = internal.clipping[0] * view_angle_rate_x;
	//y -''- 
	GLfloat y = internal.clipping[0] * view_angle_rate_y;


	//set values
	glFrustum(-x, x, -y, y, internal.clipping[0], internal.clipping[1]);

	//switch back to usual matrix
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
}

bool Interface_Init(bool window, bool fullscreen, int xres, int yres)
{
	Log_Add(0, "Initiating interface");

	//create lua state
	lua_int = luaL_newstate();
	if (!lua_int)
	{
		Log_Add(0, "ERROR: could not create new lua state");
		return false;
	}

	//lua libraries allowed/needed:
	luaopen_string(lua_int);
	luaopen_table(lua_int);
	luaopen_math(lua_int);

	//custom libraries:
	luaL_register(lua_int, "log", lua_log);
	luaL_register(lua_int, "interface", lua_interface);
	luaL_register(lua_int, "simulation", lua_simulation);

	//end of lua init (for now)


	//initiate sdl
	if (SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_JOYSTICK))
	{
		Log_Add(0, "Error: couldn't initiate video or joystick: %s", SDL_GetError());
		return false;
	}

	//set title:
	SDL_WM_SetCaption ("ReCaged " PACKAGE_VERSION " (\"" PACKAGE_CODENAME "\") (C) " PACKAGE_YEAR " Mats Wahlberg", "ReCaged");

	//TODO: set icon (SDL_WM_SetIcon, from embedded into the executable?)

	//configure properties before creating window
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1); //make sure double-buffering
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16); //good default (Z buffer)
	SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 0); //not used yet

	//try to create window
	//TODO: when SDL 1.3 is released, SDL_CreateWindow is deprecated in favor of:
	//SDL_CreateWindow and SDL_GL_CreateContext
	//ALSO (sdl>1.2): try setting core context for gl 3.x. If possible: unlegacy rendering
	int x, y;
	if (xres > 0) x=xres; else x=internal.res[0];
	if (yres > 0) y=yres; else y=internal.res[1];
	screen = SDL_SetVideoMode (x, y, 0, flags); //TODO: SDL_GetVideoInfo() provides much needed info for fallbacks here

	if (!screen)
	{
		Log_Add(0, "Error: couldn't set video mode: %s", SDL_GetError());
		return false;
	}

	//check if graphics is good enough
	if (glewInit() == GLEW_OK)
	{
		//not 1.5
		if (!GLEW_VERSION_1_5)
		{
			//should check ARB extensions if GL<1.5, but since this only affects old
			//systems (the 1.5 standard was released in 2003), I'll ignore it...
			Log_Add(0, "Error: you need GL 1.5 or later");
			return false;
		}
	}
	else
	{
		Log_Add(0, "Error: couldn't init glew");
		return false;
	}

	//hide cursor
	SDL_ShowCursor (SDL_DISABLE);

	//toggle or prevent fullscreen (if requested/disabled in either conf or args)
	if (!window && (fullscreen || internal.fullscreen))
	{
		if (SDL_WM_ToggleFullScreen(screen))
			Log_Add(1, "Enabled fullscreen mode");
		else
			Log_Add(0, "Error: unable to toggle fullscreen");
	}

	//set up window, as if resized
	Resize (screen->w, screen->h);

	//grab all joysticks/gamepads detected
	joysticks=SDL_NumJoysticks();
	if (joysticks != 0)
	{
		joystick = new SDL_Joystick*[joysticks];
		Log_Add(1, "Detected %i joystick(s). Opening:", joysticks);

		for (int i=0; i<joysticks; ++i)
		{
			Log_Add(1, "Device %i: \"%s\"", i, SDL_JoystickName(i));
			joystick[i]= SDL_JoystickOpen(i);
			if (!joystick[i])
				Log_Add(0, "Failed to open joystick %i", i);
		}
	}
	//
	//options
	//
	if (internal.culling)
		culling=true;

	//if positive and closer than far clipping
	if (internal.fog > 0.0 && internal.fog < internal.clipping[1])
	{
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogf(GL_FOG_START, internal.fog);
		glFogf(GL_FOG_END, internal.clipping[1]);
		fog=true;
	}

	//make sure everything is rendered with highest possible quality
	glHint(GL_FOG_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	//things possibly used in future:
	/*
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
	glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
	*/

	//everything ok
	return true;
}



bool Interface_Loop ()
{
	Log_Add(1, "Starting interface loop");

	//find rc.lua
	Directories dirs;
	if (!dirs.Find("rc.lua", DATA, READ))
	{
		Log_Add(0, "ERROR: could not find \"rc.lua\" script!");
		return NULL;
	}

	//load file as chunk
	if (luaL_loadfile(lua_int, dirs.Path()))
	{
		Log_Add(0, "ERROR: could not load \"rc.lua\" script: \"%s\"!",
				lua_tostring(lua_int, -1));
		lua_pop(lua_int, -1);
		return false;
	}

	//execute chunk
	if (lua_pcall(lua_int, 0, 0, 0))
	{
		Log_Add(0, "ERROR: \"%s\" while running \"rc.lua\"!",
				lua_tostring(lua_int, -1));
		lua_pop(lua_int, -1);
		return false;
	}
	//end of lua script

	return true; //if we got here, quit ok
}

//TODO:
//just make sure not rendering geoms yet
//geom_render_level = 0;
Uint32 time_old = 0;
int int_TMP(lua_State *lua)
{

		//make sure only render frame after it's been simulated
		//quckly lock mutex in order to listen to simulation broadcasts
		//(but only if there is no new frame already)
		if (internal.sync_interface && !Render_List_Updated())
		{
			SDL_mutexP(sync_mutex);
			SDL_CondWaitTimeout (sync_cond, sync_mutex, 500); //if no signal in half a second, stop waiting
			SDL_mutexV(sync_mutex);
		}

		//get time
		Uint32 time = SDL_GetTicks();
//TODO: revert to old (looping) approach
		//how much it differs from old
		Uint32 delta = time-time_old;
		time_old = time;
		//

		//current car
		Car *car = profile_head->car;

		//check all sdl events
		SDL_Event event;
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode (event.resize.w, event.resize.h, 0, flags);

					if (screen)
						Resize (screen->w, screen->h);
					else
						Log_Add(0, "Warning: resizing failed");
				break;

				case SDL_QUIT:
					tmp_running = false;
				break;

				case SDL_ACTIVEEVENT:
					if (event.active.gain == 0)
						Log_Add(1, "(FIXME: pause when losing focus (or being iconified)!)");
				break;

				//check for special key presses (tmp debug/demo keys)
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							tmp_running = false;
						break;

						//box spawning
						case SDLK_F5:
							SDL_mutexP(ode_mutex);
							box->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//sphere spawning
						case SDLK_F6:
							SDL_mutexP(ode_mutex);
							sphere->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//spawn funbox
						case SDLK_F7:
							SDL_mutexP(ode_mutex);
							funbox->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//molecule
						case SDLK_F8:
							SDL_mutexP(ode_mutex);
							molecule->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//switch car
						case SDLK_F9:
							//not null
							if (car)
							{
								//next in list
								car = car->next;
								//in case at end of list, go to head
								if (!car)
									car = Car::head;

								//set new car
								profile_head->car = car;
								camera.Set_Car(car);
							}
						break;

						//respawn car
						case SDLK_F10:
							if (car)
							{
								SDL_mutexP(ode_mutex);
								car->Respawn(track.start[0], track.start[1], track.start[2]);
								SDL_mutexV(ode_mutex);
							}
						break;

						//paus physics
						case SDLK_F11:
							if (runlevel == paused)
								runlevel = running;
							else
								runlevel = paused;
						break;

						//switch what to render
						case SDLK_F12:
							//increase geom rendering level, and reset if at last
							if (++geom_render_level >= 6)
								geom_render_level = 0;
						break;

						default:
							break;
					}
				break;

				default:
					break;
			}

			//make sure not performing anything else
			if (runlevel == done)
				break;

			//and always send this to profiles
			Profile_Input_Collect(&event);
		}

		//again, not performing anything else
		//if (runlevel == done)
			//break;

		//(tmp) camera movement keys:
		Uint8 *keys = SDL_GetKeyState(NULL);

		if (keys[SDLK_d]) //+x
			camera.Move(+0.03*delta, 0, 0);
		if (keys[SDLK_a]) //-x
			camera.Move(-0.03*delta, 0, 0);

		if (keys[SDLK_w]) //+y
			camera.Move(0, +0.03*delta, 0);
		if (keys[SDLK_s]) //-y
			camera.Move(0, -0.03*delta, 0);

		if (keys[SDLK_q]) //+z
			camera.Move(0, 0, +0.03*delta);
		if (keys[SDLK_e]) //-z
			camera.Move(0, 0, -0.03*delta);
		//

		//car control
		if (runlevel == running)
			Profile_Input_Step(delta);


		//done with sdl
		SDL_mutexV(sdl_mutex);

		//start rendering

		//check if now switching to geom rendering with no background...
		if (geom_render_level == 5 && background)
		{
			//black background
			glClearColor (0.0, 0.0, 0.0, 1.0);
			background = false;
		}
		//if now switching from geom rendering...
		else if (!background && geom_render_level != 5)
		{
			//restore track specified background
			glClearColor (track.sky[0],track.sky[1],track.sky[2],1.0);
			background = true;
		}

		//clear screen
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//check/set updated scene+camera
		Render_List_Prepare();

		//place sun
		glLightfv (GL_LIGHT0, GL_POSITION, track.position);

		//render models (if not 2 or more level of geom rendering)
		if (geom_render_level < 3)
			Render_List_Render();

		//render geoms (if nonzero level)
		if (geom_render_level)
			Geom_Render();

		//swap the 2 gl buffers
		SDL_GL_SwapBuffers();

		//keep track of how many rendered frames
		++interface_count;

	return 0;
}

void Interface_Quit(void)
{

	//TODO: move somewhere else
	//during rendering, memory might be allocated
	//(will quickly be reallocated in each race and can be removed)
	Geom_Render_Clear();
	Render_List_Clear_Interface();
	//

	Log_Add(1, "Quit interface");
	lua_close(lua_int);
	SDL_Quit();

	//close all joysticks
	for (int i=0; i<joysticks; ++i)
		if (joystick[i])
			SDL_JoystickClose(joystick[i]);
	delete[] joystick;

	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
}


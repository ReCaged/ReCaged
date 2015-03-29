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

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "common/internal.hpp"
#include "common/runlevel.hpp"
#include "common/threads.hpp"
#include "common/log.hpp"
#include "common/directories.hpp"

#include "assets/track.hpp"
#include "assets/profile.hpp"

#include "render_list.hpp"
#include "geom_render.hpp"

#include "assets/image.hpp"

SDL_Surface *screen;
Uint32 flags = SDL_OPENGL | SDL_RESIZABLE;
int bpp;
int joysticks=0;
SDL_Joystick **joystick;

//TMP: keep track of demo spawn stuff
Module *box = NULL;
Module *sphere = NULL;
Module *funbox = NULL;
Module *molecule = NULL;

//count frames
unsigned int interface_count = 0;

//needed by graphics_buffers:
float view_angle_rate_x=0.0;
float view_angle_rate_y=0.0;
//

//remember if rendering background or not
bool background = true;
//

//for vbo handling (for splash screen)
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

void Resize (int new_w, int new_h)
{
	Log_Add(1, "Resizing to: %dx%d in pixels", new_w, new_h);
	screen = SDL_SetVideoMode (new_w, new_h, bpp, flags);

	if (!screen)
	{
		Log_Add(0, "Warning: Could not update video mode on resize!");
		return; //can't really quit here, just don't do anything
	}
	int w=screen->w;
	int h=screen->h;

	glViewport (0,0,w,h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	//lets calculate FOV based on the player's _real_ perspective...
	//the player should specify an eye_distance in internal.conf
	float angle;

	if (internal.fov) //fixed
	{
		angle = ( (internal.fov/2.0) * M_PI/180);
	}
	else //dynamic
	{
		//angle between w/2 (distance from center of screen to right edge) and players eye distance
		angle = atan( (((GLfloat) w)/2.0)/internal.dist );
	}

	Log_Add(1, "Window resize: %dx%d, FOV: %f (%s)", w, h, 2*angle*180/M_PI, internal.fov? "fixed":"dynamic");

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


//simple rendering of image (splash screen) when loading
bool Interface_Splash(const char *file, int screenx, int screeny)
{
	Log_Add(2, "Using \"%s\" for splash screen", file);

	//load texture from file
	Image image;

	if (!image.Load(file))
		return false;

	//create texture and bind it
	Image_Texture *texture=image.Create_Texture();

	if (!texture)
		return false;

	glBindTexture(GL_TEXTURE_2D, texture->GetID());

	//create quad for rendering it
	GLfloat imageratio= (GLfloat)image.Width() / (GLfloat)image.Height();
	GLfloat screenratio = (GLfloat)screenx / (GLfloat)screeny;
	GLfloat x,y; //size of quad

	//wider than window, fill width (border above/below)
	if (imageratio > screenratio)
	{
		x=(GLfloat)screenx; //100%
		y=(x/imageratio); //=x*height/width
	}
	//narrower than window (fill height, borders on sides)
	else
	{
		y=(GLfloat)screeny;
		x=(y*imageratio);
	}

	//x and y gives us the bottom-right corner of the quad, if also got the
	//top-left corner. enter x0 and y0!
	GLfloat x0=((GLfloat)screenx-x)/2.0; //half of unused screen area (if any)
	GLfloat y0=((GLfloat)screeny-y)/2.0; //-''-

	//x, y, z, u, v components/channels
	GLfloat quad[]=
	{
		x0+x,	y0,	-1.0,	1.0,	0.0,
		x0,	y0,	-1.0,	0.0,	0.0,
		x0,	y0+y,	-1.0,	0.0,	1.0,
		x0+x,	y0+y,	-1.0,	1.0,	1.0,
	};

	//create vbo and upload above quad to it
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4*5, quad, GL_STATIC_DRAW);
	glVertexPointer(3, GL_FLOAT, sizeof(GLfloat)*5, BUFFER_OFFSET(0));
	glTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat)*5, BUFFER_OFFSET(sizeof(GLfloat)*3));

	//change projection matrix to ortho mode, store old matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, screenx, screeny, 0, 0, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); //clear, just in case

	//arbitrary clear colour, but black background works for most people
	glClearColor (0.0, 0.0, 0.0, 1.0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//enable lots of stuff:
	glEnableClientState(GL_VERTEX_ARRAY); //coordinates
	glEnableClientState(GL_TEXTURE_COORD_ARRAY); //UVs

	glEnable(GL_TEXTURE_2D); //texture
	glEnable(GL_BLEND); //alpha blending (looks even better without a single background colour...)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //a common function, looks good

	//
	//and everything done, just to be able to run the following, once.
	glDrawArrays(GL_QUADS, 0, 4); //ZOMG!
	SDL_GL_SwapBuffers(); //and show it to the user! :)
	//

	//and now start the clean up:
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	//delete vbo and texture
	glDeleteBuffers(1, &vbo);
	delete texture;

	//restore good old perspective matrix
	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);

	return true;
}

bool Interface_Init(bool window, bool fullscreen, int xres, int yres)
{
	Log_Add(0, "Initiating interface");

	//initiate sdl
	if (SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_JOYSTICK))
	{
		Log_Add(-1, "Could not initiate video or joystick: %s", SDL_GetError());
		return false;
	}

	//set title:
	SDL_WM_SetCaption ("RCX " PACKAGE_VERSION " (\"" PACKAGE_CODENAME "\")", "RCX");

	//TODO: set icon (SDL_WM_SetIcon, from embedded into the executable?)

	//configure properties before creating window
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1); //make sure double-buffering
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16); //good default (Z buffer)
	SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 0); //not used yet
	if (internal.msaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, internal.msaa);
	}
	const SDL_VideoInfo *info = SDL_GetVideoInfo(); //for native resolution&bpp

	//not sure if this can fail, but just in case:
	if (!info)
	{
		Log_Add(-1, "Could not get video info: %s", SDL_GetError());
		return false;
	}

	//store current bpp as global
	bpp = info->vfmt->BitsPerPixel;

	//try to create window
	//TODO: when SDL 1.3 is released, SDL_CreateWindow is deprecated in favor of:
	//SDL_CreateWindow and SDL_GL_CreateContext
	//ALSO (sdl>1.2): try setting core context for gl 3.x. If possible: unlegacy rendering
	int x, y;
	if (!window && (fullscreen || internal.fullscreen)) //fullscreen (and not forcing window)
	{
		flags |= SDL_FULLSCREEN; //add fullscreen flag
		//set resolution from screen info
		x = info->current_w;
		y = info->current_h;

		Log_Add(1, "Window creation (fullscreen): %dx%d, BPP: %d, MSAA: %d", x, y, bpp, internal.msaa);
	}
	else //windowed mode
	{
		//set resolution from conf or as forced by args
		x= (xres > 0)? xres : internal.res[0];
		y= (yres > 0)? yres : internal.res[1];

		Log_Add(1, "Window creation: %dx%d, BPP: %d, MSAA: %d", x, y, bpp, internal.msaa);
	}

	//try to set video
	screen = SDL_SetVideoMode (x, y, bpp, flags);

	//failure
	if (!screen)
	{
		if (internal.msaa)
		{
			Log_Add(-1, "Could not set video mode, will try again without MSAA: %s", SDL_GetError());
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			internal.msaa=0; //make sure rest of code knows it's disabled
			screen = SDL_SetVideoMode (x, y, bpp, flags);
		}

		if (!screen)
		{
			Log_Add(-1, "Could not set video mode: %s", SDL_GetError());
			return false;
		}
	}

	//check if graphics is good enough
	if (glewInit() == GLEW_OK)
	{
		//not 1.5
		if (!GLEW_VERSION_1_5)
		{
			//should check ARB extensions if GL<1.5, but since this only affects old
			//systems (the 1.5 standard was released in 2003), I'll ignore it...
			Log_Add(-1, "You need OpenGL version 1.5 or later. Your version is: \"%s\"", glGetString(GL_VERSION));
			//TODO: Log_Add "Or GL 1.? with extensions: EXT_?
			//(yes/no), EXT_? (yes/no) etc..."
			return false;
		}
	}
	else
	{
		Log_Add(-1, "Could not initiate GLEW");
		return false;
	}

	//hide cursor
	SDL_ShowCursor (SDL_DISABLE);

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

	//this seems like a good place to configure lighting model:
	if (internal.separate_specular) //calculate specular lighting _without_ texture
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	else
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);

	//and render splash screen, if found (not fatal if fails)
	Directories dirs;
	if (dirs.Find("recaged.png", DATA, READ))
		Interface_Splash(dirs.Path(), screen->w, screen->h);

	//things possibly used in future:
	/*
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
	glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
	*/

	//everything ok
	return true;
}

int Interface_Loop ()
{
	Log_Add(1, "Starting interface loop");

	//just make sure not rendering geoms yet
	geom_render_level = 0;

	//store current time
	Uint32 time_old = SDL_GetTicks();

	//only stop render if done with race
	while (runlevel != done)
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
					Resize (event.resize.w, event.resize.h);
				break;

				case SDL_QUIT:
					runlevel = done;
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
							runlevel = done;
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
		if (runlevel == done)
			break;

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
	}

	//during rendering, memory might be allocated
	//(will quickly be reallocated in each race and can be removed)
	Geom_Render_Clear();
	Render_List_Clear_Interface();

	return 0;
}

void Interface_Quit(void)
{
	Log_Add(1, "Quit interface");

	//close all joysticks
	for (int i=0; i<joysticks; ++i)
		if (joystick[i])
			SDL_JoystickClose(joystick[i]);
	delete[] joystick;

	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
}


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

//Required stuff:
#include <SDL/SDL.h>
#include <getopt.h>

//local stuff:
#include "shared/internal.hpp"
#include "shared/threads.hpp"
#include "shared/log.hpp"
#include "shared/directories.hpp"
#include "shared/profile.hpp"
#include "shared/track.hpp"
#include "shared/trimesh.hpp"
#include "shared/directories.hpp"


//
// main()
//

//arguments
static const struct option options[] = 
{
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{ "config", required_argument, NULL, 'c' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "window", no_argument, NULL, 'w' },
	{ "fullscreen", no_argument, NULL, 'f' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "width", required_argument, NULL, 'x' },
	{ "height", required_argument, NULL, 'y' },
	{ "portable", optional_argument, NULL, 'p' },
	{ "user", optional_argument, NULL, 'u' },
	{ "installed", optional_argument, NULL, 'i' },
	//
	//TODO (for lua)
	//run script.lua instead
	//-- end of normal options, followed by lua args
	//-- help for lua options
	//
	{ NULL, 0, NULL, 0 }
};


//main function, will change a lot in future versions...
int main (int argc, char *argv[])
{
	//make sure we can print (just stdout for now)
	//"Log" uses mutexes, so make sure sdl has initiated
	//(also init timers when at it)
	if (SDL_Init(SDL_INIT_TIMER))
	{
		printf("Error: couldn't initiate SDL: %s", SDL_GetError());
	}
	Log_Init();

	//use getopt_long to parse options to override defaults:
	char c;
	bool window=false, fullscreen=false;
	int xres=0, yres=0;
	char *port_overr=NULL, *inst_overr=NULL, *user_overr=NULL, *conf_overr=NULL;
	bool inst_force=false, port_force=false;

	//TODO: might want to compare optind and argc afterwards to detect missing or extra arguments (like file)
	while ( (c = getopt_long(argc, argv, "hVc:vqwfx:y:p::u::i::", options, NULL)) != -1 )
	{
		printf("!!!! not working right now !!!!\n");
		switch(c)
		{
			case 'V': //data directory
				Log_puts(1, "ReCaged " PACKAGE_VERSION " (\"" PACKAGE_CODENAME "\")\nCopyright (C) " PACKAGE_YEAR " Mats Wahlberg\n");
				exit(0); //stop execution
				break;

			case 'c':
				conf_overr=optarg;
				break;

			case 'v':
				Log_Change_Verbosity(+1);
				break;

			case 'q':
				Log_Change_Verbosity(-1);
				break;

			case 'w':
				window=true;
				fullscreen=false;
				break;

			case 'f':
				fullscreen=true;
				window=false;
				break;

			case 'x':
				xres=atoi(optarg);
				if (xres <= 0)
				{
					Log_Add(0, "ERROR: window width requires a positive integer");
					exit(-1);
				}
				break;

			case 'y':
				yres=atoi(optarg);
				if (yres <= 0)
				{
					Log_Add(0, "ERROR: window height requires a positive integer");
					exit(-1);
				}
				break;

			case 'p':
				port_force=true;
				inst_force=false;
				port_overr=optarg;
				user_overr=NULL;
				inst_overr=NULL;
				break;

			case 'u':
				port_force=false;
				inst_force=true;
				port_overr=NULL;
				user_overr=optarg;
				inst_overr=NULL;
				break;

			case 'i':
				port_force=false;
				inst_force=true;
				port_overr=NULL;
				user_overr=NULL;
				inst_overr=optarg;
				break;

			default: //print help output
				//TODO: "Usage: %s [OPTION]... -- [LUA OPTIONS]\n"
				Log_puts(1, "\
Usage: recaged [OPTION]...\n\
  -h, --help		display help and exit\n\
  -V, --version		display version and exit\n\
  -c, --config FILE	load settings from FILE\n\
  -v, --verbose		increase stdout verbosity\n\
  -q, --quiet		decrease stdout verbosity\n\
\n\
Options for overriding window creation:\n\
  -w, --window		render in window\n\
  -f, --fullscreen	render in fullscreen\n\
  -x, --width PIXELS	render with PIXELS width\n\
  -y, --height PIXELS	render with PIXELS height\n\
\n\
Options for overriding normal (automatic) directory detection:\n\
  -p[DIR], --portable[=DIR] Force \"portable\" mode: read/write all files in a\n\
			single directory, located around the executable if not\n\
			specified. Overrides any earlier -u and -i\n\n\
  -u[DIR], --user[=DIR]	Force \"installed\" mode: write files in home directory\n\
			and read from home and global directories. Optionally\n\
			overrides the user directory. Overrides any earlier -p\n\n\
  -i[DIR], --installed[=DIR] Force \"installed\" mode: just like -u above, but\n\
  			optionally overrides the installed (global) directory.\n\
			Both can be combined in order to specify both paths\n");

				exit(0); //stop execution
				break;
		}
	}










	//welcome message
	Log_printf(0, "\n\t-=[ Welcome to ReCaged version %s (\"%s\") ]=-\n\n", PACKAGE_VERSION, PACKAGE_CODENAME);

	//unlikely to fail, but still possible (if incorrect path overriding or something)
	if (!Directories::Init(argv[0], inst_force, port_force, inst_overr, user_overr, port_overr))
		return -1;

	//enable file logging (if possible)
	Directories dirs;
	if (dirs.Find("log.txt", CACHE, WRITE)) Log_File(dirs.Path());

	//load conf file if found
	if (conf_overr)
		Load_Conf (conf_overr, (char*)&internal, internal_index);
	else if (dirs.Find("internal.conf", CONFIG, READ)) //try find by default
		Load_Conf (dirs.Path(), (char *)&internal, internal_index);

	//disable file logging is requested
	if (!internal.logfile)
		Log_File(NULL);

	//update log verbosity according to settings in conf _and_ any arguments)
	Log_Change_Verbosity((internal.verbosity-1));

	//TODO: rotate credits/libraries order/descriptions
	Log_puts(1, "\
ReCaged is copyright (C) 2009, 2010, 2011, 2012 Mats Wahlberg\n\
\n\
ReCaged is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation, either version 3 of the License, or\n\
(at your option) any later version.\n\
\n\
ReCaged is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\
\n\n\
				-=[ Credits ]=-\n\n\
* Mats Wahlberg (\"Slinger\")		-	Creator (coder) + development 3D models\n\
* \"K.Mac\"				-	Extensive testing, hacks and new ideas\n\
* \"Spontificus\"				-	Testing, hacks and various fixes\n\n\
\n		-=[ Other Projects that made RC possible ]=-\n\n\
* \"Free Software Foundation\"		-	\"Free Software, Free Society\", supporting the free software movement\n\
* \"The GNU Project\"			-	Developing a Free OS. Its work for freedom has changed the world\n\
* \"Simple DirectMedia Layer\"		-	OS/hardware abstraction library\n\
* \"Open Dynamics Engine\"		-	Rigid body dynamics and collision detection library\n\
* \"OpenGL Extension Wrangler\"		-	OpenGL version/extension loader\n\n\n\
Default key bindings can be found (and changed) in \"data/profiles/default/keys.lst\"\n\
More keys exists for debug/testing/demo, see README if you are interested.\n\n\
- See README for more info -\n\n");


	//ok, start loading
	Log_Add(1, "Loading...");

	//initiate interface
	if (!Interface_Init(window, fullscreen, xres, yres))
		return -1;

	//initiate simulation
	if (!Simulation_Init())
	{
		//menu: warn and quit!
		Interface_Quit();
		return false;
	}


	ode_mutex = SDL_CreateMutex(); //create mutex for ode locking
	sdl_mutex = SDL_CreateMutex(); //only use sdl in 1 thread
	sync_mutex = SDL_CreateMutex();
	sync_cond = SDL_CreateCond();
	render_list_mutex = SDL_CreateMutex(); //prevent (unlikely) update/render collision

	runlevel  = running;

	//TODO: not accurate, move to when RC:LUA stops loading and inits the render loop...
	starttime = SDL_GetTicks(); //how long it took for race to start

	//launch threads
	Interface_Loop(); //we already got opengl context in main thread
	Interface_Quit();

	//cleanup
	SDL_DestroyMutex(ode_mutex);
	SDL_DestroyMutex(sdl_mutex);
	SDL_DestroyMutex(sync_mutex);
	SDL_DestroyMutex(render_list_mutex);
	SDL_DestroyCond(sync_cond);

	//done!
	Log_Add(0, "Race Done!");

	racetime = SDL_GetTicks() - starttime;
	simtime = simtime - starttime;


	//might be interesting
	//TODO: most will be removed!
	Log_puts(1, "\n\n   <[ Info ]>\n");
	Log_Add(1, "Startup time:		%ums", starttime);
	Log_Add(1, "Race time:			%ums", racetime);

	Log_Add(1, "Simulated time:		%ums (%u%% of real time)",
						simtime, (100*simtime)/racetime);

	Log_Add(1, "Average simulations/second:	%u steps/second (%u in total)",
						(1000*simulation_count)/racetime, simulation_count);

	Log_Add(1, "Simulation lag:		%u%% of steps (%u in total)",
						(100*simulation_lag)/simulation_count, simulation_lag);

	Log_Add(1, "Average frames/second:	%u FPS (%u%% of simulation steps)",
						(1000*interface_count)/racetime, (100*interface_count)/simulation_count);

	Log_puts(1, "\n Bye!\n\n");

	//close
	SDL_Quit();
	Log_Quit();
	Directories::Quit();

	return 0;
}


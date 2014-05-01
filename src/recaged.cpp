/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014 Mats Wahlberg
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
#include "shared/runlevel.hpp"
#include "shared/profile.hpp"
#include "shared/track.hpp"
#include "shared/trimesh.hpp"
#include "shared/directories.hpp"


Uint32 starttime = 0;
Uint32 racetime = 0;
Uint32 simtime = 0; 

Uint32 start_time = 0;
void Run_Race(void)
{
	//start
	Log_Add (0, "Starting Race");

	ode_mutex = SDL_CreateMutex(); //create mutex for ode locking
	sdl_mutex = SDL_CreateMutex(); //only use sdl in 1 thread

	sync_mutex = SDL_CreateMutex();
	sync_cond = SDL_CreateCond();

	render_list_mutex = SDL_CreateMutex(); //prevent (unlikely) update/render collision

	runlevel  = running;

	starttime = SDL_GetTicks(); //how long it took for race to start

	//launch threads
	SDL_Thread *simulation = SDL_CreateThread (Simulation_Loop, NULL);
	Interface_Loop(); //we already got opengl context in main thread

	//wait for threads
	SDL_WaitThread (simulation, NULL);

	//cleanup
	SDL_DestroyMutex(ode_mutex);
	SDL_DestroyMutex(sdl_mutex);
	SDL_DestroyMutex(sync_mutex);
	SDL_DestroyMutex(render_list_mutex);
	SDL_DestroyCond(sync_cond);

	//done!
	Log_Add(0, "Race Done!");

	racetime = SDL_GetTicks() - starttime;
	simtime = simulation_time - starttime;
}

//
//tmp:
//
#include "loaders/text_file.hpp"

//instead of menus...
//try to load "tmp menu selections" for menu simulation
//what we do is try to open this file, and then try to find menu selections in it
//note: if selections are not found, will still fall back on safe defaults
bool tmp_menus()
{
	std::string sprofile, sworld, strack, steam, scar, sdiameter, styre, srim; //easy text manipulation...
	Directories dirs; //for finding
	Text_File file; //for parsing
	dirs.Find("tmp_menu_selections", CONFIG, READ);
	file.Open(dirs.Path()); //just assume it opens (no harm if not)...

	//MENU: welcome to rc, please select profile or create a new profile
	sprofile = "profiles/";
	if (file.Read_Line() && file.word_count == 2 && !strcmp(file.words[0], "profile"))
		sprofile += file.words[1];
	else
		sprofile += "default";

	Profile *prof = Profile_Load (sprofile.c_str());
	if (!prof)
		return false; //GOTO: profile menu


	//initiate simulation
	if (!Simulation_Init())
	{
		//menu: warn and quit!
		Interface_Quit();
		return false;
	}


	//MENU: select race type...

	//MENU: select world
	sworld = "worlds/";
	if (file.Read_Line() && file.word_count == 2 && !strcmp(file.words[0], "world"))
		sworld+=file.words[1];
	else
		sworld+="Sandbox";

	//MENU: select track
	strack = sworld;
	strack += "/tracks/";
	if (file.Read_Line() && file.word_count == 2 && !strcmp(file.words[0], "track"))
		strack+=file.words[1];
	else
		strack+="Box";

	//TODO: probably Racetime_Data::Destroy_All() here
	if (!load_track(strack.c_str()))
		return false; //GOTO: track selection menu

	//TMP: load some objects for online spawning
	if (	!(box = Object_Template::Load("objects/misc/box"))		||
		!(sphere = Object_Template::Load("objects/misc/beachball"))||
		!(funbox = Object_Template::Load("objects/misc/funbox"))	||
		!(molecule = Object_Template::Load("objects/misc/NH4"))	)
		return false;
	//

	//MENU: players, please select team/car

	Car_Template *car_template = NULL;
	Car *car = NULL;
	//models for rim and tyre
	Trimesh_3D *tyre = NULL, *rim = NULL;

	while (1)
	{
		//no more data in file...
		if (!file.Read_Line())
		{
			//lets make sure we got at least one car here...
			if (!car)
			{
				//no, load some defaults...
				if (!car_template)
					car_template = Car_Template::Load("teams/Nemesis/cars/Venom");

				//if above failed
				if (!car_template)
					return false;

				//try to load tyre and rim (if possible)
				if (!tyre)
					tyre = Trimesh_3D::Quick_Load_Conf("worlds/Sandbox/tyres/diameter/2/Slick", "tyre.conf");
				if (!rim)
					rim = Trimesh_3D::Quick_Load_Conf("teams/Nemesis/rims/diameter/2/Split", "rim.conf");
				//good, spawn
				car = car_template->Spawn(
					track.start[0], //x
					track.start[1], //y
					track.start[2], //z
					tyre, rim);
			}

			//then break this loop...
			break;
		}

		//team selected in menu
		if (file.word_count == 2 && !strcmp(file.words[0], "team"))
		{
			steam = "teams/";
			steam += file.words[1];
		}
		//car selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "car"))
		{
			scar = steam;
			scar += "/cars/";
			scar += file.words[1];

			if (! (car_template = Car_Template::Load(scar.c_str())) )
				return false;

		}
		//tyre diameter selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "diameter"))
		{
			sdiameter = file.words[1];
		}
		//tyre selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "tyre"))
		{
			//try loading from world tyres
			styre = sworld;
			styre += "/tyres/diameter/";
			styre += sdiameter;
			styre += "/";
			styre += file.words[1];

			//if failure...
			if (! (tyre = Trimesh_3D::Quick_Load_Conf(styre.c_str(), "tyre.conf")) )
			{
				//try seing if its track specific tyre
				styre = strack;
				styre += "/tyres/diameter/";
				styre += sdiameter;
				styre += "/";
				styre += file.words[1];

				tyre = Trimesh_3D::Quick_Load_Conf(styre.c_str(), "tyre.conf");
			}
		}
		//rim selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "rim"))
		{
			//try loading from team rims
			srim = steam;
			srim += "/rims/diameter/";
			srim += sdiameter;
			srim += "/";
			srim += file.words[1];

			//if failure...
			if (! (rim = Trimesh_3D::Quick_Load_Conf(srim.c_str(), "rim.conf")) )
			{
				//try seing if car specific

				srim = scar;
				srim += "/rims/diameter/";
				srim += sdiameter;
				srim += "/";
				srim += file.words[1];

				//don't care if fails...
				rim = Trimesh_3D::Quick_Load_Conf(srim.c_str(), "rim.conf");
			}
		}
		//manual position required for spawning
		else if (file.word_count == 3 && !strcmp(file.words[0], "position"))
		{
			if (!car_template)
			{
				Log_Add(-1, "Incorrect menu list: Specify car before position!");
				return false;
			}

			car = car_template->Spawn(
					(track.start[0]+atof(file.words[1])), //x
					(track.start[1]+atof(file.words[2])), //y
					(track.start[2]), //z
					tyre, rim);
		}
	}

	//this single player/profile controls all cars for now... and ladt one by default
	prof->car = car;
	camera.Set_Car(car);

	//MENU: race configured, start? yes!
	Run_Race();

	//race done, remove all objects...
	Object::Destroy_All();

	//MENU: race done, replay, play again, quit?
	// - assuming quit -
	
	//remove loaded data (not all data, only "racetime" - for this race)
	Racetime_Data::Destroy_All();

	//MENU: back to main menu here
	// - assuming player wants to log out -
	Simulation_Quit();
	Profile_Remove_All(); //should only be one active profile right now, but any case, remove all

	//MENU: select profile
	// - assumes player wants to quit -
	Interface_Quit();

	return true;
}

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
		Log_Add(-1, "Couldn't initiate SDL: %s", SDL_GetError());
		return -1;
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
					Log_Add(-1, "Window width requires a positive integer");
					exit(-1);
				}
				break;

			case 'y':
				yres=atoi(optarg);
				if (yres <= 0)
				{
					Log_Add(-1, "Window height requires a positive integer");
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
				Log_puts(0, "\
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
	Log_puts(0, " ReCaged is copyright (C) 2009, 2010, 2011, 2012, 2013, 2014 Mats Wahlberg\n");

Log_puts(1, "\
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
  * \"Free Software Foundation\"		-	\"Free Software, Free Society\"\n\
  * \"The GNU Project\"			-	For Developing a Free OS\n\
  * \"Simple DirectMedia Layer\"		-	OS/hardware abstraction library\n\
  * \"Open Dynamics Engine\"		-	Rigid body dynamics and collision detection library\n\
  * \"OpenGL Extension Wrangler\"		-	OpenGL version/extension loader\n\n\n\
 Default key bindings can be found (and changed) in \"data/profiles/default/keys.lst\"\n\
 More keys exists for debug/testing/demo, see README if you are interested.\n\n\
 - See README for more info -\n\n");


	//ok, start loading
	Log_Add(1, "Loading...");
	runlevel = loading;

	//initiate interface
	if (!Interface_Init(window, fullscreen, xres, yres))
		return -1;

	//
	//TODO: there should be menus here, but menu/osd system is not implemented yet... also:
	//on failure, rc should not just terminate but instead abort the race and warn the user
	if (!tmp_menus())
	{
		Log_Add(-1, "One or more errors, can not start!");
		return -1; //just quit if failure
	}
	//

	//might be interesting
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


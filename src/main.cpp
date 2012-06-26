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

//Required stuff:
#include <SDL.h>

//local stuff:
#include "shared/info.hpp"
#include "shared/internal.hpp"
#include "shared/threads.hpp"
#include "shared/log.hpp"
#include "shared/profile.hpp"
#include "shared/track.hpp"
#include "shared/trimesh.hpp"


//default options (paths)
const char profiledefault[] = "profiles";
char *datadefault; //need to check path to rc before deciding this

//main function, will change a lot in future versions...
int main (int argc, char *argv[])
{
	//issue
	printf("\n	-=[ Welcome to ReCaged version %s ]=-\n\n%s\n", VERSION, ISSUE);
	//end

	//attempt to generate default data path
	//check if program was called with another pwd (got '/' in "name")
	if (char *s = strrchr(argv[0], '/'))
	{
		//"<path to self - minus self>/data"
		int length=strlen(argv[0])-strlen(s)+1;

		datadefault=new char[length+5];

		//copy the path to self (first part of argv0)
		strncpy(datadefault, argv[0], length);
		//add null termination (for strcat)
		datadefault[length]='\0';
		//append data instead of self
		strcat(datadefault, "data");
	}
	else
	{
		//just change into "data"
		datadefault=new char[5];
		strcpy(datadefault, "data");
	}

	//set default values:
	const char *datadir = datadefault;
	const char *profiledir = profiledefault;

	//use getopt to parse options to to allow overide defaults:
	char c;
	//TODO!
	while ( (c = getopt(argc, argv, "d:p:")) != -1 )
	{
		printf("!!!! not working right now !!!!\n");
		switch(c)
		{
			case 'd': //data directory
				printf("Alternative path to data directory specified (\"%s\")\n", optarg);
				datadir = optarg;
				break;

			case 'p': //profile directory
				printf("Alternative path to profile directory specified (\"%s\")\n", optarg);
				profiledir = optarg;
				break;

			default:
				puts(HELP); //print help output
				exit(-1); //stop execution
		}
	}

	//ok, try to get into the data directory
	if (chdir (datadir))
	{
		printf("Failed to cd into data directory...\n");

		//lets see if this was not the default (and the default works):
		if ( (datadir != datadefault) && !chdir(datadefault) )
			printf("Using default directory (\"%s\") instead\n", datadefault);
		else
			printf("Will try to load from current directory instead...\n");
	}

	//not needed anymore (used or not, will not be needed any more)
	delete[] datadefault;

	//ok, start loading
	printlog(0, "Loading...\n");

	log_mutex = SDL_CreateMutex(); //create mutex for (print)log

	ode_mutex = SDL_CreateMutex(); //create mutex for ode locking
	sdl_mutex = SDL_CreateMutex(); //only use sdl in 1 thread

	sync_mutex = SDL_CreateMutex();
	sync_cond = SDL_CreateCond();

	render_list_mutex = SDL_CreateMutex(); //prevent (unlikely) update/render collision







	//internal conf (tmp)
	load_conf ("internal.conf", (char *)&internal, internal_index);

	//initiate interface
	if (!Interface_Init())
		return false;

	//initiate simulation
	if (!Simulation_Init())
		return false;




	Interface_Loop(); //we already got opengl context in main thread


	//wait for threads
	if (simulation_thread)
	{
		runlevel = done;
		SDL_WaitThread (simulation_thread, NULL);
	}

	//race done, remove all objects...
	Object::Destroy_All();

	//remove loaded data (not all data, only "racetime" - for this race)
	Racetime_Data::Destroy_All();
	Profile_Remove_All(); //should only be one active profile right now, but any case, remove all












	//clean up threads
	Simulation_Quit();
	Interface_Quit();

	//remove mutexes
	SDL_DestroyMutex(ode_mutex);
	SDL_DestroyMutex(sdl_mutex);
	SDL_DestroyMutex(sync_mutex);
	SDL_DestroyMutex(render_list_mutex);
	SDL_DestroyCond(sync_cond);

	//done!
	printlog(0, "Race Done!");


	racetime = SDL_GetTicks() - starttime;
	simtime = simtime - starttime;


	//might be interesting
	//TODO: most will be removed!
	printlog(1, "\n\n   <[ Info ]>");
	printlog(1, "Race time:			%ums", racetime);

	printlog(1, "Simulated time:		%ums (%u%% of real time)",
						simtime, (100*simtime)/racetime);

	printlog(1, "Average simulations/second:	%u steps/second (%u in total)",
						(1000*simulation_count)/racetime, simulation_count);

	printlog(1, "Simulation lag:		%u%% of steps (%u in total)",
						(100*simulation_lag)/simulation_count, simulation_lag);

	printlog(1, "Average frames/second:	%u FPS (%u%% of simulation steps)",
						(1000*interface_count)/racetime, (100*interface_count)/simulation_count);

	printf("\nBye!\n\n");

	return 0;
}


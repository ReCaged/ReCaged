/*
 * ReCaged - a Free Software, Futuristic, Racing Game
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

#include <stdarg.h>
#include <SDL/SDL_mutex.h>
#include "internal.hpp"
#include "log.hpp"

//default
int stdout_verbosity = 1;
FILE *logfile = NULL;
char *logbuffer = NULL;
SDL_mutex *logmutex = NULL;

//set up logging
void Log_Init()
{
	stdout_verbosity = 1; //make sure
	logbuffer = new char[LOG_BUFFER_SIZE];
	logmutex = SDL_CreateMutex();
}

bool Log_File(const char *file)
{
	SDL_mutexP(logmutex); //just make sure no logging while closing
	if (logfile)
	{
		if (file) Log_Add(2, "Closing current log file");
		else Log_Add(2, "File logging disabled");

		fclose(logfile);
		logfile=NULL;
	}
	SDL_mutexV(logmutex); //should be fine from now

	if (file)
	{
		if ((logfile=fopen(file, "w")))
		{
			Log_Add(2, "Using \"%s\" for logging", file);
			return true;
		}
		else
		{
			Log_Add(0, "Warning: unable to open \"%s\" for logging", file);
			return false;
		}
	}

	return true;
}

void Log_Change_Verbosity(int v)
{
	stdout_verbosity+=v;

	if (stdout_verbosity<-1)
		stdout_verbosity=-1;
	else if (stdout_verbosity>2)
		stdout_verbosity=2;
}

void Log_Quit()
{
	Log_File(NULL); //make sure is closed
	delete[] logbuffer;
	SDL_DestroyMutex(logmutex);
}

//verbosity indicators (MUST have strlen 3!)
const char *indicator[] = {"=> ", " > ", " * "};

//print log message - if it's below or equal to the current verbosity level
void Log_Add (int level, const char *text, ...)
{
	//TODO: should probably check for error from fputs, putc, puts
	if (logfile || level <= stdout_verbosity)
	{
		SDL_mutexP(logmutex); //make sure no conflicts

		//begin with verbosity indicator (if in normal range)
		if (level >=0 && level <=2)
			strcpy(logbuffer, indicator[level]);
		else
			fputs(" ? ", stdout);

		//print message
		va_list list;
		va_start (list, text);
		int i=vsnprintf (logbuffer+3, LOG_BUFFER_SIZE-3, text, list);
		va_end (list);

		//safety precaution...
		if (i==-1)
		{
			puts("ERROR DURING LOG OUTPUT GENERATION!");
			SDL_mutexV(logmutex);
			return;
		}

		//write to targets (+add newling)
		if (logfile)
		{
			if (level==0) fputc('\n', logfile); //clear separation of text
			fputs(logbuffer, logfile);
			putc('\n', logfile); //should be faster than fputc...
		}
		if (level <=stdout_verbosity)
		{
			if (level==0) putchar('\n');
			puts(logbuffer);
		}

		SDL_mutexV(logmutex); //ok
	}
}

//just wrappers:
void Log_printf (int level, const char *text, ...)
{
	if (logfile || level <= stdout_verbosity)
	{
		SDL_mutexP(logmutex);
		va_list list;
		va_start (list, text);
		int i=vsnprintf (logbuffer, LOG_BUFFER_SIZE, text, list);
		va_end (list);

		if (i==-1)
		{
			puts("ERROR DURING LOG OUTPUT GENERATION!");
			SDL_mutexV(logmutex);
			return;
		}

		if (logfile) fputs(logbuffer, logfile);
		if (level <=stdout_verbosity) fputs(logbuffer, stdout);
		SDL_mutexV(logmutex);
	}
}

void Log_puts (int level, const char *text)
{
	SDL_mutexP(logmutex);
	if (logfile) fputs(text, logfile);
	if (level <= stdout_verbosity) fputs(text, stdout);
	SDL_mutexV(logmutex);
}


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

#include <stdarg.h>
#include <SDL/SDL_mutex.h>
#include "internal.hpp"
#include "log.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

//default
int stdout_verbosity = 1;
bool logram = true;
FILE *logfile = NULL;
char *logbuffer=NULL;
size_t bufferpos=0;
size_t buffersize=0;
SDL_mutex *logmutex = NULL;

//assume always called from a logging function (so mutex locked)
//size:minimum free space required (without \0), 0=increase by LOG_BUFFER_SIZE
void IncreaseBuffer(size_t size)
{
	//increase size
	if (size==0) //once
		buffersize += LOG_BUFFER_SIZE;
	else if (buffersize-bufferpos >= size) //enough already
		return;
	else //increase until enough
		while (buffersize-bufferpos < size)
			buffersize += LOG_BUFFER_SIZE;

	char *old=logbuffer;
	logbuffer = new char[buffersize];

	//no earlier buffer
	if (!old)
		return;
	//else got existing buffer to copy
	memcpy(logbuffer, old, sizeof(char)*(bufferpos+1));

	//delete old buffer
	delete[] old;
}
//

//set up logging
void Log_Init()
{
	stdout_verbosity = 1; //make sure
	logfile = NULL;
	logbuffer = NULL;
	bufferpos = 0;
	buffersize = 0;

	logmutex = SDL_CreateMutex();

	Log_RAM(true);
	Log_Add(2, "Enabled logging system");
}

void Log_Quit()
{
	Log_Add(2, "Disabling logging system");

	Log_File(NULL); //make sure is closed

	if (logbuffer)
		delete[] logbuffer;

	SDL_DestroyMutex(logmutex);
}

void Log_RAM(bool ram)
{
	SDL_mutexP(logmutex); //changing variables might mess with running log append
	logram=ram;

	if (ram)
		Log_Add(2, "Enabled logging to RAM");
	else
		Log_Add(2, "Disabling logging to RAM");

	SDL_mutexV(logmutex); //should be fine from now on

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
			Log_Add(2, "Enabled logging to file \"%s\"", file);
		else
			Log_Add(-1, "Unable to open \"%s\" for logging", file);

		//if we got (possibly) log messages already stored:
		if (logram)
		{
			Log_Add(2, "Writing existing log (including this line!) to file...");
			fputs(logbuffer, logfile);
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

//verbosity indicators (MUST have strlen 3!)
const char *indicator[] = {"\nERROR: ", "\n=> ", " > ", " * "};

//annoy the user on windoze, empty macro otherwise
#ifdef _WIN32
#define W32ERROR() MessageBoxA(NULL, "ERROR DURING LOG OUTPUT GENERATION!", "Error!", MB_ICONERROR | MB_OK);
#else
#define W32ERROR()
#endif

//simple calculation, reuse (search from current pos, bogus optimized)
#define UPDATEPOS() bufferpos=strlen(logbuffer+bufferpos)+bufferpos

//generate and append message from varargs
#define ARGPARSE() \
	va_list list; \
	size_t i; \
	while (1) \
	{ \
		va_start (list, text); \
		i=vsnprintf (logbuffer+bufferpos, buffersize-bufferpos, text, list); \
		va_end (list); \
		\
		/*if error*/ \
		if (i < 0) \
		{ \
			puts("ERROR DURING LOG OUTPUT GENERATION!"); \
			\
			/*and annoy the user on windoze*/ \
			W32ERROR() \
			SDL_mutexV(logmutex); \
			return; \
		} \
		\
		/*if ok*/ \
		if (i<buffersize-bufferpos) \
		{ \
			UPDATEPOS(); \
			break; \
		} \
		\
		/*lacking memory, try again with more*/ \
		IncreaseBuffer(0); \
	}


//print log message - if it's below or equal to the current verbosity level
void Log_Add (int level, const char *text, ...)
{
	//TODO: should probably check for error from fputs, putc, puts
	if (logram || logfile || level <= stdout_verbosity)
	{
		SDL_mutexP(logmutex); //make sure no conflicts

		 //reset buffer position if not logging to ram
		if (!logram)
			bufferpos=0;

		//index to current line added to log
		int entry=bufferpos;

		//at least enough size to put indicator
		IncreaseBuffer(9); //9=enough for 8 chars and 1 \0

		//begin with verbosity indicator (+update position)
		strcpy(logbuffer+bufferpos, indicator[level+1]);
		UPDATEPOS();

		//parse variable arguments
		ARGPARSE();

		//similar to earlier, add newline
		IncreaseBuffer(2); //enough for \n and \0
		strcpy(logbuffer+bufferpos, "\n");
		UPDATEPOS();

		//write to targets (+add newling)
		if (logfile)
			fputs(logbuffer+entry, logfile);

		//special case
		if (level == -1)
		{
			//write to stderr instead of stdout (+some newlines to
			//make it visible)
			fputs(logbuffer+entry, stderr);
#ifdef _WIN32
			//and annoy the user on windoze
			//don't want the newlines in beginning and end...
			logbuffer[bufferpos]='\0'; //replace \n with \0
			MessageBoxA(NULL, logbuffer+entry+1, "Error!", MB_ICONERROR | MB_OK);
			logbuffer[bufferpos]='\n'; //and put back
#endif
		}
		else if (level <=stdout_verbosity) //normal case, if high enough verbosity
			fputs(logbuffer+entry, stdout);

		SDL_mutexV(logmutex); //ok
	}
}

//just wrappers:
void Log_printf (int level, const char *text, ...)
{
	if (logram || logfile || level <= stdout_verbosity)
	{
		SDL_mutexP(logmutex);

		 //reset buffer position if not logging to ram
		if (!logram)
			bufferpos=0;

		//pointer to current line added to log
		int entry=bufferpos;

		ARGPARSE();

		if (logfile) fputs(logbuffer+entry, logfile);
		if (level == -1) fputs(logbuffer+entry, stderr);
		else if (level <=stdout_verbosity) fputs(logbuffer+entry, stdout);
		SDL_mutexV(logmutex);
	}
}

void Log_puts (int level, const char *text)
{
	SDL_mutexP(logmutex);
	if (logfile) fputs(text, logfile);
	if (logram) //just append to buffer
	{
		int l=strlen(text)+1; //size (with NULL)
		IncreaseBuffer(l);
		memcpy(logbuffer+bufferpos, text, sizeof(char)*l);
		UPDATEPOS();
	}
	if (level == -1) fputs(logbuffer, stderr);
	else if (level <= stdout_verbosity) fputs(text, stdout);
	SDL_mutexV(logmutex);
}


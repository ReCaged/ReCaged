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

#include <stdarg.h>
#include "internal.hpp"
#include "log.hpp"

//default
int stdout_verbosity = 1;
FILE *logfile = NULL;
char *buffer = NULL;

//set up logging
void Log_Init()
{
	//probably pointless, but anyway
	stdout_verbosity = 1;
	buffer = new char[LOG_BUFFER_SIZE];

	//TODO: create mutex
}

bool Log_File(const char *file)
{
	//TODO: use mutex!
	if (logfile)
	{
		Log_Add(2, "Closing current log file");
		fclose(logfile);
		logfile=NULL;
	}

	if (file)
	{
		if ((logfile=fopen(file, "w")))
		{
			Log_Add(2, "Using file \"%s\" for logging", file);
			return true;
		}
		else
		{
			Log_Add(0, "Warning: unable to open \"%s\" for logging", file);
			return false;
		}
	}

	//if got here
	Log_Add(2, "File logging disabled");
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
	Log_Add(2, "Logging disabled");
	delete[] buffer;
	//TODO: destroy mutex
}

//verbosity indicators (MUST have strlen 3!)
const char *indicator[] = {"=> ", " > ", " * "};

//print log message - if it's below or equal to the current verbosity level
void Log_Add (int level, const char *text, ...)
{
	//TODO: use mutex!
	//TODO: should probably check for error from fputs, putc, puts
	if (logfile || level <= stdout_verbosity)
	{
		if (level==0)
			putchar('\n');

		//begin with verbosity indicator (if in normal range)
		if (level >=0 && level <=2)
			strcpy(buffer, indicator[level]);
		else
			fputs(" ? ", stdout);

		//print message
		va_list list;
		va_start (list, text);
		int i=vsnprintf (buffer+3, LOG_BUFFER_SIZE-3, text, list);
		va_end (list);

		//safety precaution...
		if (i==-1)
		{
			puts("ERROR DURING LOG OUTPUT GENERATION!");
			return;
		}

		//write to targets (+add newling)
		if (logfile)
		{
			fputs(buffer, logfile);
			putc('\n', logfile); //should be faster than fputc...
		}
		if (level <=stdout_verbosity) puts(buffer);
	}
}

//just wrappers:
void Log_printf (int level, const char *text, ...)
{
	//TODO: use mutex!
	if (logfile || level <= stdout_verbosity)
	{
		va_list list;
		va_start (list, text);
		int i=vsnprintf (buffer, LOG_BUFFER_SIZE, text, list);
		va_end (list);

		if (i==-1)
		{
			puts("ERROR DURING LOG OUTPUT GENERATION!");
			return;
		}

		if (logfile) fputs(buffer, logfile);
		if (level <=stdout_verbosity) fputs(buffer, stdout);
	}
}

void Log_puts (int level, const char *text)
{
	//TODO: use mutex!
	if (logfile) fputs(text, logfile);
	if (level <= stdout_verbosity) fputs(text, stdout);
}


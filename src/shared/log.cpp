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

//set up logging
void Log_Init()
{
	//probably pointless, but anyway
	stdout_verbosity = 1;

	//TODO: create mutex
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
	//TODO: destroy mutex
}

//verbosity indicators
const char *indicator[] = {"=> ", " > ", " * "};

//print log message - if it's below or equal to the current verbosity level
void Log_Add (int level, const char *text, ...)
{
	if (level <= stdout_verbosity)
	{
		if (level==0)
			putchar('\n');

		//print verbosity indicator (if in normal range)
		if (level >=0 && level <=2)
			fputs(indicator[level], stdout); //puts adds newline, fputs instead
		else
			fputs(" ? ", stdout);

		//print message
		va_list list;
		va_start (list, text);
		vprintf (text, list);
		va_end (list);

		//put newline
		putchar('\n');
	}
}

//just wrappers:
void Log_printf (int level, const char *text, ...)
{
	if (level <= stdout_verbosity)
	{
		va_list list;
		va_start (list, text);
		vprintf (text, list);
		va_end (list);
	}
}

void Log_puts (int level, const char *text)
{
	if (level <= stdout_verbosity)
		fputs(text, stdout);
}


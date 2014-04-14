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

//print text to logs
#ifndef _RC_LOG_H
#define _RC_LOG_H

extern "C" {
#include <lauxlib.h>
}

//max line size (should be enough...)
#define LOG_BUFFER_SIZE 2048

//TODO: should store copy of log in memory as well

//configuration
void Log_Init();
//enable/disable log file
bool Log_File(const char *file);
//set verbosity level
void Log_Change_Verbosity(int);
//Log_File();
void Log_Quit();

//normal append to log
void Log_Add (int, const char*, ...);

//wrappers for popular text output functions
void Log_printf (int, const char*, ...);
void Log_puts (int, const char*);

//lua functions
extern const luaL_Reg lua_log[];

#endif

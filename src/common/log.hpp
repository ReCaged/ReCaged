/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014 Mats Wahlberg
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

//print text to logs

#ifndef _RCX_PRINTLOG_H
#define _RCX_PRINTLOG_H

//starting size for lines, also for logging to RAM
//(increased dynamically if needed)
#define LOG_BUFFER_SIZE 1024

//configuration
void Log_Init();
//enable/disable storing of log in ram
void Log_RAM(bool);
//enable/disable log file
bool Log_File(const char *file);
//set verbosity level (relative to current)
void Log_Change_Verbosity(int);
//Log_File();
void Log_Quit();

//normal append to log
void Log_Add (int, const char*, ...);

//wrappers for popular text output functions
void Log_printf (int, const char*, ...);
void Log_puts (int, const char*);

//
//lua library
//
#include "lua.hpp"
int Lua_Log_Init(lua_State *L);

#endif

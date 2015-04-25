/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2015 Mats Wahlberg
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

#ifndef _RCX_LUA_H
#define _RCX_LUA_H

extern "C" {
#if OVERRIDE_LUA_HEADERS
#include OVERRIDE_LUA_H
#include OVERRIDE_LUALIB_H
#include OVERRIDE_LAUXLIB_H
#else
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif
}

//helper functions (compatibility with different lua versions)
//TODO: should probably move whole Thread to class, this as (private) methods
bool RCLua_Init(class Thread *); //initiates and loads std libraries
void RCLua_Add(class Thread *, const luaL_Reg *newlibs); //add custom libs

//"common" custom lua libraries:
int (Lua_Log_Init) (lua_State *L);
const luaL_Reg common_lua_libs[] = {
	{"log",	Lua_Log_Init},
	{NULL,	NULL}};

#endif

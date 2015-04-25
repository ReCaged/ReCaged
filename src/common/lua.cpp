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

#include "lua.hpp"
#include "threads.hpp"
#include "directories.hpp"

bool RCLua_Init(Thread *thread)
{
	//set state:
	thread->lua_state = luaL_newstate();

	//check if failed
	if (thread->lua_state == NULL)
	{
		Log_Add(-1, "Could not create lua state (not enough memory)");
		return false;
	}

	//open standard libs:
	luaL_openlibs(thread->lua_state);

	//overwrite package paths according to what "directories" has found:
	lua_State *L=thread->lua_state; //just for shorter code

	//set package.path=lpath
	lua_getglobal(L, "package"); //find package
	const char *lpath=Directories::BuildLuaPath(); //get string
	lua_pushstring(L, lpath); //put/copy to top of stack
	delete[] lpath; //this string can be deleted immediately 
	lua_setfield(L, -2, "path"); //overwrite index by string

	//set package.cpath="" (empty, no c libs!)
	lua_pushstring(L, "");
	lua_setfield(L, -2, "cpath");

	//open common custom libraries:
	RCLua_Add(thread, common_lua_libs);

	return true;
}

//open custom lua libs:
void RCLua_Add(Thread *thread, const luaL_Reg *newlibs)
{
	const luaL_Reg *lib;
	for (lib = newlibs; lib->func; ++lib)
	{
		luaL_requiref(thread->lua_state, lib->name, lib->func, 1);
		lua_pop(thread->lua_state, 1);
	}
}


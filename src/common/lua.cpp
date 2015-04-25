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
#include <string>

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

	//open common custom libraries:
	RCLua_Add(thread, common_lua_libs);


	//
	//overwrite package paths according to what "directories" has found:
	//
	std::string path;

	//assemble 4 (potential) permutations to include:
	// * module is in user's data dir or installed data dir
	// * module is a plain file ("name.lua") or a dir ("name/init.lua")
	const char *base[2]={Directories::user_data, Directories::inst_data};
	const char *pattern[2]={"/?/init.lua", "/?.lua"};
	//loop with priority: user > installed, and: directory > file
	for (int b=0; b<2; ++b)
	for (int p=0; p<2; ++p)
		if (base[b]) //if path exists (found by directory)
		{
			//only add separator between paths
			if (!path.empty()) //already got
				path+=";";

			path+=base[b];
			path+=pattern[p];
		}

	//set package.path=path
	lua_getglobal(thread->lua_state, "package"); //find package
	lua_pushstring(thread->lua_state, path.c_str()); //put/copy to top of stack
	lua_setfield(thread->lua_state, -2, "path"); //overwrite index by string

	//set package.cpath="" (empty, to prevent c libs!)
	lua_pushstring(thread->lua_state, "");
	lua_setfield(thread->lua_state, -2, "cpath");

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


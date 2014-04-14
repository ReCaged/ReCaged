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

#include "../shared/object.hpp"

#include <ode/ode.h>

extern "C" {
#include HEADER_LUA_H
#include HEADER_LUALIB_H
#include HEADER_LAUXLIB_H
}
#include "../shared/threads.hpp"

#include "../shared/racetime_data.hpp"
#include "../shared/directories.hpp"
#include "../shared/trimesh.hpp"
#include "../shared/log.hpp"
#include "../shared/track.hpp"
#include "../shared/joint.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"

#include "../simulation/event_buffers.hpp"

//load data for spawning object (object data), hard-coded debug version
Object_Template *Object_Template::Load(const char *path)
{
	Log_Add(1, "Loading object: %s", path);

	//see if already loaded
	if (Object_Template *tmp=Racetime_Data::Find<Object_Template>(path))
	{
		return tmp;
	}

	//full path to script
	char script[strlen(path)+strlen("/object.lua")+1];
	strcpy(script, path);
	strcat(script, "/object.lua");

	//check if found
	Directories dirs;
	if (!dirs.Find(script, DATA, READ))
	{
		Log_Add(0, "ERROR: could not find script \"%s\"!", script);
		return NULL;
	}

	//load file as chunk
	if (luaL_loadfile(lua_sim, dirs.Path()))
	{
		Log_Add(0, "ERROR: could not load script \"%s\": \"%s\"!",
				dirs.Path(), lua_tostring(lua_sim, -1));
		lua_pop(lua_sim, -1);
		return NULL;
	}
	
	//execute chunk and look for one return
	if (lua_pcall(lua_sim, 0, 1, 0))
	{
		Log_Add(0, "ERROR: \"%s\" while running \"%s\"!",
				lua_tostring(lua_sim, -1), dirs.Path());
		lua_pop(lua_sim, -1);
		return NULL;
	}

	//if not returned ok
	if (!lua_isfunction(lua_sim, -1))
	{
		Log_Add(0, "ERROR: no spawning function returned by \"%s\"!", dirs.Path());
		return NULL;
	}

	//great!
	Object_Template *obj = new Object_Template(path);
	obj->spawn_script = luaL_ref(lua_sim, LUA_REGISTRYINDEX);
	return obj;
}

//spawn
//TODO: rotation. move both pos+rot to arrays
Object *Object_Template::Spawn (dReal x, dReal y, dReal z)
{
	Log_Add(2, "Spawning object at: %f %f %f", x,y,z);
	Object *obj = new Object();

	lua_rawgeti(lua_sim, LUA_REGISTRYINDEX, spawn_script);

	if (obj->Run(0, 0))
	{
		Log_Add(0, "ERROR: \"%s\" while spawning object!",
				lua_tostring(lua_sim, -1));
		lua_pop(lua_sim, -1);
		delete obj;
		return NULL;
	}

	//hm... didn't do anything?
	if (obj->activity == 0)
		Event_Buffer_Add_Inactive(obj);

	return obj;
}


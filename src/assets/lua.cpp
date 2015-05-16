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

#include "common/lua.hpp"
#include "model.hpp"

static int model_load(lua_State *L)
{
	Model **p=(Model**)lua_newuserdata(L, sizeof(Model*));
	*p=new Model();
	(**p).Load(lua_tostring(L, 1));
	luaL_getmetatable(L, "rcmodel");
	lua_setmetatable(L, -2);

	return 1;
}

static int model_draw(lua_State *L)
{
	Model **model=(Model**)luaL_checkudata(L, 1, "rcmodel");

	Model_Draw **p=(Model_Draw**)lua_newuserdata(L, sizeof(Model_Draw*));
	*p=(**model).Create_Draw();
	luaL_getmetatable(L, "rcmodeldraw");
	lua_setmetatable(L, -2);

	return 1;
}

static int model_delete(lua_State *L)
{
	Model **model=(Model**)luaL_checkudata(L, 1, "rcmodel");
	delete *model;
	return 0;
}
//
//for registering to thread/lua state:
//
static const luaL_Reg model_lib[] = {
	{"load",	model_load},
	{NULL,		NULL}
};

static const luaL_Reg model_methods[] = {
	{"draw",	model_draw},
	{"delete",	model_delete},
	{NULL,		NULL}
};

int Lua_Model_Init(lua_State *L)
{
	luaL_newmetatable(L, "rcmodel");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, model_methods, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, "rcmodeldraw");
	/*
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, modeldraw_methods, 0);
	lua_pop(L, 1);
	*/

	luaL_newlib(L, model_lib);
	return 1;
}

const luaL_Reg assets_lua_libs[] = {
	{"model",	Lua_Model_Init},
	{NULL,		NULL}};


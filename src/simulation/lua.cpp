
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
#include <ode/ode.h>

//TODO: support quaternions, check both types when setting rotation...
static int rotation_matrix(lua_State *L)
{
	lua_newuserdata(L, sizeof(dMatrix3));
	luaL_getmetatable(L, "rcmatrix");
	lua_setmetatable(L, -2);
	return 1;
}

static int matrix_fromaxisandangle(lua_State *L)
{
	dMatrix3 *m=(dMatrix3*)luaL_checkudata(L, 1, "rcmatrix");
	dRFromAxisAndAngle(*m, 
			lua_tonumber(L, 2),
			lua_tonumber(L, 3),
			lua_tonumber(L, 4),
			lua_tonumber(L, 5));
	return 0;
}

static const luaL_Reg rotation_lib[] = {
	//{"quaternion",	ode_quaternion},
	{"matrix",	rotation_matrix},
	{NULL,		NULL}
};

static const luaL_Reg matrix_methods[] = {
	{"fromaxisandangle",	matrix_fromaxisandangle},
	{NULL,			NULL}
};

int Lua_Rotation_Init(lua_State *L)
{
	luaL_newmetatable(L, "rcmatrix");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, matrix_methods, 0);
	lua_pop(L, 1);

	luaL_newlib(L, rotation_lib);
	return 1;
}


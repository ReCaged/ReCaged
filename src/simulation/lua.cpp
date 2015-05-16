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
#include "common/threads.hpp"
#include "body.hpp"
#include "geom.hpp"


//TODO:
// * most of this can be made really compact through macro(s)
// * use references and __gc to track and clean up everything
// * add safetychecks...
// * use upper-cases in methods, mass:setbox instead of mass:box, and similar?

static int body_create(lua_State *L)
{
	Object **obj = (Object**)luaL_checkudata(L, 1, "rcobject");

	dBodyID body = dBodyCreate (simulation_thread.world);

	Body **p=(Body**)lua_newuserdata(L, sizeof(Body*));
	*p=new Body(body, *obj);
	luaL_getmetatable(L, "rcbody");
	lua_setmetatable(L, -2);
	return 1;
}

static int body_mass(lua_State *L)
{
	Body **body=(Body**)luaL_checkudata(L, 1, "rcbody");
	dMass *m=(dMass*)luaL_checkudata(L, 2, "rcmass");

	dBodySetMass((**body).body_id, m);
	(**body).Update_Mass();

	return 0;
}

static int body_model(lua_State *L)
{
	Body **body=(Body**)luaL_checkudata(L, 1, "rcbody");
	Model_Draw **m=(Model_Draw**)luaL_checkudata(L, 2, "rcmodeldraw");

	(**body).model=*m;

	return 0;
}

static int body_position(lua_State *L)
{
	Body **body=(Body**)luaL_checkudata(L, 1, "rcbody");

	dBodySetPosition(	(**body).body_id, 
			lua_tonumber(L, 2),
			lua_tonumber(L, 3),
			lua_tonumber(L, 4));

	return 0;
}

static int body_rotation(lua_State *L)
{
	Body **body=(Body**)luaL_checkudata(L, 1, "rcbody");
	dMatrix3 *m=(dMatrix3*)luaL_checkudata(L, 2, "rcmatrix");

	dBodySetRotation((**body).body_id, *m);

	return 0;
}

static int body_delete(lua_State *L)
{
	Body **body=(Body**)luaL_checkudata(L, 1, "rcbody");
	delete *body;
	return 0;
}
static const luaL_Reg body_lib[] = {
	{"create",	body_create},
	{NULL,		NULL}
};

static const luaL_Reg body_methods[] = {
	{"mass",	body_mass},
	{"model",	body_model},
	{"position",	body_position},
	{"rotation",	body_rotation},
	{"delete",	body_delete},
	{NULL,		NULL}
};

//simple init: just expose log.add() method:
int Lua_Body_Init(lua_State *L)
{
	luaL_newmetatable(L, "rcbody");

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, body_methods, 0);
	lua_pop(L, 1);

	luaL_newlib(L, body_lib);
	return 1;
}

static int mass_create(lua_State *L)
{
	lua_newuserdata(L, sizeof(dMass));
	luaL_getmetatable(L, "rcmass");
	lua_setmetatable(L, -2);
	return 1;
}

static const luaL_Reg mass_lib[] = {
	{"create",	mass_create},
	{NULL,		NULL}
};

static int mass_boxtotal(lua_State *L)
{
	dMass *m=(dMass*)luaL_checkudata(L, 1, "rcmass");
	dMassSetBoxTotal(m, 
			lua_tonumber(L, 2),
			lua_tonumber(L, 3),
			lua_tonumber(L, 4),
			lua_tonumber(L, 5));
	return 0;
}

static const luaL_Reg mass_methods[] = {
	{"boxtotal",	mass_boxtotal},
	{NULL,		NULL}
};
int Lua_Mass_Init(lua_State *L)
{
	luaL_newmetatable(L, "rcmass");

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, mass_methods, 0);
	lua_pop(L, 1);

	luaL_newlib(L, mass_lib);
	return 1;
}
//TODO: destroy base metatable at close!
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


//
//lua methods:
//

static int geom_box(lua_State *L)
{
	/*
	//check lua args:
	//
	//number of arguments
	int n = lua_gettop(L);

kanske: n == 4 => object, n == 3 => orphan?

	//not enough/wrong arguments
	luaL_argcheck(L, n == 3 &&
			lua_isnumber(L, 1) &&
			lua_isnumber(L, 2) &&
			lua_isnumber(L, 3),
			1, "expected verbosity number");
			*/

	Object **obj = (Object**)luaL_checkudata(L, 1, "rcobject");

	dGeomID geom  = dCreateBox (0,
			lua_tonumber(L, 2),
			lua_tonumber(L, 3),
			lua_tonumber(L, 4));

	Geom **p=(Geom**)lua_newuserdata(L, sizeof(Geom*));
	*p=new Geom(geom, *obj);
	luaL_getmetatable(L, "rcgeom");
	lua_setmetatable(L, -2);
	//pushval //TODO: when completed move to lua
	//luaL_ref
	//~Geom
	//udata->p=NULL;
	//luaL_unref

	//data->Push_Ref(L); //TODO: always push ref and duplicate in constructor!
		//lua_newuserdata(L, sizeof(char));
		//luaL_setmetatable(L, "foobar");
	return 1;
}

/*
static int geom_id(lua_State *L)
{
	if (luaL_checkudata(L, 1, "foobar"))
		printf("yes\n");
	else
		printf("no\n");
	return 0;
}
*/

static int geom_position(lua_State *L)
{
	Geom **geom=(Geom**)luaL_checkudata(L, 1, "rcgeom");

	dGeomSetPosition(	(**geom).geom_id, 
			lua_tonumber(L, 2),
			lua_tonumber(L, 3),
			lua_tonumber(L, 4));

	return 0;
}

static int geom_mu(lua_State *L)
{
	Geom **geom=(Geom**)luaL_checkudata(L, 1, "rcgeom");

	(**geom).surface.mu=lua_tonumber(L, 2);

	return 0;
}

static int geom_rotation(lua_State *L)
{
	Geom **geom=(Geom**)luaL_checkudata(L, 1, "rcgeom");
	dMatrix3 *m=(dMatrix3*)luaL_checkudata(L, 2, "rcmatrix");

	dGeomSetRotation((**geom).geom_id, *m);

	return 0;
}

static int geom_body(lua_State *L)
{
	Geom **geom=(Geom**)luaL_checkudata(L, 1, "rcgeom");
	Body **body=(Body**)luaL_checkudata(L, 2, "rcbody");

	dGeomSetBody((**geom).geom_id, (**body).body_id);

	return 0;
}

static int geom_delete(lua_State *L)
{
	Geom **geom=(Geom**)luaL_checkudata(L, 1, "rcgeom");
	delete *geom;
	return 0;
}
//
//for registering to thread/lua state:
//
static const luaL_Reg geom_lib[] = {
	{"box",		geom_box},
	//{"id",	geom_id},
	{NULL,		NULL}
};

static const luaL_Reg geom_methods[] = {
	{"position",	geom_position},
	{"rotation",	geom_rotation},
	{"body",	geom_body},
	{"mu",		geom_mu},
	{"delete",	geom_delete},
	{NULL,		NULL}
};

//simple init: just expose log.add() method:
int Lua_Geom_Init(lua_State *L)
{
	luaL_newmetatable(L, "rcgeom");

	//use metatable for __index: compact and common solution:
	lua_pushvalue(L, -1); //copy
	lua_setfield(L, -2, "__index"); //set index to itself (via copy)

	//write functions to table
	luaL_setfuncs(L, geom_methods, 0);

	//remove metatable from stack
	lua_pop(L, 1);

	luaL_newlib(L, geom_lib);
	return 1;
}

//TODO: destroy base metatable at close!
//collect all libraries for simulation
const luaL_Reg simulation_lua_libs[] = {
	{"geom",	Lua_Geom_Init},
	{"body",	Lua_Body_Init},
	{"mass",	Lua_Mass_Init},
	{"rotation",	Lua_Rotation_Init},
	{NULL,		NULL}};


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

#include "object.hpp"
#include "log.hpp"
#include "track.hpp"
#include "../simulation/event_buffers.hpp"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include "../shared/threads.hpp"

#include <stdlib.h>

//allocate new script storage, and add it to list
Object_Template::Object_Template(const char *name): Racetime_Data(name)
{
	spawn_script=0;
}

Object_Template::~Object_Template()
{
	luaL_unref(tmp_lua_state, LUA_REGISTRYINDEX, spawn_script);
}

Object *Object::head = NULL;

//allocate a new object, add it to the list and returns its pointer
Object::Object ()
{
	printlog(1, "creating Object");

	prev=NULL;
	next=head;
	head=this;

	if (next)
		next->prev = this;
	else
		printlog(2, "(first registered object)");

	//default values
	components = NULL;
	activity = 0;
	selected_space = NULL;
}

//destroys an object
Object::~Object()
{
	//lets just hope the given pointer is ok...
	printlog(1, "freeing Object");

	//1: remove it from the list
	if (prev == NULL) //first link
		head = next;
	else
		prev->next = next;

	if (next) //not last link
		next->prev = prev;


	//remove components
	while (components)
		delete components; //just removes the one in top each time

	//make sure no events for this object is left
	Event_Buffer_Remove_All(this);
}

void Object::Increase_Activity()
{
	++activity;
}

void Object::Decrease_Activity()
{
	if ((--activity) == 0)
		Event_Buffer_Add_Inactive(this);
}

//destroys all objects
void Object::Destroy_All()
{
	while (head)
		delete (head);
}

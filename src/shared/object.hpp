/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
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

#ifndef _RCX_OBJECT_H
#define _RCX_OBJECT_H
#include <ode/common.h>

#include "racetime_data.hpp"
#include "trimesh.hpp"
#include "script.hpp"
#include "component.hpp"
#include "space.hpp"

//object: one "thing" on the track, from a complex building to a tree, spawning
//from "modules" using lua (in future versions). the most important role of
//"object" is to store the components, ode space and joint group for the
//spawned object

//template for spawning
class Module:public Racetime_Data
{
	public:
		static Module *Load(const char *path);
		void Spawn(dReal x, dReal y, dReal z);

	private:
		Module(const char*); //just set some default values
		//placeholder for script data, now just variables

		//script to be run when spawning object
		Script *spawn;

		//tmp vbo test graphics
		Trimesh_3D *model[10];
		//tmp trimesh test model
		Trimesh_Geom *geom[1];

		//temporary solution
		bool box;
		bool funbox;
		bool flipper;
		bool NH4;
		bool building;
		bool sphere;
		bool pillar;
		bool tetrahedron;
};

//can be added/removed at runtime ("racetime")
class Object
{
	public:
		virtual ~Object(); //(virtual makes sure also inherited classes calls this destructor)
		static void Destroy_All();

		//for increasing/decreasing activity counter
		void Increase_Activity();
		void Decrease_Activity();
	private:
		Object();
		//the following are either using or inherited from this class
		friend class Module; //needs access to constructor
		friend bool load_track (const char *);
		friend class Car;

		//things to keep track of when cleaning out object
		unsigned int activity; //counts geoms,bodies and future stuff (script timers, loops, etc)

		Component *components;
		dSpaceID selected_space;

		//to allow acces to the two above pointers
		friend class Component; //components
		friend class Geom; //selected space
		friend class Space; //selected space

		//placeholder for more data
			
		//used to find next/prev object in dynamically allocated chain
		//set next to null in last object in chain
		static Object *head;
		Object *prev, *next;
};

#endif

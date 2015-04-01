/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015, 2015 Mats Wahlberg
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

#ifndef _RCX_TRACK_H
#define _RCX_TRACK_H
#include "assets/conf.hpp"
#include "object.hpp"
#include <GL/glew.h>
#include <ode/ode.h>

//Allocated at start
//(in contrary to the other structs, this is actually not allocated on runtime!)
//TODO: all this will be moved to Thread and new rendering System (with lua)
extern struct Track_Struct {
	//placeholder for stuff like if it's raining/snowing and lightsources
	float sky[4]; //background/fog colour

	float position[4]; //light position
	float ambient[4];
	float diffuse[4];
	float specular[4];
	
	dReal gravity[3];

	dReal density; //for air drag (friction)
	dReal wind[3];

	dReal start[3];
	float cam_start[3];
	float focus_start[3];

	dReal respawn;

	Object *object;
	Space *space;
} track;
//index:

const struct Track_Struct track_defaults = {
	{0.5,0.7,0.8 ,1.0},
	{-1.0,0.5,1.0,0.0},
	{0.0,0.0,0.0 ,1.0},
	{1.0,1.0,1.0 ,1.0},
	{1.0,1.0,1.0 ,1.0},
	{0, 0, -9.82},
	1.29,
	{0.0,0.0,0.0},
	{0,-50,1.5},
	{50,-100,5},
	{0,0,0},
	-20.0};

const struct Conf_Index track_index[] = {
	{"sky",		'f',3,	offsetof(Track_Struct, sky[0])},
	{"ambient",	'f',3,	offsetof(Track_Struct, ambient[0])},
	{"diffuse",	'f',3,	offsetof(Track_Struct, diffuse[0])},
	{"specular",	'f',3,	offsetof(Track_Struct, specular[0])},
	{"position",	'f',4,	offsetof(Track_Struct, position[0])},
	{"gravity",	'R',3,	offsetof(Track_Struct, gravity)},
	{"density",	'R',1,	offsetof(Track_Struct, density)},
	{"wind",	'R',3,	offsetof(Track_Struct, wind)},
	{"start",	'R',3,	offsetof(Track_Struct, start)},
	{"cam_start",	'f',3,	offsetof(Track_Struct, cam_start)},
	{"focus_start",	'f',3,	offsetof(Track_Struct, focus_start)},
	{"respawn",	'R',1,	offsetof(Track_Struct, respawn)},
	{"",0,0}};//end

bool load_track (const char *path);

#endif

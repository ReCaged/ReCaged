/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2015 Mats Wahlberg
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

#ifndef _RCX_PROFILE_H
#define _RCX_PROFILE_H

#include "SDL/SDL_keyboard.h"

#include "simulation/joint.hpp"
#include "car.hpp"

//profile: stores the user's settings (including key list)
struct Profile {
	//the car the user is controlling
	class Car *car;

	//settings (loaded from conf)
	dReal digital_steer_speed[2], digital_throttle_speed[2];
	dReal digital_steer_center_speed[2], digital_throttle_center_speed[2];
	dReal analog_steer_speed, analog_throttle_speed;

	//keep track of last input type (to decide centering speed on no input)
	bool analog_steer, analog_throttle;

	//default camera number
	int camera_default;

	//player inputs
	struct {
		//states
		bool key_state, button_state, hat_state;
		dReal axis_state;

		//mapping:
		SDLKey key;

		Uint8 axis;
		Sint16 axis_min, axis_max;

		Uint8 button;

		Uint8 hat;
		Uint8 hatpos;
	} input[9];

	//for list
	Profile *prev,*next;
};

extern Profile *profile_head;

const Profile profile_defaults = {
	NULL, //car
	//steering throttling speed
	{0.0001, 0.004}, {0.0001, 0.004},
	{0.0040, 0.008}, {0.0040, 0.008},
	0.02, 0.02,
	false, false,
	//default camera setting
	3,
	//control+camera selection keys
	{ //set states to false/0, set default inputs. 255 are unmapped (override in keys.lst)
	{false, false, false, 0.0,	SDLK_UP,	1, -500, -32000, 0, 0, SDL_HAT_UP}, //accelerate
	{false, false, false, 0.0,	SDLK_DOWN,	1, 500, 32000, 1, 0, SDL_HAT_DOWN}, //reverse
	{false, false, false, 0.0,	SDLK_RIGHT,	0, 500, 32000, 255, 0, SDL_HAT_RIGHT}, //right
	{false, false, false, 0.0,	SDLK_LEFT,	0, -500, -32000, 255, 0, SDL_HAT_LEFT}, //left
	{false, false, false, 0.0,	SDLK_SPACE,	255, -200, -32768, 2, 255, SDL_HAT_CENTERED}, //drift brakes
	{false, false, false, 0.0,	SDLK_F1,	255, -200, -32768, 3, 255, SDL_HAT_CENTERED}, //cam1
	{false, false, false, 0.0,	SDLK_F2,	255, -200, -32768, 4, 255, SDL_HAT_CENTERED}, //cam2
	{false, false, false, 0.0,	SDLK_F3,	255, -200, -32768, 5, 255, SDL_HAT_CENTERED}, //cam3
	{false, false, false, 0.0,	SDLK_F4,	255, -200, -32768, 6, 255, SDL_HAT_CENTERED}  //cam4
	}};


const struct Conf_Index profile_index[] = {
	{"digital:steer_speed",		'R' ,2 ,offsetof(Profile, digital_steer_speed)},
	{"digital:throttle_speed",	'R' ,2 ,offsetof(Profile, digital_throttle_speed)},
	{"digital:steer_center_speed",	'R' ,2 ,offsetof(Profile, digital_steer_center_speed)},
	{"digital:throttle_center_speed",'R',2 ,offsetof(Profile, digital_throttle_center_speed)},
	{"analog:steer_speed",		'R' ,1 ,offsetof(Profile, analog_steer_speed)},
	{"analog:throttle_speed",	'R' ,1 ,offsetof(Profile, analog_throttle_speed)},

	{"camera_default",   	   	'i' ,1 ,offsetof(Profile, camera_default)},

	{"",0,0}}; //end


//functions
Profile *Profile_Load(const char*);
void Profile_Remove(Profile*);
void Profile_Remove_All();
void Profile_Input_Collect(SDL_Event*);
void Profile_Input_Step(Uint32);

#endif

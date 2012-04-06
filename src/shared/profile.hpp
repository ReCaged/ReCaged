/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Mats Wahlberg
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

#ifndef _RC_PROFILE_H
#define _RC_PROFILE_H

#include "SDL/SDL_keyboard.h"

#include "car.hpp"
#include "camera.hpp"
#include "joint.hpp"

//profile: stores the user's settings (including key list)
struct Profile {
	//the car the user is controlling
	Car *car;

	//settings (loaded from conf)
	dReal digital_steer_speed, analog_steer_speed, last_steer_speed;
	dReal digital_throttle_speed, analog_throttle_speed, last_throttle_speed;

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

	struct Camera_Settings cam[4];
	int camera;

	//for list
	Profile *prev,*next;
};

extern Profile *profile_head;

const Profile profile_defaults = {
	NULL, //car
	//steering throttling speed
	0.0075, 0.0075, 0.0,
	0.02, 0.02, 0.0,
	//control+camera selection keys
	{ //set states to false/0, set default inputs. 255 are unmapped (override in keys.lst)
	{false, false, false, 0.0,	SDLK_UP,	1, -500, -32000, 0, 0, SDL_HAT_UP},
	{false, false, false, 0.0,	SDLK_DOWN,	1, 500, 32000, 1, 0, SDL_HAT_DOWN},
	{false, false, false, 0.0,	SDLK_RIGHT,	0, 500, 32000, 255, 0, SDL_HAT_RIGHT},
	{false, false, false, 0.0,	SDLK_LEFT,	0, -500, -32000, 255, 0, SDL_HAT_LEFT},
	{false, false, false, 0.0,	SDLK_SPACE,	255, -200, -32768, 2, 255, SDL_HAT_CENTERED},
	{false, false, false, 0.0,	SDLK_F1,	255, -200, -32768, 3, 255, SDL_HAT_CENTERED},
	{false, false, false, 0.0,	SDLK_F2,	255, -200, -32768, 4, 255, SDL_HAT_CENTERED},
	{false, false, false, 0.0,	SDLK_F3,	255, -200, -32768, 5, 255, SDL_HAT_CENTERED},
	{false, false, false, 0.0,	SDLK_F4,	255, -200, -32768, 6, 255, SDL_HAT_CENTERED}
	},
	//camera settings:
	{
	//1:
	{{0,5,0.5},
	{0,3.4,0.5}, {0,0,0},
	true,
	0, 0,
	0,
	0,
	0,
	true,
	0,
	false, false,
	0, 0,
	0},
	//2:
	{{0,5,1.5},
	{0,0,2}, {0,-10,0.5},
	false,
	0, 0,
	500,
	2000,
	25,
	true,
	15,
	true, false,
	0, 0,
	0},
	//3:
	{{0,3,2},
	{0,0,0}, {0,-20,5},
	false,
	3, 70,
	70,
	700,
	10,
	false,
	10,
	true,true,
	0.7, 0.4,
	1.5},
	//4:
	{{0,0,0},
	{0,0,0}, {0,-40,16},
	false,
	4, 60,
	50,
	500,
	4,
	false,
	2,
	true,true,
	0.5, 0.5,
	0}
	//end of settings
	},
	//default camera setting
	3};


const struct Conf_Index profile_index[] = {
	{"digital:steer_speed",		'R' ,1 ,offsetof(Profile, digital_steer_speed)},
	{"digital:throttle_speed",	'R' ,1 ,offsetof(Profile, digital_throttle_speed)},
	{"analog:steer_speed",		'R' ,1 ,offsetof(Profile, analog_steer_speed)},
	{"analog:throttle_speed",	'R' ,1 ,offsetof(Profile, analog_throttle_speed)},

	{"camera_default",   	   	'i' ,1 ,offsetof(Profile, camera)},

	{"camera1:target_offset",	'f' ,3 ,offsetof(Profile, cam[0].target)},
	{"camera1:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[0].anchor)},
	{"camera1:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[0].distance)},
	{"camera1:hide_car",		'b' ,1 ,offsetof(Profile, cam[0].hide_car)},
	{"camera1:collision_radius",	'f' ,1 ,offsetof(Profile, cam[0].radius)},
	{"camera1:collision_angle",	'f' ,1 ,offsetof(Profile, cam[0].angle)},
	{"camera1:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[0].linear_stiffness)},
	{"camera1:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[0].angular_stiffness)},
	{"camera1:damping",		'f' ,1 ,offsetof(Profile, cam[0].damping)},
	{"camera1:relative_damping",	'b' ,1 ,offsetof(Profile, cam[0].relative_damping)},
	{"camera1:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[0].rotation_speed)},
	{"camera1:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[0].reverse)},
	{"camera1:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[0].in_air)},
	{"camera1:air_time",		'f', 1, offsetof(Profile, cam[0].air_time)},
	{"camera1:ground_time",		'f', 1, offsetof(Profile, cam[0].ground_time)},
	{"camera1:offset_scale_speed",	'f', 1, offsetof(Profile, cam[0].offset_scale_speed)},

	{"camera2:target_offset",	'f' ,3 ,offsetof(Profile, cam[1].target)},
	{"camera2:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[1].anchor)},
	{"camera2:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[1].distance)},
	{"camera2:hide_car",		'b' ,1 ,offsetof(Profile, cam[1].hide_car)},
	{"camera2:collision_radius",	'f' ,1 ,offsetof(Profile, cam[1].radius)},
	{"camera2:collision_angle",	'f' ,1 ,offsetof(Profile, cam[1].angle)},
	{"camera2:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[1].linear_stiffness)},
	{"camera2:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[1].angular_stiffness)},
	{"camera2:damping",		'f' ,1 ,offsetof(Profile, cam[1].damping)},
	{"camera2:relative_damping",	'b' ,1 ,offsetof(Profile, cam[1].relative_damping)},
	{"camera2:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[1].rotation_speed)},
	{"camera2:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[1].reverse)},
	{"camera2:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[1].in_air)},
	{"camera2:air_time",		'f', 1, offsetof(Profile, cam[1].air_time)},
	{"camera2:ground_time",		'f', 1, offsetof(Profile, cam[1].ground_time)},
	{"camera2:offset_scale_speed",	'f', 1, offsetof(Profile, cam[1].offset_scale_speed)},

	{"camera3:target_offset",	'f' ,3 ,offsetof(Profile, cam[2].target)},
	{"camera3:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[2].anchor)},
	{"camera3:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[2].distance)},
	{"camera3:hide_car",		'b' ,1 ,offsetof(Profile, cam[2].hide_car)},
	{"camera3:collision_radius",	'f' ,1 ,offsetof(Profile, cam[2].radius)},
	{"camera3:collision_angle",	'f' ,1 ,offsetof(Profile, cam[2].angle)},
	{"camera3:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[2].linear_stiffness)},
	{"camera3:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[2].angular_stiffness)},
	{"camera3:damping",		'f' ,1 ,offsetof(Profile, cam[2].damping)},
	{"camera3:relative_damping",	'b' ,1 ,offsetof(Profile, cam[2].relative_damping)},
	{"camera3:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[2].rotation_speed)},
	{"camera3:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[2].reverse)},
	{"camera3:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[2].in_air)},
	{"camera3:air_time",		'f', 1, offsetof(Profile, cam[2].air_time)},
	{"camera3:ground_time",		'f', 1, offsetof(Profile, cam[2].ground_time)},
	{"camera3:offset_scale_speed",	'f', 1, offsetof(Profile, cam[2].offset_scale_speed)},

	{"camera4:target_offset",	'f' ,3 ,offsetof(Profile, cam[3].target)},
	{"camera4:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[3].anchor)},
	{"camera4:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[3].distance)},
	{"camera4:hide_car",		'b' ,1 ,offsetof(Profile, cam[3].hide_car)},
	{"camera4:collision_radius",	'f' ,1 ,offsetof(Profile, cam[3].radius)},
	{"camera4:collision_angle",	'f' ,1 ,offsetof(Profile, cam[3].angle)},
	{"camera4:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[3].linear_stiffness)},
	{"camera4:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[3].angular_stiffness)},
	{"camera4:damping",		'f' ,1 ,offsetof(Profile, cam[3].damping)},
	{"camera4:relative_damping",	'b' ,1 ,offsetof(Profile, cam[3].relative_damping)},
	{"camera4:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[3].rotation_speed)},
	{"camera4:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[3].reverse)},
	{"camera4:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[3].in_air)},
	{"camera4:air_time",		'f', 1, offsetof(Profile, cam[3].air_time)},
	{"camera4:ground_time",		'f', 1, offsetof(Profile, cam[3].ground_time)},
	{"camera4:offset_scale_speed",	'f', 1, offsetof(Profile, cam[3].offset_scale_speed)},
	{"",0,0}}; //end


//functions
Profile *Profile_Load(const char*);
void Profile_Remove(Profile*);
void Profile_Remove_All();
void Profile_Input_Collect(SDL_Event*);
void Profile_Input_Step(Uint32);

#endif

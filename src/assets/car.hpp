/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014, 2015 Mats Wahlberg
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

#ifndef _RCX_CAR_H
#define _RCX_CAR_H
//car: pointer to object and extra data, adjusted for controlled cars. No
//scripting - used to keep track of components and objects (like weapons)
//bellonging to the player during the race
//Allocated at start
#include "assets.hpp"
#include "model.hpp"
#include "conf.hpp"
#include "profile.hpp"
#include "common/object.hpp"
#include "simulation/camera.hpp"
#include "simulation/body.hpp"
#include "simulation/geom.hpp"

#include <vector>
#include <string>

//for loading car.conf
struct Car_Conf
{
	//3d model
	Conf_String model; //filename+path for model
	float resize, rotate[3], offset[3];

	//motor
	dReal power, min_engine_vel, gear_limit, torque_limit;
	dReal air_torque;
	bool dist_motor[2];
	bool diff;
	bool redist[2];
	bool adapt_redist;
	dReal redist_force;

	//brake
	dReal max_brake;
	dReal dist_brake;
	dReal min_brake_vel;

	//steer
	dReal max_steer;
	dReal dist_steer;
	dReal steer_decrease;
	dReal min_steer;
	dReal limit_speed;
	bool adapt_steer;
	dReal toe[2];

	//other
	dReal body[3], body_mass;
	dReal wheel[2], wheel_mass, wheel_pos[2];
	bool wsphere, wcapsule;

	dReal body_linear_drag[3], body_angular_drag[3], wheel_linear_drag, wheel_angular_drag;
	dReal sensor[4];

	dReal suspension_pos;
	dReal suspension_spring, suspension_damping;
	bool suspension_elevation;
	dReal suspension_threshold, suspension_buffer;

	dReal down_max, down_aero, down_mass;
	bool down_air;

	dReal wheel_spring, wheel_damping, rollres, rim_angle, rim_mu, mix_angle;
	bool finiterot, alt_load, alt_load_damp;

	dReal xstatic, xpeak[2], xtail[2];
	dReal ystatic, ypeak[2], ytail[2];
	bool xaltdenom, yaltdenom;
	dReal xmindenom, ymindenom;
	dReal xmincombine, ymincombine;
	dReal xscalecombine, yscalecombine;
};

const struct Car_Conf car_conf_defaults = {
	"",
	1.0, {0,0,0}, {0,0,0},

	1600000.0, -1.0, 0.0, 50000.0,
	500.0,
	{false, true},
	true,
	{false, true},
	false,
	100.0,

	40000.0,
	0.5,
	0.5,

	35.0,
	1.0,
	0.01,
	0.0,
	100.0,
	true,
	{0.0, 0.0},

	{2.6,5.8,0.7}, 4000.0,
	{1.25, 1.4}, 60.0, {2.4, 1.8},
	false, false,

	{3.0,1.0,5.0}, {20.0, 10.0, 5.0}, 0.0, 0.5,
	{4.8, 3.6, 1.6, 1.25},

	2.05,
	160000.0, 12000.0,
	true,
	0.0, 0.0,

	150000.0, 30.0, 15000.0,
	true,

	400000.0, 1000.0, 0.1, 50.0, 0.1, 10,
	true, true, true,

	0, {0,0}, {0,0},
	0, {0,0}, {0,0},
	false, false,
	0, 0,
	0, 0,
	1.0, 1.0};

const struct Conf_Index car_conf_index[] = {
	{"model",		's',1, offsetof(struct Car_Conf, model)},
	{"model:resize",	'f',1, offsetof(struct Car_Conf, resize)},
	{"model:rotate",	'f',3, offsetof(struct Car_Conf, rotate)},
	{"model:offset",	'f',3, offsetof(struct Car_Conf, offset)},

	{"power",		'R',1, offsetof(struct Car_Conf, power)},
	{"engine:min_vel",	'R',1, offsetof(struct Car_Conf, min_engine_vel)},
	{"gear_limit",		'R',1, offsetof(struct Car_Conf, gear_limit)},
	{"torque_limit",	'R',1, offsetof(struct Car_Conf, torque_limit)},
	{"air_torque_limit",	'R',1, offsetof(struct Car_Conf, air_torque)},
	{"motor_distribution",	'b',2, offsetof(struct Car_Conf, dist_motor)},
	{"differential",	'b',1, offsetof(struct Car_Conf, diff)},
	{"torque_redistribution",'b',2, offsetof(struct Car_Conf, redist)},
	{"adaptive_redistribution",'b',1, offsetof(struct Car_Conf, adapt_redist)},
	{"redistribution_force",'R',1, offsetof(struct Car_Conf, redist_force)},

	{"brake:max",		'R',1, offsetof(struct Car_Conf, max_brake)},
	{"brake:distribution",	'R',1, offsetof(struct Car_Conf, dist_brake)},
	{"brake:min_vel",	'R',1, offsetof(struct Car_Conf, min_brake_vel)},

	{"max_steer",		'R',1, offsetof(struct Car_Conf, max_steer)},
	{"steer_distribution",	'R',1, offsetof(struct Car_Conf, dist_steer)},
	{"steer_decrease",	'R',1, offsetof(struct Car_Conf, steer_decrease)},
	{"min_decreased_steer",	'R',1, offsetof(struct Car_Conf, min_steer)},
	{"steer_limit_speed",	'R',1, offsetof(struct Car_Conf, limit_speed)},
	{"adaptive_steering",	'b',1, offsetof(struct Car_Conf, adapt_steer)},
	{"toe",			'R',2, offsetof(struct Car_Conf, toe)},

	{"body",		'R',3, offsetof(struct Car_Conf, body)},
	{"body:mass",		'R',1, offsetof(struct Car_Conf, body_mass)},
	{"wheel",		'R',2, offsetof(struct Car_Conf, wheel)},
	{"wheel:mass",		'R',1, offsetof(struct Car_Conf, wheel_mass)},
	{"wheel:position",	'R',2, offsetof(struct Car_Conf, wheel_pos)},
	{"wheel:sphere",	'b',1, offsetof(struct Car_Conf, wsphere)},
	{"wheel:capsule",	'b',1, offsetof(struct Car_Conf, wcapsule)},

	{"body:linear_drag",	'R',3, offsetof(struct Car_Conf, body_linear_drag)},
	{"body:angular_drag",	'R',3, offsetof(struct Car_Conf, body_angular_drag)},
	{"wheel:linear_drag",	'R',1, offsetof(struct Car_Conf, wheel_linear_drag)},
	{"wheel:angular_drag",	'R',1, offsetof(struct Car_Conf, wheel_angular_drag)},
	{"sensor",		'R',4, offsetof(struct Car_Conf, sensor)},

	{"suspension:position",	'R',1, offsetof(struct Car_Conf, suspension_pos)},
	{"suspension:spring",	'R',1, offsetof(struct Car_Conf, suspension_spring)},
	{"suspension:damping",	'R',1, offsetof(struct Car_Conf, suspension_damping)},
	{"suspension:elevation",'b',1, offsetof(struct Car_Conf, suspension_elevation)},
	{"suspension:threshold",'R',1, offsetof(struct Car_Conf, suspension_threshold)},
	{"suspension:buffer",	'R',1, offsetof(struct Car_Conf, suspension_buffer)},
	{"downforce:max",	'R',1, offsetof(struct Car_Conf, down_max)},
	{"downforce:aerodynamic",'R',1, offsetof(struct Car_Conf, down_aero)},
	{"downforce:mass_boost",'R',1, offsetof(struct Car_Conf, down_mass)},
	{"downforce:in_air",	'b',1, offsetof(struct Car_Conf, down_air)},
	{"wheel:spring",	'R',1, offsetof(struct Car_Conf, wheel_spring)},
	{"wheel:damping",	'R',1, offsetof(struct Car_Conf, wheel_damping)},
	{"wheel:rollres",	'R',1, offsetof(struct Car_Conf, rollres)},
	{"wheel:rim_angle",	'R',1, offsetof(struct Car_Conf, rim_angle)},
	{"wheel:rim_mu",	'R',1, offsetof(struct Car_Conf, rim_mu)},
	{"tyre:mix_angle",	'R',1, offsetof(struct Car_Conf, mix_angle)},
	{"wheel:finite_rotation",'b',1, offsetof(struct Car_Conf, finiterot)},
	{"tyre:alt_load",	'b',1, offsetof(struct Car_Conf, alt_load)},
	{"tyre:alt_load_damp",	'b',1, offsetof(struct Car_Conf, alt_load_damp)},

	{"tyre:x.static",	'R',1, offsetof(struct Car_Conf, xstatic)},
	{"tyre:x.peak",		'R',2, offsetof(struct Car_Conf, xpeak)},
	{"tyre:x.tail",		'R',2, offsetof(struct Car_Conf, xtail)},

	{"tyre:y.static",	'R',1, offsetof(struct Car_Conf, ystatic)},
	{"tyre:y.peak",		'R',2, offsetof(struct Car_Conf, ypeak)},
	{"tyre:y.tail",		'R',2, offsetof(struct Car_Conf, ytail)},

	{"tyre:x.alt_denom",	'b',1, offsetof(struct Car_Conf, xaltdenom)},
	{"tyre:y.alt_denom",	'b',1, offsetof(struct Car_Conf, yaltdenom)},

	{"tyre:x.min_denom",	'R',1, offsetof(struct Car_Conf, xmindenom)},
	{"tyre:y.min_denom",	'R',1, offsetof(struct Car_Conf, ymindenom)},

	{"tyre:x.min_combine",	'R',1, offsetof(struct Car_Conf, xmincombine)},
	{"tyre:y.min_combine",	'R',1, offsetof(struct Car_Conf, ymincombine)},
	{"tyre:x.scale_combine",'R',1, offsetof(struct Car_Conf, xscalecombine)},
	{"tyre:y.scale_combine",'R',1, offsetof(struct Car_Conf, yscalecombine)},

	{"",0,0}};//end

//for loading camera.conf
struct Camera_Conf
{
	//just store 4 camera settings structs
	Camera_Settings cam[4];
	int selected;
};

//default camera settings:
const struct Camera_Conf camera_conf_defaults = {
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
	},
	//default selected camera (change by profile)
	2
	//end
	};

const struct Conf_Index camera_conf_index[] = {
	{"camera1:target_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[0].target)},
	{"camera1:anchor_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[0].anchor)},
	{"camera1:anchor_distance",	'f' ,3 ,offsetof(Camera_Conf, cam[0].distance)},
	{"camera1:hide_car",		'b' ,1 ,offsetof(Camera_Conf, cam[0].hide_car)},
	{"camera1:collision_radius",	'f' ,1 ,offsetof(Camera_Conf, cam[0].radius)},
	{"camera1:collision_angle",	'f' ,1 ,offsetof(Camera_Conf, cam[0].angle)},
	{"camera1:linear_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[0].linear_stiffness)},
	{"camera1:angular_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[0].angular_stiffness)},
	{"camera1:damping",		'f' ,1 ,offsetof(Camera_Conf, cam[0].damping)},
	{"camera1:relative_damping",	'b' ,1 ,offsetof(Camera_Conf, cam[0].relative_damping)},
	{"camera1:rotation_speed",	'f' ,1 ,offsetof(Camera_Conf, cam[0].rotation_speed)},
	{"camera1:enable_reverse",	'b' ,1 ,offsetof(Camera_Conf, cam[0].reverse)},
	{"camera1:enable_in_air",	'b' ,1 ,offsetof(Camera_Conf, cam[0].in_air)},
	{"camera1:air_time",		'f', 1, offsetof(Camera_Conf, cam[0].air_time)},
	{"camera1:ground_time",		'f', 1, offsetof(Camera_Conf, cam[0].ground_time)},
	{"camera1:offset_scale_speed",	'f', 1, offsetof(Camera_Conf, cam[0].offset_scale_speed)},

	{"camera2:target_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[1].target)},
	{"camera2:anchor_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[1].anchor)},
	{"camera2:anchor_distance",	'f' ,3 ,offsetof(Camera_Conf, cam[1].distance)},
	{"camera2:hide_car",		'b' ,1 ,offsetof(Camera_Conf, cam[1].hide_car)},
	{"camera2:collision_radius",	'f' ,1 ,offsetof(Camera_Conf, cam[1].radius)},
	{"camera2:collision_angle",	'f' ,1 ,offsetof(Camera_Conf, cam[1].angle)},
	{"camera2:linear_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[1].linear_stiffness)},
	{"camera2:angular_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[1].angular_stiffness)},
	{"camera2:damping",		'f' ,1 ,offsetof(Camera_Conf, cam[1].damping)},
	{"camera2:relative_damping",	'b' ,1 ,offsetof(Camera_Conf, cam[1].relative_damping)},
	{"camera2:rotation_speed",	'f' ,1 ,offsetof(Camera_Conf, cam[1].rotation_speed)},
	{"camera2:enable_reverse",	'b' ,1 ,offsetof(Camera_Conf, cam[1].reverse)},
	{"camera2:enable_in_air",	'b' ,1 ,offsetof(Camera_Conf, cam[1].in_air)},
	{"camera2:air_time",		'f', 1, offsetof(Camera_Conf, cam[1].air_time)},
	{"camera2:ground_time",		'f', 1, offsetof(Camera_Conf, cam[1].ground_time)},
	{"camera2:offset_scale_speed",	'f', 1, offsetof(Camera_Conf, cam[1].offset_scale_speed)},

	{"camera3:target_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[2].target)},
	{"camera3:anchor_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[2].anchor)},
	{"camera3:anchor_distance",	'f' ,3 ,offsetof(Camera_Conf, cam[2].distance)},
	{"camera3:hide_car",		'b' ,1 ,offsetof(Camera_Conf, cam[2].hide_car)},
	{"camera3:collision_radius",	'f' ,1 ,offsetof(Camera_Conf, cam[2].radius)},
	{"camera3:collision_angle",	'f' ,1 ,offsetof(Camera_Conf, cam[2].angle)},
	{"camera3:linear_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[2].linear_stiffness)},
	{"camera3:angular_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[2].angular_stiffness)},
	{"camera3:damping",		'f' ,1 ,offsetof(Camera_Conf, cam[2].damping)},
	{"camera3:relative_damping",	'b' ,1 ,offsetof(Camera_Conf, cam[2].relative_damping)},
	{"camera3:rotation_speed",	'f' ,1 ,offsetof(Camera_Conf, cam[2].rotation_speed)},
	{"camera3:enable_reverse",	'b' ,1 ,offsetof(Camera_Conf, cam[2].reverse)},
	{"camera3:enable_in_air",	'b' ,1 ,offsetof(Camera_Conf, cam[2].in_air)},
	{"camera3:air_time",		'f', 1, offsetof(Camera_Conf, cam[2].air_time)},
	{"camera3:ground_time",		'f', 1, offsetof(Camera_Conf, cam[2].ground_time)},
	{"camera3:offset_scale_speed",	'f', 1, offsetof(Camera_Conf, cam[2].offset_scale_speed)},

	{"camera4:target_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[3].target)},
	{"camera4:anchor_offset",	'f' ,3 ,offsetof(Camera_Conf, cam[3].anchor)},
	{"camera4:anchor_distance",	'f' ,3 ,offsetof(Camera_Conf, cam[3].distance)},
	{"camera4:hide_car",		'b' ,1 ,offsetof(Camera_Conf, cam[3].hide_car)},
	{"camera4:collision_radius",	'f' ,1 ,offsetof(Camera_Conf, cam[3].radius)},
	{"camera4:collision_angle",	'f' ,1 ,offsetof(Camera_Conf, cam[3].angle)},
	{"camera4:linear_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[3].linear_stiffness)},
	{"camera4:angular_stiffness",	'f' ,1 ,offsetof(Camera_Conf, cam[3].angular_stiffness)},
	{"camera4:damping",		'f' ,1 ,offsetof(Camera_Conf, cam[3].damping)},
	{"camera4:relative_damping",	'b' ,1 ,offsetof(Camera_Conf, cam[3].relative_damping)},
	{"camera4:rotation_speed",	'f' ,1 ,offsetof(Camera_Conf, cam[3].rotation_speed)},
	{"camera4:enable_reverse",	'b' ,1 ,offsetof(Camera_Conf, cam[3].reverse)},
	{"camera4:enable_in_air",	'b' ,1 ,offsetof(Camera_Conf, cam[3].in_air)},
	{"camera4:air_time",		'f', 1, offsetof(Camera_Conf, cam[3].air_time)},
	{"camera4:ground_time",		'f', 1, offsetof(Camera_Conf, cam[3].ground_time)},
	{"camera4:offset_scale_speed",	'f', 1, offsetof(Camera_Conf, cam[3].offset_scale_speed)},
	{"",0,0}};

class Car_Module:public Assets
{
	public:
		static Car_Module *Load(const char *path);
		class Car *Create(dReal x, dReal y, dReal z, Model_Draw *wheel, class Profile *profile);

	private:
		Car_Module(const char *name); //only allocate through creation function

		//conf:
		Car_Conf conf; //loads from car.conf
		Camera_Conf camera; //loads from camera.conf

		//geoms
		struct geom_properties { //can describe any supported geom
			int type;
			dReal size[3];
			Model_Mesh *mesh;
			dReal pos[3];
			dReal rot[3];
			Surface surf;
		};

		std::vector<geom_properties> geoms;

		//3D rendering model
		Model_Draw *model;
};

class Car:public Object
{
	public:
		//destructor (remove car)
		~Car();

		//change car position (and reset rotation and velocity)
		void Recreate(dReal x, dReal y, dReal z);

		static void Physics_Step(dReal step);


	private:
		Car(); //not allowed to be allocated freely
		friend class Car_Module; //only one allowed to create Car objects
		friend class Camera; //needs access to car info

		//configuration data (copied from Car_Module)
		Camera_Conf camera;
		dReal power, min_engine_vel, gear_limit;
		dReal airtorque;
		dReal body_mass, wheel_mass;
		dReal down_max, down_aero, down_mass;
		bool down_air, elevation;

		dReal max_steer, steerdecr, min_steer, limit_speed, oldsteerlimit;
		dReal max_brake, min_brake_vel;

		bool diff;
		bool fwd, rwd;
		bool fwredist, rwredist;
		bool adapt_steer, adapt_redist;
		dReal redist_force;
		dReal dsteer, dbrake;
		dReal fwtoe, rwtoe;
		bool finiterot;

		//just for keeping track
		dBodyID bodyid,wheel_body[4];
		dJointID joint[4];

		//wheels
		Geom *wheel_geom_data[4];
		bool gotwheel[4];

		//flipover sensors
		Geom *sensor1, *sensor2;
		dReal dir; //direction, 1 or -1

		//controlling values
		bool drift_brakes;
		dReal throttle, steering; //-1.0 to +1.0
		dReal velocity; //keep track of car velocity

		//tmp: wheel+hinge position...
		dReal jx, wx, wy;

		//used when/if recreating suspensions (on recreation)
		dReal sCFM, sERP;
		dReal sthreshold, sbuffer;

		//appart from the object list, keep a list of all cars
		static Car *head;
		Car *prev, *next;

		//controls car
		friend void Profile_Input_Step(Uint32 step);

		//tmp: needs access to above pointers
		friend int Interface_Loop ();
};

#endif

/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014 Mats Wahlberg
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

#ifndef _RC_CAR_H
#define _RC_CAR_H
//car: pointer to object and extra data, adjusted for controlled cars. No
//scripting - used to keep track of components and objects (like weapons)
//bellonging to the player during the race
//Allocated at start
#include "racetime_data.hpp"
#include "object.hpp"
#include "body.hpp"
#include "geom.hpp"
#include "trimesh.hpp"
#include "surface.hpp"
#include "assets/conf.hpp"

#include <vector>
#include <string>

//for loading car.conf
struct Car_Conf
{
	//3d model
	Conf_String model; //filename+path for model
	float resize, rotate[3], offset[3];

	//motor
	dReal power, gear_limit, torque_limit;
	dReal air_torque;
	bool dist_motor[2];
	bool diff;
	bool redist[2];
	bool adapt_redist;
	dReal redist_force;

	//brak
	dReal max_brake;
	dReal dist_brake;

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

	dReal body_linear_drag[3], body_angular_drag, wheel_linear_drag, wheel_angular_drag;
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

	1600000.0, 0.0, 50000.0,
	500.0,
	{false, true},
	true,
	{false, true},
	false,
	100.0,

	40000.0,
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

	{3.0,1.0,5.0}, 10.0, 0.0, 0.5,
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
	{"gear_limit",		'R',1, offsetof(struct Car_Conf, gear_limit)},
	{"torque_limit",	'R',1, offsetof(struct Car_Conf, torque_limit)},
	{"air_torque_limit",	'R',1, offsetof(struct Car_Conf, air_torque)},
	{"motor_distribution",	'b',2, offsetof(struct Car_Conf, dist_motor)},
	{"differential",	'b',1, offsetof(struct Car_Conf, diff)},
	{"torque_redistribution",'b',2, offsetof(struct Car_Conf, redist)},
	{"adaptive_redistribution",'b',1, offsetof(struct Car_Conf, adapt_redist)},
	{"redistribution_force",'R',1, offsetof(struct Car_Conf, redist_force)},

	{"max_brake",		'R',1, offsetof(struct Car_Conf, max_brake)},
	{"brake_distribution",	'R',1, offsetof(struct Car_Conf, dist_brake)},

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
	{"body:angular_drag",	'R',1, offsetof(struct Car_Conf, body_angular_drag)},
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


class Car_Module:public Racetime_Data
{
	public:
		static Car_Module *Load(const char *path);
		class Car *Spawn(dReal x, dReal y, dReal z, Trimesh_3D *tyre, Trimesh_3D *rim);

	private:
		Car_Module(const char *name); //only allocate through spawn function

		//conf:
		struct Car_Conf conf; //loads from conf

		//geoms
		struct geom { //can describe any supported geom
			int type;
			dReal size[3];
			Trimesh_Geom *mesh;
			dReal pos[3];
			dReal rot[3];
			Surface surf;
		};

		std::vector<class geom> geoms;

		//3D rendering model
		Trimesh_3D *model;
};

class Car:public Object
{
	public:
		//destructor (remove car)
		~Car();

		//change car position (and reset rotation and velocity)
		void Respawn(dReal x, dReal y, dReal z);

		static void Physics_Step(dReal step);


	private:
		Car(); //not allowed to be allocated freely
		friend class Car_Module; //only one allowed to create Car objects
		friend class Camera; //needs access to car info

		//configuration data (copied from Car_Module)
		dReal power, gear_limit;
		dReal airtorque;
		dReal body_mass, wheel_mass;
		dReal down_max, down_aero, down_mass;
		bool down_air, elevation;

		dReal max_steer, steerdecr, min_steer, limit_speed, oldsteerlimit;
		dReal max_brake;

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

		//used when/if recreating suspensions (on respawn)
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

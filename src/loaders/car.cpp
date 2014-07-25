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

#include "../shared/internal.hpp"
#include "../shared/racetime_data.hpp"
#include "../shared/car.hpp"
#include "../shared/camera.hpp"
#include "../shared/log.hpp"
#include "../shared/track.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"
#include "../shared/joint.hpp"
#include "text_file.hpp"
#include "../shared/directories.hpp"


Car_Template *Car_Template::Load (const char *path)
{
	Log_Add(1, "Loading car: %s", path);

	//see if already loaded
	if (Car_Template *tmp=Racetime_Data::Find<Car_Template>(path))
		return tmp;

	//apparently not
	Car_Template *target = new Car_Template(path);

	//car.conf
	char conf[strlen(path)+9+1];//+1 for \0
	strcpy (conf,path);
	strcat (conf,"/car.conf");

	Directories dirs;
	if (!(dirs.Find(conf, DATA, READ) && Load_Conf(dirs.Path(), (char *)&target->conf, car_conf_index))) //try to load conf
		return NULL;

	//geoms.lst
	char lst[strlen(path)+10+1];
	strcpy (lst, path);
	strcat (lst, "/geoms.lst");

	Text_File file;
	if (dirs.Find(lst, DATA, READ) && file.Open(dirs.Path()))
	{
		//default surface parameters
		Surface surface;
		struct geom tmp_geom;
		int pos;

		while (file.Read_Line())
		{
			//surface options
			if (file.words[0][0] == '>')
			{
				pos = 1;
				//as long as there are two words left (option name and value)
				while ( (file.word_count-pos) >= 2)
				{
					if (!strcmp(file.words[pos], "mu"))
						surface.mu = strtod(file.words[++pos], (char**)NULL);
					else if (!strcmp(file.words[pos], "bounce"))
						surface.bounce = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "spring"))
						surface.spring = strtod(file.words[++pos], (char**)NULL);
					else if (!strcmp(file.words[pos], "damping"))
						surface.damping = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "sensitivity"))
						surface.sensitivity = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "rollres"))
						surface.rollres = atof(file.words[++pos]);
					else
					{
						Log_Add(0, "WARNING: surface option \"%s\" unknown", file.words[pos]);
					}

					//one step forward
					pos+=1;
				}
			}
			//geom to spawn
			else
			{
				if (!strcmp(file.words[0], "sphere") && file.word_count >= 2)
				{
					tmp_geom.type = 0;
					tmp_geom.size[0] = atof(file.words[1]);
					pos = 2;
				}
				else if (!strcmp(file.words[0], "capsule") && file.word_count >= 3)
				{
					tmp_geom.type = 1;
					tmp_geom.size[0] = atof(file.words[1]);
					tmp_geom.size[1] = atof(file.words[2]);
					pos = 3;
				}
				else if (!strcmp(file.words[0], "box") && file.word_count >= 4)
				{
					tmp_geom.type = 2;
					tmp_geom.size[0] = atof(file.words[1]);
					tmp_geom.size[1] = atof(file.words[2]);
					tmp_geom.size[2] = atof(file.words[3]);
					pos = 4;
				}
				else if (!strcmp(file.words[0], "trimesh") && file.word_count >= 3)
				{
					tmp_geom.type = 3;

					//create complete path+filename
					char model[strlen(path)+1+strlen(file.words[1])+1];
					strcpy(model, path);
					strcat(model, "/");
					strcat(model, file.words[1]);

					tmp_geom.mesh = Trimesh_Geom::Quick_Load(model, atof(file.words[2]), 0, 0, 0, 0, 0, 0);

					//failed to load
					if (!tmp_geom.mesh)
					{
						Log_Add(-1, "Trimesh geom in car geom list could not be loaded!");
						continue; //don't add
					}

					pos = 3;
				}
				else
				{
					Log_Add(-1, "Geom \"%s\" in car geom list not recognized/malformed!", file.words[0]);
					continue; //go to next line
				}

				//we now got a geom, look for options and add
				//default position+rotation
				tmp_geom.pos[0]=0.0;
				tmp_geom.pos[1]=0.0;
				tmp_geom.pos[2]=0.0;

				tmp_geom.rot[0]=0.0;
				tmp_geom.rot[1]=0.0;
				tmp_geom.rot[2]=0.0;

				for (; pos+4<=file.word_count; pos+=4)
				{
					if (file.words[pos][0]=='p') //=p[osition]
					{
						tmp_geom.pos[0]=atof(file.words[pos+1]);
						tmp_geom.pos[1]=atof(file.words[pos+2]);
						tmp_geom.pos[2]=atof(file.words[pos+3]);
					}
					else if (file.words[pos][0]=='r') //=r[otation]
					{
						tmp_geom.rot[0]=atof(file.words[pos+1]);
						tmp_geom.rot[1]=atof(file.words[pos+2]);
						tmp_geom.rot[2]=atof(file.words[pos+3]);
					}
				}

				//set surface options
				tmp_geom.surf = surface;

				//store
				target->geoms.push_back(tmp_geom);
			}
		}
	}
	else
		Log_Add(0, "WARNING: can not open list of car geoms (%s)!", lst);

	//helper datas:

	//wheel simulation class (friction + some custom stuff):
	//target->wheel.join_d2 = target->conf.join_dist*target->conf.join_dist;
	target->wheel.merge_dot = cos(target->conf.merge_angle*M_PI/180.0);
	target->wheel.rim_dot = sin(target->conf.rim_angle*M_PI/180.0);
	//cylinder moment of inertia tensor for Z = (mass*r²)/2
	target->wheel.inertia = target->conf.wheel_mass*target->conf.w[0]*target->conf.w[0]/2.0; 
	target->wheel.rollres = target->conf.rollres;

	//check and copy data from conf to wheel class
	//x
	target->wheel.x_static_mu = target->conf.xstatic;
	target->wheel.x_peak_pos = target->conf.xpeak[0];
	target->wheel.x_peak_mu = target->conf.xpeak[1];
	target->wheel.x_tail_pos = target->conf.xtail[0];
	target->wheel.x_tail_mu = target->conf.xtail[1];

	//y
	target->wheel.y_static_mu = target->conf.ystatic;
	target->wheel.y_peak_pos = target->conf.ypeak[0];
	target->wheel.y_peak_mu = target->conf.ypeak[1];
	target->wheel.y_tail_pos = target->conf.ytail[0];
	target->wheel.y_tail_mu = target->conf.ytail[1];
	//

	target->wheel.x_alt_denom = target->conf.xaltdenom;
	target->wheel.y_alt_denom = target->conf.yaltdenom;
	target->wheel.x_min_denom = target->conf.xmindenom;
	target->wheel.y_min_denom = target->conf.ymindenom;
	target->wheel.x_min_combine = target->conf.xmincombine;
	target->wheel.y_min_combine = target->conf.ymincombine;
	target->wheel.x_scale_combine = target->conf.xscalecombine;
	target->wheel.y_scale_combine = target->conf.yscalecombine;


	//make sure the values are correct
	//steering distribution
	if (target->conf.dist_steer >1.0 || target->conf.dist_steer <0.0 )
	{
		Log_Add(-1, "Front/rear steering distribution should be range 0 to 1! (enabling front)");
		target->conf.dist_steer = 1.0;
	}

	//check if neither front or rear drive
	if ( (!target->conf.dist_motor[0]) && (!target->conf.dist_motor[1]) )
	{
		Log_Add(-1, "Either front and rear motor distribution must be enabled! (enabling 4WD)");
		target->conf.dist_motor[0] = true;
		target->conf.dist_motor[1] = true;
	}

	//braking distribution
	if (target->conf.dist_brake>1.0 || target->conf.dist_brake<0.0 )
	{
		Log_Add(-1, "Front/rear braking distribution should be range 0 to 1! (enabling rear)");
		target->conf.dist_brake = 0.0;
	}


	//load model if specified
	if (target->conf.model[0] == '\0') //empty string
	{
		Log_Add(1, "WARNING: no car 3D model specified\n");
		target->model=NULL;
	}
	else
	{
		char file[strlen(path)+1+strlen(target->conf.model)+1];
		strcpy(file, path);
		strcat(file, "/");
		strcat(file, target->conf.model);

		if ( !(target->model = Trimesh_3D::Quick_Load(file,
				target->conf.resize,
				target->conf.rotate[0], target->conf.rotate[1], target->conf.rotate[2],
				target->conf.offset[0], target->conf.offset[1], target->conf.offset[2])) )
			return NULL;
	}

	return target;
}


Car *Car_Template::Spawn (dReal x, dReal y, dReal z,  Trimesh_3D *tyre, Trimesh_3D *rim)
{
	Log_Add(1, "spawning car at: %f %f %f", x,y,z);

	//begin copying of needed configuration data
	Car *car = new Car();

	car->wheel = &wheel;

	car->power = conf.power;

	//if limit by torque
	if (conf.torque_limit)
		car->gear_limit = conf.power/conf.torque_limit;
	else //else, limit by rotation speed
		car->gear_limit = conf.gear_limit;


	car->max_brake = conf.max_brake;
	car->max_steer = conf.max_steer;
	car->steerdecr = conf.steer_decrease;
	car->min_steer = conf.min_steer;
	car->limit_speed = conf.limit_speed;
	car->airtorque = conf.air_torque;

	car->body_mass = conf.body_mass;
	car->wheel_mass = conf.wheel_mass;

	car->down_max = conf.down_max;
	car->down_air = conf.down_air;
	car->down_aero = conf.down_aero;
	car->down_mass = conf.down_mass;

	car->elevation = conf.suspension_elevation;
	car->hinge2_dbrakes = conf.handbrake_lock;

	car->dsteer = conf.dist_steer;
	car->dbrake = conf.dist_brake;
	car->fwd = conf.dist_motor[0];
	car->rwd = conf.dist_motor[1];
	car->fwredist = conf.redist[0];
	car->rwredist = conf.redist[1];
	car->fwtoe = conf.toe[0]*M_PI/180.0;
	car->rwtoe = conf.toe[1]*M_PI/180.0;

	car->diff = conf.diff;
	car->adapt_steer = conf.adapt_steer;
	car->adapt_redist = conf.adapt_redist;
	car->redist_force = conf.redist_force;

	car->turn = conf.turn;

	//start building
	new Space(car);

	dMass m;
	car->bodyid = dBodyCreate (world);
	dBodySetAutoDisableFlag (car->bodyid, 0); //never disable main body
	

	//set mass
	dMassSetBoxTotal (&m,conf.body_mass,conf.body[0], conf.body[1], conf.body[2]); //mass+sides
	dBodySetMass (car->bodyid, &m);

	//set up air (and liquid) drag for body
	Body *bdata = new Body (car->bodyid, car);
	bdata->Set_Advanced_Linear_Drag (conf.body_linear_drag[0], conf.body_linear_drag[1], conf.body_linear_drag[2]);
	//rotational drag
	bdata->Set_Angular_Drag (conf.body_angular_drag);


	dBodySetPosition (car->bodyid, x, y+car->offset, z);


	//ok, set rendering model:
	bdata->model = model;

	
	//done, add collision geoms:
	dGeomID geom;
	Geom *gdata;
	struct geom tmp_geom;
	dMatrix3 rot;
	for (size_t i=0; i<geoms.size(); ++i)
	{
		tmp_geom = geoms[i];

		if (tmp_geom.type == 0) //sphere
		{
			geom = dCreateSphere(0,tmp_geom.size[0]);
			gdata = new Geom(geom, car);
		}
		else if (tmp_geom.type == 1) //capsule
		{
			geom = dCreateCapsule(0,tmp_geom.size[0],tmp_geom.size[1]);
			gdata = new Geom (geom, car);
		}
		else if (tmp_geom.type == 2) //box
		{
			geom = dCreateBox(0,tmp_geom.size[0],tmp_geom.size[1],tmp_geom.size[2]);
			gdata = new Geom (geom, car);
		}
		else // (tmp_geom.type == 3) //trimesh
		{
			gdata = tmp_geom.mesh->Create_Geom(car);
			geom = gdata->geom_id;
		}

		dGeomSetBody (geom, car->bodyid);

		if (tmp_geom.pos[0]||tmp_geom.pos[1]||tmp_geom.pos[2]) //need offset
			dGeomSetOffsetPosition(geom,tmp_geom.pos[0],tmp_geom.pos[1],tmp_geom.pos[2]);

		if (tmp_geom.rot[0]||tmp_geom.rot[1]||tmp_geom.rot[2]) //need rotation
		{
			dRFromEulerAngles(rot, tmp_geom.rot[0]*M_PI/180.0, tmp_geom.rot[1]*M_PI/180.0, tmp_geom.rot[2]*M_PI/180.0);
			dGeomSetOffsetRotation(geom, rot);
		}

		gdata->surface = tmp_geom.surf;

	}

	//side detection sensors:
	dReal *s = conf.s;

	geom = dCreateBox(0,s[0],s[1],s[2]);
	car->sensor1 = new Geom (geom, car);
	car->sensor1->surface.spring = 0.0; //untouchable "ghost" geom - sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,-s[3]);

	geom = dCreateBox(0,s[0],s[1],s[2]);
	car->sensor2 = new Geom (geom, car);
	car->sensor2->surface.spring = 0.0; //sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,s[3]);

	//wheels:
	Geom *wheel_data[4];
	dGeomID wheel_geom;
	dBodyID wheel_body[4];

	//3=z axis of cylinder
	dMassSetCylinderTotal (&m, conf.wheel_mass, 3, conf.w[0], conf.w[1]);

	for (int i=0;i<4;++i)
	{
		//create cylinder
		if (conf.wsphere)
			wheel_geom = dCreateSphere (0, conf.w[0]);
		else if (conf.wcapsule)
			wheel_geom = dCreateCapsule (0, conf.w[0], conf.w[1]);
		else //normal
			wheel_geom = dCreateCylinder (0, conf.w[0], conf.w[1]);


		//(body)
		wheel_body[i] = dBodyCreate (world);

		//never disable wheel body
		dBodySetAutoDisableFlag (wheel_body[i], 0);

		//disable gyroscopic affect?
		if (!conf.gyro)
			dBodySetGyroscopicMode(wheel_body[i], 0);


		//set mass
		dBodySetMass (wheel_body[i], &m);

		//and connect to body
		dGeomSetBody (wheel_geom, wheel_body[i]);

		//allocate (geom) data
		wheel_data[i] = new Geom(wheel_geom, car);

		//data:
		//note: we set the usual geom parameters as if the wheel was just a
		//normal rigid geom, with the rim mu as the mu. this is good if indeed
		//the rim part of the wheel is colliding. but if it's the tyre part
		//a lot of things will change (erp,cfm,mu,mode). this extra data is
		//generated by the Wheel class, conveniently pointed to from the geom.
		//
		//friction (use rim mu by default until knowing it's tyre)
		wheel_data[i]->surface.mu = conf.rim_mu;
		//spring&damping even against rim (transitions likely to cause bumps)
		wheel_data[i]->surface.spring = conf.wheel_spring;
		wheel_data[i]->surface.damping = conf.wheel_damping;
		//points at our wheel simulation class (indicates wheel)
		wheel_data[i]->wheel = &wheel;

		//drag
		bdata = new Body (wheel_body[i], car);
		bdata->Set_Linear_Drag (conf.wheel_linear_drag);
		//rotational drag
		bdata->Set_Angular_Drag (conf.wheel_angular_drag);

		//(we need easy access to wheel body ids
		car->wheel_body[i] = wheel_body[i];
		car->wheel_geom_data[i] = wheel_data[i];

		//graphics.
		//note: it's now possible to render two models for each wheel:
		//one for the geom and one for the body
		//this is great in this case where the wheels got two models (rim+tyre)
		bdata->model = rim;
		wheel_data[i]->model = tyre;
	}

	//place and rotate wheels
	//(does not care about toe, which will be set during simulation)
	dRFromAxisAndAngle (rot, 0, 1, 0, M_PI/2);
	dBodySetPosition (wheel_body[0], x+conf.wp[0], y+conf.wp[1], z);
	dBodySetRotation (wheel_body[0], rot);
	dBodySetPosition (wheel_body[1], x+conf.wp[0], y-conf.wp[1], z);
	dBodySetRotation (wheel_body[1], rot);

	dRFromAxisAndAngle (rot, 0, 1, 0, -M_PI/2);
	dBodySetPosition (wheel_body[2], x-conf.wp[0], y-conf.wp[1], z);
	dBodySetRotation (wheel_body[2], rot);
	dBodySetPosition (wheel_body[3], x-conf.wp[0], y+conf.wp[1], z);
	dBodySetRotation (wheel_body[3], rot);

	//tmp: might need these later on
	car->jx = conf.jx;
	car->wx = conf.wp[0];
	car->wy = conf.wp[1];

	//create joints (hinge2) for wheels
	dReal stepsize = internal.stepsize/internal.multiplier;
	dReal sERP = stepsize*conf.suspension_spring/(stepsize*conf.suspension_spring+conf.suspension_damping);
	dReal sCFM = 1.0/(stepsize*conf.suspension_spring+conf.suspension_damping);
	for (int i=0; i<4; ++i)
	{
		car->joint[i]=dJointCreateHinge2 (world, 0);
		new Joint(car->joint[i], car);
		//body is still body of car main body
		dJointAttach (car->joint[i], car->bodyid, wheel_body[i]);
		dJointSetHinge2Axis1 (car->joint[i],0,0,1);
		dJointSetHinge2Axis2 (car->joint[i],1,0,0);

		//setup suspension
		dJointSetHinge2Param (car->joint[i],dParamSuspensionERP,sERP);
		dJointSetHinge2Param (car->joint[i],dParamSuspensionCFM,sCFM);

		//lock steering axis on all wheels
		dJointSetHinge2Param (car->joint[i],dParamLoStop,0);
		dJointSetHinge2Param (car->joint[i],dParamHiStop,0);
	}

	//to make it possible to tweak the hinge2 anchor x position:
	
	dJointSetHinge2Anchor (car->joint[0],x+conf.jx,y+conf.wp[1],z);
	dJointSetHinge2Anchor (car->joint[1],x+conf.jx,y-conf.wp[1],z);
	dJointSetHinge2Anchor (car->joint[2],x-conf.jx,y-conf.wp[1],z);
	dJointSetHinge2Anchor (car->joint[3],x-conf.jx,y+conf.wp[1],z);

	//return
	return car;
}

void Car::Respawn (dReal x, dReal y, dReal z)
{
	Log_Add(1, "respawning car at: %f %f %f", x,y,z);

	//remember old car position
	const dReal *pos = dBodyGetPosition(bodyid);
	float oldx = pos[0];
	float oldy = pos[1];
	float oldz = pos[2];

	//will use this to reset rotation
	dMatrix3 r;

	//body:
	dRSetIdentity(r); //no rotation
	dBodySetPosition(bodyid, x, y+offset, z);
	dBodySetRotation(bodyid, r);

	//wheels:
	//right side rotation
	dRFromAxisAndAngle (r, 0, 1, 0, M_PI/2);
	dBodySetPosition (wheel_body[0], x+wx, y+wy, z);
	dBodySetRotation (wheel_body[0], r);
	dBodySetPosition (wheel_body[1], x+wx, y-wy, z);
	dBodySetRotation (wheel_body[1], r);

	//left side
	dRFromAxisAndAngle (r, 0, 1, 0, -M_PI/2);
	dBodySetPosition (wheel_body[2], x-wx, y-wy, z);
	dBodySetRotation (wheel_body[2], r);
	dBodySetPosition (wheel_body[3], x-wx, y+wy, z);
	dBodySetRotation (wheel_body[3], r);

	//remove velocities
	dBodySetLinearVel(bodyid, 0.0, 0.0, 0.0);
	dBodySetAngularVel(bodyid, 0.0, 0.0, 0.0);

	dBodySetLinearVel(wheel_body[0], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[0], 0.0, 0.0, 0.0);
	dBodySetLinearVel(wheel_body[1], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[1], 0.0, 0.0, 0.0);
	dBodySetLinearVel(wheel_body[2], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[2], 0.0, 0.0, 0.0);
	dBodySetLinearVel(wheel_body[3], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[3], 0.0, 0.0, 0.0);

	//set camera position (move it as much as we moved the car)
	//TODO: in future (with multiple cameras), loop through them all and change those that looks at this car
	//for (Camera camera = Camera::head; camera; camera=camera->next)
	//{
		if (camera.car == this)
		{
			camera.pos[0] += (x-oldx);
			camera.pos[1] += (y-oldy);
			camera.pos[2] += (z-oldz);
		}
	//}
}

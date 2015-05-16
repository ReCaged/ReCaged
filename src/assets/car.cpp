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

#include "car.hpp"
#include "text_file.hpp"
#include "assets.hpp"
#include "assets/track.hpp"
#include "common/internal.hpp"
#include "common/log.hpp"
#include "common/directories.hpp"
#include "common/threads.hpp"
#include "simulation/camera.hpp"
#include "simulation/geom.hpp"
#include "simulation/body.hpp"
#include "simulation/joint.hpp"

//for creation:
Car_Module::Car_Module(const char *name) :Assets(name)
{
}

Car *Car::head = NULL;

//allocates car, add to list...
Car::Car(void)
{
	Log_Add(2, "configuring Car class");

	//default values
	dir = 1; //initiate to 1 for default

	//control values
	drift_brakes = true; //if the user does nothing, lock wheels
	throttle = 0;
	steering = 0;
	oldsteerlimit = 0;
	velocity = 0;

	//linking
	next=head;
	head=this;

	if (next)
		next->prev=this;

	prev=NULL;
}

//run _before_ starting full erase of object/component lists (at race end)
Car::~Car()
{
	Log_Add(2, "clearing Car class");

	//remove from list
	if (!prev) //head
		head = next;
	else //not head
		prev->next = next;

	if (next) //not last
		next->prev = prev;
}

//loading/creating:
Car_Module *Car_Module::Load (const char *path)
{
	Log_Add(1, "Loading car: %s", path);

	//see if already loaded
	if (Car_Module *tmp=Assets::Find<Car_Module>(path))
		return tmp;

	//apparently not
	Car_Module *target = new Car_Module(path);

	//set conf values to defaults
	target->conf = car_conf_defaults;
	target->camera = camera_conf_defaults;

	//try loading confs (still got defaults if it fails):
	std::string filepath;
	Directories dirs;

	filepath=path;
	filepath+="/car.conf";

	if (! (dirs.Find(filepath.c_str(), DATA, READ) &&
		Load_Conf(dirs.Path(), (char *)&target->conf, car_conf_index)) )
		Log_Add(0, "WARNING: can not open car configuration (%s)!", filepath.c_str());


	filepath=path;
	filepath+="/camera.conf";

	if (! (dirs.Find(filepath.c_str(), DATA, READ) &&
		Load_Conf(dirs.Path(), (char *)&target->camera, camera_conf_index)) )
		Log_Add(0, "WARNING: can not open camera configuration (%s)!", filepath.c_str());

	//geoms.lst
	filepath=path;
	filepath+="/geoms.lst";

	Text_File file;
	if (dirs.Find(filepath.c_str(), DATA, READ) && file.Open(dirs.Path()))
	{
		//default surface parameters
		Surface surface;
		struct geom_properties tmp_geom;
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
			//geom to create
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

					tmp_geom.mesh = Model_Mesh::Quick_Load(model, atof(file.words[2]), 0, 0, 0, 0, 0, 0);

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
		Log_Add(0, "WARNING: can not open list of car geoms (%s)!", filepath.c_str());

	//helper datas:

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

		if ( !(target->model = Model_Draw::Quick_Load(file,
				target->conf.resize,
				target->conf.rotate[0], target->conf.rotate[1], target->conf.rotate[2],
				target->conf.offset[0], target->conf.offset[1], target->conf.offset[2])) )
			return NULL;
	}

	return target;
}


Car *Car_Module::Create (dReal x, dReal y, dReal z, Model_Draw *wheel3D, Profile *profile)
{
	Log_Add(1, "creating car at: %f %f %f", x,y,z);

	//begin copying of needed configuration data
	Car *car = new Car();

	//copy camera settings (and set default, from player profile)
	car->camera = camera;
	car->camera.selected = profile->camera_default-1; //safe checked by profile, just change index

	//hmmm.... should do the same for all below (just add safetychecks):
	car->power = conf.power;

	//if limit by torque
	if (conf.torque_limit)
		car->gear_limit = conf.power/conf.torque_limit;
	else //else, limit by rotation speed
		car->gear_limit = conf.gear_limit;

	car->min_engine_vel = conf.min_engine_vel;

	car->max_brake = conf.max_brake;
	car->min_brake_vel = conf.min_brake_vel;

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

	car->dsteer = conf.dist_steer;
	car->dbrake = conf.dist_brake;
	car->fwd = conf.dist_motor[0];
	car->rwd = conf.dist_motor[1];
	car->fwredist = conf.redist[0];
	car->rwredist = conf.redist[1];
	car->fwtoe = conf.toe[0]*M_PI/180.0;
	car->rwtoe = conf.toe[1]*M_PI/180.0;

	car->finiterot = conf.finiterot;

	car->diff = conf.diff;
	car->adapt_steer = conf.adapt_steer;
	car->adapt_redist = conf.adapt_redist;
	car->redist_force = conf.redist_force;

	//start building
	new Space(car);

	dMass m;
	car->bodyid = dBodyCreate (simulation_thread.world);
	dBodySetAutoDisableFlag (car->bodyid, 0); //never disable main body
	

	//set mass
	dMassSetBoxTotal (&m,conf.body_mass,conf.body[0], conf.body[1], conf.body[2]); //mass+sides
	dBodySetMass (car->bodyid, &m);

	//set up air (and liquid) drag for body
	Body *bdata = new Body (car->bodyid, car);
	bdata->Set_Axis_Linear_Drag (conf.body_linear_drag[0], conf.body_linear_drag[1], conf.body_linear_drag[2]);
	//rotational drag
	bdata->Set_Axis_Angular_Drag (conf.body_angular_drag[0], conf.body_angular_drag[1], conf.body_angular_drag[2]);


	dBodySetPosition (car->bodyid, x, y, z);


	//ok, set rendering model:
	bdata->model = model;

	
	//done, add collision geoms:
	dGeomID geom;
	Geom *gdata;
	struct geom_properties tmp_geom;
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
			gdata = tmp_geom.mesh->Create_Mesh(car);
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
	geom = dCreateBox(0,conf.sensor[0],conf.sensor[1],conf.sensor[2]);
	car->sensor1 = new Geom (geom, car);
	car->sensor1->surface.spring = 0.0; //untouchable "ghost" geom - sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,-conf.sensor[3]);

	geom = dCreateBox(0,conf.sensor[0],conf.sensor[1],conf.sensor[2]);
	car->sensor2 = new Geom (geom, car);
	car->sensor2->surface.spring = 0.0; //sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,conf.sensor[3]);

	//wheel simulation class (friction + some custom stuff):
	Wheel *wheel[4];
	for (int i=0; i<4; ++i)
	{
		wheel[i]=new Wheel;
		wheel[i]->mix_dot = cos(conf.mix_angle*M_PI/180.0);
		wheel[i]->rim_dot = sin(conf.rim_angle*M_PI/180.0);
		wheel[i]->rollres = conf.rollres;
		wheel[i]->alt_load = conf.alt_load;
		wheel[i]->alt_load_damp = conf.alt_load_damp;

		//check and copy data from conf to wheel class
		//x
		wheel[i]->x_static_mu = conf.xstatic;
		wheel[i]->x_peak_pos = conf.xpeak[0];
		wheel[i]->x_peak_mu = conf.xpeak[1];
		wheel[i]->x_tail_pos = conf.xtail[0];
		wheel[i]->x_tail_mu = conf.xtail[1];

		//y
		wheel[i]->y_static_mu = conf.ystatic;
		wheel[i]->y_peak_pos = conf.ypeak[0];
		wheel[i]->y_peak_mu = conf.ypeak[1];
		wheel[i]->y_tail_pos = conf.ytail[0];
		wheel[i]->y_tail_mu = conf.ytail[1];
		//

		wheel[i]->x_alt_denom = conf.xaltdenom;
		wheel[i]->y_alt_denom = conf.yaltdenom;
		wheel[i]->x_min_denom = conf.xmindenom;
		wheel[i]->y_min_denom = conf.ymindenom;
		wheel[i]->x_min_combine = conf.xmincombine;
		wheel[i]->y_min_combine = conf.ymincombine;
		wheel[i]->x_scale_combine = conf.xscalecombine;
		wheel[i]->y_scale_combine = conf.yscalecombine;
	}

	//wheels:
	Geom *wheel_data[4];
	dGeomID wheel_geom;
	dBodyID wheel_body[4];

	//3=z axis of cylinder
	dMassSetCylinderTotal (&m, conf.wheel_mass, 3, conf.wheel[0], conf.wheel[1]);

	for (int i=0;i<4;++i)
	{
		//create cylinder
		if (conf.wsphere)
			wheel_geom = dCreateSphere (0, conf.wheel[0]);
		else if (conf.wcapsule)
			wheel_geom = dCreateCapsule (0, conf.wheel[0], conf.wheel[1]);
		else //normal
			wheel_geom = dCreateCylinder (0, conf.wheel[0], conf.wheel[1]);


		//(body)
		wheel_body[i] = dBodyCreate (simulation_thread.world);

		//never disable wheel body
		dBodySetAutoDisableFlag (wheel_body[i], 0);

		//enable finite rotation?
		if (conf.finiterot)
			dBodySetFiniteRotationMode(wheel_body[i], 1);

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
		wheel_data[i]->wheel = wheel[i];

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
		bdata->model = wheel3D;
	}

	//place and rotate wheels
	//(does not care about toe, which will be set during simulation)
	dRFromAxisAndAngle (rot, 0, 1, 0, M_PI/2);
	dBodySetPosition (wheel_body[0], x+conf.wheel_pos[0], y+conf.wheel_pos[1], z);
	dBodySetRotation (wheel_body[0], rot);
	dBodySetPosition (wheel_body[1], x+conf.wheel_pos[0], y-conf.wheel_pos[1], z);
	dBodySetRotation (wheel_body[1], rot);

	dRFromAxisAndAngle (rot, 0, 1, 0, -M_PI/2);
	dBodySetPosition (wheel_body[2], x-conf.wheel_pos[0], y-conf.wheel_pos[1], z);
	dBodySetRotation (wheel_body[2], rot);
	dBodySetPosition (wheel_body[3], x-conf.wheel_pos[0], y+conf.wheel_pos[1], z);
	dBodySetRotation (wheel_body[3], rot);

	//might need these later on
	car->jx = conf.suspension_pos;
	car->wx = conf.wheel_pos[0];
	car->wy = conf.wheel_pos[1];

	car->sthreshold = conf.suspension_threshold;
	car->sbuffer = conf.suspension_buffer;

	//create joints (hinge2) for wheels
	dReal stepsize = internal.stepsize/internal.multiplier;
	car->sERP = stepsize*conf.suspension_spring/(stepsize*conf.suspension_spring+conf.suspension_damping);
	car->sCFM = 1.0/(stepsize*conf.suspension_spring+conf.suspension_damping);
	Joint *jointd;
	for (int i=0; i<4; ++i)
	{
		car->joint[i]=dJointCreateHinge2 (simulation_thread.world, 0);
		jointd = new Joint(car->joint[i], car);
		jointd->carwheel = &car->gotwheel[i];

		//wheel is not missing (yet)
		car->gotwheel[i]=true;

		//set damage settings: (if seems ok)
		if (conf.suspension_threshold > 0 && isnormal(conf.suspension_threshold))
		{
			Log_Add(2, "enabling damageable wheel suspension");
			jointd->Set_Buffer_Event(conf.suspension_threshold, conf.suspension_buffer,
					(int*)1337);
		}

		//body is still body of car main body
		dJointAttach (car->joint[i], car->bodyid, wheel_body[i]);
		dJointSetHinge2Axis1 (car->joint[i],0,0,1);
		dJointSetHinge2Axis2 (car->joint[i],1,0,0);

		//setup suspension
		dJointSetHinge2Param (car->joint[i],dParamSuspensionERP,car->sERP);
		dJointSetHinge2Param (car->joint[i],dParamSuspensionCFM,car->sCFM);

		//lock steering axis on all wheels
		dJointSetHinge2Param (car->joint[i],dParamLoStop,0);
		dJointSetHinge2Param (car->joint[i],dParamHiStop,0);
	}

	//to make it possible to tweak the hinge2 anchor x position:
	dJointSetHinge2Anchor (car->joint[0],x+conf.suspension_pos,y+conf.wheel_pos[1],z);
	dJointSetHinge2Anchor (car->joint[1],x+conf.suspension_pos,y-conf.wheel_pos[1],z);
	dJointSetHinge2Anchor (car->joint[2],x-conf.suspension_pos,y-conf.wheel_pos[1],z);
	dJointSetHinge2Anchor (car->joint[3],x-conf.suspension_pos,y+conf.wheel_pos[1],z);

	//return
	return car;
}

void Car::Recreate (dReal x, dReal y, dReal z)
{
	Log_Add(1, "recreating car at: %f %f %f", x,y,z);

	//remember old car position
	const dReal *pos = dBodyGetPosition(bodyid);
	float oldx = pos[0];
	float oldy = pos[1];
	float oldz = pos[2];

	//will use this to reset rotation
	dMatrix3 r;

	//body:
	dRSetIdentity(r); //no rotation
	dBodySetPosition(bodyid, x, y, z);
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

	//repair broken suspensions
	Joint *jointd;
	for (int i=0; i<4; ++i)
	{
		if (!gotwheel[i])
		{
			joint[i]=dJointCreateHinge2 (simulation_thread.world, 0);
			jointd = new Joint(joint[i], this);
			jointd->carwheel = &gotwheel[i];

			//like before
			if (sthreshold > 0 && isnormal(sthreshold))
			{
				Log_Add(2, "enabling damageable wheel suspension");
				jointd->Set_Buffer_Event(sthreshold, sbuffer,
						(int *)1337);
			}

			//body is still body of car main body
			dJointAttach (joint[i], bodyid, wheel_body[i]);
			dJointSetHinge2Axis1 (joint[i],0,0,1);
			dJointSetHinge2Axis2 (joint[i],1,0,0);

			//setup suspension
			dJointSetHinge2Param (joint[i],dParamSuspensionERP,sERP);
			dJointSetHinge2Param (joint[i],dParamSuspensionCFM,sCFM);

			//lock steering axis on all wheels
			dJointSetHinge2Param (joint[i],dParamLoStop,0);
			dJointSetHinge2Param (joint[i],dParamHiStop,0);

			gotwheel[i]=true;
		}
	}

	//refresh all anchors (doesn't hurt)
	dJointSetHinge2Anchor (joint[0],x+jx,y+wy,z);
	dJointSetHinge2Anchor (joint[1],x+jx,y-wy,z);
	dJointSetHinge2Anchor (joint[2],x-jx,y-wy,z);
	dJointSetHinge2Anchor (joint[3],x-jx,y+wy,z);

	//set camera position (move it as much as we moved the car)
	//TODO: in future (with multiple cameras), loop through them all and change those that looks at this car
	//for (Camera camera = Camera::head; camera; camera=camera->next)
	//{
		if (default_camera.car == this)
		{
			default_camera.pos[0] += (x-oldx);
			default_camera.pos[1] += (y-oldy);
			default_camera.pos[2] += (z-oldz);
		}
	//}
}

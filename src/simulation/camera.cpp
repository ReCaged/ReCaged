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

#include <math.h>

#include "camera.hpp"
#include "common/internal.hpp"
#include "common/threads.hpp"
#include "assets/track.hpp"

//for creation:
Camera default_camera; //single instance for now (one camera)

//just make sure all variables are initialized to safe defaults
Camera::Camera()
{
	settings = camera_settings_default;
	car = NULL;
	hide = NULL;

	//right, dir, up
	rotation[0]=0; rotation[1]=1; rotation[2]=0; 
	rotation[3]=1; rotation[4]=1; rotation[5]=0; 
	rotation[6]=0; rotation[6]=1; rotation[7]=1; 

	pos[0]=0;
	pos[1]=0;
	pos[2]=0;

	vel[0]=0;
	vel[1]=0;
	vel[2]=0;

	air_timer = 0;
	offset_scale = 0;
	reverse = false;
	in_air = false;
}

void Camera::Set_Settings (Camera_Settings *set)
{
	settings = *set;

	//if this camera mode doesn't have reverse enabled, make sure camera isn't stuck in reverse
	if (!settings.reverse)
		reverse = false;

	//if this camera mode has "air mode" disabled, make sure camera isn't stuck in "in air" mode from before
	if (!settings.in_air)
	{
		in_air = false;
		offset_scale = 1;
	}

	//if not rendering car
	if (settings.hide_car)
		hide=car;
	else
		hide=NULL;
}

void Camera::Set_Car (Car *c)
{
	car = c;

	//if this was a real car (not NULL), configure settings accordingly, or rest
	if (car)
		Set_Settings(&car->camera.cam[car->camera.selected]);
	else
		settings = camera_settings_default;

	//just make sure car stays hidden if current settings wants that
	if (settings.hide_car)
		hide=car;
	else
		hide=NULL;
}

//for physics:

//length of vector (=|V|)
#define VLength(V) (sqrt( (V)[0]*(V)[0] + (V)[1]*(V)[1] + (V)[2]*(V)[2] ))

//normalization of vector (A=A/|A|)
#define VNormalize(V){ \
	float l=VLength(V); \
	(V)[0]/=l; (V)[1]/=l; (V)[2]/=l;}

//cross product (A=BxC)
#define VCross(A,B,C){ \
	(A)[0]=(B)[1]*(C)[2]-(B)[2]*(C)[1]; \
	(A)[1]=(B)[2]*(C)[0]-(B)[0]*(C)[2]; \
	(A)[2]=(B)[0]*(C)[1]-(B)[1]*(C)[0];}

//dot product (=AxB)
#define VDot(A,B) ( (A)[0]*(B)[0] + (A)[1]*(B)[1] + (A)[2]*(B)[2] )

//subtraction of one vector from another (A=B-C)
#define VSubtract(A,B,C){ \
       (A)[0]=(B)[0]-(C)[0]; \
       (A)[1]=(B)[1]-(C)[1]; \
       (A)[2]=(B)[2]-(C)[2];}

void Camera::Set_Pos(float px, float py, float pz, float tx, float ty, float tz)
{
	//set position directly
	pos[0]=px;
	pos[1]=py;
	pos[2]=pz;

	//direction to look at
	float new_dir[3] = {tx-px, ty-py, tz-pz};

	//no direction (keep original rotation)
	if (new_dir[0] == 0 && new_dir[1] == 0 && new_dir[2] == 0)
		return;

	//ok, find up direction (might be tricky)
	//along z, set up to right...
	float new_right[3], new_up[3];
	if (new_dir[0] == 0 && new_dir[1] == 0)
	{
		new_up[0] = 1; new_up[1] = 0; new_up[2] = 0; 
	}
	else //no, ok to use up = real up
	{
		new_up[0] = 0; new_up[1] = 0; new_up[2] = 1; 
	}

	VCross(new_right, new_dir, new_up); //calculate right
	VCross(new_up, new_right, new_dir); //recalculate proper up

	//normalize:
	VNormalize(new_dir);
	VNormalize(new_right);
	VNormalize(new_up);

	//set matrix to these values
	rotation[0]=new_right[0]; rotation[1]=new_dir[0]; rotation[2]=new_up[0];
	rotation[3]=new_right[1]; rotation[4]=new_dir[1]; rotation[5]=new_up[1];
	rotation[6]=new_right[2]; rotation[7]=new_dir[2]; rotation[8]=new_up[2];
}

void Camera::Move(float x, float y, float z)
{
	if (simulation_thread.runlevel == running)
	{
		settings.distance[0] += x;
		settings.distance[1] += y;
		settings.distance[2] += z;
	}
	else //camera got no settings, or is paused
	{
		//we probably got a car?
		if (car)
		{
			//good, lets look at center of car
			const dReal *p = dBodyGetPosition(car->bodyid);
			Set_Pos((pos[0]+x), (pos[1]+y), (pos[2]+z), p[0], p[1], p[2]);
		}
		//ok, no car... lets just keep old rotation
		else
		{
			pos[0]+=x;
			pos[1]+=y;
			pos[2]+=z;
		}
	}
}


//
//spring physics for calculating acceleration
//


void Camera::Accelerate(dReal step)
{
	//calculate some needed values
	dVector3 result;

	if (reverse && !in_air) //move target and position to opposite side (if not just spinning in air)
		dBodyVectorToWorld(car->bodyid, settings.distance[0]*car->dir, -settings.distance[1], settings.distance[2]*car->dir, result);
	else //normal
		dBodyVectorToWorld(car->bodyid, settings.distance[0]*car->dir, settings.distance[1], settings.distance[2]*car->dir, result);

	float c_pos[3]={(float)result[0], (float)result[1], (float)result[2]};

	//position and velocity of anchor
	dVector3 a_pos;
	dBodyGetRelPointPos (car->bodyid, settings.anchor[0], settings.anchor[1], settings.anchor[2]*car->dir, a_pos);

	//relative pos and vel of camera (from anchor)
	float r_pos[3] = {(float)(pos[0]-a_pos[0]), (float)(pos[1]-a_pos[1]), (float)(pos[2]-a_pos[2])};

	//vector lengths
	float r_pos_l = VLength(r_pos);
	//how far from car we want to stay
	//(TODO: could be computed just once - only when changing camera)
	float c_pos_l = VLength(c_pos);

	//to prevent unstable division by too small numbers...
	if (r_pos_l < 0.000001)
		r_pos_l = 0.000001;

	//and (somewhat ironically) to make sure this becomes exactly 0...
	//contrary to r_pos above there are fail-safes for low c_pos, so this
	//just makes sure no weird stuff like sqrt(0²+0²+0²)>0
	if (c_pos_l < 0.000001)
		c_pos_l = 0.0;

	//unit vectors
	float r_pos_u[3] = {r_pos[0]/r_pos_l, r_pos[1]/r_pos_l, r_pos[2]/r_pos_l};
	float c_pos_u[3] = {c_pos[0]/c_pos_l, c_pos[1]/c_pos_l, c_pos[2]/c_pos_l};

	//"linear spring" between anchor and camera (based on distance)
	float dist = r_pos_l-c_pos_l;

	if (settings.linear_stiffness == 0) //disabled smooth movement, jump directly
	{
		//chanses are we have an anchor distance of 0, then vel=0
		if (c_pos_l == 0)
		{
			//position at wanted
			pos[0]=a_pos[0];
			pos[1]=a_pos[1];
			pos[2]=a_pos[2];

			//velocity 0
			vel[0]=0;
			vel[1]=0;
			vel[2]=0;
		}
		else
		{
			//set position
			pos[0]-=r_pos_u[0]*dist;
			pos[1]-=r_pos_u[1]*dist;
			pos[2]-=r_pos_u[2]*dist;

			//velocity towards/from anchor = 0
			//vel towards anchor
			float dot = (r_pos_u[0]*vel[0] + r_pos_u[1]*vel[1] + r_pos_u[2]*vel[2]);

			//remove vel towards anchor
			vel[0]-=r_pos_u[0]*dot;
			vel[1]-=r_pos_u[1]*dot;
			vel[2]-=r_pos_u[2]*dot;
		}
	}
	else //smooth movement
	{
		//how much acceleration (based on distance from wanted distance)
		float acceleration = step*(settings.linear_stiffness)*dist;

		vel[0]-=r_pos_u[0]*acceleration;
		vel[1]-=r_pos_u[1]*acceleration;
		vel[2]-=r_pos_u[2]*acceleration;
	}

	//perpendicular "angular spring" to move camera behind car
	if (c_pos_l > 0 && !in_air) //actually got distance, and camera not in "air mode"
	{
		//dot between wanted and current rotation
		float dot = (c_pos_u[0]*r_pos_u[0] + c_pos_u[1]*r_pos_u[1] + c_pos_u[2]*r_pos_u[2]);

		if (dot < 1.0) //if we aren't exactly at wanted position (and prevent possibility of acos a number bigger than 1.0)
		{
			//angle
			float angle = acos(dot);

			//how much acceleration
			float accel = step*angle*(settings.angular_stiffness);

			//direction of acceleration (remove part of wanted that's along current pos)
			float dir[3];
			dir[0]=c_pos_u[0]-dot*r_pos_u[0];
			dir[1]=c_pos_u[1]-dot*r_pos_u[1];
			dir[2]=c_pos_u[2]-dot*r_pos_u[2];

			//not unit, get length and modify accel to compensate for not unit
			accel /= VLength(dir);

			vel[0]+=(accel*dir[0]);
			vel[1]+=(accel*dir[1]);
			vel[2]+=(accel*dir[2]);
		}
	}
}

void Camera::Collide(dReal step)
{
	//
	//check for collision, and if so, remove possible movement into collision direction
	//

	if (settings.radius > 0)
	{
		dGeomID geom = dCreateSphere (0, settings.radius);
		dGeomSetPosition(geom, pos[0], pos[1], pos[2]);

		dContactGeom contact[internal.contact_points];
		int count = dCollide ( (dGeomID)(track.space->space_id), geom, internal.contact_points, &contact[0], sizeof(dContactGeom));

		int i;
		float V;
		float A;
		float Amax = cos(settings.angle*M_PI/180.0);

		for (i=0; i<count; ++i)
		{
			//remove movement into colliding object
			//velocity along collision axis
			V = vel[0]*contact[i].normal[0] +
				vel[1]*contact[i].normal[1] +
				vel[2]*contact[i].normal[2];

			A = rotation[1]*contact[i].normal[0] +
				rotation[4]*contact[i].normal[1] +
				rotation[7]*contact[i].normal[2];

			//right direction (not away from collision), in valid angle?
			if (V > 0 && A < Amax)
			{
				//remove direction
				vel[0]-=V*contact[i].normal[0];
				vel[1]-=V*contact[i].normal[1];
				vel[2]-=V*contact[i].normal[2];
			}
		}

		dGeomDestroy (geom);
	}
}

void Camera::Damp(dReal step)
{
	//
	//damping of current velocity
	//

	//if relative damping, convert velocity
	dVector3 a_vel; //anchor velocity
	if (settings.relative_damping)
	{
		//damping (of relative movement)
		dBodyGetRelPointVel (car->bodyid, settings.anchor[0], settings.anchor[1], settings.anchor[2]*car->dir, a_vel);

		//make velocity relative car
		vel[0]-=(float)a_vel[0];
		vel[1]-=(float)a_vel[1];
		vel[2]-=(float)a_vel[2];
	}

	//apply to velocity
	float damping=expf(-settings.damping*step);
	vel[0]*=damping;
	vel[1]*=damping;
	vel[2]*=damping;

	//and back (if relative)
	if (settings.relative_damping)
	{
		//back to world velocity
		vel[0]+=(float)a_vel[0];
		vel[1]+=(float)a_vel[1];
		vel[2]+=(float)a_vel[2];
	}
}

//
//the following is smooth rotation and focusing
//

//rotate vector V towards vector V1, along axis A by angle (angle1 - between V and V1)
//this could use a simple rotation matrix instead, but a trig solution seems cleanest
void VRotate(float *V, float *V1, float *A, float angle)
{
	//remove part of vectors along axis:
	float proj = VDot(V, A); //how much of V (and V1) is along axis
	float Vp[3] = {proj*A[0], proj*A[1], proj*A[2]}; //part of Vector Along axis

	//modify given vector so perpendicular to axis (remove projection on axis)
	V[0]-=Vp[0]; V[1]-=Vp[1]; V[2]-=Vp[2];

	// V
	// ^       V1
	// | angle ^
	// |__   /
	// |  \ /
	// |   /
	// |_ /
	// *-|----------> Vx
	//
	// (need Vx to calculate V1)
	//
	
	//just use cross product to find Vx
	float Vx[3];
	VCross(Vx, V, A);

	//note: since A is unit, perpendicular to V, |Vx|=|V| !)

	//new value on V:
	//(V1 negative proj on Vx: rotation "wrong way")
	if (VDot(Vx, V1) < 0.0)
	{
		V[0] = (V[0]*cos(angle)) - (Vx[0]*sin(angle));
		V[1] = (V[1]*cos(angle)) - (Vx[1]*sin(angle));
		V[2] = (V[2]*cos(angle)) - (Vx[2]*sin(angle));
	}
	else
	{
		V[0] = (V[0]*cos(angle)) + (Vx[0]*sin(angle));
		V[1] = (V[1]*cos(angle)) + (Vx[1]*sin(angle));
		V[2] = (V[2]*cos(angle)) + (Vx[2]*sin(angle));
	}

	//removed part of V along axis before, give it back:
	V[0]+=Vp[0]; V[1]+=Vp[1]; V[2]+=Vp[2];
}

//rotate and focus camera
//could of course use spring physics, but this is simpler and works ok
void Camera::Rotate(dReal step)
{
	//
	//the camera rotation is in form of a 3x3 rotation matrix
	//the wanted (target) rotation is also in a 3x3 rotation matrix
	//
	//on order to move from the current matrix to the wanted (but not directly)
	//the matrix is rotated. the rotation is performed around one single axis
	//which will have to be computed...
	//
	//The simplest way of expressing this is as Mold*x = Mold*x, where Mold
	//and Mnew are the matrices and x is the vector of rotation. Of course
	//x is an eigenvector (with eigenvalue=1), but a simpler way of
	//deriving it is to use a geometric perspective: each column in the
	//matrices are vectors for their coordinate systems, and the difference
	//between the new and old vector must be a vector perpendicular to the
	//axis of rotation. Thus one can simply use the cross product from two
	//of these vectors! Since we got 3 vectors (and only need 2) just pick
	//the two best (giving highest calculation precision).
	//

	//---
	//first: needed values
	//---

	//while working with new camera values, store old here
	float c_right[3] = {rotation[0], rotation[3], rotation[6]};
	float c_dir[3] = {rotation[1], rotation[4], rotation[7]};
	float c_up[3] = {rotation[2], rotation[5], rotation[8]};


	//calculate wanted
	float t_right[3];
	float t_dir[3];
	float t_up[3];

	dVector3 result;
	if (reverse && !in_air) //move target and position to opposite side (if not just spinning in air)
		dBodyGetRelPointPos (car->bodyid, settings.target[0]*car->dir, -settings.target[1], settings.target[2]*car->dir, result);
	else //normal
	{
		dBodyGetRelPointPos (car->bodyid,
				settings.target[0]*offset_scale*car->dir,
				settings.target[1]*offset_scale,
				settings.target[2]*car->dir*offset_scale, result);
	}

	t_dir[0]=result[0]-pos[0];
	t_dir[1]=result[1]-pos[1];
	t_dir[2]=result[2]-pos[2];


	if (in_air) //if in air, use absolute up instead
	{
		float norm=sqrt(track.gravity[0]*track.gravity[0] +
				track.gravity[1]*track.gravity[1] +
				track.gravity[2]*track.gravity[2]);
		int i;

		if (norm > 0)
			for (i=0; i<3; ++i)
				t_up[i]=-track.gravity[i]/norm;
		else //zero gravity... lets just assume current "up" is fine...
			for (i=0; i<3; ++i)
				t_up[i]=c_up[i];
	}
	else //use car up
	{
		const dReal *rotation = dBodyGetRotation (car->bodyid);
		t_up[0] = rotation[2]*car->dir;
		t_up[1] = rotation[6]*car->dir;
		t_up[2] = rotation[10]*car->dir;
	}
	
	//target right from dirXup
	VCross(t_right, t_dir, t_up);

	//modify t_up to be perpendicular to t_dir (and t_right)
	VCross(t_up, t_right, t_dir);

	//will need all vectors to be unit...
	VNormalize(t_dir);
	VNormalize(t_up);
	VNormalize(t_right);



	//no smooth rotation?...
	if (settings.rotation_speed == 0.0)
	{
		rotation[0]=t_right[0]; rotation[1]=t_dir[0]; rotation[2]=t_up[0];
		rotation[3]=t_right[1]; rotation[4]=t_dir[1]; rotation[5]=t_up[1];
		rotation[6]=t_right[2]; rotation[7]=t_dir[2]; rotation[8]=t_up[2];
		return;
	}


	//---
	//find axis to rotate around
	//---
	//
	//NOTE: while one could calculate the axis from any cross product (3 alternatives),
	//lets calculate all 3 and see which one could be most accurate.
	//The same for calculating the maximum rotation around the axis - any 3 can be used,
	//but here we just choose one of the two axis change that were used to calculate the axis
	//(since the one discarded must be the least reliable)
	
	//"difference" between current and wanted rotations
	float d_right[3], d_dir[3], d_up[3];
	VSubtract(d_right, t_right, c_right);
	VSubtract(d_dir, t_dir, c_dir);
	VSubtract(d_up, t_up, c_up);

	//3 alternative rotation axes (are equal)
	float A1[3], A2[3], A3[3];
	VCross(A1, d_right, d_dir);
	VCross(A2, d_dir, d_up);
	VCross(A3, d_up, d_right);

	//compare lengths and choose the one that seems most accurate
	float L1, L2, L3;
	L1 = VLength(A1);
	L2 = VLength(A2);
	L3 = VLength(A3);

	//what we select
	float L, *A, *D;
	//L - length, A - axis, D - change of direction)

	//if 1 is bigger than 2
	if (L1 > L2)
	{
		L = L1;
		A = A1;
		D = d_right;
	}
	else //no, 2 is bigger
	{
		L = L2;
		A = A2;
		D = d_dir;
	}
	if (L3 > L) //wait! - 3 was even bigger
	{
		L = L3;
		A = A3;
		D = d_up;
	}

	//make sure not too small
	//(since too equal current and wanted rotation, or possibly some computation error?)
	if (L < 0.000001) //TODO: good fail-safe?
	{
		rotation[0]=t_right[0]; rotation[1]=t_dir[0]; rotation[2]=t_up[0];
		rotation[3]=t_right[1]; rotation[4]=t_dir[1]; rotation[5]=t_up[1];
		rotation[6]=t_right[2]; rotation[7]=t_dir[2]; rotation[8]=t_up[2];
		return;
	}

	//ok, normalize axis:
	A[0]/=L; A[1]/=L; A[2]/=L;


	//---
	//nice, got an axis, lets start rotation...
	//---

	//max needed rotation angle:
	//since difference in direction is perpendicular to axis of rotation
	//and kinda like the hypotenuse of the c and t directions, Vmax is easily gotten:
	float Vmax = 2.0*asin(VLength(D)/2.0);

	//D can get just a bit above 1.0 (calculation precision errors)
	//if this happens, asin returns NAN.
	if (isnan(Vmax))
		Vmax=M_PI; //180°

	//and how much to rotate!
	float Vspeed = step*settings.rotation_speed*Vmax;

	//check if we can reach target in this step, if so just jump
	if (Vspeed > Vmax)
	{
		rotation[0]=t_right[0]; rotation[1]=t_dir[0]; rotation[2]=t_up[0];
		rotation[3]=t_right[1]; rotation[4]=t_dir[1]; rotation[5]=t_up[1];
		rotation[6]=t_right[2]; rotation[7]=t_dir[2]; rotation[8]=t_up[2];
		return;
	}


	//
	//nope, we will have to rotate the axes...
	//

	//while VRotate got a fairly good accuracy, it got limited calculation precision.
	//so vectors might not be perpendicular and unit after many simulations...
	//thus: only rotate two axes:
	VRotate(c_dir, t_dir, A, Vspeed);
	VRotate(c_up, t_up, A, Vspeed);

	//calculate the third, and recalculate one of the first two:
	VCross(c_right, c_dir, c_up);
	VCross(c_dir, c_up, c_right);

	//and make them unit:
	VNormalize(c_right);
	VNormalize(c_dir);
	VNormalize(c_up);

	//---
	//update values:
	//---

	//just copy the (now modified) values back
	rotation[0] = c_right[0]; rotation[1] = c_dir[0]; rotation[2] = c_up[0];
	rotation[3] = c_right[1]; rotation[4] = c_dir[1]; rotation[5] = c_up[1];
	rotation[6] = c_right[2]; rotation[7] = c_dir[2]; rotation[8] = c_up[2];
}

//collide camera with track, generate acceleration on camera if colliding
void Camera::Physics_Step(dReal step)
{
	if (car)
	{
		//
		//perform movement based on last velocity
		//
		pos[0]+=vel[0]*step;
		pos[1]+=vel[1]*step;
		pos[2]+=vel[2]*step;

		//
		//if camera got a targeted car and proper settings, simulate movment:
		//

		//check if reverse
		if (settings.reverse) //enabled
		{
			if (car->dir*car->throttle > 0.0 && car->velocity > 0.0) //wanting and going forward
				reverse = false;
			else if (car->dir*car->throttle < 0.0 && car->velocity < 0.0) //wanting and going backwards
				reverse = true;
		}

		//and if in air
		if (settings.in_air) //in air enabled
		{
			if (!(car->sensor1->colliding) && !(car->sensor2->colliding)) //in air
			{
				if (in_air) //in ground mode
				{
					//smooth transition between offset and center (and needed)
					if (settings.offset_scale_speed != 0 && offset_scale > 0)
						offset_scale -= (settings.offset_scale_speed*step);
					else //jump directly
						offset_scale = 0;
				}
				if (!in_air) //camera not in "air mode"
				{
					if (air_timer > settings.air_time)
					{
						in_air = true; //go to air mode
						air_timer = 0; //reset timer
					}
					else
						air_timer += step;
				}
			}
			else //not in air
			{
				if (in_air) //camera in "air mode"
				{
					if (air_timer > settings.ground_time)
					{
						in_air = false; //leave air mode
						air_timer = 0; //reset timer
					}
					else
						air_timer += step;
				}
				else //camera in "ground mode"
				{
					//smooth transition between center and offset (and needed)
					if (settings.offset_scale_speed != 0 && offset_scale < 1)
						offset_scale += (settings.offset_scale_speed*step);
					else //jump directly
						offset_scale = 1;
				}
			}
		}

		//
		//perform calculation of next step velocity
		//
		Accelerate(step);
		Damp(step);
		Collide(step);

		//rotate camera (focus and rotation)
		//Accelerate might change pos, so this is done after it
		Rotate(step);
	}
}


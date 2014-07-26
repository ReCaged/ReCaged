/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014 Mats Wahlberg
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


#include "wheel.hpp"
#include "../shared/geom.hpp"
#include "../shared/internal.hpp" 
#include "../shared/track.hpp"
#include "../simulation/collision_feedback.hpp"

//This code tries to implement a reasonably simple and realistic tyre friction model.
//(it's mostly inspired by different equations based on Pacejka's "magic formula")
//It also determines if the tyre or rim of the wheel is colliding.
//I'm probably not using all correct variable names, and I might have made some typo
//somewhere. There are also a lot of features that could/should be implemented:
//
//	* force feedback - can be calculated here, but sdl lack force feedback right now
//				(supports gamepads, but not ff specific)
//
//	* the improved wheel friction might make it necessary to improve the suspension
//		(toe, caster, camber, etc...)
//
//	* todo...............
//

//useful vector calculations (from physics/camera.cpp):
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

//copy one vector to another (A=B)
#define VCopy(A,B){ \
	(A)[0]=(B)[0]; \
	(A)[1]=(B)[1]; \
	(A)[2]=(B)[2];}

//addition of one vector to another (A=B+C)
#define VAdd(A,B,C){ \
	(A)[0]=(B)[0]+(C)[0]; \
	(A)[1]=(B)[1]+(C)[1]; \
	(A)[2]=(B)[2]+(C)[2];}

//subtraction of one vector from another (A=B-C)
#define VSubtract(A,B,C){ \
	(A)[0]=(B)[0]-(C)[0]; \
	(A)[1]=(B)[1]-(C)[1]; \
	(A)[2]=(B)[2]-(C)[2];}

//multiplication with scalar (A=B*C)
#define VMultiply(A,B,C){ \
	(A)[0]=(B)[0]*(C); \
	(A)[1]=(B)[1]*(C); \
	(A)[2]=(B)[2]*(C);}

//just some default, crappy, values (to ensure safe simulations)
Wheel::Wheel()
{
	x_static_mu = 0.0;
	x_peak_pos = 0.0;
	x_peak_mu = 0.0;
	x_tail_pos = 0.0;
	x_tail_mu = 0.0;

	y_static_mu = 0.0;
	y_peak_pos = 0.0;
	y_peak_mu = 0.0;
	y_tail_pos = 0.0;
	y_tail_mu = 0.0;

	x_alt_denom = false;
	y_alt_denom = false;

	x_min_denom = 0.0;
	y_min_denom = 0.0;

	x_min_combine = 0.0;
	y_min_combine = 0.0;
	x_scale_combine = 1.0;
	y_scale_combine = 1.0;

	rim_dot = 1.0;
	rollres = 0.0;
	mix_dot = 1.0;
	alt_load = true;
	alt_load_damp = true;

	inertia = 1.0;
}

//find similar, close contact points and merge them
//assumes same geom order
void Wheel::Mix_Contacts(dContact contact[], int count, dReal wheelaxle[], dReal wheeldivide[])
{
	int i,j;

	for (i=0; i<count; ++i)
		wheeldivide[i]=1.0;

	bool rim[count];
	for (i=0; i<count; ++i)
	{
		if (fabs(VDot(contact[i].geom.normal, wheelaxle)) > rim_dot)
			rim[i]=true;
		else
			rim[i]=false;
	}

	for (i=0; i<count; ++i)
	{
		//ignore rims
		if (rim[i])
			continue;

		//check matches
		for (j=i+1; j<count; ++j)
		{
			//normals are similar enough
			if (!rim[j] && (VDot(contact[j].geom.normal, contact[i].geom.normal) > mix_dot))
			{
				wheeldivide[i]+=1.0;
				wheeldivide[j]+=1.0;
			}
		}
	}
}

//simulation of wheel
void Wheel::Configure_Contacts(	dBodyID wbody, dBodyID obody, Geom *g1, Geom *g2,
				dReal wheelaxle[], Surface *surface, dContact *contact,
				dReal stepsize)
{
	//(copy the position (c++ doesn't allow vector assignment, right now...)
	dReal pos[3]; //contact point position
	pos[0] = contact->geom.pos[0];
	pos[1] = contact->geom.pos[1];
	pos[2] = contact->geom.pos[2];

	//the "not-so-magic formula":

	//
	//1) input values:
	//


	//directions:
	//(X:longitudinal/wheel travel, Y:lateral/sideway, Z:normal/"up"):
	dReal X[3], Y[3], Z[3];

	//Z: vertical to ground/normal direction
	Z[0] = contact->geom.normal[0];
	Z[1] = contact->geom.normal[1];
	Z[2] = contact->geom.normal[2];


	//X: along wheel direction (perpendicular to Y and Z)
	//note: will be tangental, so ok.
	VCross (X, Z, wheelaxle);
	VNormalize(X);

	//Y is not correct (not tangental to ground), but X is, and Z is also ok.
	//Y can be recalculated from X and Z, but first, the "incorrect" Y is useful:

	//rim (outside range for tyre)
	//(rim mu calculated as the already defaults)
	if (fabs(VDot (Z, wheelaxle)) > rim_dot) //angle angle between normal and wheel axis
		return; //nothing more to do, rim mu already calculated

	//
	//ok, we can now calculate the correct Y!
	VCross(Y, X, Z);
	//note: no need to normalize, since both X and Y are perpendicular and unit.
	//


	//slip ratio and slip angle:
	dReal slip_ratio, slip_angle;

	//first, get interesting velocities:
	//(velocity of wheel, point on wheel, relative point on wheel (and of point on ground))
	dVector3 Vwheel, Vpoint, Vrpoint, Vground;

	//velocity of wheel
	const dReal *Vtmp = dBodyGetLinearVel (wbody);
	//copy values to out own variables
	Vwheel[0] = Vtmp[0];
	Vwheel[1] = Vtmp[1];
	Vwheel[2] = Vtmp[2];

	//velocity of point on wheel
	dBodyGetPointVel(wbody, pos[0], pos[1], pos[2], Vpoint);

	//not assuming we're on a static surface, so check:
	if (obody) //the surface got a body (can move)
	{
		//get groun vel
		dBodyGetPointVel(obody, pos[0], pos[1], pos[2], Vground);

		//remove ground velocity from others
		//(this makes velocities relative to ground)
		VSubtract(Vpoint, Vpoint, Vground);
		VSubtract(Vwheel, Vwheel, Vground);
	}

	//the velocity of point relative to velocity of wheel:
	VSubtract(Vrpoint, Vpoint, Vwheel);


	//now, lets start calculate needed velocities from these values:

	dReal Vr = VDot(X, Vrpoint); //velocity from wheel rotation
	dReal Vx = VDot(X, Vwheel); //velocity of wheel along heading

	//Vsx and Vsy(=Vy) (slip velocity along x and y, but not negative):
	dReal Vsx = fabs(Vx + Vr); //Vr should be opposite sign of Vx, so this is the difference
	//Vsy = Vy = VDot (Y, Vwheel); but lets go overkill and measure point:
	dReal Vsy = fabs(VDot(Y, Vpoint)); //velocity of point of wheel sideways instead

	//make sure not using negative here either
	Vr=fabs(Vr);
	Vx=fabs(Vx);

	//slip_ratio: defined as: slip_x/vel_x
	//an alternative definition: slip_x/max{vel_x, vel_rotation_x}
	dReal denom = x_alt_denom? fmax(Vr, Vx) : Vx;

	//prevent (unstable) division by too low
	if (denom < x_min_denom)
		denom = x_min_denom;

	//and now simply:
	slip_ratio = Vsx/denom;

	//slip_angle: angle (in degrees) between heading and actual direction
	//of movement (along ground). definition: atan(vel_y/vel_x)
	//alternative (custom) definition: atan(vel_y/vel_rotation_x)
	denom = y_alt_denom? Vr : Vx;

	//again, prevents division by small numbers
	if (denom < y_min_denom)
		denom = y_min_denom;

	//as defined (and convert to degrees)
	slip_angle = (180.0/M_PI)* atan(Vsy/denom);

	//manually calculate Fz?
	dReal Fz=0;
	if (alt_load)
	{
		dReal spring=contact->surface.soft_erp/(contact->surface.soft_cfm*stepsize);
		Fz=spring*contact->geom.depth;

		if (alt_load_damp)
		{
			dReal damping=(1.0-contact->surface.soft_erp)/contact->surface.soft_cfm;
			dReal Vz=VDot(Z, Vpoint);
			Fz-=damping*Vz;
		}

		if (Fz < 0.0)
			Fz=0.0;
	}
	//
	//2.1) compute output values (almost):
	//
	//Uses second and third degree polynoms to interpolate points to get a
	//magic-formula-like curve.
	//

	dReal t, x_mu, y_mu;

	//longitudinal
	t=slip_ratio*surface->sensitivity;
	if (t < x_peak_pos)
	{
		t=t/x_peak_pos;
		x_mu=x_static_mu+(x_peak_mu-x_static_mu)*t*(2.0-t);
	}
	else if (t < x_tail_pos)
	{
		t=(t-x_peak_pos)/(x_tail_pos-x_peak_pos);
		x_mu=x_peak_mu+(x_tail_mu-x_peak_mu)*t*t*(3.0-2.0*t);
	}
	else
		x_mu=x_tail_mu;

	//lateral
	t=slip_angle*surface->sensitivity;
	if (t < y_peak_pos)
	{
		t=t/y_peak_pos;
		y_mu=y_static_mu+(y_peak_mu-y_static_mu)*t*(2.0-t);
	}
	else if (t < y_tail_pos)
	{
		t=(t-y_peak_pos)/(y_tail_pos-y_peak_pos);
		y_mu=y_peak_mu+(y_tail_mu-y_peak_mu)*t*t*(3.0-2.0*t);
	}
	else
		y_mu=y_tail_mu;

	//
	//2.2) combined slip/grip (scale Fx and Fy to combine)
	//
	//fdir1 and fdir2 are set so that a different mu can be given for fdir1
	//(along wheel heading) and fdir2 (sideways). Since ODE uses a less
	//realistic "box" (actually pyramid) friction approximation (movement
	//along both direction 1 and 2 results in more friction than movement
	//along only one direction). To solve this, lets apply a "combined
	//slip/grip" model. There are many different solutions to this.... This
	//is just based on an circle/ellipse friction curve and the x&y slips.
	//It's not at all as advanced as "Modified Nicolas-Comstock Model" (as
	//suggested by SAE), or the method suggested in:
	//http://www.control.lth.se/documents/2003/gaf%2B03.pdf
	//But it is reliable and simple and does seem to give a good result.
	//

	//prevent unstable at low velocity (prevents/oscilates static friction)
	denom=(Vsx<x_min_combine)? x_min_combine: Vsx;

	//Vsx/Vs=Vsx/sqrt(Vsx²+Vsy²)=1/sqrt(1+Vsy²/Vsx²), Vsx>x_combine_vel:
	//(x_scale_combine changes how much impact the slip should have)
	x_mu/=sqrt(1.0+x_scale_combine*Vsy*Vsy/(denom*denom));

	//same for y:
	denom=(Vsy<y_min_combine)? y_min_combine: Vsy;
	y_mu/=sqrt(1.0+y_scale_combine*Vsx*Vsx/(denom*denom));

	//
	//2.3) set new friction values
	//

	//enable: separate mu for dir 1&2, specify dir 1
	//(note: dir2 is automatically calculated by ode)
	contact->surface.mode |= dContactMu2 | dContactFDir1;

	//specify X direction
	contact->fdir1[0] = X[0];
	contact->fdir1[1] = X[1];
	contact->fdir1[2] = X[2];

	//specify mu1 and mu2 (scale with driving surface)
	contact->surface.mu = x_mu*surface->mu;
	contact->surface.mu2 = y_mu*surface->mu;

	if (alt_load)
	{
		contact->surface.mode ^= dContactApprox1;
		contact->surface.mu *= Fz;
		contact->surface.mu2 *= Fz;
	}

	//
	//3) rolling resistance (braking torque based on normal force)
	//
	//(rolling speed is ignored, doesn't make much difference)
	//
	dReal torque = rollres*surface->rollres*contact->geom.depth; //braking torque

	//rotation inertia (relative to ground if got body)
	dReal rotation;
	if (obody)
	{
		const dReal *orot = dBodyGetAngularVel(obody);
		const dReal *wrot = dBodyGetAngularVel(wbody);
		rotation =	wheelaxle[0]*(wrot[0]-orot[0])+
				wheelaxle[1]*(wrot[1]-orot[1])+
				wheelaxle[2]*(wrot[2]-orot[2]);
	}
	else //just rotation of wheel
	{
		const dReal *wrot = dBodyGetAngularVel(wbody);
		rotation =	wheelaxle[0]*wrot[0]+
				wheelaxle[1]*wrot[1]+
				wheelaxle[2]*wrot[2];
	}
	dReal needed = -rotation*inertia/stepsize;

	//same (negative/positive?) direction for torque
	if (needed < 0.0)
		torque = -torque;

	//can brake in this step
	if (torque/needed > 1.0) //torque bigger than needed
		torque = needed; //decrease torque to needed

	dBodyAddRelTorque(wbody, 0.0, 0.0, torque);
	//TODO: if the ground has a body, perhaps the torque should affect it too?
	//perhaps add the braking force (at the point of the wheel) to the ground body?
}


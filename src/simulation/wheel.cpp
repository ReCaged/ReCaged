/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015 Mats Wahlberg
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
#include "geom.hpp"
#include "collision_feedback.hpp"
#include "common/internal.hpp" 
#include "common/threads.hpp" 
#include "assets/track.hpp"

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

Wheel *Wheel::head = NULL;
//just some default, crappy, values (to ensure safe simulations)
Wheel::Wheel()
{
	//add to list:
	next = head;
	head = this;
	prev = NULL;

	//default (safe) values
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

	rollrestorque=0.0;
	rollresjoint=dJointCreateAMotor(simulation_thread.world, 0);
	rollreswbody=NULL;
	rollresobody=NULL;
}

//safely remove from list
Wheel::~Wheel()
{
	//element before?
	if (prev)
		prev->next=next;
	else
		head=next;

	//element after?
	if (next)
		next->prev=prev;

	dJointDestroy(rollresjoint);
}

//find similar, close contact points and merge them
//assumes same geom order
void Wheel::Physics_Step()
{
	int i,j, count;
	dJointID joint;
	Geom *g1, *g2;
	dContact *contact;
	for (Wheel *wheel=head; wheel; wheel=wheel->next)
	{
		//create contact points (and modify for similar joints)
		count=wheel->points.size();
		dReal wheeldivide[count];
		for (i=0; i<count; ++i)
			wheeldivide[i]=1.0;

		for (i=0; i<count; ++i)
		{
			//check matches
			for (j=i+1; j<count; ++j)
			{
				//normals are similar enough
				if (VDot(wheel->points[j].contact.geom.normal, wheel->points[i].contact.geom.normal) > wheel->mix_dot)
				{
					wheeldivide[i]+=1.0;
					wheeldivide[j]+=1.0;
				}
			}

			contact = &wheel->points[i].contact;

			//divide spring&damping values by wheeldivide
			//(ironically, multiplying  cfm accomplishes that)
			contact->surface.soft_cfm *= wheeldivide[i];

			//and scale friction
			contact->surface.mu /= wheeldivide[i];
			contact->surface.mu2 /= wheeldivide[i];

			//create
			joint = dJointCreateContact (simulation_thread.world, simulation_thread.contactgroup, contact);
			dJointAttach (joint, wheel->points[i].b1, wheel->points[i].b2);

			//check if reading collision data
			g1 = wheel->points[i].g1;
			g2 = wheel->points[i].g2;
			if (g1->buffer_event || g2->buffer_event || g1->force_to_body || g2->force_to_body)
				new Collision_Feedback(joint, g1, g2);
		}

		//remove
		wheel->points.clear();


		//create rolling resistance (using amotor joint)
		if (wheel->rollrestorque > 0.0)
		{
			//attach to (possibly new) bodies
			dJointAttach(wheel->rollresjoint, wheel->rollreswbody, wheel->rollresobody);
			//enable (one axis)
			dJointSetAMotorNumAxes(wheel->rollresjoint, 1);
			//set axis direction, relative to wheel (since updates
			//every step, doesn't matter that much)
			dJointSetAMotorAxis(	wheel->rollresjoint, 0, 1,
						wheel->rollresaxis[0],
						wheel->rollresaxis[1],
						wheel->rollresaxis[2]);
			//and set wanted (max) resistance torque
			dJointSetAMotorParam(wheel->rollresjoint, dParamFMax, wheel->rollrestorque);

			//clear until next time
			wheel->rollrestorque=0.0;
		}
		else //disable
			dJointSetAMotorNumAxes(wheel->rollresjoint, 0); //0=disabled
	}
}

//simulation of wheel
void Wheel::Add_Contact(	dBodyID b1, dBodyID b2, Geom *g1, Geom *g2,
				bool wheelis1, dReal wheelaxle[], Surface *surface,
				dContact *contact, dReal stepsize)
{
	//(copy the position (c++ doesn't allow vector assignment, right now...)
	dReal pos[3]; //contact point position
	pos[0] = contact->geom.pos[0];
	pos[1] = contact->geom.pos[1];
	pos[2] = contact->geom.pos[2];

	dBodyID wbody= wheelis1? b1: b2;
	dBodyID obody= wheelis1? b2: b1;

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
	{
		dJointID c = dJointCreateContact (simulation_thread.world,simulation_thread.contactgroup,contact);
		dJointAttach (c,b1,b2);

		if (g1->buffer_event || g2->buffer_event || g1->force_to_body || g2->force_to_body)
			new Collision_Feedback(c, g1, g2);

		return; //nothing more to do, rim mu already calculated
	}

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

	//store (applied later)
	pointstore pstore={*contact, b1, b2, g1, g2};
	points.push_back(pstore);

	//
	//4) rolling resistance (surface/wheel braking torque)
	//
	//(rolling speed and compression is ignored
	//

	//wheel rolling resistance (scaled by surface)
	dReal res = rollres*surface->rollres;

	//if more than current detected rolling resistance
	if (res > rollrestorque)
	{
		//store torque, axle direction and bodies
		rollrestorque=res;
		rollresaxis[0]=wheelaxle[0];
		rollresaxis[1]=wheelaxle[1];
		rollresaxis[2]=wheelaxle[2];
		rollreswbody=wbody;
		rollresobody=obody;
	}
}


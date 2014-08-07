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

#include "../shared/car.hpp"
#include "../shared/track.hpp"
#include "../shared/internal.hpp"

void Car::Physics_Step(dReal step)
{
	int i;
	Car *carp = head;
	bool ground; //on ground (not in air)
	while (carp != NULL)
	{
		//both sensors are triggered, not flipping, only downforce
		if (carp->sensor1->colliding && carp->sensor2->colliding)
			ground = true;
		//only one sensor, flipping+downforce
		else if (carp->sensor1->colliding)
		{
			ground = true;
			carp->dir = 1.0;
		}
		//same
		else if (carp->sensor2->colliding)
		{
			ground = true;
			carp->dir = -1.0;
		}
		//no sensor active, no flipping, no downforce
		else
			ground = false;

		//rotation speed of wheels
		dReal rotv[4] = {0.0, 0.0, 0.0, 0.0};
		for (i=0; i<4; ++i)
			if (carp->gotwheel[i])
				rotv[i] = dJointGetHinge2Angle2Rate (carp->joint[i]);

		//get car velocity:
		//TODO/IMPORTANT: need to find a reliable solution to this...

		//calculated from the average wheel rotation?
		//(wheels will often spin, resulting in too high value...)
		//carp->velocity = carp->dir*carp->wheel->radius*(rotv[0]+rotv[1]+rotv[2]+rotv[3])/4;

		//calculate from absolute velocity of body?
		//(doesn't take account of non-static surfaces (platforms)...)
		const dReal *vel = dBodyGetLinearVel (carp->bodyid);
		const dReal *rot = dBodyGetRotation  (carp->bodyid);
		carp->velocity = (rot[1]*vel[0] + rot[5]*vel[1] + rot[9]*vel[2]);

		//calculate velocity relative to surface in contact with side sensor?
		//(not implemented yet. will need bitfield to mask away non-ground gepms)
		//

		//downforce
		if (carp->down_max > 0)
		{
			if (ground)
			{
				dVector3 relgrav;
				dBodyVectorFromWorld(carp->bodyid, track.gravity[0], track.gravity[1], track.gravity[2], relgrav);
				dReal grav=-relgrav[2]*carp->dir;
				dReal gravforce=grav*(carp->body_mass+4*carp->wheel_mass);

				dReal missing = carp->down_max - gravforce;

				if (missing > 0)
				{
					dReal available = 0;

					if (carp->down_mass > 0 && grav > 0)
						available += gravforce;

					if (carp->down_aero > 0)
					{
						dVector3 relvel;

						dBodyVectorFromWorld(carp->bodyid,
								vel[0]-track.wind[0], vel[1]-track.wind[1], vel[2]-track.wind[2],
								relvel);

						//velocity along x (sideways) and y (forwards), squared, multiplied by density and constant
						available += (relvel[0]*relvel[0]+relvel[1]*relvel[1]) * track.density*carp->down_aero;
					}

					dReal force; //applied downforce
					if (missing < available)
						force=missing;
					else
						force=available;

					//apply force to wheels ("elevate suspension")
					//TODO: could add maximum ammount of force
					//(prevent too weird elevations)
					if (carp->elevation)
					{
						//compensate total force on body (not wheels) by gravity+downforce
						dReal elevate=force+grav*carp->body_mass;
						//remove this force from downforce
						force-=elevate;

						//apply 1/4 of this force to each wheel
						dVector3 wheelforce;
						//const dReal *wheelpos;
						dBodyVectorToWorld(carp->bodyid, 0,0, -carp->dir*elevate/4.0, wheelforce);
						for (i=0; i<4; ++i)
							if (carp->gotwheel[i])
							{
								dBodyAddForce(carp->wheel_body[i], wheelforce[0], wheelforce[1],  wheelforce[2]);
								//wheelpos = dBodyGetPosition(carp->wheel_body[i]);
								//dBodyAddForceAtPos (carp->bodyid,
										//-wheelforce[0], -wheelforce[1], -wheelforce[2],
										//wheelpos[0], wheelpos[1], wheelpos[2]);
							}
					}

					dBodyAddRelForce (carp->bodyid,0,0, -carp->dir*force);
				}
			}
			else if (carp->down_air > 0 && carp->down_mass)
			{
				dReal grav= sqrt(	track.gravity[0]*track.gravity[0]+
							track.gravity[1]*track.gravity[1]+
							track.gravity[2]*track.gravity[2]);
				dReal gravforce=grav*(carp->body_mass+4*carp->wheel_mass);
				dReal missing = carp->down_max - gravforce;

				if (missing > 0)
				{
					dReal available = grav*carp->down_mass;

					if (missing < available)
					{
						dBodyAddForce(	carp->bodyid,
								missing/grav*track.gravity[0],
								missing/grav*track.gravity[1],
								missing/grav*track.gravity[2]);
					}
					else
					{
						dBodyAddForce(	carp->bodyid,
								carp->down_mass*track.gravity[0],
								carp->down_mass*track.gravity[1],
								carp->down_mass*track.gravity[2]);
					}
				}
			}
		}

		//calculate turning:
		dReal A[4];

		//length from turning axis to front and rear wheels
		dReal L1 = (carp->wy*2.0)*carp->dsteer;
		dReal L2 = (carp->wy*2.0)*(carp->dsteer-1.0);

		//turning radius (done by _assuming_ turning only with front wheels - but works for all situations)
		dReal maxsteer = carp->max_steer*(M_PI/180.0); //can not steer more than this
		//steering not allowed to decrease more than this
		dReal minsteer = carp->min_steer*(M_PI/180.0); //don't decrease more than this

		//should not steer more than this (-turning radius = v²*conf- and then calculated into turning angle)
		dReal limitsteer = atan( (carp->wy*2.0) /(carp->steerdecr * fabs(carp->velocity*carp->velocity)) );
		//and add minimum steering to limited steering (vel=inf -> limitsteer=minsteer+0)
		limitsteer += minsteer;

		//if limit is within what the actual turning can handle, use the limited as the max:
		if (limitsteer < maxsteer)
			maxsteer = limitsteer;

		//smooth transition from old max steer to new max steer
		//(if enabled)
		dReal speed = carp->limit_speed*(M_PI/180.0)*step;
		dReal old = carp->oldsteerlimit;
		//if not 0, not infinity, and not enough to move in this step
		if (speed && speed != dInfinity && fabs(maxsteer-old) > speed)
		{
			if (old < maxsteer)
				maxsteer = old+speed;
			else
				maxsteer = old-speed;
		}

		//store max steer
		carp->oldsteerlimit = maxsteer;

		//calculate turning radius
		dReal R = (carp->wy*2.0)/tan(maxsteer*carp->steering);

		//turning radius plus/minus distance to wheel hinge/axe/joint (similar to L1 and L2)
		dReal R1 = R-carp->jx;
		dReal R2 = R+carp->jx;

		if (carp->adapt_steer)
		{
			//turning angle of each wheel:
			A[0] = atan(L1/(R1));
			A[1] = atan(L2/(R1));
			A[2] = atan(L2/(R2));
			A[3] = atan(L1/(R2));
		}
		else //dumb
		{
			//different distribution and direction of fron and rear wheels
			dReal front = maxsteer*carp->dsteer*carp->steering;
			dReal rear = -maxsteer*(1.0-carp->dsteer)*carp->steering;
			A[0] = front;
			A[1] = rear;
			A[2] = rear;
			A[3] = front;
		}

		dReal steer[4] = {
			A[0]-carp->fwtoe,
			A[1]-carp->rwtoe,
			A[2]+carp->rwtoe,
			A[3]+carp->fwtoe};

		//apply steering
		if (carp->turn)
		{
			for (i=0; i<4; ++i)
				if (carp->gotwheel[i])
				{
					dJointSetHinge2Param (carp->joint[i],dParamLoStop,steer[i]);
					dJointSetHinge2Param (carp->joint[i],dParamHiStop,steer[i]);
				}
		}
		//

		//finite rotation
		if (carp->finiterot)
		{
			dVector3 axle;
			for (i=0; i<4; ++i)
				if (carp->gotwheel)
				{
					dBodyVectorToWorld(carp->bodyid, cos(steer[i]), -sin(steer[i]), 0.0, axle);
					dBodySetFiniteRotationAxis(carp->wheel_body[i], axle[0], axle[1], axle[2]);
				}
		}
		//braking/accelerating:

		//the torque we want to apply
		dReal torque[4] = {0,0,0,0};


		//useful values:
		dReal kpower = carp->power*carp->throttle; //motor power
		dReal krbrake = (1.0-carp->dbrake)*carp->max_brake*carp->throttle/2.0; //braking power for rear wheels
		dReal kfbrake = carp->dbrake*carp->max_brake*carp->throttle/2.0; //braking power for front wheels
		dReal kbrake[4] = {kfbrake, krbrake, krbrake, kfbrake}; //brake power for each wheel (to make things easier)

		//check if using the built-in hinge2 motor for handbraking, and release it
		if (carp->hinge2_dbrakes)
		{
			if (carp->gotwheel[1])
				dJointSetHinge2Param (carp->joint[1],dParamFMax2, 0.0);
			if (carp->gotwheel[2])
				dJointSetHinge2Param (carp->joint[2],dParamFMax2, 0.0);
		}

		//no fancy motor/brake solution, lock rear wheels to handbrake turn (oversteer)
		if (carp->drift_brakes)
		{
			if (carp->hinge2_dbrakes) //use super-brake
			{
				//request ode to apply as much force as needed to completely lock wheels
				if (carp->gotwheel[1])
					dJointSetHinge2Param (carp->joint[1],dParamFMax2, dInfinity);
				if (carp->gotwheel[2])
					dJointSetHinge2Param (carp->joint[2],dParamFMax2, dInfinity);
			}
			else
			{
				//apply enough on rear wheels to theoretically "lock" them
				//note: based on moment of inertia by rotation relative to car body...
				//not the most reliable solution but should suffice
			}
		}
		else
		{
			if (carp->throttle) //if driver is throttling (brake/accelerate)
			{
				//motor torque based on gearbox output rotation:
				//check if using good torque calculation or bad one:
				if (carp->diff) //even distribution
				{
					dReal rotation;
					if (carp->fwd&&carp->rwd) //4wd
					{
						rotation = fabs(rotv[0]+rotv[1]+rotv[2]+rotv[3])/4.0;

						//same torque for all wheels
						torque[0]=torque[1]=torque[2]=torque[3]=kpower/4.0;
					}
					else if (carp->rwd) //rwd
					{
						rotation = fabs(rotv[1]+rotv[2])/2.0;

						//(wheel 0 and 3 = 0 -> torque=0)
						torque[1]=torque[2]=kpower/2.0;
					}
					else //fwd
					{
						rotation = fabs(rotv[0]+rotv[3])/2.0;
						torque[0]=torque[3]=kpower/2.0;
					}

					//if less than optimal rotation (for gearbox), set to this level
					if (rotation < carp->gear_limit)
						rotation = carp->gear_limit;

					//apply
					for (i=0; i<4; ++i)
						torque[i]/=rotation;
				}
				else //uneven: one motor+gearbox for each wheel....
				{
					if (carp->fwd&&carp->rwd)
						torque[0]=torque[1]=torque[2]=torque[3]=kpower/4.0;
					else if (carp->rwd)
						torque[1]=torque[2]=kpower/2.0;
					else
						torque[0]=torque[3]=kpower/2.0;

					for (i=0; i<4; ++i)
					{
						if (fabs(rotv[i]) < carp->gear_limit)
							torque[i]/=carp->gear_limit;
						else
							torque[i]/=fabs(rotv[i]);
					}
				}

				//check if wanting to brake
				//dReal needed;
				for (i=0; i<4; ++i)
				{
					//if rotating in the oposite way of wanted, use brakes
					if (rotv[i]*kpower < 0.0) //(different signs makes negative)
					{
						//this much torque (in this direction) is needed to brake wheel
						//not too reliable, since ignores the mass of the car body, and the fact
						//that the brakeing will have to slow down the car movement too...

						//to make transition between brakeing and acceleration smooth
						//use different ways of calculate braking torque:
						//if ( needed/kbrake[i] < 1.0) //more braking than needed
							//torque[i] += needed; //brake as needed + keep possible motor
						//else //not enough brake to stop... full brake
							torque[i] += kbrake[i]; //full brake + possible motor
					}
				}
			}

			//redistribute torque if enabled (nonzero force + enabled somewhere)
			if (carp->redist_force && (carp->fwredist || carp->rwredist) )
			{
				dReal radius[4] = {1,1,1,1}; //default, sane, turning radius for all wheels

				//adaptive redistribution and steering:
				//the wheels are wanted to rotate differently when turning.
				//this done by calculating the turning radius of the wheels.
				//otherwise they'll all be set to the same (1m)
				if (carp->adapt_redist && carp->steering)
				{
					//calculate proper turning radius for all wheels
					radius[0] =  sqrt(R1*R1+L1*L1); //right front
					radius[1] =  sqrt(R1*R1+L2*L2); //right rear
					radius[2] =  sqrt(R2*R2+L2*L2); //left rear
					radius[3] =  sqrt(R2*R2+L1*L1); //left front

					//and since wheels might not turn around their center, remove "offset"
					dReal offset = carp->wx-carp->jx; //difference between joint and wheel x
					if (R > 0.0) //turning right
					{
						radius[0] -= offset;
						radius[1] -= offset;
						radius[2] += offset;
						radius[3] += offset;
					}
					else //left
					{
						radius[0] += offset;
						radius[1] += offset;
						radius[2] -= offset;
						radius[3] -= offset;
					}

					//when reversing, the car will turn reversed, which the player
					//understands. but when reversing and driver turnings while
					//pressing forward, the driver usually expects the turning to
					//be normal, like driving forwards (at least I...).
					if (carp->velocity < 0.0 && carp->throttle*carp->dir > 0.0)
					{
						//ok, so how solve this? swapping the wheels'turning
						//radius (left/right) will reverse the redistribution.
						dReal tmp;
						tmp = radius[0]; radius[0]=radius[3]; radius[3]=tmp;
						tmp = radius[1]; radius[1]=radius[2]; radius[2]=tmp;
					}
				}

				dReal average; //average rotation ("per meters of turning radius")
				if (carp->fwredist && carp->rwredist)
				{
					average = (rotv[0]/radius[0]+rotv[1]/radius[1]
							+rotv[2]/radius[2]+rotv[3]/radius[3])/4.0;

					torque[0] -= (rotv[0]-radius[0]*average)*carp->redist_force;
					torque[1] -= (rotv[1]-radius[1]*average)*carp->redist_force;
					torque[2] -= (rotv[2]-radius[2]*average)*carp->redist_force;
					torque[3] -= (rotv[3]-radius[3]*average)*carp->redist_force;
				}
				else if (carp->rwredist)
				{
					average = (rotv[1]/radius[1]+rotv[2]/radius[2])/2.0;
					torque[1] -= (rotv[1]-radius[1]*average)*carp->redist_force;
					torque[2] -= (rotv[2]-radius[2]*average)*carp->redist_force;
				}
				else
				{
					average = (rotv[0]/radius[0]+rotv[3]/radius[3])/2.0;
					torque[0] -= (rotv[0]-radius[0]*average)*carp->redist_force;
					torque[3] -= (rotv[3]-radius[3]*average)*carp->redist_force;
				}
			}
		}
	
		//if a wheel is in air, lets limit the torques
		dReal airtorque = carp->airtorque;
		for (i=0; i<4; ++i)
		{
			//is in air
			if (!carp->wheel_geom_data[i]->colliding)
			{
				//above limit...
				if (torque[i] > airtorque)
					torque[i] = airtorque;
				else if (torque[i] < -airtorque)
					torque[i] = -airtorque;
			}
		}

		//apply torques
		for (i=0; i<4; ++i)
			if (carp->gotwheel[i])
				dJointAddHinge2Torques (carp->joint[i],0, torque[i]);
		//

		//done, next car...
		carp=carp->next;
	}
}

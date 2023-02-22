/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2015 Mats Wahlberg
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

#include "assets/profile.hpp"

//extract state change for used inputs
//LUA note: will be performed by a lua "module" in future
void Profile_Input_Collect(SDL_Event *event)
{
	for (Profile *prof=profile_head; prof; prof=prof->next)
	{
		switch (event->type)
		{
			//keyboard:
			case (SDL_KEYDOWN):
				for (int i=0; i<9; ++i)
					if (prof->input[i].key!=SDLK_UNKNOWN && event->key.keysym.sym == prof->input[i].key)
						prof->input[i].key_state=true;
				break;
			case (SDL_KEYUP):
				for (int i=0; i<9; ++i)
					if (prof->input[i].key!=SDLK_UNKNOWN && event->key.keysym.sym == prof->input[i].key)
						prof->input[i].key_state=false;
				break;

			//joystick:
			case (SDL_JOYAXISMOTION):
				for (int i=0; i<9; ++i)
					if (prof->input[i].axis!=255 && event->jaxis.axis == prof->input[i].axis)
					{
						//determine positive direciton:
						if (prof->input[i].axis_min < prof->input[i].axis_max) //+
							prof->input[i].axis_state=
								((float)(event->jaxis.value - prof->input[i].axis_min))
								/((float)(prof->input[i].axis_max - prof->input[i].axis_min));
						else //-
							prof->input[i].axis_state=
								((float)(-event->jaxis.value + prof->input[i].axis_min))
								/((float)(prof->input[i].axis_min - prof->input[i].axis_max));

						if (prof->input[i].axis_state < 0.0)
							prof->input[i].axis_state=0.0;
						else if (prof->input[i].axis_state > 1.0)
							prof->input[i].axis_state=1.0;
					}
				break;
			case (SDL_JOYBUTTONDOWN):
				for (int i=0; i<9; ++i)
					if (prof->input[i].button!=255  && event->jbutton.button == prof->input[i].button)
						prof->input[i].button_state=true;
				break;
			case (SDL_JOYBUTTONUP):
				for (int i=0; i<9; ++i)
					if (prof->input[i].button!=255  && event->jbutton.button == prof->input[i].button)
						prof->input[i].button_state=false;
				break;
			case (SDL_JOYHATMOTION):
				for (int i=0; i<9; ++i)
					if (prof->input[i].hat!=255  && event->jhat.hat == prof->input[i].hat)
						prof->input[i].hat_state=(event->jhat.value & prof->input[i].hatpos)? true:false;
				break;

			//otherwise
			default:
				break;
		}
	}
}

//
//act based on inputs
//

//helper function: increases/decreases value of target variable to approach the
//wanted value using speed of change determined by speedbase (minimum speed)
//and speedincrease (speed increases by distance between target and wanted)
void Approach(dReal *target, dReal wanted,
		dReal speedbase, dReal speedincrease,
		dReal steptime)
{
	//how much the actual throttle is of from the wanted
	dReal difference=*target-wanted;

	//and if the difference is positive of negative (0 is no problem)
	bool positive=difference>0.0?true: false;

	//increased speed of change by distance from wanted position
	if (speedincrease > 0.0)
	{
		dReal quotient=speedbase/speedincrease;

		if (positive)
			difference=(difference+quotient)*exp(-speedincrease*steptime)-quotient;
		else
			difference=(difference-quotient)*exp(-speedincrease*steptime)+quotient;
	}
	//constant speed of change...
	else
	{
		if (positive)
			difference-=speedbase*steptime;
		else
			difference+=speedbase*steptime;
	}

	if (positive)
	{
		//there is distance left to push throttle
		if (difference > 0.0)
			*target=wanted+difference;
		else //overshoot, or at zero
			*target=wanted;
	}
	else
	{
		if (difference < 0.0)
			*target=wanted+difference;
		else
			*target=wanted;
	}
}

//helper function 2 (for digital/keyboard input): if input is in other
//direction of current steering/throttle, first use centering speeds to return
//to neutral (0) and (if reached) use normal speeds for one more step.
void Approach_Over_Zero(dReal *target, dReal wanted,
		dReal speedbase, dReal speedincrease,
		dReal centerspeedbase, dReal centerspeedincrease,
		dReal steptime)
{
	//target 
	if (*target * wanted < 0.0)
	{
		//note: "wanted" instead of 0.0, to get any extra speed from
		//the larger difference (if got distance-based speed)
		Approach(target, wanted,
			centerspeedbase, centerspeedincrease,
			steptime);
		//but it might overshoot...

		//was it enough? (yes, this floating equality test _is_ ok)
		if (*target * wanted >= 0.0)
		{
			//reset to zero (remove any overshoot)
			*target=0.0;

			//and allow second call to Approach() below
		}
		//not enough, need to keep centering at next step
		else
			return; //stop here
	}

	//just the normal steering (if not different direction, or centered above)
	Approach(target, wanted,
		speedbase, speedincrease,
		steptime);
}


//LUA note: will be performed by remote control for specific vehicle-type
void Profile_Input_Step(Uint32 step)
{
	dReal steptime=(dReal) step;
	bool digital[9];

	for (Profile *prof=profile_head; prof; prof=prof->next)
	{
		//combine all digital inputs ("OR" them together):
		for (int i=0; i<9; ++i)
			digital[i] = prof->input[i].key_state||prof->input[i].button_state||prof->input[i].hat_state;

		//set camera settings if got a car(input fallback to analog)
		if (prof->car)
		{
			int camera_select=-1;

			if (digital[5]||prof->input[5].axis_state)
				camera_select=0;
			else if (digital[6]||prof->input[6].axis_state)
				camera_select=1;
			else if (digital[7]||prof->input[7].axis_state)
				camera_select=2;
			else if (digital[8]||prof->input[8].axis_state)
				camera_select=3;

			if (camera_select != -1)
			{
				Camera_Conf *camera=&(prof->car->camera);
				camera->selected=camera_select;
				default_camera.Set_Settings (&camera->cam[camera->selected]);
			}
		}


		//if selected car, read input
		if (prof->car)
		{
			Car *carp = prof->car;

			//drift brakes: try digital, fallback to analog
			if (digital[4]||prof->input[4].axis_state)
				carp->drift_brakes = true;
			else
				carp->drift_brakes = false;

			//check for explicit digital throttle, fallback to analog
			//if all zeroed, keep centering with last input (default digital)
			if (digital[0] && !digital[1]) //digital forward
			{
				Approach_Over_Zero(&carp->throttle, 1.0*carp->dir,
						prof->digital_throttle_speed[0], prof->digital_throttle_speed[1],
						prof->digital_throttle_center_speed[0], prof->digital_throttle_center_speed[1],
						steptime);

				//for later
				prof->analog_throttle=false;
			}
			else if (!digital[0] && digital[1]) //digital left
			{
				Approach_Over_Zero(&carp->throttle, -1.0*carp->dir,
						prof->digital_throttle_speed[0], prof->digital_throttle_speed[1],
						prof->digital_throttle_center_speed[0], prof->digital_throttle_center_speed[1],
						steptime);

				//store for later
				prof->analog_throttle=false;
			}
			else if (prof->input[0].axis_state || prof->input[1].axis_state) //analog throttle
			{
				//combine both analog inputs into wanted
				Approach(&carp->throttle, (prof->input[0].axis_state - prof->input[1].axis_state)*carp->dir,
						prof->analog_throttle_speed, 0.0, steptime);

				prof->analog_throttle=true;
			}
			else //digital/analog neutral
			{
				//check last input method, center using it:
				if (prof->analog_throttle)
					Approach(&carp->throttle, 0.0,
						prof->analog_throttle_speed, 0.0, steptime);
				else
					Approach(&carp->throttle, 0.0,
						prof->digital_throttle_center_speed[0], prof->digital_throttle_center_speed[1],
						steptime);
			}

			//steering, same as throttle...
			if (digital[2] && !digital[3])
			{
				Approach_Over_Zero(&carp->steering, 1.0*carp->dir,
						prof->digital_steer_speed[0], prof->digital_steer_speed[1],
						prof->digital_steer_center_speed[0], prof->digital_steer_center_speed[1],
						steptime);

				prof->analog_steer=false;
			}
			else if (!digital[2] && digital[3])
			{
				Approach_Over_Zero(&carp->steering, -1.0*carp->dir,
						prof->digital_steer_speed[0], prof->digital_steer_speed[1],
						prof->digital_steer_center_speed[0], prof->digital_steer_center_speed[1],
						steptime);

				prof->analog_steer=false;
			}
			else if (prof->input[2].axis_state || prof->input[3].axis_state)
			{
				Approach(&carp->steering, (prof->input[2].axis_state - prof->input[3].axis_state)*carp->dir,
						prof->analog_steer_speed, 0.0, steptime);

				prof->analog_steer=true;
			}
			else
			{
				if (prof->analog_steer)
					Approach(&carp->steering, 0.0,
						prof->analog_steer_speed, 0.0, steptime);
				else
					Approach(&carp->steering, 0.0,
						prof->digital_steer_center_speed[0], prof->digital_steer_center_speed[1],
						steptime);
			}
		}
	}
}

/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Mats Wahlberg
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

#include "shared/profile.hpp"

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

//act based on inputs
//LUA note: will be performed by remote control for specific vehicle-type
void Profile_Input_Step(Uint32 step)
{
	bool digital[9];
	dReal wanted, change_speed; //wanted "analog"+speed of change
	int i;

	for (Profile *prof=profile_head; prof; prof=prof->next)
	{
		//combine all digital inputs ("OR" them together):
		for (i=0; i<9; ++i)
			digital[i] = prof->input[i].key_state||prof->input[i].button_state||prof->input[i].hat_state;

		//set camera settings (fallback to analog)
		if (digital[5]||prof->input[5].axis_state)
			camera.Set_Settings (&prof->cam[0]);
		else if (digital[6]||prof->input[6].axis_state)
			camera.Set_Settings (&prof->cam[1]);
		else if (digital[7]||prof->input[7].axis_state)
			camera.Set_Settings (&prof->cam[2]);
		else if (digital[8]||prof->input[8].axis_state)
			camera.Set_Settings (&prof->cam[3]);


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
			//if all zeroed, keep centering with last speed
			if (digital[0] && !digital[1])
			{
				wanted = 1.0*carp->dir;
				prof->last_throttle_speed = prof->digital_throttle_speed;
			}
			else if (!digital[0] && digital[1])
			{
				wanted = -1.0*carp->dir;
				prof->last_throttle_speed = prof->digital_throttle_speed;
			}
			else if (prof->input[0].axis_state || prof->input[1].axis_state)
			{
				//combine both analog inputs into wanted
				wanted = (prof->input[0].axis_state - prof->input[1].axis_state)*carp->dir;
				prof->last_throttle_speed = prof->analog_throttle_speed;
			}
			else
				wanted = 0.0;

			change_speed = prof->last_throttle_speed*(dReal)step;

			if (carp->throttle > wanted)
			{
				if ((carp->throttle-=change_speed) <wanted)
					carp->throttle=wanted;
			}
			else if (carp->throttle < wanted)
			{
				if ((carp->throttle+=change_speed) >wanted)
					carp->throttle=wanted;
			}


			//steering, same as throttle...
			if (digital[2] && !digital[3])
			{
				wanted = 1.0*carp->dir;
				prof->last_steer_speed = prof->digital_steer_speed;
			}
			else if (!digital[2] && digital[3])
			{
				wanted = -1.0*carp->dir;
				prof->last_steer_speed = prof->digital_steer_speed;
			}
			else if (prof->input[2].axis_state || prof->input[3].axis_state)
			{
				//combine both analog inputs into wanted
				wanted = (prof->input[2].axis_state - prof->input[3].axis_state)*carp->dir;
				prof->last_steer_speed = prof->analog_steer_speed;
			}
			else
				wanted = 0.0;

			change_speed = prof->last_steer_speed*(dReal)step;

			if (carp->steering > wanted)
			{
				if ((carp->steering-=change_speed) <wanted)
					carp->steering=wanted;
			}
			else if (carp->steering < wanted)
			{
				if ((carp->steering+=change_speed) >wanted)
					carp->steering=wanted;
			}
		}
	}
}

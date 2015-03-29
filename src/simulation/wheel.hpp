/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015 Mats Wahlberg
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

#ifndef _RCX_WHEEL_H
#define _RCX_WHEEL_H

#include <ode/ode.h>
#include "simulation/geom.hpp"

//wheel friction simulation class (created by Car_Module, used by Physics/Geom.cpp)
class Wheel
{
	public:
		void Add_Contact(dBodyID b1, dBodyID b2, Geom *g1, Geom *g2,
				bool wheelis1, dReal wheelaxle[],
				class Surface *surface, dContact *contact,
				dReal stepsize);

		static void Physics_Step();

		//used primarily by car, but can be used independently
		Wheel();
		~Wheel();

		//friction data:
		dReal x_static_mu;
		dReal x_peak_pos, x_peak_mu;
		dReal x_tail_pos, x_tail_mu;

		dReal y_static_mu;
		dReal y_peak_pos, y_peak_mu;
		dReal y_tail_pos, y_tail_mu;

		bool x_alt_denom, y_alt_denom;
		dReal x_min_denom, y_min_denom;

		dReal x_min_combine, y_min_combine;
		dReal x_scale_combine, y_scale_combine;

		//extra tyre data:
		dReal rim_dot;
		dReal rollres;
		dReal mix_dot;
		bool alt_load, alt_load_damp;

	private:
		//keep track of wheels (double link)
		static Wheel *head;
		Wheel *prev;
		Wheel *next;

		//values created by simulation
		struct pointstore {
			dContact contact;
			dBodyID b1, b2;
			Geom *g1, *g2;
		};

		std::vector<pointstore> points;

		//rolling resistance
		dJointID rollresjoint;
		dReal rollresaxis[3];
		dReal rollrestorque;
		dBodyID rollreswbody;
		dBodyID rollresobody;
};

#endif

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

#ifndef _RC_WHEEL_H
#define _RC_WHEEL_H

#include <ode/ode.h>
#include "../shared/surface.hpp"
#include "../shared/geom.hpp"

//wheel friction simulation class (created by Car_Template, used by Physics/Geom.cpp)
class Wheel
{
	public:
		int Merge_Doubles(dContact *contact, dReal wheelaxle[], int oldcount);

		void Configure_Contacts(dBodyID wbody, dBodyID obody, Geom *g1, Geom *g2,
					dReal wheelaxle[], Surface *surface, dContact *contact,
					dReal stepsize);

	private:
		//not allowing creation and modifying of class unless by friend
		Wheel();

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
		dReal merge_dot;

		//tmp:
		dReal inertia;

		//only car and car template (wheen loading) is allowed
		friend class Car;
		friend class Car_Template;
};

#endif

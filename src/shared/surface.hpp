/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2014 Mats Wahlberg
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

#ifndef _RCX_SURFACE_H
#define _RCX_SURFACE_H
#include "ode/ode.h"
//Geom: (meta)data for geometrical shape (for collision detection), for: 
//contactpoint generation (friction and saftness/hardness). also contains
//rendering data for geom

//surface properties:
class Surface
{
	public:
		Surface(); //just sets default values

		//options
		dReal mu, bounce;
		dReal spring, damping;
		dReal sensitivity, rollres;
};

#endif

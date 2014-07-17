/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2014 Mats Wahlberg
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

#include "surface.hpp"

//set defaults:
Surface::Surface()
{
	//collision contactpoint data
	mu = 0.0;
	spring = dInfinity; //infinite spring constant (disabled)
	damping = dInfinity; //ignore damping from this surface (only used with spring)
	bounce = 0.0; //no bouncyness

	//friction scaling for tyre
	sensitivity = 1.0;
	rollres = 1.0;
}


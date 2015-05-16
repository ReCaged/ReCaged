/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
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

#ifndef _RCX_SPACE_H
#define _RCX_SPACE_H
#include "common/component.hpp"
#include "common/object.hpp"

//Space: (meta)data for spaces (collects geoms that should not collide)
//(in contrary to other components, this can't be rendered or cause events)
//
//Dynamic allocation
class Space: public Component
{
	public:
		//methods
		Space (Object *obj);
		~Space();

		//variables
		dSpaceID space_id;

	private:
		//no need for global list of spaces...
};

#endif

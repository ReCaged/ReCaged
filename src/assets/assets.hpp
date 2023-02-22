/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2015 Mats Wahlberg
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

#ifndef _ReCaged_ASSETS_H
#define _ReCaged_ASSETS_H

#include <typeinfo>
#include <string.h>
#include "common/log.hpp"

class Assets
{
	public:
		static void Clear_TMP(); //Temporary solution until completely automated reference/GC

		//find data that matches specified name and type
		//NOTE: actual function template declared in header, since each object that uses it must
		//instantiate needed function (this follows the "Borland model", which is supported by g++)
		template<typename T>
		static T *Find(const char *name)
		{
			Assets *tmp;
			T *casted;

			for (tmp=head; tmp; tmp=tmp->next) //loop
			{
				//type conversion+casting ok
				if ((!strcmp(tmp->name, name)) && (casted=dynamic_cast<T*>(tmp)))
				{
					Log_Add(2, "asset data already existed for \"%s\" (already loaded)", name);
					return casted;
				}
			}

			return NULL; //else
		}

	protected:
		Assets(const char *name);
		//just make sure the subclass destructor gets called
		virtual ~Assets();

	private:
		char *name; //name of specific data

		static Assets *head;
		Assets *prev, *next;
};
#endif

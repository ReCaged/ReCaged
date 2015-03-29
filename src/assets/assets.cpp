/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2015 Mats Wahlberg
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

#include "assets.hpp"
#include "common/log.hpp"
#include <stdio.h>
#include <string.h>

Assets *Assets::head = NULL;

Assets::Assets(const char *n)
{
	name = new char[strlen(n)+1];
	strcpy (name, n);

	next = head;
	head = this;
}

Assets::~Assets()
{
	Log_Add(2, "removing asset called \"%s\"", name);
	delete[] name;
}

void Assets::Clear_TMP()
{
	Log_Add(2, "destroying all assets");

	Assets *tmp, *data = head;
	while (data)
	{
		tmp = data;
		data=data->next;

		delete tmp;
	}

	head = NULL;
}


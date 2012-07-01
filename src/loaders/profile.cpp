/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
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

//#include "../shared/shared.hpp"
#include "../shared/profile.hpp"
#include "../shared/log.hpp"
#include "text_file.hpp"
//#include "loaders.hpp"

// required to iterate through an enum in C++
template <class Enum>
Enum & enum_increment(Enum & value, Enum begin, Enum end)
{
	return value = (value == end) ? begin : Enum(value + 1);
}

SDLKey & operator++ (SDLKey & key)
{
	return enum_increment(key, SDLK_FIRST, SDLK_LAST);
}

//translate button name to key number
SDLKey get_key (char *name)
{
	Log_Add(2, "translating key name");
	SDLKey key;

	for (key=SDLK_FIRST; key<SDLK_LAST; ++key)
		if (strcmp(SDL_GetKeyName(key), name) == 0)
		{
			Log_Add(2, "name match found");
			return key;
		}

	//we only get here if no match found
	Log_Add(0, "ERROR: Key name %s didn't match any known key!", name);
	return UNUSED_KEY;
}

//load profile (conf and key list)
Profile *Profile_Load (const char *path)
{
	Log_Add(1, "Loading profile: %s", path);

	//create
	Profile *prof = new Profile; //allocate
	prof->next = profile_head;
	prof->prev = NULL;
	profile_head = prof;
	if (prof->next)
		prof->next->prev=prof;

	*prof = profile_defaults; //set all to defaults

	//load personal conf
	char conf[strlen(path)+13+1];//+1 for \0
	strcpy (conf,path);
	strcat (conf,"/profile.conf");

	load_conf(conf, (char *)prof, profile_index); //try to load conf

	//set camera
	if (prof->camera >0 && prof->camera <5)
		camera.Set_Settings (&(prof->cam[prof->camera -1]));
	else
		Log_Add(0, "ERROR: default camera should be a value between 1 and 4!");

	//load key list
	char list[strlen(path)+9+1];
	strcpy (list,path);
	strcat (list,"/keys.lst");

	Log_Add(1, "Loading key list: %s", list);
	Text_File file;

	if (file.Open(list))
	{
		while (file.Read_Line())
		{
			Log_Add(2, "action: %s", file.words[0]);

			//find match
			int i;
			for (i=0; (profile_key_list[i].offset != 0) && (strcmp(profile_key_list[i].name, file.words[0])); ++i);

			if (profile_key_list[i].offset == 0) //we reached end (no found)
				Log_Add(0, "ERROR: no key action match: %s!",file.words[0]);
			else //found
			{
				Log_Add(2, "match found");
				if (file.word_count == 2) //got a key name
					*(SDLKey*)((char*)prof+profile_key_list[i].offset) = get_key(file.words[1]);
				else
					Log_Add(0, "ERROR: no key specified for action \"%s\"", file.words[i]);
			}
		}
	}

	return prof;
}


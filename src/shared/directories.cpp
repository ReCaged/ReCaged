/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2012 Mats Wahlberg
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

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "directories.hpp"
#include "../shared/log.hpp"

#ifndef _WIN32
#define SEPARATOR '/'
#else
#define SEPARATOR '\\'
#endif

char *Directories::user_conf=NULL, *Directories::user_data=NULL, *Directories::user_cache=NULL;
char *Directories::inst_conf=NULL, *Directories::inst_data=NULL;

bool Directories::Check_Path(char *path, Directories::operation op)
{
	if (!path || path[0]=='\0') //doubt this kind of stupidity, but...
	{
		Log_Add(0, "WARNING: incorrect path for \"Check_Path\"!");
		return false;
	}
	else if (op == READ)
	{
		if (!access(path, R_OK))
			return true;
		else
			return false;
	}
	else if (op == WRITE)
	{
		char test[strlen(path)+1]; strcpy(test, path);
		char tmp;

		//just blindly try to create all missing directories...
		int i=0;
		do
		{
			++i;

			//might still use normal slashes even on w32
			if (path[i] == SEPARATOR || path[i] == '/' || path[i] == '\0')
			{
				tmp=path[i];
				path[i]='\0';

				//create if missing
				if (access(path, F_OK))
				{
#ifndef _WIN32
					if (!mkdir(path, 0700)) //permission as specified by xdg
#else
					if (!mkdir(path)) //w32 lacks file permission mode, Ha!
#endif
						Log_Add(2, "created missing directory \"%s\"", path);
				}

				path[i]=tmp;
			}
		}
		while (path[i]!='\0');

#ifdef _WIN32
		//W_OK not supported on the Antiposix
		tmpfile[strlen(path)+9];
		strcpy(tmpfile, path);
		strcat(tmpfile, "/tmpfile");
		FILE *fp = fopen(tmpfile, "w");
		if (fp) //could open tmpfile for writing!
		{
			fclose(fp);
			unlink(tmpfile); //don't care if works...
			return true;
		}
#else
		if (!access(path, W_OK))
			return true;
#endif
		else
			return false;
	}
	else //TODO: append, etc... probably like WRITE above
	{
		Log_Add(0, "TODO: more modes to check!");
		return false;
	}

}

//concatenate path1+path2, check if ok and copy to target string
bool Directories::Try_Set_Path(char **target, Directories::operation op,
		const char *path1, const char *path2)
{
	//something is wrong
	if (!path1 && !path2)
		return false;

	char path[strlen(path1) +1 + strlen(path2) +1];
	strcpy(path, path1);
	strcat(path, "/");
	strcat(path, path2);

	if (Check_Path(path, op))
	{
		if (*target)
			delete *target;
		*target = new char[strlen(path)+1];
		strcpy(*target, path);

		return true;
	}
	else
		return false;
}

void Directories::Init(	const char *arg0, bool installed_force, bool portable_force,
			const char *installed_override, const char *user_override )
{
	//safe
	user_conf = NULL;
	user_data = NULL;
	user_cache = NULL;
	inst_conf = NULL;
	inst_data = NULL;

	//
	//installed data/conf dirs:
	//
	bool found=true; //tru if found both data+conf, assumed true until otherwise
	if (	installed_override &&
		Try_Set_Path(&inst_data, READ, installed_override, "data") &&
		Try_Set_Path(&inst_conf, READ, installed_override, "config") )
		Log_Add(1, "Alternate path to installed directories specified");
	else
	{
		if (installed_override)
			Log_Add(0, "WARNING: path to installed directories incorrect. Ignoring!");

		int pos; //find from the right
		for (pos=strlen(arg0); (pos>0 && arg0[pos]!=SEPARATOR); --pos);

#ifndef _WIN32
		if (pos!=0)
		{
			char path[pos+1]; //NOTE: skipping the slash
			memcpy(path, arg0, sizeof(char)*pos);
			path[pos]='\0';
#else
			//if w32: try GetModuleFileName, arg0, or "this dir"
			const char *wstr;
			//if (GetModuleFileName(...)) TODO!
			/*else*/ if (pos!=0)
				wstr=arg0; //TODO: add to log?
			else
			{
				pos=1; //TODO: add to log?
				wstr=".";
			}

			char path[pos+1];
			memcpy(path, wstr, sizeof(char)*pos);
			path[pos]='\0';
#endif

			if (	(Try_Set_Path(&inst_data, READ, path, "data") &&
				Try_Set_Path(&inst_conf, READ, path, "config")) ||
				(Try_Set_Path(&inst_data, READ, path, "../data") &&
				Try_Set_Path(&inst_conf, READ, path, "../config")) )
			{
				Log_Add(2, "Installed directories located around the executable");
				//TODO: ifdef w32: if (registry.path == path), w32_installed=true;
			}
			else
				found=false;
#ifndef _WIN32
		}
		else
			found=false;
#endif
	}

	//no path, or wasn't useful
	if (!found)
	{
		if (	Try_Set_Path(&inst_data, READ, DATADIR, "") &&
			Try_Set_Path(&inst_conf, READ, CONFDIR, "") )
			Log_Add(2, "Installed directories as specified during build configuration");
		else
		{
			if (!inst_data)
				Log_Add(0, "WARNING: found NO installed data directory!");

			if (!inst_conf)
				Log_Add(0, "WARNING: found NO installed config directory!");
		}
	}

	//TODO: ifdef w32: if (w32_installed): found=false; log: installed, not testing for write access.

	//NOTE: if found==true then both installed data+conf directories exists and aren't
	//in an obvious installed path (as set during build configuration, or w32 installation)

	//
	//user dirs:
	//

	//check if going installed or portable
	const char *var;

	//avoid contradiction...
	if (installed_force && portable_force)
	{
		Log_Add(0, "WARNING: ignoring contradicting installed/portable overrides");
		installed_force=false;
		portable_force=false;
	}

	//must be threated read only?
	if (installed_force)
		Log_Add(1, "Forced to treat installed directories as read-only");
	//no. if found both installation paths, check if writeable (or forced portable)
	else if (	( found && Try_Set_Path(&user_conf, WRITE, inst_conf, "") &&
			Try_Set_Path(&user_data, WRITE, inst_data, "") ) || portable_force)
	{
		if (portable_force)
			Log_Add(1, "Forced to ignore any read-only, installed, directory");

		//don't need these anymore
		delete inst_conf; inst_conf=NULL;
		delete inst_data; inst_data=NULL;

		if (user_conf && user_data) //these should be okay for user access
		{
			Log_Add(2, "Installed data and config directories are writeable, using as user directories");
			//find good path for cache. not checking in user home, since would just confuse
			//try: relative to found conf and data, then /tmp.
			if (	(Try_Set_Path(&user_cache, WRITE, user_conf, "../cache")) ||
				(Try_Set_Path(&user_cache, WRITE, user_data, "../cache")) )
				Log_Add(2, "Found suitable cache directory next to the others");
#ifndef _WIN32
			else if (Try_Set_Path(&user_cache, WRITE, "/tmp/recaged/cache", ""))
				Log_Add(0, "WARNING: found no suitable user cache directory, using \"/tmp\" instead!");
#else
			//w32 got variable for tmp dir (check TMP and TEMP)...
			else if ( (var=getenv("TMP") || var=getenv("TEMP")) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/cache"))
				Log_Add(0, "WARNING: found no suitable user cache directory, using temporary directory instead!");
#endif
			else
				Log_Add(0, "WARNING: found NO user cache directory!");

			return;
		}
		else
			Log_Add(2, "ignoring installed directories and using user directories for all read/write");
	}
	else
		Log_Add(2, "Installed directories are read-only, finding user directories");

	//user config
	if (user_override && Try_Set_Path(&user_conf, WRITE, user_override, "config") )
		Log_Add(1, "Alternate path to user config directory specified");
	else
	{
		if (user_override)
			Log_Add(0, "WARNING: path to user files is incorrect (for config). Ignoring!");

		if ( (var=getenv("XDG_CONFIG_HOME")) && Try_Set_Path(&user_conf, WRITE, var, "recaged") )
			Log_Add(2, "XDG path to user config directory");
		else if ( (var=getenv("HOME")) && Try_Set_Path(&user_conf, WRITE, var, ".config/recaged") )
			Log_Add(2, "Fallback path to user config directory (HOME)");
#ifdef _WIN32
		else if ( (var=getenv("USERPROFILE")) && Try_Set_Path(&user_conf, WRITE, var, "recaged/config") )
			Log_Add(2, "Fallback, \"native\", path to user config directory (USERPROFILE)");
		else if ( (var=getenv("TMP") || var=getenv("TEMP")) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/config"))
			Log_Add(0, "WARNING: found no suitable user config directory, using temporary directory instead!");
#else
		else if ( Try_Set_Path(&user_conf, WRITE, "/tmp/recaged/config", "") )
			Log_Add(0, "WARNING: found no suitable user config directory, using \"/tmp/recaged/config\" instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user config directory!");
	}

	//user data
	if (user_override && Try_Set_Path(&user_data, WRITE, user_override, "data") )
		Log_Add(1, "Alternate path to user data directory specified");
	else
	{
		if (user_override)
			Log_Add(0, "WARNING: path to user files is incorrect (for data). Ignoring!");

		if ( (var=getenv("XDG_DATA_HOME")) && Try_Set_Path(&user_data, WRITE, var, "recaged") )
			Log_Add(2, "XDG path to user data directory");
		else if ( (var=getenv("HOME")) && Try_Set_Path(&user_data, WRITE, var, ".local/share/recaged") )
			Log_Add(2, "Fallback path to user data directory (HOME)");
#ifdef _WIN32
		else if ( (var=getenv("USERPROFILE")) && Try_Set_Path(&user_data, WRITE, var, "recaged/data") )
			Log_Add(2, "Fallback, \"native\", path to user data directory (USERPROFILE)");
		else if ( (var=getenv("TMP") || var=getenv("TEMP")) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/data"))
			Log_Add(0, "WARNING: found no suitable user data directory, using temporary directory instead!");
#else
		else if ( Try_Set_Path(&user_data, WRITE, "/tmp/recaged/data", "") )
			Log_Add(0, "WARNING: found no suitable user data directory, using \"/tmp/recaged/data\" instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user data directory!");
	}

	//user cache
	if (user_override && Try_Set_Path(&user_cache, WRITE, user_override, "cache") )
		Log_Add(1, "Alternate path to user cache directory specified");
	else
	{
		if (user_override)
			Log_Add(0, "WARNING: path to user files is incorrect (for cache). Ignoring!");

		if ( (var=getenv("XDG_CACHE_HOME")) && Try_Set_Path(&user_cache, WRITE, var, "recaged") )
			Log_Add(2, "XDG path to user cache directory");
		else if ( (var=getenv("HOME")) && Try_Set_Path(&user_cache, WRITE, var, ".cache/recaged") )
			Log_Add(2, "Fallback path to user cache directory (HOME)");
#ifdef _WIN32
		else if ( (var=getenv("USERPROFILE")) && Try_Set_Path(&user_cache, WRITE, var, "recaged/cache") )
			Log_Add(2, "Fallback, \"native\", path to user cache directory (USERPROFILE)");
		else if ( (var=getenv("TMP") || var=getenv("TEMP")) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/cache"))
			Log_Add(0, "WARNING: found no suitable user cache directory, using temporary directory instead!");
#else
		else if ( Try_Set_Path(&user_cache, WRITE, "/tmp/recaged/cache", "") )
			Log_Add(0, "WARNING: found no suitable user cache directory, using \"/tmp/recaged/cache\" instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user cache directory!");
	}
}

void Directories::Quit()
{
	delete user_conf; user_conf = NULL;
	delete user_data; user_data = NULL;
	delete user_cache; user_cache = NULL;

	delete inst_conf; inst_conf = NULL;
	delete inst_data; inst_data = NULL;
}


Directories::Directories()
{
	file_path=NULL;
}

Directories::~Directories()
{
	delete file_path;
}

const char *Directories::Find(const char *path,
		Directories::type type, Directories::operation op)
{
	delete file_path; file_path=NULL;

	//TODO!

	//always check user dirs first
//	"dirs"/"file" = path
//	Check_Path(dirs)
//	access...
	//file_path=....

	//if (!file_path)
		//installed

	return NULL;
}

const char *Directories::Path()
{
	return file_path;
}


void Directories::debug()
{
	FILE *f=fopen("tmp.txt", "w");
	fprintf(f, "uconf %s\nudata \%s\nucache %s\niconf %s\nidata %s\n", user_conf, user_data, user_cache, inst_conf, inst_data);
	fclose(f);
}

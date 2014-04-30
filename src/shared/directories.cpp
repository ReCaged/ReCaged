/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2012, 2013 Mats Wahlberg
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//arbitrarily-sized buffer for some w32 functions
#define W32BUFSIZE 400
//note: wont realloc buffer: I've HAD IT with the crap doze api and its bugs!
//(example: GitModuleFileName on xp/2k: doesn't return buffer-size errors, and
//forgets NULL termination of the string it returns...)
#endif

#include "directories.hpp"
#include "../shared/log.hpp"

char *Directories::user_conf=NULL, *Directories::user_data=NULL, *Directories::user_cache=NULL;
char *Directories::inst_conf=NULL, *Directories::inst_data=NULL;

bool Directories::Check_Path(char *path, dir_operation op)
{
	if (!path || path[0]=='\0') //doubt this kind of stupidity, but...
	{
		Log_Add(0, "WARNING: incorrect path for \"Check_Path\"!");
		return false;
	}

	if (op == READ)
	{
		if (!access(path, R_OK))
			return true;
		else
			return false;
	}
	else if (op == WRITE)
	{
		char test[strlen(path)+1]; strcpy(test, path);
		char tmp=0;
		int i=0;

		//just blindly try to create all missing directories...
		do
		{
			++i;

#ifndef _WIN32
			if (path[i] == '/' || path[i] == '\0')
#else
			//w32: also check backslashes
			if (path[i] == '\\' || path[i] == '/' || path[i] == '\0')
#endif
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

				//in case first of several slashes in a row:
				if (path[i] != '\0')
#ifndef _WIN32
					for (; path[i+1]=='/'; ++i);
#else
					for (; path[i+1]=='/' || path[i+1]=='\\'; ++i);
#endif
			}
		}
		while (path[i]!='\0');

#ifdef _WIN32
		//W_OK not supported on the Antiposix
		char tmpfile[strlen(path)+9];
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

	Log_Add(0, "WARNING: request for appendable directory for \"Check_Path\"");
	return false;
}

//concatenate path1+path2, check if ok and copy to target string
bool Directories::Try_Set_Path(char **target, dir_operation op,
		const char *path1, const char *path2)
{
	//something is wrong
	if (!path1 || !path2)
	{
		Log_Add(2, "Discarded incomplete path testing");
		return false;
	}

	char path[strlen(path1) +1 + strlen(path2) +1];
	strcpy(path, path1);
	strcat(path, "/");
	strcat(path, path2);

	if (Check_Path(path, op))
	{
		if (*target) delete *target;
		*target = new char[strlen(path)+1];
		strcpy(*target, path);

		return true;
	}
	else
		return false;
}

//concatenate path1+path2 for file path, and check if file is possible
bool Directories::Try_Set_File(dir_operation op,
				const char *path1, const char *path2)
{
	if (!path1 || !path2)
	{
		Log_Add(2, "Discarded incomplete file testing (read/write)");
		return false;
	}

	char file[strlen(path1) +1 +strlen(path2) +1];
	strcpy(file, path1);
	strcat(file, "/");
	strcat(file, path2);

	//this could require special testing/creation of path to file
	if (op == WRITE)
	{
		//NOTE: path2 is not system-generated, so assume user
		//specified with proper slashes instead of the w32 backslash
		//crap. Also: if path2 got no slashes, the explicitly
		//inserted one above will always provide a fallback

		//(similar to Directories::Init(), find leftmost-rightmost slash)
		int i;
		for (i=strlen(file); (i>0 && file[i]!='/'); --i);
		for (; i>0 && file[i-1]=='/'; --i);

		char path[i+1];
		memcpy(path, file, sizeof(char)*i);
		path[i]='\0';

		if (!Check_Path(path, WRITE))
		{
			Log_Add(0, "Unable to write/create path \"%s\"", path);
			return false;
		}
	}

	//use fopen() instead of access() for write/append check (and everything on Antiposix)
	int amode=0;
	const char *fmode=NULL;

	switch (op)
	{
		case READ:
#ifndef _WIN32
			amode=R_OK;
#else
			fmode="r";
#endif
			break;

		case WRITE:
			fmode="w";
			break;

		case APPEND:
			Log_Add(0, "WARNING: request for appendable directory for \"Check_Path\"");
			//amode, fmode are NULL, no checks will occur
			break;
	}

	bool viable=false;
	if (amode && !access(file, amode))
		viable=true;

	if (fmode) //if fmode, knows that amode=0 for sure
	{
		FILE *fp = fopen(file, fmode);
		if (fp)
		{
			//don't care about removing possibly created file (if "w")
			fclose(fp);
			viable=true;
		}
	}

	if (!viable)
	{
		if (op == WRITE) Log_Add(0, "Warning: unable to open \"%s\" for writing!", file);
		else Log_Add(2, "File \"%s\" did not exist", file);
		return false;
	}

	//else, we're ready to go!
	file_path = new char[strlen(file)+1];
	strcpy(file_path, file);
	return true;
}

//like above, but for appending (requires more quirks)
bool Directories::Try_Set_File_Append(const char *user, const char *inst, const char *path)
{
	if (!(user && path))
	{
		Log_Add(2, "Discarded incomplete file testing (append)");
		return false;
	}

	char upath[strlen(user)+1+strlen(path)+1];
	strcpy(upath, user);
	strcat(upath, "/");
	strcat(upath, path);

	int i;
	for (i=strlen(upath); (i>0 && upath[i]!='/'); --i);
	for (; i>0 && upath[i-1]=='/'; --i);

	if (i > 0)
	{
		char fullpath[i+1];
		memcpy(fullpath, upath, sizeof(char)*i);
		fullpath[i]='\0';

		if (!Check_Path(fullpath, WRITE))
		{
			Log_Add(0, "Unable to write/create path \"%s\"", path);
			return false;
		}
	}

	bool exists=access(upath, F_OK)? false: true;
	FILE *fp=fopen(upath, "ab");
	if (!fp)
	{
		Log_Add(0, "Warning: Unable to open \"%s\" for appending!", upath);
		return false;
	}

	//didn't exists in user dir before+got installed dir
	if (!exists && inst)
	{
		//does exist as installed?
		char ipath[strlen(inst)+1+strlen(path)+1];
		strcpy(ipath, inst);
		strcat(ipath, "/");
		strcat(ipath, path);

		FILE *ifp=fopen(ipath, "rb");
		if (ifp)
		{
			Log_Add(2, "Copying \"%s\" to \"%s\" before appending", ipath, upath);

			//TODO: move/copy to separate method (dedicated file copying)
			//NOTE: fopen() not as fast as open() (which is low-level posix)
			//(but - SURPRISE! - open() seems to cause problems on losedows...)
			char *buf = new char[COPY_BUFFER_SIZE];
			size_t s;
			while ((s = fread(buf, 1, COPY_BUFFER_SIZE, ifp)))
				fwrite(buf, 1, s, fp);

			delete[] buf;
			fclose (ifp);
		}
	}

	fclose(fp);
	file_path = new char[strlen(upath)+1];
	strcpy(file_path, upath);
	return true;
}

//will only fail if user tries to override detection with weird paths (instead of ignoring them)
bool Directories::Init(	const char *arg0, bool installed_force, bool portable_force,
			const char *installed_override, const char *user_override, const char *portable_override)
{
	//safe
	user_conf = NULL;
	user_data = NULL;
	user_cache = NULL;
	inst_conf = NULL;
	inst_data = NULL;

	//avoid contradiction... should never occur anyway...
	if (	(installed_force || installed_override || user_override) &&
		(portable_force || portable_override)	)
	{
		Log_Add(0, "ERROR: contradicting installed/portable overrides");
		return false;
	}

	//
	//installed data/conf dirs:
	//
	bool found=true; //true if found both data+conf, assumed true until otherwise

#ifdef _WIN32
	char *w32buf=NULL;
	char *path=NULL;
#endif

	if (installed_override || portable_override) //no need to search, user provided
	{
		const char *override=NULL; //figure out which one
		if (installed_override) override=installed_override;
		else override=portable_override;

		if (	Try_Set_Path(&inst_data, READ, override, "data") &&
			Try_Set_Path(&inst_conf, READ, override, "config") )
			Log_Add(1, "Alternate path to installed directory specified");
		else
		{
			Log_Add(0, "ERROR: overriding path to installed directory (\"%s\") is incorrect", override);
			return false;
		}
	}
	else //figure out from arg[0], or getmodulefilename (on windows)
	{
		int pos; //find from the right
#ifdef _WIN32
		w32buf = new char[W32BUFSIZE];
		const char *wstr;

		//try w32 api thing first, but arg0 if not working
		if (GetModuleFileNameA(NULL, w32buf, W32BUFSIZE) != 0) //OK
		{
			Log_Add(2, "Path to program retrieved from w32 crap");
			w32buf[W32BUFSIZE-1]='\0'; //in case of insanity (see line 36 above)
			wstr=w32buf;
		}
		else //failure...
		{
			Log_Add(0, "Warning: unable to retrieve path to program (from w32 crap), trying arg0");
			wstr=arg0;
		}

		//note: checks for slashes too, but I don't think this can happen on w32?
		for (pos=strlen(wstr)-1; (pos>=0 && wstr[pos]!='\\' && wstr[pos]!='/'); --pos);
		for (; pos>0 && (wstr[pos-1]=='/' || wstr[pos-1]=='\\'); --pos);
#else

		//just check arg[0] on normal systems:
		for (pos=strlen(arg0)-1; (pos>=0 && arg0[pos]!='/'); --pos); //find from right
		for (; pos>0 && arg0[pos-1]=='/'; --pos); //in case several slashes in a row
#endif

		//pos should now be leftmost slash in rightmost group of slashes, or -1
		if (pos==-1) //-1: passed through all without finding separator
			found=false;
		else //found
		{
#ifdef _WIN32
			path = new char[pos+1];
			memcpy(path, wstr, sizeof(char)*pos);
#else
			char path[pos+1]; //room for s[0],s[1],s[2],...,s[pos-1],'\0'
			memcpy(path, arg0, sizeof(char)*pos);
#endif
			path[pos]='\0';
			if (	(Try_Set_Path(&inst_data, READ, path, "data") &&
				Try_Set_Path(&inst_conf, READ, path, "config")) ||
				(Try_Set_Path(&inst_data, READ, path, "../data") &&
				Try_Set_Path(&inst_conf, READ, path, "../config")) )
				Log_Add(2, "Installed directories located around the executable");
			else
			{
				Log_Add(0, "Found no installed directories around the executable");
				found=false;
			}
		}
	}

#ifdef _WIN32
	//try to get path to installed dir from registry
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ReCaged", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (!w32buf)
			w32buf = new char[W32BUFSIZE];

		unsigned long type;
		unsigned long size=W32BUFSIZE;
		if (RegQueryValueExA(hKey, "Installed", NULL, &type, (LPBYTE) w32buf, &size) == ERROR_SUCCESS)
		{
			if (type == REG_SZ) //should be safe...
			{
				w32buf[W32BUFSIZE-1]='\0'; //just in case registry reading is insane as well...
				//in case reg str ends with one or more (back-)slashes, remove them
				int pos;
				//start at '\0' in end, and go left until last (back)slash
				for (pos=strlen(w32buf); pos>0 && (w32buf[pos-1]=='/' || w32buf[pos-1]=='\\'); --pos);

				//pos will be valid in any case
				w32buf[pos]='\0';
			}
			else //wont be needing this any more...
			{
				delete[] w32buf;
				w32buf=NULL;
			}
		}
		RegCloseKey(hKey); //no function with A suffix here
	}
	else if (w32buf) //wont be needing this any more...
	{
		delete[] w32buf;
		w32buf = NULL;
	}

	if (!w32buf)
		Log_Add(2, "No registry string found for path to installed copy (probably not installed)");

	//found a (possibly writeable) path, and if not forcing portable mode: check if matches registry (if so, don't write there)
	if (found && !portable_force)
	{
		if (w32buf && strcasecmp(w32buf, path) == 0)
		{
			Log_Add(1, "Path to program matches registry string, running in installed mode");
			found = false;
		}
	}
	else //no path, or wasn't useful
#else
	if (!found) //no path, or wasn't useful
#endif
	{
		if (	Try_Set_Path(&inst_data, READ, DATADIR, "") &&
			Try_Set_Path(&inst_conf, READ, CONFDIR, "") )
			Log_Add(2, "Installed directories as specified during build configuration");
#ifdef _WIN32
		else if (w32buf	&&	Try_Set_Path(&inst_data, READ, w32buf, "data") &&
					Try_Set_Path(&inst_conf, READ, w32buf, "config") )
			Log_Add(2, "Installed directories as specified during installation (from registry)");
		else if (	(Try_Set_Path(&inst_data, READ, "data", "") &&
				Try_Set_Path(&inst_conf, READ, "config", "")) ||
				(Try_Set_Path(&inst_data, READ, "../data", "") &&
				Try_Set_Path(&inst_conf, READ, "../config", "")) )
				Log_Add(0, "WARNING: unable to retrieve path to program, but found directories in PWD (assuming installed)");
#endif
		else
		{
			if (!inst_data)
				Log_Add(0, "WARNING: found NO installed data directory!");

			if (!inst_conf)
				Log_Add(0, "WARNING: found NO installed config directory!");
		}
	}

#ifdef _WIN32
	if (w32buf)
		delete[] w32buf;
	if (path)
		delete[] path;
#endif

	//NOTE: if found==true then both installed data+conf directories exists and aren't
	//in an obvious installed path (as set during build configuration, or w32 installation)

	//
	//user dirs:
	//

	//check if going installed or portable
	const char *var;

	//must be treated read only?
	if (installed_force)
		Log_Add(1, "Installed mode forced: treating installed directories as read-only");
	//no. if found both installation paths, check if writeable (or forced portable)
	//else if (w32 && GetReg strcmp not equals installed?, then also:
	else if (	found && Try_Set_Path(&user_conf, WRITE, inst_conf, "") &&
			Try_Set_Path(&user_data, WRITE, inst_data, "") )
	{
		//don't need these anymore
		delete inst_conf; inst_conf=NULL;
		delete inst_data; inst_data=NULL;

		Log_Add(2, "Installed data and config directories are writeable, using as user directories (portable mode)");
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
		else if ( ((var=getenv("TMP")) || (var=getenv("TEMP"))) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/cache"))
			Log_Add(0, "WARNING: found no suitable user cache directory, using temporary directory instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user cache directory!");

		Log_Add(2, "User config directory: \"%s\"", user_conf);
		Log_Add(2, "User data directory: \"%s\"", user_data);
		if (user_cache) Log_Add(2, "User cache directory: \"%s\"", user_cache);

		return true; //great
	}
	//did exist in not obvious read-only location, but no write access: was required for portable mode...
	else if (found && portable_force)
	{
		Log_Add(0, "ERROR: portable mode forced, but no write access to directories around executable");
		return false;
	}
	else
		Log_Add(2, "Installed directories are read-only, finding user directories");

	//user config
	if (user_override)
	{
		if (Try_Set_Path(&user_conf, WRITE, user_override, "config"))
			Log_Add(1, "Alternate path to user directory specified (for config)");
		else
		{
			Log_Add(0, "ERROR: overriding path to user directory (\"%s\") is incorrect (for config)", user_override);
			return false;
		}
	}
	else
	{
		if ( (var=getenv("XDG_CONFIG_HOME")) && Try_Set_Path(&user_conf, WRITE, var, "recaged") )
			Log_Add(2, "XDG path to user config directory");
		else if ( (var=getenv("HOME")) && Try_Set_Path(&user_conf, WRITE, var, ".config/recaged") )
			Log_Add(2, "Fallback path to user config directory (HOME)");
#ifdef _WIN32
		else if ( (var=getenv("USERPROFILE")) && Try_Set_Path(&user_conf, WRITE, var, "recaged/config") )
			Log_Add(2, "Fallback, \"native\", path to user config directory (USERPROFILE)");
		else if ( ((var=getenv("TMP")) || (var=getenv("TEMP"))) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/config"))
			Log_Add(0, "WARNING: found no suitable user config directory, using temporary directory instead!");
#else
		else if ( Try_Set_Path(&user_conf, WRITE, "/tmp/recaged/config", "") )
			Log_Add(0, "WARNING: found no suitable user config directory, using \"/tmp/recaged/config\" instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user config directory!");
	}

	//user data
	if (user_override)
	{
		if (Try_Set_Path(&user_data, WRITE, user_override, "data"))
			Log_Add(1, "Alternate path to user directory specified (for data)");
		else
		{
			Log_Add(0, "ERROR: overriding path to user directory (\"%s\") is incorrect (for data)", user_override);
			return false;
		}
	}
	else
	{
		if ( (var=getenv("XDG_DATA_HOME")) && Try_Set_Path(&user_data, WRITE, var, "recaged") )
			Log_Add(2, "XDG path to user data directory");
		else if ( (var=getenv("HOME")) && Try_Set_Path(&user_data, WRITE, var, ".local/share/recaged") )
			Log_Add(2, "Fallback path to user data directory (HOME)");
#ifdef _WIN32
		else if ( (var=getenv("USERPROFILE")) && Try_Set_Path(&user_data, WRITE, var, "recaged/data") )
			Log_Add(2, "Fallback, \"native\", path to user data directory (USERPROFILE)");
		else if ( ((var=getenv("TMP")) || (var=getenv("TEMP"))) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/data"))
			Log_Add(0, "WARNING: found no suitable user data directory, using temporary directory instead!");
#else
		else if ( Try_Set_Path(&user_data, WRITE, "/tmp/recaged/data", "") )
			Log_Add(0, "WARNING: found no suitable user data directory, using \"/tmp/recaged/data\" instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user data directory!");
	}

	//user cache
	if (user_override)
	{
		if (Try_Set_Path(&user_cache, WRITE, user_override, "cache"))
			Log_Add(1, "Alternate path to user directory specified (for cache)");
		else
		{
			Log_Add(0, "ERROR: overriding path to user directory (\"%s\") is incorrect (for cache)", user_override);
			return false;
		}
	}
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
		else if ( ((var=getenv("TMP")) || (var=getenv("TEMP"))) && Try_Set_Path(&user_cache, WRITE, var, "/recaged/cache"))
			Log_Add(0, "WARNING: found no suitable user cache directory, using temporary directory instead!");
#else
		else if ( Try_Set_Path(&user_cache, WRITE, "/tmp/recaged/cache", "") )
			Log_Add(0, "WARNING: found no suitable user cache directory, using \"/tmp/recaged/cache\" instead!");
#endif
		else
			Log_Add(0, "WARNING: found NO user cache directory!");
	}

	if (inst_conf) Log_Add(2, "Installed config directory: \"%s\"", inst_conf);
	if (inst_data) Log_Add(2, "Installed data directory: \"%s\"", inst_data);
	if (user_conf) Log_Add(2, "User config directory: \"%s\"", user_conf);
	if (user_data) Log_Add(2, "User data directory: \"%s\"", user_data);
	if (user_cache) Log_Add(2, "User cache directory: \"%s\"", user_cache);

	return true; //all good, I think...
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
		dir_type type, dir_operation op)
{
	delete file_path; file_path=NULL;

	const char *user=NULL, *inst=NULL;
	switch (type)
	{
		case CONFIG:
			Log_Add(2, "Locating config file \"%s\":", path);
			user=user_conf;
			inst=inst_conf;
			break;

		case DATA:
			Log_Add(2, "Locating data file \"%s\":", path);
			user=user_data;
			inst=inst_data;
			break;

		case CACHE:
			Log_Add(2, "Locating cache file \"%s\":", path);
			user=user_cache;
			break;
	}

	switch (op)
	{
		case READ: //try user, then installed
			if ( Try_Set_File(op, user, path) )
				Log_Add(2, "Readable in user directory: \"%s\"", file_path);
			else if ( Try_Set_File(op, inst, path) )
				Log_Add(2, "Readable in installation directory: \"%s\"", file_path);
			else
				Log_Add(0, "Unable to find file \"%s\" for reading!", path);
			break;

		case WRITE: //try user
			if ( Try_Set_File(op, user, path) )
				Log_Add(2, "Writeable in user directory: \"%s\"", file_path);
			else
				Log_Add(0, "Unable to resolve/create file \"%s\" for writing!", path);
			break;

		case APPEND: //user, but copy from installed if needed/possible
			if ( Try_Set_File_Append(user, inst, path) )
				Log_Add(2, "Appendable in user directory: \"%s\"", file_path);
			else
				Log_Add(0, "Unable to resolve/create file \"%s\" for appending!", path);
			break;
	}

	return file_path;
}

const char *Directories::Path()
{
	return file_path;
}


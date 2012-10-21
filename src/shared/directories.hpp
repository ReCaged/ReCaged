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

#ifndef _RC_DIRECTORIES_H
#define _RC_DIRECTORIES_H

//
// Find() returns (if possible) a good path to a file (otherwise NULL), given the parameters:
//
// * path: path and file (relative to directory detected as usable)
//
// * type: one of:
// CONFIG: user/global configuration
// DATA: contents (like objects)
// CACHE: temporary files (like logs)
//
// * operation: one of:
// READ: check if file can be read (prefer user-created over installed)
// WRITE: create any missing dirs and check if file can be written (in user dirs)
// APPEND: like WRITE, but preserves existing file if it exists (otherwise copies installed, if exists)
//
// Init() must be run before creating classes, Path() returns latest path (or NULL on error)
//
// TODO: future functions:
// * Remove(): remove file+any empty dirs caused (in user dirs)
// * List(): lists union of files in path from both user+installed dirs
// * Copy(): copy file (prefer user over installed) if possible to new dir
//

#define COPY_BUFFER_SIZE 4096 //should be good (plus covers common block/sectorsizes)
typedef enum {READ, WRITE, APPEND} dir_operation;
typedef enum {CONFIG, DATA, CACHE} dir_type;

class Directories
{
	public:
		static void Init(const char *arg0, bool installed_force, bool portable_force,
				const char *installed_override, const char *user_override);
		static void Quit();

		Directories();
		~Directories();

		const char *Find(const char *path, dir_type type, dir_operation op);
		const char *Path(); //return again

	private:
		static bool Check_Path(char*path, dir_operation op); //NOTE: requires non-const string
		static bool Try_Set_Path(char **target, dir_operation op,
				const char *path1, const char *path2);
		static char *user_conf, *user_data, *user_cache;
		static char *inst_conf, *inst_data;

		bool Try_Set_File(dir_operation op,
				const char *path1, const char *path2);
		bool Try_Set_File_Append(const char *user, const char *inst,
				const char *path);
		char *file_path;
};

#endif

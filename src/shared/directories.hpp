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

class Directories
{
	public:
		static void Init(const char *arg0, bool installed_force, bool portable_force,
				const char *installed_override, const char *user_override);
		static void Quit();

		Directories();
		~Directories();

		static void debug();
		//What it's all about: "Find" returns (if possible) the path to the file
		//depending on if reading/writing (if so also create necessary directories)
		//and what type of file that will be used
		typedef enum {READ, WRITE, TODO} operation; //TODO: APPEND, etc... If needed.
		typedef enum {CONFIG, DATA, CACHE} type;
		const char *Find(const char *path,
				Directories::type type,
				Directories::operation op);
		const char *Path(); //return again
		//TODO: Remove(const char *path, DIrectories::type); //find, rm+removes all empty dirs

	private:
		static bool Check_Path(char*path, Directories::operation op); //NOTE: requires non-const string
		static bool Try_Set_Path(char **target, Directories::operation op,
				const char *path1, const char *path2);
		static char *user_conf, *user_data, *user_cache;
		static char *inst_conf, *inst_data;

		bool Try_Set_File(Directories::operation op,
				const char *path1, const char *path2);
		char *file_path;
};

#endif

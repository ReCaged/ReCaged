/*
 * ReCaged - a Free Software, Futuristic, Racing Game
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

#include "shared/trimesh.hpp"
#include "shared/log.hpp"
#include "shared/directories.hpp"

//
//for trimesh file loading
//

//wrapper for loading
bool Trimesh::Load(const char *file)
{
	Log_Add(2, "Loading trimesh from file \"%s\" (identifying suffix)", file);

	if (file == NULL)
	{
		Log_Add(0, "WARNING: empty file path+name for trimesh");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		Log_Add(-1, "No suffix for 3D file \"%s\"", file);
		return false;
	}

	//set name to filename (without full path)
	name=file;

	//find
	Directories dirs;
	if (!dirs.Find(file, DATA, READ))
		return false;

	//see if match:
	if (!strcasecmp(suffix, ".obj"))
		return Load_OBJ(dirs.Path());
	else if (!strcasecmp(suffix, ".road"))
		return Load_Road(dirs.Path());
	//else if (!strcasecmp(suffix, ".3ds"))
		//return Load_3DS(dirs.Path());
	
	//else, no match
	Log_Add(-1, "Unknown 3D file suffix for \"%s\"", file);
	return false;
}

//for materials
bool Trimesh::Load_Material(const char *file)
{
	Log_Add(2, "Loading material from file \"%s\" (identifying suffix)", file);

	if (file == NULL)
	{
		Log_Add(0, "WARNING: empty file path+name for material");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		Log_Add(-1, "No suffix for material file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".mtl"))
		return Load_MTL(file);

	//else, no match
	Log_Add(-1, "Unknown material file suffix for \"%s\"", file);
	return false;
}

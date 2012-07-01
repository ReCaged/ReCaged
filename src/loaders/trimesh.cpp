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

#include "../shared/trimesh.hpp"
#include "../shared/log.hpp"

//
//for trimesh file loading
//

//wrapper for loading
bool Trimesh::Load(const char *file)
{
	Log_printf(1, "Loading trimesh from file \"%s\"", file);
	Log_printf(2, "determining file type from suffix");

	if (file == NULL)
	{
		Log_printf(0, "WARNING: empty file path+name for trimesh");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		Log_printf(0, "ERROR: no suffix for file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".obj"))
		return Load_OBJ(file);
	else if (!strcasecmp(suffix, ".road"))
		return Load_Road(file);
	//else if (!strcasecmp(suffix, ".3ds"))
		//return Load_3DS(file);
	
	//else, no match
	Log_printf(0, "ERROR: unknown 3D file suffix for \"%s\"", file);
	return false;
}

//for materials
bool Trimesh::Load_Material(const char *file)
{
	Log_printf(1, "Loading material from file \"%s\"", file);
	Log_printf(2, "determining file type from suffix");

	if (file == NULL)
	{
		Log_printf(0, "WARNING: empty file path+name for material");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		Log_printf(0, "ERROR: no suffix for file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".mtl"))
		return Load_MTL(file);

	//else, no match
	Log_printf(0, "ERROR: unknown 3D file suffix for \"%s\"", file);
	return false;
}

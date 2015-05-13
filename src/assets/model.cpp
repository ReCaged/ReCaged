/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015 Mats Wahlberg
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

//"helper"/loader functions, code for creating collision detection/rendering
//models is in the other model_*.cpp files (became too much for a single file!)

#include "model.hpp"
#include "common/log.hpp"
#include "common/directories.hpp"
#include <limits.h>

//length of vector
#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))

//
//for trimesh initialization+helper functions:
//

//default values for material
const Model::Material Model::Material_Default = 
{
	"unknown material",
	{
	{0.2, 0.2, 0.2, 1.0},
	{0.8, 0.8, 0.8, 1.0},
	{0.0, 0.0, 0.0, 1.0},
	{0.0, 0.0, 0.0, 1.0},
	0.0
	}
	//no value for vector of triangles
};


unsigned int Model::Find_Material(const char *name)
{
	size_t end = materials.size();

	for (size_t i=0; i<end; ++i)
	{
		if (materials[i].name == name)
			return i;
	}

	//failure
	Log_Add(-1, "could not find trimesh material %s", name);
	return INDEX_ERROR;
}

bool Model::Compare_Name(const char *n)
{
	if (!name.compare(n))
		return true;
	return false;
}

//makes sure all normals are unit
void Model::Normalize_Normals()
{
	Log_Add(2, "Making sure all normals are unit for trimesh");

	size_t end = normals.size();
	float l;

	for (size_t i=0; i<end; ++i)
	{
		l=v_length(normals[i].x, normals[i].y, normals[i].z);
		normals[i].x /=l;
		normals[i].y /=l;
		normals[i].z /=l;
	}
}

//creates missing normals (if needed)
//counter-clockwise order of triangles assumed
void Model::Generate_Missing_Normals()
{
	Log_Add(2, "Generating missing normals for trimesh");

	unsigned int *nindex, *vindex;;
	Vector_Float v1, v2, v3;

	float ax,ay,az, bx,by,bz, l;
	Vector_Float new_normal;
	unsigned new_normal_number;


	size_t m, mend = materials.size();
	size_t t, tend;

	//all triangles in all materials
	for (m=0; m<mend; ++m)
		if ((tend = materials[m].triangles.size()))
			for (t=0; t<tend; ++t)
			{
				//normal indices
				nindex=materials[m].triangles[t].normal;

				//one or more indices are unspecified
				if (nindex[0] == INDEX_ERROR || nindex[1] == INDEX_ERROR || nindex[2] == INDEX_ERROR)
				{
					//vertex indices
					vindex=materials[m].triangles[t].vertex;

					//copy vertices:
					v1=vertices[vindex[0]];
					v2=vertices[vindex[1]];
					v3=vertices[vindex[2]];

					//create two vectors (a and b) from the first point to the two others:
					ax = (v2.x-v1.x);
					ay = (v2.y-v1.y);
					az = (v2.z-v1.z);

					bx = (v3.x-v1.x);
					by = (v3.y-v1.y);
					bz = (v3.z-v1.z);

					//cross product gives normal:
					new_normal.x = (ay*bz)-(az*by);
					new_normal.y = (az*bx)-(ax*bz);
					new_normal.z = (ax*by)-(ay*bx);
					
					//make unit:
					l = v_length(new_normal.x,new_normal.y,new_normal.z);
					new_normal.x /= l;
					new_normal.y /= l;
					new_normal.z /= l;

					//store it:
					//note: since indexing the normal array isn't needed for any later stage
					//(will be "unindexed"), don't bother about duplicates
					normals.push_back(new_normal);

					//set up indices:
					new_normal_number = normals.size()-1;
					nindex[0] = new_normal_number;
					nindex[1] = new_normal_number;
					nindex[2] = new_normal_number;
				}
			}
}

//resize, rotate, change offset stuff (TODO: DELETE THIS!):
void Model::Resize(float r)
{
	if (r == 1.0) //no need
		return;

	if (r == 0.0) //easy mistake
	{
		Log_Add(0, "You've made a typo: resize is 1.0, not 0.0 - Ignoring...");
		return;
	}

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		vertices[i].x *= r;
		vertices[i].y *= r;
		vertices[i].z *= r;
	}
}

void Model::Rotate(float x, float y, float z)
{
	if (x==0 && y==0 && z==0)
		return;

	//rotation matrix:
	dMatrix3 rot;
	dRFromEulerAngles (rot, x*(M_PI/180), y*(M_PI/180), z*(M_PI/180));

	Vector_Float v, rotated;

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		v=vertices[i];
		rotated.x = v.x*rot[0]+v.y*rot[4]+v.z*rot[8];
		rotated.y = v.x*rot[1]+v.y*rot[5]+v.z*rot[9];
		rotated.z = v.x*rot[2]+v.y*rot[6]+v.z*rot[10];

		vertices[i]=rotated;
	}

	end = normals.size();

	for (i=0; i != end; ++i)
	{
		v=normals[i];
		rotated.x = v.x*rot[0]+v.y*rot[4]+v.z*rot[8];
		rotated.y = v.x*rot[1]+v.y*rot[5]+v.z*rot[9];
		rotated.z = v.x*rot[2]+v.y*rot[6]+v.z*rot[10];

		normals[i]=rotated;
	}
}

void Model::Offset(float x, float y, float z)
{
	if (x==0 && y==0 && z==0)
		return;

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		vertices[i].x += x;
		vertices[i].y += y;
		vertices[i].z += z;
	}
}

//uggly: change to bounding box instead of sphere...
float Model::Find_Longest_Distance()
{
	size_t end = vertices.size();
	size_t i;
	float biggest=0.0, length;

	//for optimum performance, no sqrt or similar, just store the one biggest axis
	for (i=0; i<end; ++i)
	{
		length = v_length(vertices[i].x, vertices[i].y, vertices[i].z);

		if (length > biggest)
			biggest=length;
	}

	Log_Add(2, "Furthest vertex distance in trimesh (\"radius\"): %f", biggest);

	return biggest;
}

std::string Model::Relative_Path(const char *file)
{
	std::string path;
	path=name; //name of this model (from class variables)

	//position of last slash in model filename (or npos)
	size_t last=path.rfind('/');

	//remove everything from last slash, or whole string
	path.erase((last==std::string::npos? 0: last));

	//append file to it
	path+="/";
	path+=file;

	return path;
}
//
//for trimesh file loading
//

//wrapper for loading
bool Model::Load(const char *file)
{
	Log_Add(2, "Loading model from file \"%s\" (identifying suffix)", file);

	//empty old data (if any)
	vertices.clear();
	texcoords.clear();
	normals.clear();
	materials.clear();

	//set name to filename
	name=file;

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
	{
		Log_Add(-1, "could not find 3D file \"%s\" for model", file);
		return false;
	}

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
bool Model::Load_Material(const char *file)
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

	//find
	Directories dirs;
	if (!dirs.Find(file, DATA, READ))
	{
		Log_Add(-1, "could not find material file \"%s\" for model", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".mtl"))
		return Load_MTL(dirs.Path());

	//else, no match
	Log_Add(-1, "Unknown material file suffix for \"%s\"", file);
	return false;
}

/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014 Mats Wahlberg
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

#include "shared/trimesh.hpp"

#include "shared/log.hpp"

#include "text_file.hpp"


bool Trimesh::Load_OBJ(const char *f)
{
	Log_Add(2, "Loading trimesh from OBJ file %s", f);

	Text_File file;

	//check if ok...
	if (!file.Open(f))
		return false;
	
	//empty old data (if any)
	vertices.clear();
	//texcoords.clear();
	normals.clear();
	materials.clear();

	//
	//ok, start processing
	//
	Vector_Float vector;
	Triangle_Uint triangle; //for building a triangle
	unsigned int matnr = INDEX_ERROR; //keep track of current material (none right now)
	unsigned int tmpmatnr;
	unsigned int vi, ni;
	int count;

	while (file.Read_Line())
	{
		// "v" vertex and 4 words
		if (file.words[0][0]=='v' && file.words[0][1]=='\0' && file.word_count==4)
		{
			vector.x=atof(file.words[1]);
			vector.y=atof(file.words[2]);
			vector.z=atof(file.words[3]);

			vertices.push_back(vector); //add to list
		} // "vn" normal and 4 words
		else if (file.words[0][0]=='v' && file.words[0][1]=='n' && file.words[0][2]=='\0' && file.word_count==4)
		{
			vector.x=atof(file.words[1]);
			vector.y=atof(file.words[2]);
			vector.z=atof(file.words[3]);

			normals.push_back(vector); //add to list
		} // "f" index and more than 3 words (needs at least 3 indices)
		else if (file.words[0][0]=='f' && file.words[0][1]=='\0' && file.word_count>3)
		{
			//no material right now, warn and create default:
			if (matnr == INDEX_ERROR)
			{
				Log_Add(-1, "\"%s\" did not specify material to use before index, using default", f);
				materials.push_back(Material_Default); //add new material (with defaults)
				matnr = 0;
			}

			for (int i=1; i<file.word_count; ++i)
			{
				// - format: v/(t)/(n) - vertex, texture, normal
				// only v is absolutely needed, and not optional
				// t ignored for now
				ni = INDEX_ERROR;
				count = sscanf(file.words[i], "%u/%*u/%u", &vi, &ni);

				if (count == 0) //nothing read
				{
					Log_Add(-1, "\"%s\" got malformed index, ignoring", f);
					break;
				}
				else //at least v read
				{
					--vi; //obj starts count on 1, change to 0

					if (count == 2) //read everything - v,t,n
					{
						--ni; //1->0
					}
					else if (count == 1) //only v, not t (not provided), parhaps n is stil there
					{
						if (sscanf(file.words[i], "%*u//%u", &ni) == 1)
							--ni; //1->0
					}
				}

				//now we got indices, see what to do with them
				//note: this checking is ugly from performance point of view, but helps keep the code clean
				//the first two times, just store indices, then start build triangles for each new index
				if (i>2) //time to build
				{
					//move latest to second latest
					triangle.vertex[1]=triangle.vertex[2]; //vertex
					triangle.normal[1]=triangle.normal[2]; //normal

					//add new
					triangle.vertex[2]=vi; //vertex
					triangle.normal[2]=ni; //normal

					//store
					materials[matnr].triangles.push_back(triangle);
				}
				else if (i==2) //second time
				{
					triangle.vertex[2]=vi;
					triangle.normal[2]=ni;
				}
				else //first time
				{
					triangle.vertex[0]=vi;
					triangle.normal[0]=ni;
				}
			}
		}
		else if (!strcmp(file.words[0], "usemtl") && file.word_count==2)
		{
			tmpmatnr = Find_Material(file.words[1]);

			if (tmpmatnr == INDEX_ERROR)
				Log_Add(0, "WARNING: ignoring change of material (things will probably look wrong)");
			else
				matnr = tmpmatnr;

			//else, we now have material switch for next triangles
		}
		else if (!strcmp(file.words[0], "mtllib") && file.word_count==2)
		{
			char filename[strlen(f)+strlen(file.words[1])+1]; //enough to hold both obj and mtl path+filename
			strcpy(filename, f); //copy obj path+filename
			char *last = strrchr(filename, '/'); //last slash in obj filename

			if (last) //if obj file had a path before filename (most likely):
			{
				last[1]='\0'; //indicate end at end of path (after /)
				strcat(filename, file.words[1]); //add mtl filename/path to obj path
			}
			else //just what's requested
			{
				strcpy(filename, file.words[1]); //overwrite with mtl filename
			}
				
			Load_Material(filename); //if not succesfull, continue - might not need any materials anyway?
		}
	}

	//check that at least something got loaded:
	if (materials.empty() || vertices.empty())
	{
		Log_Add(0, "\"%s\" seems to exist, but is empty?!", f);
		return false;
	}

	//ok, lets just make sure all data is good:

	//check so vertex indices are ok (not outside valid range)
	//takes a little time, but is good for safety
	size_t triangle_count = 0;
	size_t ml=materials.size();
	size_t vl=vertices.size();
	size_t nl=normals.size();
	for (size_t mat=0; mat<ml; ++mat) //all materials
	{
		size_t tl=materials[mat].triangles.size();
		for (size_t tri=0; tri<tl; ++tri) //all triangles
		{
			//points at triangle
			Triangle_Uint *trip=&materials[mat].triangles[tri];

			if (trip->vertex[0] >= vl || trip->vertex[1] >= vl || trip->vertex[2] >= vl)
			{
				Log_Add(-1, "vertex index in \"%s\" out of range, trying to bypass problem (not rendering)", f);
				trip->vertex[0] = trip->vertex[1] = trip->vertex[2] = 0; //set them all to 0
			}
			if (	(trip->normal[0] >= nl && trip->normal[0] != INDEX_ERROR) ||
				(trip->normal[1] >= nl && trip->normal[1] != INDEX_ERROR) ||
				(trip->normal[2] >= nl && trip->normal[2] != INDEX_ERROR)	)
			{
				Log_Add(-1, "normal index in \"%s\" out of range, trying to bypass problem (generating new)", f);
				trip->normal[0] = trip->normal[1] = trip->normal[2] = INDEX_ERROR; //set them all to 0
			}
		}
		triangle_count+=tl; //count triangles (for info output later on)
	}

	Normalize_Normals();
	Generate_Missing_Normals(); //creates missing normals - unit, don't need normalizing

	Log_Add(2, "OBJ loading info: %u triangles, %u materials", triangle_count, materials.size());

	return true;
}

bool Trimesh::Load_MTL(const char *f)
{
	Log_Add(2, "Loading trimesh material(s) from MTL file %s", f);

	Text_File file;
	Material_Float *material;

	//check if ok...
	if (!file.Open(f))
		return false;
	
	//
	//start processing
	//
	unsigned int mat_nr=INDEX_ERROR;

	while (file.Read_Line())
	{
		if ( (!strcmp(file.words[0], "newmtl")) && file.word_count == 2 )
		{
			mat_nr = materials.size(); //how much used, which number to give this material
			materials.push_back(Material_Default); //add new material (with defaults)
			materials[mat_nr].name = file.words[1]; //set name
		}
		else if (mat_nr == INDEX_ERROR)
			Log_Add(-1, "\"%s\" wants to specify material properties for unnamed material?! Ignoring", f);
		else
		{
			//direct pointer to material struct
			material=&materials[mat_nr].material;

			//material properties:
			if (file.words[0][0] == 'K') //colours?
			{
				if (file.words[0][1] == 'a' && file.word_count == 4) //ambient
				{
					material->ambient[0] = atof(file.words[1]);
					material->ambient[1] = atof(file.words[2]);
					material->ambient[2] = atof(file.words[3]);
				}
				else if (file.words[0][1] == 'd' && file.word_count == 4) //diffuse
				{
					material->diffuse[0] = atof(file.words[1]);
					material->diffuse[1] = atof(file.words[2]);
					material->diffuse[2] = atof(file.words[3]);
				}
				else if (file.words[0][1] == 's' && file.word_count == 4) //specular
				{
					material->specular[0] = atof(file.words[1]);
					material->specular[1] = atof(file.words[2]);
					material->specular[2] = atof(file.words[3]);
				}

				//the following seems to be an unofficial extension of the mtl format (which us usefull):
				else if (file.words[0][1] == 'e' && file.word_count == 4) //emission
				{
					material->emission[0] = atof(file.words[1]);
					material->emission[1] = atof(file.words[2]);
					material->emission[2] = atof(file.words[3]);
				}
			}
			else if (file.words[0][0] == 'N') //some other stuff?
			{
				//only one of these are used:
				if (file.words[0][1] == 's' && file.word_count == 2) //shininess
				{
					//from what I've read, this vary between 0 to 1000 for obj
					//materials[mat_nr].shininess = (atof(file.words[1])*(128.0/1000.0));
					//...but all mtl files I've seen are under 128 (valid opengl range),
					//so lets just load it directly (without converting)...?
					material->shininess = atof(file.words[1]);

					//seems like there are mtl files out there with Ns>128?
					if (material->shininess > 128.0)
					{
						material->shininess=128.0;
						Log_Add(-1, "\"%s\" file got Ns>128, please tell me (Mats) to fix the mtl loader!", f);
					}
				}
			}


		}
	}

	//check if we got any data:
	if (mat_nr == INDEX_ERROR)
	{
		Log_Add(-1, "\"%s\" seems to exist, but is empty?!", f);
		return false;
	}

	//see if any material sets ambient to 0, if so set it to diffuse instead
	for (unsigned int i=0; i<=mat_nr; ++i)
	{
		material = &materials[i].material;
		if (	material->ambient[0] == 0 &&
			material->ambient[1] == 0 &&	
			material->ambient[2] == 0	)
		{
			Log_Add(2, "NOTE: found material with ambient colour of 0, setting to diffuse instead");
			material->ambient[0] = material->diffuse[0];
			material->ambient[1] = material->diffuse[1];
			material->ambient[2] = material->diffuse[2];
		}
	}
			
	//else, ok
	return true;
}


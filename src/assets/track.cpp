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

#include <stdlib.h>

#include "track.hpp"
#include "text_file.hpp"
#include "common/object.hpp"

#include "common/internal.hpp"
#include "common/log.hpp"
#include "common/lua.hpp"
#include "common/directories.hpp"
#include "common/threads.hpp"

#include "simulation/geom.hpp"
#include "simulation/camera.hpp"

//TODO: remove this!
struct Track_Struct track = track_defaults;

//
//keep track of all loaded models (cleared between loading)
//
std::vector<Model*> models;

//helper function for finding or loading model files
Model *FindOrLoadMesh(const char *path, const char *name)
{
	//merge path and name (path+/+name+\0)...
	char file[strlen(path)+1+strlen(name)+1];
	strcpy(file, path);
	strcat(file, "/");
	strcat(file, name);

	//already loaded?
	for (size_t i=0; i!=models.size(); ++i)
		if (models[i]->Compare_Name(file))
		{
			Log_Add(2, "model already loaded");
			return models[i];
		}

	//else, try loading...
	Model *mesh = new Model();
	if (mesh->Load(file))
	{
		models.push_back(mesh);
		return mesh; //ok, worked
	}
	//else¸ failure
	delete mesh;
	return NULL;
}

//remove all loading meshes
void RemoveMeshes()
{
	for (size_t i=0; i!=models.size(); ++i)
		delete models[i];
	models.clear();
}
//
//


bool load_track (const char *path)
{
	Log_Add(1, "Loading track: %s", path);
	Directories dirs;

	//
	//conf
	//
	char conf[strlen(path)+11+1];
	strcpy (conf,path);
	strcat (conf,"/track.conf");

	if (!(dirs.Find(conf, DATA, READ)) && Load_Conf(dirs.Path(), (char *)&track, track_index))
		Log_Add(0, "WARNING: no config file for track, falling back to defaults");

	//set camera default values, some from track specs
	default_camera.Set_Pos(track.cam_start[0], track.cam_start[1], track.cam_start[2],
			track.focus_start[0], track.focus_start[1], track.focus_start[2]);

	if (track.fog_mode < 0 || track.fog_mode > 3)
	{
		Log_Add(-1, "Incorrect fog mode for track, disabling");
		track.fog_mode=0;

	}
	//all data loaded, start building
	//background (for now)
	glClearColor (track.background[0],track.background[1],track.background[2],track.background[3]);
	//fog
	glFogfv(GL_FOG_COLOR, track.fog_colour);
	
	//sun position and colour
	glLightfv (GL_LIGHT0, GL_AMBIENT, track.ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, track.diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, track.specular);
	glLightfv (GL_LIGHT0, GL_POSITION, track.position);

	//set track specific global ode params:
	dWorldSetGravity (simulation_thread.world, track.gravity[0], track.gravity[1], track.gravity[2]);

	//
	//geoms
	//
	//using trimesh geoms and one plane for describing world, store in track object
	track.object = new Object();
	track.space = new Space (track.object);

	//loading of model files
	char glist[strlen(path)+10+1];
	strcpy (glist,path);
	strcat (glist,"/geoms.lst");

	Log_Add(2, "Loading track geom list: %s", glist);
	Text_File file;

	if (dirs.Find(glist, DATA, READ) && file.Open(dirs.Path()))
	{
		//store default global surface properties for all geoms
		Surface global;
		//keep track of latest geom create
		Geom *latestgeom=NULL;

		while (file.Read_Line())
		{
			//if requesting optional stuff
			if (!strcmp(file.words[0], ">") && file.word_count >= 2)
			{
				//model manipulation
				if (!strcmp(file.words[1], "modify"))
				{
					Log_Add(2, "overriding model properties");

					Model *mesh = FindOrLoadMesh(path, file.words[2]);

					if (!mesh)
					{
						RemoveMeshes();
						delete track.object;
						return false;
					}

					//now process the rest for extra options
					int pos = 3;
					while (pos < file.word_count)
					{
						int left=file.word_count-pos;

						//resize, takes the word resize and one value
						if (!strcmp(file.words[pos], "resize") && left >= 2)
						{
							mesh->Resize(atof(file.words[pos+1]));
							pos+=2;
						}
						//rotate, takes the word rotate and 3 values
						else if (!strcmp(file.words[pos], "rotate") && left >= 4)
						{
							mesh->Rotate(atof(file.words[pos+1]), //x
									atof(file.words[pos+2]), //y
									atof(file.words[pos+3])); //z
							pos+=4;
						}
						//offset, takes the word offset and 3 values
						else if (!strcmp(file.words[pos], "offset") && left >= 4)
						{
							mesh->Offset(atof(file.words[pos+1]), //x
									atof(file.words[pos+2]), //y
									atof(file.words[pos+3])); //z
							pos+=4;
						}
						else
						{
							Log_Add(0, "WARNING: models loading option \"%s\" not known", file.words[pos]);
							++pos;
						}
					}
				}
				//surface manipulation (of latest geom)
				else if (!strcmp(file.words[1], "surface") && file.word_count >= 3)
				{
					Log_Add(2, "changing surface properties");
					Surface *surface=NULL;
					int pos=0;

					if (!strcmp(file.words[2], "global"))
					{
						surface = &global;
						pos = 3;
					}
					else if (!strcmp(file.words[2], "material") && file.word_count >= 4)
					{
						//locate surface named as specified
						surface = latestgeom->Find_Material_Surface(file.words[3]);
						pos = 4;
					}
					else
						Log_Add(0, "WARNING: surface type must be either global or material");

					if (!surface)
					{
						Log_Add(0, "WARNING: could not find specified surface");
						continue;
					}

					//as long as there are two words left (option name and value)
					while ( (file.word_count-pos) >= 2)
					{
						//note: mu and damping uses strtod instead of atof.
						//as usuall, this it's because losedows not being standardized
						//and failing to support infinity support for atof... :-p
						//only added for spring and mu - the only ones supporting it
						if (!strcmp(file.words[pos], "mu"))
							surface->mu = strtod(file.words[++pos], (char**)NULL);
						else if (!strcmp(file.words[pos], "bounce"))
							surface->bounce = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "spring"))
							surface->spring = strtod(file.words[++pos], (char**)NULL);
						else if (!strcmp(file.words[pos], "damping"))
							surface->damping = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "sensitivity"))
							surface->sensitivity = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "rollres"))
							surface->rollres = atof(file.words[++pos]);
						else
							Log_Add(0, "WARNING: models surface option \"%s\" unknown", file.words[pos]);

						//one step forward
						pos+=1;
					}

				}
				else
					Log_Add(0, "WARNING: optional line in geoms.lst malformed");

			}
			//geom to create
			else if (file.word_count == 8 || file.word_count == 7)
			{
				Model *mesh1, *mesh2;
				Model_Draw *model;
				Model_Mesh *geom;
				float x,y,z;

				//no alternative render model
				if ( file.word_count == 7)
				{
					if (	!(mesh1 = FindOrLoadMesh(path, file.words[6])) ||
						!(geom = mesh1->Create_Mesh()) ||
						!(model = mesh1->Create_Draw()) )
					{
						RemoveMeshes();
						delete track.object;
						return false;
					}
				}
				//one collision and one render model
				else if (file.word_count == 8)
				{
					if (	!(mesh1 = FindOrLoadMesh(path, file.words[6])) ||
						!(mesh2 = FindOrLoadMesh(path, file.words[7])) ||
						!(geom = mesh1->Create_Mesh()) ||
						!(model = mesh2->Create_Draw()) )
					{
						RemoveMeshes();
						delete track.object;
						return false;
					}
				}
				//in case failure to load
				else
				{
					continue; //go to next line
				}

				//ok, now geom and model should contain useful data...
				latestgeom = geom->Create_Mesh(track.object); //create geom from geom-trimesh
				
				//configure geom
				latestgeom->model = model; //render geom with model
				latestgeom->surface = global; //start by using global specified properties

				//position
				x = atof(file.words[0]);
				y = atof(file.words[1]);
				z = atof(file.words[2]);
				dGeomSetPosition(latestgeom->geom_id, x,y,z);
				
				//rotation
				dMatrix3 rot;
				x = atof(file.words[3])*M_PI/180.0;
				y = atof(file.words[4])*M_PI/180.0;
				z = atof(file.words[5])*M_PI/180.0;

				dRFromEulerAngles(rot, x,y,z);
				dGeomSetRotation(latestgeom->geom_id, rot);
			}
			else
			{
				Log_Add(0, "WARNING: did not understand line in geom list...");
				continue;
			}
		}
	}
	else
	{
		Log_Add(-1, "No geom list for track! Can not create any terrain...");
		RemoveMeshes();
		delete track.object;
		return false;
	}

	RemoveMeshes();

	//
	//objects
	//
	char olist[strlen(path)+12+1];
	strcpy (olist,path);
	strcat (olist,"/objects.lua");

	Log_Add(2, "Loading track object list: %s", olist);

	//don't fail if can't find file, maybe there is no need for it anyway
	if (dirs.Find(olist, DATA, READ))
	{
		Log_Add(1, "Running lua script: %s", dirs.Path());
		lua_State *L=simulation_thread.lua_state;
		int status = luaL_dofile(L, dirs.Path());

		if (status != LUA_OK)
		{
			const char *m=lua_tostring(L, -1);
			Log_Add(-1, "Lua error: %s", m);
			lua_pop(L, -1);
		}
	}
	else
		Log_Add(0, "WARNING: no object list for track, no default objects created");

	//that's it!
	return true;
}


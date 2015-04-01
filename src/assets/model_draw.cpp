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

//
//for vbo 3d rendering trimesh:
//
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "image.hpp"
#include "model.hpp"
#include "conf.hpp"
#include "common/internal.hpp"
#include "common/log.hpp"
#include "common/directories.hpp"

//length of vector
#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))

//keep track of VBOs (new generated if not enough room in already existing)
class VBO: public Assets
{
	public:
		//find a vbo with enough room, if not create a new one
		static VBO *Find_Enough_Room(unsigned int needed)
		{
			Log_Add(2, "Locating vbo to hold %u bytes of data", needed);

			//in case creating
			GLsizei size=DEFAULT_VBO_SIZE;
			bool dedicated=false;

			//check so enough space in even a new vbo:
			if (needed > DEFAULT_VBO_SIZE)
			{
				Log_Add(2, "creating new vbo for single model, %u bytes of size", needed);
				dedicated=true;
				size=needed;
			}
			else
			{
				//see if already exists
				for (VBO *p=head; p; p=p->next)
					if ( !p->dedicated && (p->usage)+needed <= (unsigned int) DEFAULT_VBO_SIZE ) //not dedicated+enough to hold
					{
						Log_Add(2, "reusing already existing vbo for model");
						return p;
					}

				//else, did not find enough room, create
				Log_Add(2, "creating new vbo for multiple models, %u bytes of size", DEFAULT_VBO_SIZE);
			}


			//create and bind vbo:
			GLuint target;
			glGenBuffers(1, &target); //create buffer
			glBindBuffer(GL_ARRAY_BUFFER, target); //bind
			glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW); //fill/allocate

			//check if allocated ok:
			if (GLenum error = glGetError()) //if got error...
			{
				//...should be a memory issue...
				if (error == GL_OUT_OF_MEMORY)
					Log_Add(-1, "Insufficient graphics memory, can not store rendering models...");
				else //...but might be a coding error
					Log_Add(-1, "Unexpected opengl error!!! Fix this!");

				//anyway, we return NULL to indicate failure
				return NULL;
			}

			//ok, so create a class to track it (until not needed anymore)
			return new VBO(target, dedicated);
		}

		GLuint id; //position of buffer (for mapping)
		GLsizei usage; //how much of buffer is used (possibly GLint instead?)
		bool dedicated;

	private:
		//normally, Assets is only for tracking loaded data, one class for each loaded...
		//but this is slightly different: one vbo class can store several model sets
		//(making it a Assets makes sure all VBOs gets deleted at the same time as models)
		VBO(GLuint target, bool dedicated): Assets("VBO tracking class") //name all vbo classes this...
		{
			//place on top of list
			next=head;
			head=this;
			id=target;
			usage=0; //no data yet
			this->dedicated=dedicated;
		}
		~VBO()
		{
			glDeleteBuffers(1, &id);
			//VBOs only removed on end of race (are racetime_data), all of them, so can safely just destroy old list
			head = NULL;
		}

		static VBO *head;
		VBO *next;
};

VBO *VBO::head=NULL;

//
//Model_Draw stuff:
//

//constructor
Model_Draw::Model_Draw(const char *name, float r, GLuint vbo, Material *mpointer, unsigned int mcount):
	Assets(name), materials(mpointer), material_count(mcount), radius(r), vbo_id(vbo)
{
}

//only called together with all other racetime_data destruction (at end of race)
Model_Draw::~Model_Draw()
{
	//remove local data:
	delete[] materials;
}

//loading of a 3d model directly
Model_Draw *Model_Draw::Quick_Load(const char *name, float resize,
		float rotx, float roty, float rotz,
		float offx, float offy, float offz)
{
	//check if already exists
	if (Model_Draw *tmp=Assets::Find<Model_Draw>(name))
		return tmp;

	//no, load
	Model model;

	//failure to load
	if (!model.Load(name))
		return NULL;

	//pass modification requests (will be ignored if defaults)
	model.Resize(resize);
	model.Rotate(rotx, roty, rotz);
	model.Offset(offx, offy, offz);

	//create a geom from this and return it
	return model.Create_Draw();
}

//simplified
Model_Draw *Model_Draw::Quick_Load(const char *name)
{
	//check if already exists
	if (Model_Draw *tmp=Assets::Find<Model_Draw>(name))
		return tmp;

	//no, load
	Model model;

	//failure to load
	if (!model.Load(name))
		return NULL;

	//create a geom from this and return it
	return model.Create_Draw();
}

//even simpler: all data grabbed from conf file...
Model_Draw *Model_Draw::Quick_Load_Conf(const char *path, const char *file)
{
	//small conf for filename and model manipulation stuff
	struct mconf {
		Conf_String model;
		float resize, rotate[3], offset[3];
	} modelconf = { //defaults
		"",
		1.0, {0.0,0.0,0.0}, {0.0,0.0,0.0}
	};
	//index (for loading)
	const struct Conf_Index modelconfindex[] = {
	{"model",		's',1, offsetof(mconf, model)},
	{"resize",		'f',1, offsetof(mconf, resize)},
	{"rotate",		'f',3, offsetof(mconf, rotate)},
	{"offset",		'f',3, offsetof(mconf, offset)},
	{"",0,0}};

	//build path+file string for loading conf
	char conf[strlen(path)+1+strlen(file)+1];
	strcpy(conf, path);
	strcat(conf, "/");
	strcat(conf, file);

	//load conf
	Directories dirs;
	if (!(dirs.Find(conf, DATA, READ) && Load_Conf(dirs.Path(), (char*)&modelconf, modelconfindex)))
		return NULL;

	//if we got no filename from the conf, nothing more to do
	if (!modelconf.model)
	{
		Log_Add(2, "WARNING: could not find model filename in conf \"%s\"", conf);
		return NULL;
	}

	//build path+file for model
	char model[strlen(path)+1+strlen(modelconf.model)+1];
	strcpy(model, path);
	strcat(model, "/");
	strcat(model, modelconf.model);

	//load
	return Model_Draw::Quick_Load(model, modelconf.resize,
			modelconf.rotate[0], modelconf.rotate[1], modelconf.rotate[2],
			modelconf.offset[0], modelconf.offset[1], modelconf.offset[2]);
}

//method for creating a Model_Draw from Trimesh
Model_Draw *Model::Create_Draw()
{
	//already uploaded?
	if (Model_Draw *tmp = Assets::Find<Model_Draw>(name.c_str()))
		return tmp;

	//check how many vertices (if any)
	unsigned int vcount=0; //how many vertices
	unsigned int mcount=0; //how many materials
	size_t material_count=materials.size();
	size_t tmp;
	for (unsigned int mat=0; mat<material_count; ++mat)
	{
		//vertices (3 per triangle)
		tmp = 3*materials[mat].triangles.size();
		vcount+=tmp;

		//but if this material got 0 triangles, don't use it
		if (tmp)
			++mcount;
	}

	if (!vcount)
	{
		Log_Add(-1, "trimesh is empty (at least no triangles)");
		return NULL;
	}
	//mcount is always secured

	//each triangle requires 3 vertices - vertex defined as "Vertex" in "Model_Draw"
	unsigned int needed_vbo_size = sizeof(Model_Draw::Vertex)*(vcount);
	VBO *vbo = VBO::Find_Enough_Room(needed_vbo_size);

	if (!vbo)
		return NULL;

	//
	//ok, ready to go!
	//
	
	Log_Add(2, "number of vertices: %u", vcount);

	//quickly find furthest vertex of obj, so can determine radius
	float radius = Find_Longest_Distance();
	
	//build a tmp list of all vertices sorted by material to minimize calls to be copied
	//to vbo (graphics memory), and list of materials (with no duplicates or unused)

	//first: how big should vertex list be?
	Model_Draw::Vertex *vertex_list = new Model_Draw::Vertex[vcount];

	//make material list as big as the number of materials
	Model_Draw::Material *material_list = new Model_Draw::Material[mcount];


	//some values needed:
	unsigned int m,t; //looping of Material and Triangle
	size_t m_size=materials.size();
	size_t t_size;
	mcount=0; //reset to 0 (will need to count again)
	vcount=0; //the same here
	unsigned int vcount_old=0; //keep track of last block of vertices

	//points at current indices:
	unsigned int *vertexi, *texcoordi, *normali;

	//loop through all materials, and for each used, loop through all triangles
	//(removes indedexing -make copies- and interleaves the vertices+normals)
	for (m=0; m<m_size; ++m)
	{
		//if this material is used for some triangle:
		if ((t_size = materials[m].triangles.size()))
		{
			//
			//copy vertices data:
			//
			for (t=0; t<t_size; ++t)
			{
				//store indices:
				vertexi = materials[m].triangles[t].vertex;
				texcoordi = materials[m].triangles[t].texcoord;
				normali = materials[m].triangles[t].normal;

				//vertex
				vertex_list[vcount].x = vertices[vertexi[0]].x;
				vertex_list[vcount].y = vertices[vertexi[0]].y;
				vertex_list[vcount].z = vertices[vertexi[0]].z;

				//texcoord
				vertex_list[vcount].u = texcoords[texcoordi[0]].x;
				vertex_list[vcount].v = texcoords[texcoordi[0]].y;

				//normal
				vertex_list[vcount].nx = normals[normali[0]].x;
				vertex_list[vcount].ny = normals[normali[0]].y;
				vertex_list[vcount].nz = normals[normali[0]].z;

				//jump to next
				++vcount;

				//vertex
				vertex_list[vcount].x = vertices[vertexi[1]].x;
				vertex_list[vcount].y = vertices[vertexi[1]].y;
				vertex_list[vcount].z = vertices[vertexi[1]].z;

				//texcoord
				vertex_list[vcount].u = texcoords[texcoordi[1]].x;
				vertex_list[vcount].v = texcoords[texcoordi[1]].y;

				//normal
				vertex_list[vcount].nx = normals[normali[1]].x;
				vertex_list[vcount].ny = normals[normali[1]].y;
				vertex_list[vcount].nz = normals[normali[1]].z;

				//jump to next
				++vcount;

				//vertex
				vertex_list[vcount].x = vertices[vertexi[2]].x;
				vertex_list[vcount].y = vertices[vertexi[2]].y;
				vertex_list[vcount].z = vertices[vertexi[2]].z;

				//texcoord
				vertex_list[vcount].u = texcoords[texcoordi[2]].x;
				vertex_list[vcount].v = texcoords[texcoordi[2]].y;

				//normal
				vertex_list[vcount].nx = normals[normali[2]].x;
				vertex_list[vcount].ny = normals[normali[2]].y;
				vertex_list[vcount].nz = normals[normali[2]].z;

				//jump to next
				++vcount;
			}
			//
			//copy material data:
			//
			material_list[mcount].material = materials[m].material;

			//
			//textures (disabled by default):
			//
			material_list[mcount].diffusetex = 0;

			//got (diffuse) texture, try to use
			if (!materials[m].diffusetex.empty())
			{
				//check if already exists
				if (Image_Texture *tmp=Assets::Find<Image_Texture>(materials[m].diffusetex.c_str()))
					material_list[mcount].diffusetex=tmp->GetID();

				//no, load
				Image image;

				//if we could load, try to create texture
				if (image.Load(materials[m].diffusetex.c_str()))
				{
					Image_Texture *texture = image.Create_Texture();

					if (texture)
						material_list[mcount].diffusetex=texture->GetID();
				}
			}

			//set up rendering tracking:
			material_list[mcount].start=vcount_old;
			material_list[mcount].size=(vcount-vcount_old);

			//actually, the start should be offsetted by the current usage of vbo
			//(since this new data will be placed after the last model)
			//NOTE: instead of counting in bytes, this is counting in "vertices"
			material_list[mcount].start += (vbo->usage)/sizeof(Model_Draw::Vertex);

			//next time, this count will be needed
			vcount_old=vcount;

			//increase material counter
			++mcount;
		}
	}

	Log_Add(2, "number of (used) materials: %u", mcount);

	//create Model_Draw class from this data:
	//set the name. NOTE: both Model_Draw and Model_Mesh will have the same name
	//this is not a problem since they are different classes and Assets::Find will notice that
	Model_Draw *mesh = new Model_Draw(name.c_str(), radius, vbo->id, material_list, mcount);

	//assume this vbo is not bound
	glBindBuffer(GL_ARRAY_BUFFER, vbo->id);

	//transfer data to vbo...
	glBufferSubData(GL_ARRAY_BUFFER, vbo->usage, needed_vbo_size, vertex_list);

	//increase vbo usage counter
	vbo->usage+=needed_vbo_size;

	//free tmp memory
	delete[] vertex_list;

	//ok, done
	return mesh;
}



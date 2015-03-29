/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014, 2015 Mats Wahlberg
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

#include <GL/glew.h>
#include "render_list.hpp"

#include "assets/trimesh.hpp"
#include "common/threads.hpp"
#include "common/internal.hpp"
#include "common/log.hpp"
#include "simulation/geom.hpp"
#include "simulation/body.hpp"
#include "simulation/camera.hpp"

#include <stdlib.h>
#include <ode/ode.h>

//options
bool culling=false;
bool fog=false;


//just normal (component) list for now:

//consists of two buffers:
//each element in buffer:
struct list_element
{
	GLfloat matrix[16]; //4x4
	Trimesh_3D *model; //model to render
	Object *object; //object to which this component belongs
};

//keeps track of a buffer of elements:
//the buffers are initially set to 0 size, and increased when needed
//(but never decreased. but since not so big)
struct list_buffer
{
	bool updated;
	size_t count;
	size_t size;
	list_element *list;
	float camera_pos[3];
	float camera_rot[9];
	Object *camera_hide;
};

//buffers
list_buffer buffer1 = {false, 0, 0, NULL, {0,0,0}, {0,0,0, 0,0,0, 0,0,0}, NULL};
list_buffer buffer2 = {false, 0, 0, NULL, {0,0,0}, {0,0,0, 0,0,0, 0,0,0}, NULL};
list_buffer buffer3 = {false, 0, 0, NULL, {0,0,0}, {0,0,0, 0,0,0, 0,0,0}, NULL};

//pointers at buffers
list_buffer *buffer_render = &buffer1;
list_buffer *buffer_switch = &buffer2;
list_buffer *buffer_generate = &buffer3;

//remove allocated data in buffers
void Render_List_Clear_Interface()
{
	if (buffer_render->size)
	{
		buffer_render->updated=false;
		buffer_render->size=0;
		buffer_render->count=0;
		delete[] buffer_render->list;
		buffer_render->list=NULL;
	}

	//don't know which thread will clear
	SDL_mutexP(render_list_mutex);
	if (buffer_switch->size)
	{
		buffer_switch->updated=false;
		buffer_switch->size=0;
		buffer_switch->count=0;
		delete[] buffer_switch->list;
		buffer_switch->list=NULL;
	}
	SDL_mutexV(render_list_mutex);
}

//remove allocated data in buffers
void Render_List_Clear_Simulation()
{
	if (buffer_generate->size)
	{
		buffer_generate->updated=false;
		buffer_generate->size=0;
		buffer_generate->count=0;
		delete[] buffer_generate->list;
		buffer_generate->list=NULL;
	}

	//don't know which thread will clear
	SDL_mutexP(render_list_mutex);
	if (buffer_switch->size)
	{
		buffer_switch->updated=false;
		buffer_switch->size=0;
		buffer_switch->count=0;
		delete[] buffer_switch->list;
		buffer_switch->list=NULL;
	}
	SDL_mutexV(render_list_mutex);
}


//update
void Render_List_Update()
{
	//TMP: store "camera" in rendering list
	memcpy(buffer_generate->camera_pos, camera.pos, sizeof(float)*3);
	memcpy(buffer_generate->camera_rot, camera.rotation, sizeof(float)*9);
	buffer_generate->camera_hide = camera.hide;

	//add data as usual:

	//pointers:
	buffer_generate->count=0; //set to zero (empty)

	//variables
	const dReal *pos, *rot;
	GLfloat *matrix;

	for (Geom *g=Geom::head; g; g=g->next)
	{
		if (g->model)
		{
			//if buffer full...
			if (buffer_generate->count == buffer_generate->size)
			{
				Log_Add(2, "Render list was too small, resizing");

				//copy to new memory
				list_element *oldlist = buffer_generate->list;
				buffer_generate->size+=INITIAL_RENDER_LIST_SIZE;
				buffer_generate->list = new list_element[buffer_generate->size];
				memcpy(buffer_generate->list, oldlist, sizeof(list_element)*buffer_generate->count);
				delete[] oldlist;
			}

			pos = dGeomGetPosition(g->geom_id);
			rot = dGeomGetRotation(g->geom_id);
			matrix = buffer_generate->list[buffer_generate->count].matrix;

			//set matrix
			matrix[0]=rot[0];
			matrix[1]=rot[4];
			matrix[2]=rot[8];
			matrix[3]=0;
			matrix[4]=rot[1];
			matrix[5]=rot[5];
			matrix[6]=rot[9];
			matrix[7]=0;
			matrix[8]=rot[2];
			matrix[9]=rot[6];
			matrix[10]=rot[10];
			matrix[11]=0;
			matrix[12]=pos[0];
			matrix[13]=pos[1];
			matrix[14]=pos[2];
			matrix[15]=1;

			//set what to render
			buffer_generate->list[buffer_generate->count].model = g->model;

			//set object owning this component:
			buffer_generate->list[buffer_generate->count].object = g->object_parent;

			//increase counter
			++(buffer_generate->count);
		}
	}

	//same as above, but for bodies
	for (Body *b=Body::head; b; b=b->next)
	{
		if (b->model)
		{
			//if buffer full...
			if (buffer_generate->count == buffer_generate->size)
			{
				Log_Add(2, "Render list was too small, resizing");

				//copy to new memory
				list_element *oldlist = buffer_generate->list;
				buffer_generate->size+=INITIAL_RENDER_LIST_SIZE;
				buffer_generate->list = new list_element[buffer_generate->size];
				memcpy(buffer_generate->list, oldlist, sizeof(list_element)*buffer_generate->count);
				delete[] oldlist;
			}

			pos = dBodyGetPosition(b->body_id);
			rot = dBodyGetRotation(b->body_id);
			matrix = buffer_generate->list[buffer_generate->count].matrix;

			//set matrix
			matrix[0]=rot[0];
			matrix[1]=rot[4];
			matrix[2]=rot[8];
			matrix[3]=0;
			matrix[4]=rot[1];
			matrix[5]=rot[5];
			matrix[6]=rot[9];
			matrix[7]=0;
			matrix[8]=rot[2];
			matrix[9]=rot[6];
			matrix[10]=rot[10];
			matrix[11]=0;
			matrix[12]=pos[0];
			matrix[13]=pos[1];
			matrix[14]=pos[2];
			matrix[15]=1;

			//set what to render
			buffer_generate->list[buffer_generate->count].model = b->model;

			//set object owning this component:
			buffer_generate->list[buffer_generate->count].object = b->object_parent;

			//increase counter
			++(buffer_generate->count);
		}
	}

	//mark as updated
	buffer_generate->updated=true;

	//move...
	SDL_mutexP(render_list_mutex);
	list_buffer *p=buffer_switch;
	buffer_switch=buffer_generate;
	buffer_generate=p;
	SDL_mutexV(render_list_mutex);
}
 
//just to make it possible to check from outside
bool Render_List_Updated()
{

	if (buffer_switch->updated)
		return true;
	return false;
}


//check if new data+matrix
void Render_List_Prepare()
{
	//only if anything to do
	if (Render_List_Updated())
	{
		SDL_mutexP(render_list_mutex);
		list_buffer *p=buffer_switch;
		buffer_switch=buffer_render; //old buffer, not needed
		buffer_render= p;
		SDL_mutexV(render_list_mutex);

		//build matrix for camera projection:
		//rotation (right, up, forward)
		float *pos = buffer_render->camera_pos;
		float *rot = buffer_render->camera_rot;
		float matrix[16];
		//m0-m3
		matrix[0]=rot[0]; matrix[1]=rot[2]; matrix[2]=-rot[1]; matrix[3]=0.0;
		//m4-m7
		matrix[4]=rot[3]; matrix[5]=rot[5]; matrix[6]=-rot[4]; matrix[7]=0.0;
		//m4-m7
		matrix[8]=rot[6]; matrix[9]=rot[8]; matrix[10]=-rot[7]; matrix[11]=0.0;

		//m12-m14, translation
		matrix[12]=-matrix[0]*pos[0]-matrix[4]*pos[1]-matrix[8]*pos[2];
		matrix[13]=-matrix[1]*pos[0]-matrix[5]*pos[1]-matrix[9]*pos[2];
		matrix[14]=-matrix[2]*pos[0]-matrix[6]*pos[1]-matrix[10]*pos[2];

		//m15
		matrix[15]=1.0;

		//overwrite current matrix
		glLoadMatrixf(matrix);
	}
}

//updated on resizing, needed here:
extern float view_angle_rate_x, view_angle_rate_y;

//for setting up buffers (attribute pointers)
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

void Render_List_Render()
{
	//pointers to data
	size_t *count=&(buffer_render->count);
	list_element *list=buffer_render->list;
	float *camera_pos=buffer_render->camera_pos;
	float *camera_rot=buffer_render->camera_rot;
	Object *camera_hide=buffer_render->camera_hide;

	//variables/pointers
	unsigned int m_loop;
	Trimesh_3D *model;
	Material_Float *material;
	float *matrix;
	Trimesh_3D::Material *materials;
	unsigned int material_count;
	float radius;

	GLuint bound_vbo = 0; //keep track of which vbo is bound
	GLuint texture; //which texture to use
	GLuint bound_texture = 0; //keep track of which texture is bound
	bool texture_enabled = true; //keep track of if textures enabled or not

	//data needed to eliminate models not visible by camera:
	float dir_proj, up_proj, right_proj; //model position relative to camera
	float dir_min, dir_max, up_max, right_max; //limit for what camera can render
	float pos[3]; //relative pos

	//configure rendering options:

	//enable lighting
	glEnable (GL_LIGHT0);
	glEnable (GL_LIGHTING);

	glShadeModel (GL_SMOOTH); //by default, can be changed

	//glClearDepth (1.0); pointless to define this?

	//depth testing (proper overlapping)
	glDepthFunc (GL_LESS);
	glEnable (GL_DEPTH_TEST);

	//texture
	glEnable (GL_TEXTURE_2D);

	//enable anti aliasing?
	if (internal.msaa)
		glEnable(GL_MULTISAMPLE);

	//
	//options:
	//

	//culling of backs
	if (culling)
		glEnable(GL_CULL_FACE);

	//fog
	if (fog)
		glEnable(GL_FOG);

	//enable rendering of vertices and normals
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	//NOTE: new opengl vbo rendering commands (2.0 I think). For compatibility lets stick to 1.5 instead
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Trimesh_3D::Vertex), (BUFFER_OFFSET(0)));
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(sizeof(float)*3));

	for (size_t i=0; i<(*count); ++i)
	{
		//for cleaner code, set pointers:
		model = list[i].model;
		matrix = list[i].matrix;
		materials = model->materials;
		material_count = model->material_count;
		radius = model->radius;

		//check if object is not visible from current camera:
		//model pos relative to camera
		pos[0] = matrix[12]-camera_pos[0];
		pos[1] = matrix[13]-camera_pos[1];
		pos[2] = matrix[14]-camera_pos[2];

		//position of camera relative to camera/screen
		//this is really just a matrix multiplication, but we separate each line into a variable
		right_proj = pos[0]*camera_rot[0]+pos[1]*camera_rot[3]+pos[2]*camera_rot[6];
		dir_proj = pos[0]*camera_rot[1]+pos[1]*camera_rot[4]+pos[2]*camera_rot[7];
		up_proj = pos[0]*camera_rot[2]+pos[1]*camera_rot[5]+pos[2]*camera_rot[8];

		//limit of what range is rendered (compensates for "radius" of model that might still be seen)
		dir_min = internal.clipping[0] - radius; //behind close clipping
		dir_max = internal.clipping[1] + radius; //beyound far clipping
		right_max = view_angle_rate_x*(dir_proj+radius) + radius; //right/left
		up_max = view_angle_rate_y*(dir_proj+radius) + radius; //above/below

		//check if visible (or hidden for this camera):
		if (	(list[i].object == camera_hide)				||
			(dir_proj > dir_max)	|| (dir_proj < dir_min)		||
			(right_proj > right_max)|| (-right_proj > right_max)	||
			(up_proj > up_max)	|| (-up_proj > up_max)		)
			continue;
		//
		
		glPushMatrix();

			glMultMatrixf (matrix);

			if (model->vbo_id != bound_vbo)
			{
				//bind and configure the new vbo
				glBindBuffer(GL_ARRAY_BUFFER, model->vbo_id);

				//configure attributes
				glVertexPointer(3, GL_FLOAT, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(0));
				glTexCoordPointer(2, GL_FLOAT, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(sizeof(float)*3));
				glNormalPointer(GL_FLOAT, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(sizeof(float)*5));

				//indicate this is used now
				bound_vbo = model->vbo_id;
			}

			//loop through materials, and draw section(s) of model with this material
			for (m_loop=0; m_loop< (material_count); ++m_loop)
			{
				//pointer for less clutter
				material = &materials[m_loop].material;

				//texture enable/disable
				texture = materials[m_loop].diffusetex;
				if (texture)
				{
					if (!texture_enabled)
					{
						glEnable(GL_TEXTURE_2D);
						texture_enabled=true;
					}
					if (texture != bound_texture)
					{
						glBindTexture(GL_TEXTURE_2D, texture);
						bound_texture=texture;
					}
				}
				else if (texture_enabled)
				{
					glDisable(GL_TEXTURE_2D);
					texture_enabled=false;
				}

				//set
				glMaterialfv(GL_FRONT, GL_AMBIENT, material->ambient);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, material->diffuse);
				glMaterialfv(GL_FRONT, GL_SPECULAR, material->specular);
				glMaterialfv(GL_FRONT, GL_EMISSION, material->emission);
				glMaterialf (GL_FRONT, GL_SHININESS, material->shininess);

				//draw
				glDrawArrays(GL_TRIANGLES, materials[m_loop].start, materials[m_loop].size);
			}

		glPopMatrix();
	}

	//mark as old
	buffer_render->updated=false;
}


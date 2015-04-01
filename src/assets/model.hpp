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

#ifndef _RCX_MODEL_H
#define _RCX_MODEL_H

#include <vector>
#include <string>
#include <limits.h>

#include <GL/glew.h>
#include <ode/ode.h>

#include "assets.hpp"

//definitions:
struct Vector_Float{
	float x, y, z;
};

//each triangle is 6 indices: 3=vertices, 3=normals
//(not using texture for now)
struct Triangle_Uint
{
	unsigned int vertex[3];
	//unsigned int texcoord[3];
	unsigned int normal[3];
};

//material properties
struct Material_Float
{
	GLfloat ambient[4];
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat emission[4];
	GLfloat shininess;
};

//for indicating missing data
#define INDEX_ERROR UINT_MAX


//specialized trimesh classes:

//for rendering (node) generation
#define DEFAULT_VBO_SIZE 4194304 //usual size for trimesh VBOs
class Model_Draw: public Assets
{
	public:
		//TODO: REMOVE ALL 3!
		//will load and modify a 3d file to use only as a rendering trimesh
		static Model_Draw *Quick_Load(const char* name, float resize,
				float rotx, float roty, float rotz,
				float offx, float offy, float offz);
		//the same, but no modification needed
		static Model_Draw *Quick_Load(const char* name);
		//all data provided by conf
		static Model_Draw *Quick_Load_Conf(const char* path, const char* file);

		//creates rendering node (TODO)
		//class Node *Create_Node(class Object *obj);

	private:
		//
		//data to store:
		//

		//element (vertex+normal) for interleaved array
		struct Vertex
		{
			GLfloat x,y,z;
			GLfloat nx,ny,nz;
		};

		//material (all lements are grouped by materials for performance)
		struct Material
		{
			GLuint start; //where in vbo this material is used
			GLsizei size; //how much to render

			Material_Float material;
		};

		Model_Draw(const char* n, float r, GLuint vbo, Material* m, unsigned int mc); //constructor
		~Model_Draw(); //destructor
		friend class Model; //only Model is allowed to create this...

		//everything needed to render:
		Material *materials;
		unsigned int material_count;
		float radius; //for checking if visible or not


		//VBO and position in VBO of array:
		GLuint vbo_id; //which vbo got this model

		//only graphics list rendering can access this stuff
		friend void Render_List_Render();
};

//for collision detection (geom) generation
class Model_Mesh: public Assets
{
	public:
		//TODO: REMOVE ALL 2!
		//will load and modify a 3d file to use only as a collision trimesh
		static Model_Mesh *Quick_Load(const char* name, float resize,
				float rotx, float roty, float rotz,
				float offx, float offy, float offz);
		//the same, but no modification needed
		static Model_Mesh *Quick_Load(const char* name);

		//creates geom from trimesh
		class Geom *Create_Mesh(class Object *obj);

		//definition needed
		struct Material
		{
			char *name; //name of material
			int end; //last triangle number using this materisl
		};

	private:
		Model_Mesh(const char*, //name
				Vector_Float *v, unsigned int vcount, //vertices
				unsigned int *i, unsigned int icount, //indices
				Vector_Float *n); //normals
		~Model_Mesh();

		friend class Model; //only Model is allowed to create this...

		//
		//data for trimesh:
		//
		int triangle_count;
		int material_count;
		Material *materials;

		dTriMeshDataID data; //collected, pointers
		Vector_Float *vertices;
		unsigned int *indices;
		Vector_Float *normals; //not needed, but already calculating gives extra performance
};



//model storage class - not used during race, only while loading/processing
class Model
{
	public:

		//wrapper that decides loading function by file suffix:
		bool Load(const char*);

		//TODO:
		//void Generate_Box(...
		//void Generate_Sphere(...
		//void Generate_Cylinder(...
		//void Generate_Capsule(...
		//void Repaint(...

		//create "dedicated" (used during race) meshes from this one:
		Model_Draw *Create_Draw(); //for rendering
		Model_Mesh *Create_Mesh(); //for collision

		//TODO: remove tools (move to creation of actual assets!)
		//tools:
		void Resize(float);
		void Rotate(float,float,float);
		void Offset(float, float, float);
		//check if name matches specified
		bool Compare_Name(const char*);
	private:
		//like Load, for material files (private)
		bool Load_Material(const char*);

		//TODO: can be used for delaying loading!
		//just for the other trimesh classes (for asset name)
		std::string name;

		//tools:
		void Normalize_Normals(); //make sure normals are unit (for some loaders, like obj, maybe not...)
		void Generate_Missing_Normals(); //if loaded incomplete normals, solve
		unsigned int Find_Material(const char *); //find first matching material by name
		float Find_Longest_Distance(); //find vertex furthest from center, and return its length

		//functions for loading 3d files:
		//obj files (obj.cpp)
		bool Load_OBJ(const char *);
		bool Load_MTL(const char *); //used by obj loader
		//road files (road.cpp, custom road generator)
		bool Load_Road(const char *);
		//3ds files (3ds.cpp)
		//bool Load_3DS(const char *);

		//
		//actual data to store:
		//

		//all actual values (indexed below)
		std::vector<Vector_Float> vertices;
		//std::vector<Vector_Float> texcoords;
		std::vector<Vector_Float> normals;

		//
		//indices:
		//

		//triangles are grouped by material:

		//material:
		struct Material
		{
			std::string name;

			Material_Float material;

			//all triangles with this material
			std::vector<Triangle_Uint> triangles;
		};

		//all materials of this model
		std::vector<Material> materials;

		//default material
		static const Material Material_Default;
};

#endif

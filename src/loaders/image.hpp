/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2014 Mats Wahlberg
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

#ifndef _RC_IMAGE_H
#define _RC_IMAGE_H

#include <vector>
#include <string>
#include <limits.h>

#include <SDL/SDL_stdinc.h>
#include <GL/glew.h>
#include <ode/ode.h>

#include "shared/racetime_data.hpp"

//used to store texture version of image
class Image_Texture: public Racetime_Data
{
	public:
		GLuint GetID();

	private:
		Image_Texture(const char *name, GLuint newid);
		~Image_Texture(); //destructor

		GLuint id; //only data we need to store

		friend class Image; //Image acts as fabricator

		//only graphics list rendering can access this stuff
		friend void Render_List_Render();
};

//used to temporarily store image
class Image
{
	public:
		bool Load(const char*);

		//create "dedicated" (used during race) timeshes from this one:
		Image_Texture *Create_Texture();

		//constructor/destructor for handling allocated pixel storage
		Image();
		~Image();

	private:
		//remember name for created data
		std::string name;

		//functions for loading:
		bool Load_BMP(const char *);
		//bool Load_PNG

		//actual data to store:
		Uint8 *pixels;
		enum image_format{RGB, BGR, RGBA, BGRA} format;
		int width, height;
};

#endif

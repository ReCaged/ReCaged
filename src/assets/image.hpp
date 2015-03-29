/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2014 Mats Wahlberg
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

#ifndef _RCX_IMAGE_H
#define _RCX_IMAGE_H

#include <vector>
#include <string>
#include <limits.h>

#include <SDL/SDL_stdinc.h>
#include <GL/glew.h>
#include <ode/ode.h>

#include "assets.hpp"

//used to store texture version of image
class Image_Texture: public Assets
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

		//to allow destruction of temporary splash image texture
		friend bool Interface_Splash(const char*, int, int);
};

//used to temporarily store image
class Image
{
	public:
		//identifies file and runs appropriate loader
		bool Load(const char*);

		//info
		int Width(), Height();

		//create "dedicated" (used during race) timeshes from this one:
		Image_Texture *Create_Texture();

		//constructor/destructor for handling allocated pixel storage
		Image();
		~Image();

	private:
		//remember name for created data
		std::string name;

		//TODO: remove this? And don't assume nice bit depths! Each ROW
		//should be (stored) byte-aligned, but the actual pixels can be
		//much different (eg 1b)... read based on "rowbytes" instead!
		void Allocate();

		//functions for loading:
		bool Load_BMP(const char *);
		bool Load_PNG(const char *);
		bool Load_JPG(const char *);

		//actual data to store:
		uint8_t *pixels;
		enum image_format{RGB, BGR, RGBA, BGRA, GRAY} format;
		unsigned int width, height, components, bitdepth;
		//(componentdepth is number of bits per component and pixel)
};

#endif

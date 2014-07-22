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

//
//for vbo 3d rendering trimesh:
//
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "shared/internal.hpp"
#include "shared/log.hpp"
#include "image.hpp"


//handle pixels pointer/storage
Image::Image():
	pixels(NULL)
{
}

Image::~Image()
{
	delete[] pixels;
	pixels=NULL;
}

//may I present the main event:
bool Image::Load (const char *file)
{
	Log_Add(1, "Loading image from file \"%s\"", file);
	Log_Add(2, "determining file type from suffix");

	name = file;
	delete[] pixels;

	if (file == NULL)
	{
		Log_Add(0, "WARNING: empty file path+name for image");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		Log_Add(-1, "no suffix for file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".bmp"))
		return Load_BMP(file);

	//else, no match
	Log_Add(-1, "unknown image file suffix for \"%s\"", file);
	return false;
}

//and here's the real work for now:
bool Image::Load_BMP(const char *file)
{
	Log_Add(2, "Loading image from BMP file \"%s\"", file);

	SDL_Surface *surface = SDL_LoadBMP(file);

	if (!surface)
	{
		Log_Add(-1, "Unable to open image file \"%s\": %s", file, SDL_GetError());
		return false;
	}

	width=surface->w;
	height=surface->h;
	Log_Add(2, "resolution: %ix%i", width, height);

	if (surface->format->BytesPerPixel == 4) //32b = alpha
	{
		//check order
		if (surface->format->Rmask == 0x000000ff)
		{
			Log_Add(2, "format: RGBA, 32 bits");
			format = RGBA;
		}
		else
		{
			Log_Add(2, "format: BGRA, 32 bits");
			format = BGRA;
		}
	}
	else if (surface->format->BytesPerPixel == 3) //24b = no alpha
	{
		if (surface->format->Rmask == 0x000000ff)
		{
			Log_Add(2, "format: RGB, 24 bits");
			format = RGB;
		}
		else
		{
			Log_Add(2, "format: BGR, 24 bits");
			format = BGR;
		}
	}
	else
	{
		Log_Add(-1, "Unsupported image format in \"%s\"!", file);
		SDL_FreeSurface(surface);
		return false;
	}
 
	//copy:
	size_t size=surface->pitch*surface->h;
	pixels = new Uint8[size];
	memcpy(pixels, surface->pixels, sizeof(Uint8)*size);
	SDL_FreeSurface(surface);

	return true;
}
Image_Texture *Image::Create_Texture()
{
	Log_Add(2, "Creating texture from image");

	//heigh, width power of two?
	if ( (width == 0) || width & (width-1) )
	{
		Log_Add(-1, "Unsupported image width (%i) in \"%s\" (not power of 2)!", width, name.c_str());
		return NULL;
	}
 
	if ( (height == 0) || height & (height-1) )
	{
		Log_Add(-1, "Unsupported image height (%i) in \"%s\" (not power of 2)!", height, name.c_str());
		return NULL;
	}

	//what format?
	GLenum pixel_format, internal_format;

	switch (format)
	{
		case RGBA:
			pixel_format=GL_RGBA;
			internal_format=GL_RGBA8;
			break;
		case BGRA:
			pixel_format=GL_BGRA;
			internal_format=GL_RGBA8;
			break;
		case RGB:
			pixel_format=GL_RGB;
			internal_format=GL_RGB8; //or RGBA8?
			break;
		case BGR:
			pixel_format=GL_BGR;
			internal_format=GL_RGB8; //or RGBA8?
			break;
		default:
			Log_Add(-1, "Unsupported image format in \"%s\"!", name.c_str());
			return NULL;
			break;
	}

	//create:
	Log_Add(2, "Generating gpu texture");
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	//TODO: add more options
	switch (internal.filter)
	{
		case 1:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;

		default:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
	}

	//upload:
	Log_Add(2, "Uploading texture to gpu");
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, pixel_format, GL_UNSIGNED_BYTE, pixels);

	return new Image_Texture(name.c_str(), id);
}

//constructor for setting up
Image_Texture::Image_Texture(const char *name, GLuint newid):
	Racetime_Data(name), id(newid)
{
}

//access id number
GLuint Image_Texture::GetID()
{
	return id;
}

//remove texture
Image_Texture::~Image_Texture()
{
	Log_Add(2, "Removing texture image");
	glDeleteTextures(1, &id);
}


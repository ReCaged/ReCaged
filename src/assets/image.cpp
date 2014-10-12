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
#include <jpeglib.h>
#include <setjmp.h> //for insanity of jpeg/png libraries
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
	Log_Add(2, "Loading image from file \"%s\" (identifying suffix)", file);

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
	else if (!strcasecmp(suffix, ".jpg") || !strcasecmp(suffix, ".jpeg"))
		return Load_JPG(file);

	//else, no match
	Log_Add(-1, "unknown image file suffix for \"%s\"", file);
	return false;
}

//just so not forgetting the message output when allocating
void Image::Allocate()
{
	//Ugly, yes. For now it's assumed that componentdepth is multiple of 8
	size_t size=(width*height*components*componentdepth)/8;

	Log_Add(2, "Allocating image buffer of %u bytes", size);
	pixels = new uint8_t[size];
}

//loaders:
//BMP:
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

	//TODO: indexed BMPs!
	if (surface->format->BytesPerPixel == 4) //32b = alpha
	{
		components=4;
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
		components=3;
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

	//assumes 8 bits per component and pixel (so 24 or 32 bits)
	componentdepth=8;
	//TODO: else {surface->pitch when not using whole bytes for pixels...}

	//copy:
	Allocate();
	memcpy(pixels, surface->pixels, sizeof(uint8_t)*height*surface->pitch);
	SDL_FreeSurface(surface);

	return true;
}

//JPEG:

//error handling (as in example.c from jpeglib)
struct my_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
	struct my_error_mgr *err=(struct my_error_mgr *) cinfo->err;
	char errormsg[JMSG_LENGTH_MAX];
	( *(cinfo->err->format_message) ) (cinfo, errormsg);
	Log_Add(-1, "Error reading JPEG: %s", errormsg);
	longjmp(err->setjmp_buffer, 1);
}

bool Image::Load_JPG(const char *file)
{
	Log_Add(2, "Loading image from JPEG file \"%s\"", file);

	FILE *fp=fopen(file, "rb");

	if (!fp)
	{
		Log_Add(-1, "Unable to open image file \"%s\"", file);
		return false;
	}

	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	//set up the (insane, sigh) error handling used by libjpeg...
	cinfo.err=jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit=my_error_exit;

	//creepy voodoo stuff...
	if (setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
		return false;
	}

	//create (destroy in error jmp, or when finished)
	jpeg_create_decompress(&cinfo);

	//read from fp
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	//metadata to store
	width=cinfo.output_width;
	height=cinfo.output_height;

	if (cinfo.num_components == 4)
	{
		components=4;
		format=RGBA;
	}
	else if (cinfo.num_components == 3)
	{
		components=3;
		format=RGB;
	}
	else
	{
		//obviously something to support for the future (grayscale!)
		Log_Add(-1, "Unsupported image number of components (%i) in \"%s\" (not 3 or 4)!", cinfo.num_components, name.c_str());
		return false;
	}

	componentdepth=BITS_IN_JSAMPLE; //not 100% sure this is right...

	//try to allocate memory
	Allocate();

	//read, line by line
	uint8_t *ppointer; //libjpeg wants pointer to pointer...
	int scanlinebytes=width*components*componentdepth/8; //mmm... should be right?...
	while (cinfo.output_scanline < cinfo.output_height)
	{
		ppointer=pixels +cinfo.output_scanline*scanlinebytes;
		jpeg_read_scanlines(&cinfo, &ppointer, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(fp);

	return true;
}

//fabricators:
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
	GLenum pixel_format, internal_format, pixel_type;

	//note: RGBA8 is suggested by the opengl wiki, might want to look over
	//this when adding grayscale, floating point and boolean textures...
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
			internal_format=GL_RGB8; //or RGBA?
			break;
		case BGR:
			pixel_format=GL_BGR;
			internal_format=GL_RGB8; //or RGBA?
			break;
		default:
			Log_Add(-1, "Unsupported image format in \"%s\"!", name.c_str());
			return NULL;
			break;
	}

	if (componentdepth==8)
		pixel_type=GL_UNSIGNED_BYTE;
	/*else if (componentdepth==16)
		pixel_type=GL_UNSIGNED_SHORT;*/
	else
	{
		Log_Add(-1, "Unsupported image component depth (%i) in \"%s\" (not 8 bits)!", componentdepth, name.c_str());
		return NULL;
	}

	//create:
	Log_Add(2, "Generating gpu texture (might use about %i bytes of video ram)", width*height*4); //most cards internally converts everything to RGBA8
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
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, pixel_format, pixel_type, pixels);

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


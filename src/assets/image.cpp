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

//
//for vbo 3d rendering trimesh:
//
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <png.h>
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
	else if (!strcasecmp(suffix, ".png"))
		return Load_PNG(file);
	else if (!strcasecmp(suffix, ".jpg") || !strcasecmp(suffix, ".jpeg"))
		return Load_JPG(file);

	//else, no match
	Log_Add(-1, "unknown image file suffix for \"%s\"", file);
	return false;
}

//just so not forgetting the message output when allocating
void Image::Allocate()
{
	//Ugly, yes. For now it's assumed that bitdepth is multiple of 8
	size_t size=(width*height*components*bitdepth)/8;

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
	bitdepth=8;
	//TODO: else {surface->pitch when not using whole bytes for pixels...}

	//copy:
	Allocate();
	memcpy(pixels, surface->pixels, sizeof(uint8_t)*height*surface->pitch);
	SDL_FreeSurface(surface);

	return true;
}


//
//PNG:
//

//error handling...
void user_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
	Log_Add(-1, "Error loading PNG: \"%s\"", error_msg);
	longjmp(png_jmpbuf(png_ptr), 1);
}

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
	Log_Add(-1, "Warning when loading PNG: \"%s\"", warning_msg);
}

bool Image::Load_PNG(const char *file)
{
	Log_Add(2, "Loading image from PNG file \"%s\"", file);

	FILE *fp=fopen(file, "rb");

	if (!fp)
	{
		Log_Add(-1, "Unable to open image file \"%s\"", file);
		return false;
	}

	uint8_t header[8];
	size_t headerbytes=fread(header, 1, 8, fp);
	if ( (headerbytes==0) || png_sig_cmp(header, 0, headerbytes))
	{
		Log_Add(-1, "File type of \"%s\" is not PNG", file);
		fclose (fp);
		return false;
	}

	//start the fun stuff
	
	//initialization:
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, user_error_fn, user_warning_fn);
	if (!png_ptr)
	{
		Log_Add(-1, "Unable to create PNG read structure");
		fclose (fp);
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		Log_Add(-1, "Unable to create PNG info structure");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose (fp);
		return false;
	}

	//error jump...
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		Log_Add(-1, "Error loading PNG");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return false;
	}

	//set io method
	png_init_io(png_ptr, fp);

	//compensate for the already read bytes (earlier signature check)
	png_set_sig_bytes(png_ptr, headerbytes);

	//time to read... using the low-level functions...
	png_read_info(png_ptr, info_ptr);

	//image properties
	png_uint_32 pngwidth, pngheight;
	int pngdepth, pngtype;
	png_get_IHDR(png_ptr, info_ptr, &pngwidth, &pngheight,
			&pngdepth, &pngtype, NULL, NULL, NULL);

	//temporary safetycheck...
	if (pngwidth==0 || pngheight==0 || (pngdepth!=8 && pngdepth!=16) )
	{
		Log_Add(-1, "Unsupported format in \"%s\"!", name.c_str());
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return false;
	}

	//convert to our simple formats:
	width=pngwidth;
	height=pngheight;
	bitdepth=pngdepth;
	switch (pngtype)
	{
		case PNG_COLOR_TYPE_RGB_ALPHA:
			components=4;
			format=RGBA;
			break;

		case PNG_COLOR_TYPE_RGB:
			components=3;
			format=RGB;
			break;

		case PNG_COLOR_TYPE_GRAY:
			components=1;
			format=GRAY;
			break;

		//TODO! Much more to try here, bitfield, perhaps indixed pngs
		//needs special handling too? Some other day...
		default:
			Log_Add(-1, "Unsupported image colour type (%i) in \"%s\" (not RGB or RGBA)!", pngtype, name.c_str());
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			return false;
			break;
	}

	//load
	Allocate();

	//png_read_update_info(png_ptr, info_ptr); not needed?
	//libpng expects an array of pointers to rows (like libjpeg, but this
	//is more important: necessary in case the png is interlaced)
	png_bytep *row_pointers = new png_bytep[height];

	//not sure this is the best solution...
	int rowbytes=width*components*bitdepth/8;

	//point to pixel data storage
	for (int i=0; i<height; ++i)
		row_pointers[i]=pixels+rowbytes*i;

	png_read_image(png_ptr, row_pointers); //and just like that...

	delete[] row_pointers;

	//ah, now just clean up
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);

	return true;
}


//
//JPEG:
//

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
	Log_Add(-1, "Error loading JPEG: %s", errormsg);
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

	switch (cinfo.num_components)
	{
		case 4:
			components=4;
			format=RGBA;
			break;

		case 3:
			components=3;
			format=RGB;
			break;

		case 1:
			components=1;
			format=GRAY;
			break;

		//obviously more to support in the future! Possibly other
		//formats, maybe indexed are different?)
		default:
			Log_Add(-1, "Unsupported image number of components (%i) in \"%s\" (not 3 or 4)!", cinfo.num_components, name.c_str());
			jpeg_destroy_decompress(&cinfo);
			fclose(fp);
			return false;
			break;
	}

	bitdepth=BITS_IN_JSAMPLE; //not 100% sure this is right...

	//try to allocate memory
	Allocate();

	//read, line by line
	uint8_t *ppointer; //libjpeg wants pointer to pointer...
	int rowbytes=width*components*bitdepth/8; //mmm... should be right?...
	while (cinfo.output_scanline < cinfo.output_height)
	{
		ppointer=pixels +cinfo.output_scanline*rowbytes;
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
			internal_format=GL_RGB8; //or RGBA8? Driver may promote to RGBA8 internally anyway...
			break;
		case BGR:
			pixel_format=GL_BGR;
			internal_format=GL_RGB8; //or RGBA8? -''-
			break;
		case GRAY:
			pixel_format=GL_LUMINANCE;
			internal_format=GL_RGB8; //or RGBA8? -''-
			break;
		default:
			Log_Add(-1, "Unsupported image format in \"%s\"!", name.c_str());
			return NULL;
			break;
	}

	if (bitdepth==8)
		pixel_type=GL_UNSIGNED_BYTE;
	else if (bitdepth==16)
		pixel_type=GL_UNSIGNED_SHORT;
	else
	{
		Log_Add(-1, "Unsupported image component bit depth (%i) in \"%s\" (not 8 or 16 bits)!", bitdepth, name.c_str());
		return NULL;
	}

	//create:

	//(most cards internally converts everything to RGBA8)
	Log_Add(2, "Generating gpu texture (might use about %i bytes of video ram)", width*height*4);

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


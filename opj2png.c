/**
    jp2png-cgi
    Copyright (C) 2013  René van der Ark

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <string.h>
#include <openjpeg.h>
#include "opj_res.h"
#include "opj2png.h"


inline void setRGB(png_byte *ptr, int r, int g, int b) {
	ptr[0] = r; ptr[1] = g; ptr[2] = b;
}

int writePNG(struct opj_res *res, char *title) {
	int code = 0;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row;

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		error_callback("Could not allocate write struct\n", NULL);
		code = 1;
		goto finalise;
	}

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		error_callback("Could not allocate info struct\n", NULL);
		code = 1;
		goto finalise;
	}

	// Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		error_callback("Error during png creation\n", NULL);
		code = 1;
		goto finalise;
	}


	png_init_io(png_ptr, stdout);


	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, res->image->comps[0].w, res->image->comps[0].h,
			8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);


	// Set title
	if (title != NULL) {
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = "Title";
		title_text.text = title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);
	}

	png_write_info(png_ptr, info_ptr);

	// Allocate memory for one row (3 bytes per pixel - RGB)
	row = (png_bytep) malloc(3 * res->image->comps[0].w * sizeof(png_byte));

	// Write image data
	int x, y;
	for (y=0 ; y < res->image->comps[0].h ; y++) {
		for (x=0 ; x < res->image->comps[0].w ; x++) {
			int i = y * res->image->comps[0].w + x;
			setRGB(&(row[x*3]), res->image->comps[0].data[i], res->image->comps[1].data[i], res->image->comps[2].data[i]);
		}
		png_write_row(png_ptr, row);
	}

	// End write
	png_write_end(png_ptr, NULL);

	finalise:
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row);

	return code;
}


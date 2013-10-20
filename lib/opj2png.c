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

// PNG code: http://www.labbookpages.co.uk

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <string.h>
#include <jpeglib.h>
#include <openjpeg.h>
#include "opj_res.h"
#include "opj2png.h"


static void setRGB(png_byte *ptr, int r, int g, int b) {
	ptr[0] = r; ptr[1] = g; ptr[2] = b;
}

int writePNG(struct opj_res *res, char *title, unsigned xPos, unsigned yPos, unsigned w, unsigned h, unsigned num_comps) {
	int code = 0;

	if(w == 0) { w = res->image->comps[0].w; }
	if(h == 0) { h = res->image->comps[0].h; }
	if(xPos >= res->image->comps[0].w) { xPos = 0; }
	if(yPos >= res->image->comps[0].h) { yPos = 0; }
	if(xPos + w >= res->image->comps[0].w) { w =  res->image->comps[0].w - xPos; }
	if(yPos + h >= res->image->comps[0].h) { h =  res->image->comps[0].h - yPos; }

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		code = 1;
		goto finalise;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		code = 1;
		goto finalise;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		code = 1;
		goto finalise;
	}


	png_init_io(png_ptr, stdout);


	png_set_IHDR(png_ptr, info_ptr, w, h,
			8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);


	if (title != NULL) {
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = "Title";
		title_text.text = title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);
	}

	png_write_info(png_ptr, info_ptr);

	row = (png_bytep) malloc(3 * w * sizeof(png_byte));

	
	unsigned x, y;
	for (y = yPos ; y < yPos + h ; y++) {
		for (x = xPos ; x < xPos + w ; x++) {
			int i = y * res->image->comps[0].w + x;
			if(num_comps < 3) {
				setRGB(&(row[(x-xPos)*3]), res->image->comps[0].data[i], res->image->comps[0].data[i], res->image->comps[0].data[i]);
			} else {
				setRGB(&(row[(x-xPos)*3]), res->image->comps[0].data[i], res->image->comps[1].data[i], res->image->comps[2].data[i]);
			}
		}
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, NULL);

	finalise:
	if (info_ptr != NULL) {
		png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
		free(info_ptr);
	}
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row);

	return code;
}


int writeJPEG(struct opj_res *res, unsigned xPos, unsigned yPos, unsigned w, unsigned h, unsigned num_comps) {
        int code = 0;

        if(w == 0) { w = res->image->comps[0].w; }
        if(h == 0) { h = res->image->comps[0].h; }
        if(xPos >= res->image->comps[0].w) { xPos = 0; }
        if(yPos >= res->image->comps[0].h) { yPos = 0; }
        if(xPos + w >= res->image->comps[0].w) { w =  res->image->comps[0].w - xPos; }
        if(yPos + h >= res->image->comps[0].h) { h =  res->image->comps[0].h - yPos; }

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, stdout);

        cinfo.image_width      = w;
        cinfo.image_height     = h;
	if(num_comps < 3)  {
		cinfo.in_color_space = JCS_GRAYSCALE; 
        	cinfo.input_components = 1;
	} else { 
		cinfo.in_color_space   = JCS_RGB; 
        	cinfo.input_components = 3;
	}
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality (&cinfo, 100, 1);
        jpeg_start_compress(&cinfo, 1);

	JSAMPLE rgb[w*num_comps];
        JSAMPROW row_pointer[1];

        while (cinfo.next_scanline < cinfo.image_height) {
	        unsigned x;
                for (x = xPos ; x < xPos + w ; x++) {
                        int i = (cinfo.next_scanline + yPos) * res->image->comps[0].w + x;
                        if(num_comps < 3) {
				rgb[x-xPos] = res->image->comps[0].data[i];
                        } else {
				rgb[(x-xPos)*num_comps] = res->image->comps[0].data[i];
				rgb[(x-xPos)*num_comps+1] = res->image->comps[1].data[i];
				rgb[(x-xPos)*num_comps+2] = res->image->comps[2].data[i];
                        }
                }
                row_pointer[0] = (JSAMPROW) & rgb;
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

	return code;
}

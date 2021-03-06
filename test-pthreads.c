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
#include <pthread.h>
#include <openjpeg.h>
#include <string.h>
#include <math.h>
#include <jpeglib.h>
#include "lib/urldecode.h"
#include "lib/url2cache.h"
#include "lib/log.h"
#include "lib/opj_res.h"
#include "lib/opj2png.h"

#define SPAWN_THREADS 7
#define MAX_ARGS 6

typedef struct urlparams {
	int reduction_factor;
	char *url;
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
	unsigned quality;
	unsigned short write_header;
	FILE *fp;
} urlparams_t;

typedef struct chunked_scanline {
    JSAMPLE *rgb;
	unsigned n_chunks;
	unsigned chunks_done;
} chunked_scanline_t;

typedef struct shared_image_resource {
	unsigned tiles_done;
	unsigned tilesX;
	unsigned tilesY;
	unsigned sourceTilesX;
	unsigned num_comps;
	unsigned x0;
	unsigned y0;
	unsigned x1;
	unsigned y1;
	OPJ_UINT32 tw;
	OPJ_UINT32 th;

	chunked_scanline_t *scanlines;

} shared_image_resource_t;

typedef struct thr_arg {
	opj_dparameters_t *params;
	shared_image_resource_t *shared_resource;
	unsigned *tiles;
    unsigned n_tiles;
	char *cachefile;
} thr_args_t;



static void buffer_scanlines(opj_image_t *image, unsigned tile_index, shared_image_resource_t *shared_resource) {
	unsigned tileY = tile_index / shared_resource->sourceTilesX;
	unsigned tileX = tile_index - (shared_resource->sourceTilesX * tileY);
	unsigned xPos = tileX * shared_resource->tw;
	unsigned yPos = tileY * shared_resource->th;
	unsigned x, y;
/*	fprintf(stderr,"Writing scanlines for x0/y0/x1/y1: %d/%d/%d/%d\n", xPos, yPos,xPos + image->comps[0].w,yPos + image->comps[0].h);*/
	for(y = yPos; y < yPos + image->comps[0].h && y < shared_resource->y1; y++) {
		int yOut = y - shared_resource->y0;
		if(yOut < 0) { continue; }
		for(x = xPos; x < xPos + image->comps[0].w && x < shared_resource->x1; x++) {
			unsigned pix = (x - xPos) + ((y - yPos) * image->comps[0].w);
			int xOut = x - shared_resource->x0;
			if(xOut < 0) { continue; }
			if(shared_resource->num_comps < 3) {
				shared_resource->scanlines[yOut].rgb[xOut] = image->comps[0].data[pix];
			} else {
				shared_resource->scanlines[yOut].rgb[xOut*shared_resource->num_comps] = image->comps[0].data[pix];
				shared_resource->scanlines[yOut].rgb[xOut*shared_resource->num_comps+1] = image->comps[1].data[pix];
				shared_resource->scanlines[yOut].rgb[xOut*shared_resource->num_comps+2] = image->comps[2].data[pix];
			}
		}
		shared_resource->scanlines[y].chunks_done++;
	}

}

static void *processTile(void *args) {
		unsigned *tiles = ((thr_args_t*)args)->tiles;
		unsigned n_tiles = ((thr_args_t*)args)->n_tiles;
		unsigned tile_index, i;
		for(i = 0; i < n_tiles; i++) {
			tile_index = tiles[i];
			opj_dparameters_t *params = ((thr_args_t*)args)->params;
			opj_res_t res = opj_init(((thr_args_t*)args)->cachefile, params);
			opj_get_decoded_tile(res.l_codec, res.l_stream, res.image, tile_index);
			buffer_scanlines(res.image, tile_index, ((thr_args_t*)args)->shared_resource);
/*			char foo[40];
			sprintf(foo, "/var/cache/jp2/tile-%d.jpg", tile_index);
			FILE *fp = fopen(foo, "wb");
			writeJPEG(&res, 0, 0, 0, 0, 3, fp);
			fclose(fp);*/
			opj_cleanup(&res);
		}
		((thr_args_t*)args)->shared_resource->tiles_done++;
		free(((thr_args_t*)args)->tiles);
		pthread_exit(NULL);
}

static OPJ_UINT32 reduce( OPJ_UINT32 val, OPJ_UINT32 redux ) {
	double v = (double) val;
	while(redux-- > 0) {
		v /= 2.0;
	}
	return (OPJ_UINT32) ceil(v);
}

static void init_params(urlparams_t *params) {
	params->reduction_factor = 0;
	params->x = 0;
	params->y = 0;
	params->w = 0;
	params->h = 0;
	params->quality = 100;
	params->url = NULL;
	params->fp = stdout;
	params->write_header = 1;
}

static void parseParam(char k, char *v, urlparams_t *p) {
	switch(k) {
		case 'r': p->reduction_factor = strtol(v, NULL, 0); return;
		case 'x': p->x = strtol(v, NULL, 0); return;
		case 'y': p->y = strtol(v, NULL, 0); return;
		case 'w': p->w = strtol(v, NULL, 0); return;
		case 'h': p->h = strtol(v, NULL, 0); return;
		case 'q': p->quality = strtol(v, NULL, 0); return;
		case 'u': p->url = url_decode(v); return;
		case 'o': p->write_header = 0; p->fp = fopen(url_decode(v), "wb"); return;
		default: return;
	}
}

static void parse_query(urlparams_t *p, char *qstr) {
	init_params(p);

	int i = 0;
	int j;
	char *args[MAX_ARGS];
	args[i] = strtok(qstr, "&");
	while(args[i++] && i < MAX_ARGS) {
		args[i] = strtok(NULL, "&");
	}
	
	for(j = 0; j < i; j++) {
		char *k = strtok(args[j], "=");
		char *v = strtok(NULL, "=");
		if(!k) { break; }
		parseParam(k[0], v, p);
	}
}

static int get_decoded_area(urlparams_t *urlparams, shared_image_resource_t *shared_resource) {
	char *cachedir = CACHEDIR;
	char *cachefile;
	if(urlparams->url == NULL) { return 1; }
	if(getenv("JP2_CACHEDIR") != NULL)  { cachedir = getenv("JP2_CACHEDIR"); }

	cachefile = download_to_cache(urlparams->url, cachedir);

	if(cachefile == NULL) { return 2; }

	opj_res_t decoder_resources;
	opj_dparameters_t decoder_parameters;

	opj_set_default_decoder_parameters(&decoder_parameters);
	decoder_parameters.cp_reduce = urlparams->reduction_factor;
	decoder_parameters.cp_layer = urlparams->quality;
	

	decoder_resources = opj_init(cachefile, &decoder_parameters);
	opj_codestream_info_v2_t* info = opj_get_cstr_info(decoder_resources.l_codec);

	unsigned y1_resized = reduce(decoder_resources.image->y1, decoder_parameters.cp_reduce);
	unsigned x1_resized = reduce(decoder_resources.image->x1, decoder_parameters.cp_reduce);
	if(urlparams->h == 0) { urlparams->h = y1_resized; }
	if(urlparams->w == 0) { urlparams->w = x1_resized; }

	shared_resource->y0 = urlparams->y;
	shared_resource->x0 = urlparams->x; 
	shared_resource->y1 = urlparams->h + urlparams->y;
	shared_resource->x1 = urlparams->w + urlparams->x;

	if(shared_resource->x1 > x1_resized) { shared_resource->x1 = x1_resized; }
	if(shared_resource->y1 > y1_resized) { shared_resource->y1 = y1_resized; }

	shared_resource->tw = reduce(info->tdx, decoder_parameters.cp_reduce);
	shared_resource->th = reduce(info->tdy, decoder_parameters.cp_reduce);
	shared_resource->tilesX = (unsigned) ceil((double)shared_resource->x1 / (double)shared_resource->tw);
	shared_resource->tilesY = (unsigned) ceil((double)shared_resource->y1 / (double)shared_resource->th);
	shared_resource->sourceTilesX = info->tw;

	unsigned tileX = (unsigned) floor((double) urlparams->x / (double)shared_resource->tw);
	unsigned tileY = (unsigned) floor((double) urlparams->y / (double)shared_resource->th);

	if(shared_resource->tilesX + tileX > info->tw) { shared_resource->tilesX = info->tw - tileX; }
	if(shared_resource->tilesY + tileY > info->th) { shared_resource->tilesY = info->th - tileY; }

	unsigned n = 0;
	pthread_t t[SPAWN_THREADS];
	thr_args_t args[SPAWN_THREADS];

	shared_resource->tiles_done = 0;
	shared_resource->num_comps = decoder_resources.image->numcomps;
	shared_resource->scanlines = malloc(sizeof(chunked_scanline_t) * (shared_resource->y1+1));

	unsigned y;
	for(y = 0; y < shared_resource->y1; y++) {
		shared_resource->scanlines[y].chunks_done = 0;
		shared_resource->scanlines[y].n_chunks = shared_resource->x1 / shared_resource->tw;
		shared_resource->scanlines[y].rgb = malloc(sizeof(JSAMPLE) * (shared_resource->x1+1) * shared_resource->num_comps);
	}



	unsigned cur_thread, x;
	unsigned tiles_per_thread = 1 + ((shared_resource->tilesX * shared_resource->tilesY) / SPAWN_THREADS);
	for(n = 0; n < SPAWN_THREADS; n++) {
		args[n].shared_resource = shared_resource;
		args[n].cachefile = cachefile;
		args[n].params = &decoder_parameters;
		args[n].n_tiles = 0;
		args[n].tiles = malloc(sizeof(unsigned) * tiles_per_thread);
	}

	for(cur_thread = 0, y = tileY; y < tileY + shared_resource->tilesY; y++) {
		for(x = tileX; x < tileX + shared_resource->tilesX; x++) {
			unsigned tile_index = x + (y * info->tw);
			args[cur_thread].tiles[args[cur_thread].n_tiles++] = tile_index;
			if(++cur_thread == SPAWN_THREADS) { cur_thread = 0; }
		}
	}

	opj_destroy_cstr_info(&info);
	opj_cleanup(&decoder_resources);

	for(n = 0; n < SPAWN_THREADS; n++) {
		char distrib_info[50];
		sprintf(distrib_info, "tiles alotted to thread %d:", n);
		log_debug(distrib_info);
		unsigned i;
		for(i = 0; i < args[n].n_tiles; i++) {
			fprintf(stderr, "%d ", args[n].tiles[i]);
		}
		fprintf(stderr, "\n");
		pthread_create(&t[n], NULL, processTile, &args[n]);
	}

	while(shared_resource->tiles_done < SPAWN_THREADS) {  }

	free(cachefile);
	return 0;
}

static int encodeJPEG(urlparams_t *urlparams, shared_image_resource_t *shared_resource) {

	FILE *fp = urlparams->fp;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = shared_resource->x1 - shared_resource->x0;
	cinfo.image_height = shared_resource->y1 - shared_resource->y0;
	if(shared_resource->num_comps < 3)	{
		cinfo.in_color_space = JCS_GRAYSCALE;
		cinfo.input_components = 1;
	} else {
		cinfo.in_color_space	= JCS_RGB;
		cinfo.input_components = 3;
	}

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality (&cinfo, urlparams->quality, 1);
	jpeg_start_compress(&cinfo, 1);
	JSAMPROW row_pointer[1];
	JSAMPLE rgb[shared_resource->x1 * shared_resource->num_comps];
	unsigned x,y,i;
	for(y = 0; y < shared_resource->y1; y++) {
		for(x = 0; x < shared_resource->x1; x++) {
			for(i = 0; i < shared_resource->num_comps; i++) {
				rgb[x*shared_resource->num_comps+i] = shared_resource->scanlines[y].rgb[x*shared_resource->num_comps+i];
			}
		}
		row_pointer[0] = (JSAMPROW) &rgb;
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);

		free(shared_resource->scanlines[y].rgb);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	

	free(shared_resource->scanlines);
	fclose(fp);
	return 0;
}

int main(void) {
	urlparams_t urlparams;
	shared_image_resource_t shared_resource;


	parse_query(&urlparams, getenv("QUERY_STRING"));
	log_debug("start decoding");
	int status = get_decoded_area(&urlparams, &shared_resource);
	if(status > 0)  {
		puts("Content-type: text/plain"); 
		puts("Status: 500 Internal Server Error\n");	
		puts("Internal Server Error");
		return 1;
	}

	log_debug("start encoding");
	if(urlparams.write_header) {
		puts("Content-type: image/jpeg"); 
		puts("Pragma: public");
		puts("Cache-Control: max-age=360000");
		puts("Status: 200 OK\n");
	}
	status = encodeJPEG(&urlparams, &shared_resource);
	log_debug("done");
	pthread_exit(NULL);
	return 0;
}

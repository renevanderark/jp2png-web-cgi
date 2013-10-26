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
#include <sys/time.h>
#include <jpeglib.h>
#include "lib/urldecode.h"
#include "lib/url2cache.h"
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
	unsigned num_comps;
	unsigned x1;
	unsigned y1;
	OPJ_UINT32 tw;
	OPJ_UINT32 th;

	chunked_scanline_t *scanlines;

} shared_image_resource_t;

typedef struct thr_arg {
		opj_dparameters_t *params;
		shared_image_resource_t *tres;
		unsigned tile_index_start;
		unsigned tile_index_end;
		char *cachefile;
} thr_args_t;



static void buffer_scanlines(opj_image_t *image, unsigned tile_index, shared_image_resource_t *tres) {
	unsigned tileY = tile_index / tres->tilesX;
	unsigned tileX = tile_index - (tres->tilesX * tileY);
	unsigned xPos = tileX * tres->tw;
	unsigned yPos = tileY * tres->th;
	unsigned x, y, i = 0;
	
	for(y = yPos; y < yPos + image->comps[0].h; y++) {
		for(x = xPos; x < xPos + image->comps[0].w; x++, i++) {
			if(tres->num_comps < 3) {
				tres->scanlines[y].rgb[x] = image->comps[0].data[i];
			} else {
				tres->scanlines[y].rgb[x*tres->num_comps] = image->comps[0].data[i];
				tres->scanlines[y].rgb[x*tres->num_comps+1] = image->comps[1].data[i];
				tres->scanlines[y].rgb[x*tres->num_comps+2] = image->comps[2].data[i];
			}
		}
		tres->scanlines[y].chunks_done++;
	}
}

static void *processTile(void *args) {
		unsigned tile_index_start = ((thr_args_t*)args)->tile_index_start;
		unsigned tile_index_end = ((thr_args_t*)args)->tile_index_end;
		unsigned tile_index;
		for(tile_index = tile_index_start; tile_index < tile_index_end; tile_index++) {
			opj_dparameters_t *params = ((thr_args_t*)args)->params;
			opj_res_t res = opj_init(((thr_args_t*)args)->cachefile, params);
			opj_get_decoded_tile(res.l_codec, res.l_stream, res.image, tile_index);
			buffer_scanlines(res.image, tile_index, ((thr_args_t*)args)->tres);
			opj_cleanup(&res);
		}
		((thr_args_t*)args)->tres->tiles_done++;
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

static int get_decoded_area(urlparams_t *urlparams, shared_image_resource_t *tres) {
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

	tres->tilesX = info->tw;
	tres->tilesY = info->th;
	tres->tw = reduce(info->tdx, decoder_parameters.cp_reduce);
	tres->th = reduce(info->tdy, decoder_parameters.cp_reduce);

	tres->y1 = reduce(decoder_resources.image->y1, decoder_parameters.cp_reduce);
	tres->x1 = reduce(decoder_resources.image->x1, decoder_parameters.cp_reduce);

	unsigned n = 0;
	pthread_t t[SPAWN_THREADS];
	thr_args_t args[SPAWN_THREADS];

	tres->tiles_done = 0;
	tres->num_comps = decoder_resources.image->numcomps;
	tres->scanlines = malloc(sizeof(chunked_scanline_t) * (tres->y1+1));

	unsigned y;
	for(y = 0; y < tres->y1; y++) {
		tres->scanlines[y].chunks_done = 0;
		tres->scanlines[y].n_chunks = tres->x1 / tres->tw;
		tres->scanlines[y].rgb = malloc(sizeof(JSAMPLE) * (tres->x1+1) * tres->num_comps);
	}

	opj_destroy_cstr_info(&info);
	opj_cleanup(&decoder_resources);

	for(n = 0; n < SPAWN_THREADS; n++) {
		args[n].tres = tres;
		args[n].cachefile = cachefile;
		args[n].tile_index_start = (n * tres->tilesX * tres->tilesY) / SPAWN_THREADS;
		args[n].tile_index_end = ((n+1) * tres->tilesX * tres->tilesY) / SPAWN_THREADS;
		args[n].params = &decoder_parameters;
		pthread_create(&t[n], NULL, processTile, &args[n]);
	}

	while(tres->tiles_done < SPAWN_THREADS) {  }

	free(cachefile);
	return 0;
}

int main(void) {
	urlparams_t urlparams;
	shared_image_resource_t shared_resource;


	parse_query(&urlparams, getenv("QUERY_STRING"));
	(void) get_decoded_area(&urlparams, &shared_resource);

	if(urlparams.write_header) {
		puts("Content-type: image/jpeg"); 
		puts("Pragma: public");
		puts("Cache-Control: max-age=360000");
		puts("Status: 200 OK\n");
	}




	FILE *fp = urlparams.fp;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr		jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = shared_resource.x1;
	cinfo.image_height = shared_resource.y1;
	if(shared_resource.num_comps < 3)	{
		cinfo.in_color_space = JCS_GRAYSCALE;
		cinfo.input_components = 1;
	} else {
		cinfo.in_color_space	= JCS_RGB;
		cinfo.input_components = 3;
	}

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality (&cinfo, urlparams.quality, 1);
	jpeg_start_compress(&cinfo, 1);
	JSAMPROW row_pointer[1];
	JSAMPLE rgb[shared_resource.x1 * shared_resource.num_comps];
	unsigned x,y,i;
	for(y = 0; y < shared_resource.y1; y++) {
		for(x = 0; x < shared_resource.x1; x++) {
			for(i = 0; i < shared_resource.num_comps; i++) {
				rgb[x*shared_resource.num_comps+i] = shared_resource.scanlines[y].rgb[x*shared_resource.num_comps+i];
			}
		}
		row_pointer[0] = (JSAMPROW) &rgb;
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);

		free(shared_resource.scanlines[y].rgb);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	

	free(shared_resource.scanlines);
	fclose(fp);
	fprintf(stderr, "done doing\n");
	pthread_exit(NULL);
	return 0;
}

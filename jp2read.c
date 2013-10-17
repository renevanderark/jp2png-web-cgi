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
#include <openjpeg.h>
#include <string.h>
#include <time.h>
#include <png.h>
#include "lib/url2cache.h"
#include "lib/opj_res.h"
#include "lib/opj2png.h"
#include "lib/urldecode.h"

typedef enum {GET_HEADER, READ_TILE, READ_IMAGE} operation_t;



#define MAX_ARGS 9

struct params {
	int tile_index;
	int reduction_factor;
	char *filename;
	char *url;
	char *jsonp_callback;
	operation_t operation;
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
	unsigned num_comps;
};

static struct params *init_params(void) {
	struct params *p = malloc(sizeof(struct params));
	p->tile_index = 0;
	p->reduction_factor = 0;
	p->x = 0;
	p->y = 0;
	p->w = 0;
	p->h = 0;
	p->num_comps = 3;
	p->operation = GET_HEADER;
	p->filename = NULL;
	p->url = NULL;
	p->jsonp_callback = NULL;
	return p;
}

static void parseParam(char k, char *v, struct params *p) {
	switch(k) {
		case 'i': p->operation = READ_IMAGE; return;
		case 't': p->tile_index = strtol(v, NULL, 0); p->operation = READ_TILE; return;
		case 'r': p->reduction_factor = strtol(v, NULL, 0); return;
		case 'x': p->x = strtol(v, NULL, 0); return;
		case 'y': p->y = strtol(v, NULL, 0); return;
		case 'w': p->w = strtol(v, NULL, 0); return;
		case 'h': p->h = strtol(v, NULL, 0); return;
		case 'f': p->filename = url_decode(v); return;
		case 'u': p->url = url_decode(v); return;
		case 'n': p->num_comps = strtol(v, NULL, 0); return;
		case 'c': p->jsonp_callback = url_decode(v); return;
		default: return;
	}
}

static struct params *parse(char *qstr) {
	struct params *p = init_params();
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
	return p;
}

static struct opj_res getTile(struct params *p, char *msg) {
	struct opj_res resources;

	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters(&parameters);
	parameters.cp_reduce = p->reduction_factor;
	parameters.cp_layer = 100;

	if(p->filename) { 
		resources = opj_init(p->filename, &parameters); 
	} else if(p->url) { 
		char *cachedir = CACHEDIR;
		if(getenv("JP2_CACHEDIR") != NULL)  { cachedir = getenv("JP2_CACHEDIR"); }
		char *cachefile = download_to_cache(p->url, cachedir);
		if(cachefile != NULL) {
			resources = opj_init(cachefile, &parameters);
		} else {
			resources.status = 1;
			if(p->url) { sprintf(msg, "Resource is unreachable: '%s'", p->url); }
			return resources;
		}
	}


	switch(p->operation) {
		case READ_TILE:
			if(!opj_get_decoded_tile(resources.l_codec, resources.l_stream, resources.image, p->tile_index)) {
				resources.status = 1;
			}
			break;
		case READ_IMAGE:
			if(!(opj_decode(resources.l_codec, resources.l_stream, resources.image) && opj_end_decompress(resources.l_codec, resources.l_stream))) {
				resources.status = 1;
			}
			break;
		default: break; /* should not occur here */
	}

	if(resources.status == 1) {
		if(p->url) { sprintf(msg, "Cannot read resource: '%s'", p->url); }
		else if(p->filename) { sprintf(msg, "Cannot read resource: '%s'", p->filename); }
	}

	return resources;
}

static int getJp2Specs (struct params *p, char *data) {
	struct opj_res resources;
	int read_status = READ_FAILURE;

	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters(&parameters);

	if(p->filename) {
		resources = opj_init(p->filename, &parameters); 
	} else if(p->url) { 
		char *cachedir = CACHEDIR;
		if(getenv("JP2_CACHEDIR") != NULL)  { cachedir = getenv("JP2_CACHEDIR"); }
		char *cachefile = download_to_cache(p->url, cachedir);
		if(cachefile != NULL) {
			resources = opj_init(cachefile, &parameters);
		} else {
			sprintf(data, "{\"error\": \"Resource unreachable (cachedir: %s)\"}", cachedir); 
			return READ_FAILURE; 
		}
	} else { 
		sprintf(data, "{\"error\": \"No resource specified\"}"); 
		return READ_FAILURE; 
	}


	if(resources.status == 0) {
		opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);
		sprintf(data, "{\"x1\":%d,\"y1\":%d, \"tw\": %d, \"th\": %d, \"tdx\": %d, \"tdy\": %d, \"num_res\": %d, \"num_comps\": %d}", 
			resources.image->x1, 
			resources.image->y1,
			info->tw,
			info->th,
			info->tdx,
			info->tdy,
			info->m_default_tile_info.tccp_info[0].numresolutions,
			resources.image->numcomps
		);
		opj_destroy_cstr_info(&info);
		read_status = READ_SUCCESS;
	} else {
		char msg[255];
		if(p->url) { sprintf(msg, "Cannot read resource '%s'", p->url); }
		else if(p->filename) { sprintf(msg, "Cannot read resource '%s'", p->filename); }
		sprintf(data, "{\"error\": \"%s\"}", msg);
		read_status = READ_FAILURE;
	}

	opj_cleanup(&resources);
	return read_status;
}


static void getTime(char *tm) {
	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime(tm, 50, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
}

int main(void) {
	char data[255];
	char timestamp[50];
	getTime(timestamp);
	struct opj_res res;
	int status = READ_FAILURE;
	struct params *p = parse(getenv("QUERY_STRING"));

	switch(p->operation) {
		case READ_TILE:
		case READ_IMAGE:
			res = getTile(p, data);
			if(res.status == 0) {
				puts("Content-type: image/png");
				puts("Pragma: public");
				puts("Cache-Control: max-age=360000");
				printf("Last-Modified: %s\n", timestamp);
				printf("Status: 200 OK\n\n");
				writePNG(&res, "dynatile", p->x, p->y, p->w, p->h, p->num_comps);
			} else {
				puts("Content-type: application/json");
				printf("Status: 500 Internal Server Error\n\n{\"error\": \"%s\"}\n", data);
			}
			opj_cleanup(&res);
			break;
		case GET_HEADER:
			puts("Content-type: application/json");
			if( (status = getJp2Specs(p, data)) ) {
				puts("Pragma: public");
				puts("Cache-Control: max-age=360000");
				printf("Last-Modified: %s\n", timestamp);
				puts("Status: 200 OK\n\n");
				if(p->jsonp_callback != NULL) {
					printf("%s(%s);\n", p->jsonp_callback, data);
				} else {
					printf("%s\n", data);
				}
			} else {
				if(p->jsonp_callback != NULL) {
					printf("Status: 500 Internal Server Error\n\n%s(%s);\n", p->jsonp_callback, data);
				} else {
					printf("Status: 500 Internal Server Error\n\n%s\n", data);
				}
			}
			break;
		default:
			;
	}
	if(p->filename != NULL) { free(p->filename); }
	if(p->jsonp_callback != NULL) { free(p->jsonp_callback); }

	free(p);
	return status;
}

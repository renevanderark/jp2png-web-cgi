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
#include "opj_res.h"

typedef enum {GET_HEADER, READ_TILE} operation_t;

#define MAX_ARGS 8

struct params {
	int tile_index;
	int reduction_factor;
	char *filename;
	operation_t operation;
	int x;
	int y;
	int w;
	int h;
};

struct params *init_params() {
	struct params *p = malloc(sizeof(struct params));
	p->tile_index = 0;
	p->reduction_factor = 0;
	p->x = 0;
	p->y = 0;
	p->w = -1;
	p->h = -1;
	p->operation = GET_HEADER;
	p->filename = NULL;
	return p;
}

void parseParam(char k, char *v, struct params *p) {
	switch(k) {
		case 't': p->tile_index = atoi(v); p->operation = READ_TILE; return;
		case 'r': p->reduction_factor = atoi(v); return;
		case 'x': p->x = atoi(v); return;
		case 'y': p->y = atoi(v); return;
		case 'w': p->w = atoi(v); return;
		case 'h': p->h = atoi(v); return;
		case 'f': p->filename = v; return;
		default: return;
	}
}

struct params *parse(char *qstr) {
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

struct opj_res getTile(struct params *p) {
	FILE *fptr = fopen(p->filename, "rb");
	struct opj_res resources;
	resources.status = READ_FAILURE;

	if(fptr != NULL && is_jp2(fptr)) {
		opj_dparameters_t parameters;
		opj_set_default_decoder_parameters(&parameters);
		parameters.cp_reduce = p->reduction_factor;
		parameters.cp_layer = 100;
		struct opj_res resources = opj_init(p->filename, &parameters);

		if(resources.status == 0 && opj_get_decoded_tile(resources.l_codec, resources.l_stream, resources.image, p->tile_index)) {
			return resources;
		}
	}

	return resources;
}


int getJp2Specs (const char *filename, char *data) {
	int i = 0;

	FILE *fptr = fopen(filename, "rb");
	if(fptr != NULL && is_jp2(fptr)) {
		opj_dparameters_t parameters;
		opj_set_default_decoder_parameters(&parameters);
		struct opj_res resources = opj_init(filename, &parameters);

		opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);
		if(resources.status == 0) {
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
		}
		opj_destroy_cstr_info(&info);
		opj_cleanup(&resources);
		return READ_SUCCESS;
	} else {
		char msg[255];
		sprintf(msg, "Cannot read file '%s'", filename);
		sprintf(data, "{\"error\": \"%s\"}", msg);
		error_callback(msg, NULL);
		error_callback(filename, NULL);
		return READ_FAILURE;
	}
}

int main(int argc, char **argv) {
	char data[255];
	struct opj_res res;
	int status = READ_FAILURE;
	struct params *p = parse(getenv("QUERY_STRING"));

	switch(p->operation) {
		case READ_TILE:
			res = getTile(p);
			if(res.status == 0) {
				puts("Content-type: image/png");
				printf("Status: 200 OK\n\n");
				writePNG(&res, "dynatile", p->x, p->y, p->w, p->h);
			} else {
				puts("Content-type: application/json");
				printf("Status: 500 Internal Server Error\n\n{\"error\": \"generic\"}");
			}
			opj_cleanup(&res);
			break;
		case GET_HEADER:
			puts("Content-type: application/json");
			if(status = getJp2Specs(p->filename, data)) {
				printf("Status: 200 OK\n\n%s", data);
			} else {
				printf("Status: 500 Internal Server Error\n\n%s", data);
			}
			break;
		default:
			;
	}

	free(p);
	return status;
}

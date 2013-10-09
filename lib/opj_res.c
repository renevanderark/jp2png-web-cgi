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
#include <string.h>
#include <openjpeg.h>
#include <libmemcached/memcached.h>
#include "opj_memcached_stream.h"
#include "opj_url_stream.h"
#include "opj_res.h"

static void error_callback(const char *msg, void *client_data) {(void)client_data; fprintf(stderr, "[ERROR] %s\n", msg);}
static void warning_callback(const char *msg, void *client_data) { (void)client_data; fprintf(stderr, "[WARNING] %s\n", msg);}
static void info_callback(const char *msg, void *client_data) {(void)client_data; fprintf(stderr, "[INFO] %s\n", msg);}

struct opj_res opj_init_res(void) {
	struct opj_res resources;

	resources.status = -1;
	resources.l_stream = NULL;
	resources.l_codec = NULL;
	resources.image = NULL;
	resources.open_file = NULL;
	resources.p_url = NULL;

	return resources;
}

int opj_init_from_stream(opj_dparameters_t *parameters, struct opj_res *resources) {

	resources->image = NULL;
	resources->l_codec = opj_create_decompress(OPJ_CODEC_JP2);

	if(!opj_setup_decoder(resources->l_codec, parameters)) {
		opj_stream_destroy(resources->l_stream);
		opj_destroy_codec(resources->l_codec);
		return 2;
	}

	if(!opj_read_header(resources->l_stream, resources->l_codec, &(resources->image))) {
		opj_stream_destroy(resources->l_stream);
		opj_destroy_codec(resources->l_codec);
		opj_image_destroy(resources->image);
		return 3;
	}

	opj_set_info_handler(resources->l_codec, info_callback,00);
	opj_set_warning_handler(resources->l_codec, warning_callback,00);
	opj_set_error_handler(resources->l_codec, error_callback,00);
	return 0;
}

struct opj_res opj_init(const char *fname, opj_dparameters_t *parameters) {

	struct opj_res resources = opj_init_res();
	FILE *fptr = fopen(fname, "rb");
	if(fptr == NULL) {
		resources.status = 1;
		return resources;
	}

	resources.open_file = fptr;
	resources.l_stream = opj_stream_create_default_file_stream(fptr,1);
	if(!resources.l_stream) { 
		resources.status = 1; 
		return resources;
	}
	resources.status = opj_init_from_stream(parameters, &resources);
	return resources;
}

struct opj_res opj_init_memcached_from_url(const char *url, opj_dparameters_t *parameters, memcached_st *memc) {
	struct opj_res resources = opj_init_res();
	resources.memcached_chunk = malloc(sizeof(struct memcached_chunk));
	resources.l_stream = opj_init_memcached_stream_from_url(url, memc, resources.memcached_chunk);
	resources.status = opj_init_from_stream(parameters, &resources);
	return resources;
}


struct opj_res opj_init_from_url(const char *url, opj_dparameters_t *parameters) {
	struct opj_res resources = opj_init_res();
	resources.l_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, 1);
	if(!resources.l_stream) { 
		resources.status = 1;
		return resources; 
	}
	resources.p_url = malloc(sizeof(struct opj_url_stream_data));
	resources.p_url->url = url;
	resources.p_url->position = 0;
	resources.p_url->size = get_url_data_length(resources.p_url->url);
	if(resources.p_url->size == 0) {
		resources.status = 1;
		return resources;
	}

	opj_stream_set_user_data(resources.l_stream, resources.p_url);
	opj_stream_set_user_data_length(resources.l_stream, resources.p_url->size);
	opj_stream_set_read_function(resources.l_stream, (opj_stream_read_fn) opj_read_from_url);
	opj_stream_set_skip_function(resources.l_stream, (opj_stream_skip_fn) opj_skip_from_url);
	opj_stream_set_seek_function(resources.l_stream, (opj_stream_seek_fn) opj_seek_from_url);
	resources.status = opj_init_from_stream(parameters, &resources);
	return resources;
}

void opj_cleanup(struct opj_res *resources) {
	if(resources->l_stream) { opj_stream_destroy(resources->l_stream); }
	if(resources->l_codec) { opj_destroy_codec(resources->l_codec); }
	if(resources->image) { opj_image_destroy(resources->image); }
	if(resources->p_url) { free(resources->p_url); }
	if(resources->open_file) { fclose(resources->open_file); }
}


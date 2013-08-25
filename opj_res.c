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
#include <openjpeg.h>
#include "opj_res.h"

void error_callback(const char *msg, void *client_data) {(void)client_data; fprintf(stderr, "[ERROR] %s\r\n", msg);}
void warning_callback(const char *msg, void *client_data) { (void)client_data; fprintf(stderr, "[WARNING] %s\r\n", msg);}
void info_callback(const char *msg, void *client_data) {(void)client_data; fprintf(stderr, "[INFO] %s\r\n", msg);}

struct opj_res opj_init(const char *fname, opj_dparameters_t *parameters) {

	struct opj_res resources;
	resources.status = 0;
	resources.image = NULL;
	FILE *fptr = fopen(fname, "rb");
	resources.open_file = fptr;
	resources.l_stream = opj_stream_create_default_file_stream(fptr,1);
	resources.l_codec = opj_create_decompress(OPJ_CODEC_JP2);
	if(!resources.l_stream) { resources.status = 1; }
	if(!opj_setup_decoder(resources.l_codec, parameters)) {
		opj_stream_destroy(resources.l_stream);
		opj_destroy_codec(resources.l_codec);
		resources.status = 2;
	}

	if(!opj_read_header(resources.l_stream, resources.l_codec, &(resources.image))) {
		opj_stream_destroy(resources.l_stream);
		opj_destroy_codec(resources.l_codec);
		opj_image_destroy(resources.image);
		resources.status = 3;
	}

	opj_set_info_handler(resources.l_codec, info_callback,00);
	opj_set_warning_handler(resources.l_codec, warning_callback,00);
	opj_set_error_handler(resources.l_codec, error_callback,00);
	return resources;
}

void opj_cleanup(struct opj_res *resources) {
	if(resources->l_stream) { opj_stream_destroy(resources->l_stream); }
	if(resources->l_codec) { opj_destroy_codec(resources->l_codec); }
	if(resources->image) { opj_image_destroy(resources->image); }
	if(resources->open_file) { fclose(resources->open_file); }
}

int is_jp2(FILE *fptr) {
	unsigned char buf[12];
	unsigned int l_nb_read;

	l_nb_read = fread(buf, 1, 12, fptr);
	fseek(fptr, 0, SEEK_SET);

	int retval = memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(buf, JP2_MAGIC, 4) == 0;
	fclose(fptr);
	return retval;
}

#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <openjpeg.h>
#include "opj_res.h"
#include "opj2png.h"
#include "opj_url_stream.h"


int main(int argc, char **argv) {

	opj_stream_t* l_stream = NULL;
	struct opj_url_stream_data *p_url = NULL;

	l_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, 1);
	if(!l_stream) { return 1; }

	p_url = malloc(sizeof(struct opj_url_stream_data));
	p_url->url = argv[1];
	p_url->position = 0;
	p_url->size = 0;
	p_url->size = get_url_data_length(p_url->url);

	opj_stream_set_user_data(l_stream, p_url);
	opj_stream_set_user_data_length(l_stream, p_url->size);
	opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_url);
	opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_url);
	opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_url);

	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters(&parameters);
	parameters.cp_reduce = 2;
	struct opj_res resources = opj_init_from_stream(l_stream, &parameters);

	opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);
	if(resources.status == 0) {
		fprintf(stderr, "{\"x1\":%d,\"y1\":%d, \"tw\": %d, \"th\": %d, \"tdx\": %d, \"tdy\": %d, \"num_res\": %d, \"num_comps\": %d}\n", 
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

	opj_get_decoded_tile(resources.l_codec, resources.l_stream, resources.image, 2);
	writePNG(&resources, "dynatile",0, 0, 128, 128, resources.image->numcomps);

	opj_destroy_cstr_info(&info);
	opj_cleanup_stream(&resources);

	free(p_url);

	return 0;
}

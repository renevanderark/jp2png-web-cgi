#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h> 
#include <curl/easy.h>
#include <openjpeg.h>
#include <png.h>
#include "opj_res.h"
#include "opj2png.h"

int main(int argc, char **argv) {
	char *buf;
	size_t size;
	FILE *fp = open_memstream(&buf, &size);
	CURL *ch;
	struct opj_res resources;
	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, argv[1]);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)fp);
	curl_easy_perform(ch);
	curl_easy_cleanup(ch);

	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters(&parameters);

	resources.l_stream = opj_stream_create_default_file_stream(fp,1);
	if(!resources.l_stream) { puts("FAIL");}
	resources.l_codec = opj_create_decompress(OPJ_CODEC_JP2);
	if(!opj_setup_decoder(resources.l_codec, &parameters)) {
		puts("FAIL");
	}

	if(!opj_read_header(resources.l_stream, resources.l_codec, &(resources.image))) {
		puts("FAIL");
	}

	opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);
/*	printf("{\"x1\":%d,\"y1\":%d, \"tw\": %d, \"th\": %d, \"tdx\": %d, \"tdy\": %d, \"num_res\": %d, \"num_comps\": %d}", 
				resources.image->x1, 
				resources.image->y1,
				info->tw,
				info->th,
				info->tdx,
				info->tdy,
				info->m_default_tile_info.tccp_info[0].numresolutions,
				resources.image->numcomps
	);*/
	opj_get_decoded_tile(resources.l_codec, resources.l_stream, resources.image, 0);
	writePNG(&resources, "foo", 0, 0, info->tdx, info->tdy, 3);
	fclose(fp);
}

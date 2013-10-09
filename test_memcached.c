#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openjpeg.h>
#include <png.h>
#include <unistd.h>
#include <libmemcached/memcached.h>
#include "lib/opj_memcached_stream.h"
#include "lib/opj_res.h"
#include "lib/opj2png.h"

int main(int argc, char **argv) {
	memcached_st *memc = memcached_create(NULL);
	memcached_server_st *servers = NULL;
	struct memcached_chunk memcached_chunk;

	memcached_return rc;
	servers = memcached_server_list_append(servers, "localhost", 11211, &rc);

	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters(&parameters);
	parameters.cp_reduce = 2;

	rc = memcached_server_push(memc, servers);
	if (rc != MEMCACHED_SUCCESS) { return 1; }


	if(argc < 2) { return 1; }

	struct opj_res resources = opj_init_memcached_from_url(argv[1], &parameters, memc);

/*	resources.l_stream = opj_init_memcached_stream_from_url(argv[1], memc, &memcached_chunk);*/


	if(resources.status == 0) {

		opj_get_decoded_tile(resources.l_codec, resources.l_stream, resources.image, 55);
/*		opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);
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
		opj_destroy_cstr_info(&info);*/
		writePNG(&resources, "dynatile", 0, 0, 128, 128, 1);
	}

//	if(l_stream == NULL) { return 1; }

//	opj_stream_destroy(l_stream);

	memcached_server_free(servers);
	memcached_free(memc);
	return 0;

/*
	unsigned i;
	struct memcached_chunk rd_chunk;
	if(!init_chunk(argv[1], servers, &rd_chunk)) { return 1; }

	if(!read_total_size(&rd_chunk)) { 
		if(!download_to_cache(argv[1], servers)) { return 1; }
		if(!read_total_size(&rd_chunk)) { return 1; }
	}
	fprintf(stderr, "total size read from db: %zu\n", rd_chunk.total_size);


	FILE *fp = fopen("output.jp2", "wa");
	for(i = 0; i <= rd_chunk.total_size / MAX_CHUNK_SIZE; i++) {
		rd_chunk.idx = i;
		read_chunk(&rd_chunk);
		fwrite(rd_chunk.data, rd_chunk.size, 1, fp);
	}
	memcached_free(rd_chunk.memc);
	memcached_server_free(servers);
	free(servers);
	fclose(fp);*/
	return 0;
}

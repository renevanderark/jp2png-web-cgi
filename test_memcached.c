#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libmemcached/memcached.h>
#include <openjpeg.h>
#include "lib/opj_memcached_stream.h"

int main(int argc, char **argv) {
	memcached_st *memc = memcached_create(NULL);
	memcached_server_st *servers = NULL;
	memcached_return rc;
	servers = memcached_server_list_append(servers, "localhost", 11211, &rc);

	rc = memcached_server_push(memc, servers);
	if (rc != MEMCACHED_SUCCESS) { return 1; }


	if(argc < 2) { return 1; }

	opj_stream_t *l_stream = opj_init_memcached_stream_from_url(argv[1], memc);
	if(l_stream == NULL) { return 1; }

	opj_stream_destroy(l_stream);

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

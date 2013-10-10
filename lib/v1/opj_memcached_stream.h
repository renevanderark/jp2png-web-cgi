/**  1024 *1024-96 **/
#define MAX_CHUNK_SIZE 1024


struct memcached_chunk {
	const char *basekey;
	unsigned idx;
	size_t size;
	char data[MAX_CHUNK_SIZE];
	memcached_st *memc;
	OPJ_UINT64 total_size;
	OPJ_UINT64 read_position;
};

opj_stream_t *opj_init_memcached_stream_from_url(const char *url, memcached_st *memc, struct memcached_chunk *rd_chunk);


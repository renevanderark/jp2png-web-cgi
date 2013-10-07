#include <stdlib.h>
#include <string.h>
#include <curl/curl.h> 
#include <curl/easy.h>
#include <unistd.h>
#include <libmemcached/memcached.h>
#include <openjpeg.h>
#include "opj_memcached_stream.h"


static int init_chunk(const char *basekey, memcached_st *memc, struct memcached_chunk *chunk) {
	chunk->idx = 0;
	chunk->basekey = basekey;
	chunk->total_size = 0;
	chunk->size = 0;
	chunk->memc = memc;
	chunk->read_position = 0;

	return 1;
}

static void write_total_size(struct memcached_chunk *chunk) {
	memcached_return rc;
	char strval[50];
	sprintf(strval, "%zu", chunk->total_size);
	rc = memcached_set(chunk->memc, chunk->basekey, strlen(chunk->basekey), strval, sizeof(OPJ_UINT64), (time_t) 0, (uint32_t) 0);
	if (rc == MEMCACHED_SUCCESS) { fprintf(stderr,"Key stored successfully: %s\n", chunk->basekey); }
	else { fprintf(stderr,"Couldn't store key: %s\n",memcached_strerror(chunk->memc, rc)); }
}

static void write_chunk(struct memcached_chunk *chunk) {
	char key[255];
	sprintf(key, "%s-%010u", chunk->basekey, chunk->idx);
	memcached_return rc;
	rc = memcached_set(chunk->memc, key, strlen(key), chunk->data, chunk->size, (time_t)0, (uint32_t)0);
	if (rc == MEMCACHED_SUCCESS) { fprintf(stderr,"Key stored successfully: %s\n", key); }
	else { fprintf(stderr,"Couldn't store key: %s\n",memcached_strerror(chunk->memc, rc)); }
	chunk->total_size += chunk->size;
	chunk->idx++;
	chunk->size = 0; 
}

static int read_total_size(struct memcached_chunk *chunk) {
	memcached_return rc;
	size_t value_length;
	uint32_t flags;
	char *total_size = memcached_get(chunk->memc, chunk->basekey, strlen(chunk->basekey), &value_length, &flags, &rc);
	if (rc == MEMCACHED_SUCCESS) { fprintf(stderr,"Key read successfully: %s\n", chunk->basekey); }
	else { fprintf(stderr,"Couldn't read key: %s\n",memcached_strerror(chunk->memc, rc)); return 0; }
	chunk->total_size = strtoumax(total_size, NULL, 10);
	return 1;
}

static void read_chunk(struct memcached_chunk *chunk) {
	char key[255];
	memcached_return rc;
	uint32_t flags;
	sprintf(key, "%s-%010u", chunk->basekey, chunk->idx);
	char *data = memcached_get(chunk->memc, key, strlen(key), &chunk->size, &flags, &rc);
	memcpy((char*)&chunk->data, data, chunk->size);
	free(data);
}

static size_t write_to_cache(unsigned char *ptr, size_t size, size_t nmb, struct memcached_chunk *chunk) {
	unsigned long i;
	for(i = 0; i < size*nmb; i++) {
		if(chunk->size >= MAX_CHUNK_SIZE) { 
			write_chunk(chunk);
		}
		chunk->data[chunk->size++] = ptr[i];
	}
	return size*nmb;
}

static int check_headers(const char *url) {
	CURL *ch;
	long http_code = 0;

	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(ch, CURLOPT_NOBODY, 1);
	curl_easy_perform(ch);

	CURLcode curl_code = curl_easy_perform(ch);
	curl_easy_getinfo(ch, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(ch);
	curl_global_cleanup();

	fprintf(stderr, "HTTP status code: %zu\n", http_code);

	if(curl_code != CURLE_OK && curl_code != CURLE_ABORTED_BY_CALLBACK) { return 0; }
	if(http_code != 200 ) { return 0; }
	return 1;
}

static OPJ_UINT64 download_to_cache(const char *url, memcached_st *memc) {
	struct memcached_chunk chunk;
	if(!init_chunk(url, memc, &chunk)) { return 0; }

	if(read_total_size(&chunk)) { return chunk.total_size; }

	if(!check_headers(url)) { return 0; }

	CURL *ch;

	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, (void*)write_to_cache);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)&chunk);
	curl_easy_perform(ch);

	curl_easy_cleanup(ch);
	curl_global_cleanup();
	write_chunk(&chunk);

	write_total_size(&chunk);
	fprintf(stderr, "total size: %zu\n", chunk.total_size);
	return chunk.total_size;
}


static OPJ_OFF_T opj_skip_from_cache (OPJ_OFF_T p_nb_bytes, struct memcached_chunk *chunk) {
	chunk->read_position += p_nb_bytes;
	if(chunk->read_position > chunk->total_size) {
		return -1;
	}
	return p_nb_bytes;
}

static OPJ_BOOL opj_seek_from_cache (OPJ_OFF_T p_nb_bytes, struct memcached_chunk *chunk) {
	chunk->read_position = p_nb_bytes;
	if(chunk->read_position > chunk->total_size) {
		return OPJ_FALSE;
	}
	return OPJ_TRUE;
}

static OPJ_SIZE_T opj_read_from_cache(char * p_buffer, OPJ_SIZE_T p_nb_bytes, struct memcached_chunk *chunk) {
	OPJ_SIZE_T l_nb_read = 0;
	OPJ_UINT64 chunk_position;
	while(l_nb_read < p_nb_bytes && chunk->read_position < chunk->total_size) {
		unsigned i;
		chunk->idx = chunk->read_position / MAX_CHUNK_SIZE;
		chunk_position = chunk->read_position - (chunk->idx * MAX_CHUNK_SIZE);

		read_chunk(chunk);
		for(i = chunk_position; i < chunk->size; i++, l_nb_read++, chunk->read_position++) {
			p_buffer[l_nb_read] = chunk->data[i];
		}
	}

	return l_nb_read ? l_nb_read : (OPJ_SIZE_T)-1;
}


opj_stream_t *opj_init_memcached_stream_from_url(const char *url, memcached_st *memc) {
	OPJ_UINT64 filesize = download_to_cache(url, memc);
	if(filesize == 0) { 
		fprintf(stderr, "Failed to download file from url: %s", url);
		return NULL; 
	}

	opj_stream_t *l_stream = opj_stream_create(MAX_CHUNK_SIZE, 1);
	if(!l_stream) { 
		fprintf(stderr, "Failed to initialize stream");
		return NULL; 
	}

	struct memcached_chunk rd_chunk;
	if(!init_chunk(url, memc, &rd_chunk)) {
		fprintf(stderr, "Failed to initialize stream");
		return NULL;
	}

	read_total_size(&rd_chunk);
	if(rd_chunk.total_size == 0) {
		fprintf(stderr, "Failed to initialize stream");
		return NULL;
	}

	opj_stream_set_user_data(l_stream, &rd_chunk);
	opj_stream_set_user_data_length(l_stream, filesize);
	opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_cache);
	opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_cache);
	opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_cache);

	read_total_size(&rd_chunk);
	FILE *f = fopen("output.jp2", "wa");
	while(rd_chunk.read_position < filesize) { 
		char buf[MAX_CHUNK_SIZE];
		int i;
		OPJ_UINT64 read = opj_read_from_cache((char*)&buf, sizeof(buf) - 1, &rd_chunk);
		fwrite(buf, MAX_CHUNK_SIZE,1,f);
	}
	fclose(f);
	return l_stream;
}



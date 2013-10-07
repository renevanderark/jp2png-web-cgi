#include <stdlib.h>
#include <string.h>
#include <curl/curl.h> 
#include <curl/easy.h>
#include <unistd.h>
#include <libmemcached/memcached.h>
#include <openjpeg.h>

/**  1024 *1024-96 **/
#define MAX_CHUNK_SIZE 1024


struct memcached_chunk {
	char *basekey;
	unsigned idx;
	size_t size;
	OPJ_UINT64 total_size;
	char data[MAX_CHUNK_SIZE];
	memcached_st *memc;
};


static int init_chunk(char *basekey, memcached_server_st *servers, struct memcached_chunk *chunk) {
	memcached_return rc;

	chunk->idx = 0;
	chunk->basekey = basekey;
	chunk->total_size = 0;
	chunk->size = 0;
	chunk->memc = memcached_create(NULL);

	rc = memcached_server_push(chunk->memc, servers);
	if (rc != MEMCACHED_SUCCESS) { return 0; }

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

static int check_headers(char *url) {
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

int download_to_cache(char *url, memcached_server_st *servers) {
	struct memcached_chunk chunk;
	if(!check_headers(url)) { return 0; }
	if(!init_chunk(url, servers, &chunk)) { return 0; }

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
	memcached_free(chunk.memc);
	return 1;
}

int main(int argc, char **argv) {
	memcached_server_st *servers = NULL;
	memcached_return rc;
	servers = memcached_server_list_append(servers, "localhost", 11211, &rc);

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
	fclose(fp);
	return 0;
}
